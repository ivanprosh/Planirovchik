
#include "stdafx.h"
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <locale.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <thread>
#include <queue>

using namespace std;

void (*algorithm)(int i);
class TDetail;

// ќписание станков
class TMachine
{
public:
    bool ready;
    bool readyfuture; //станок готов со след. такта к обработке
    int rab_n; // может обрабатывать одновременно
    std::vector<TDetail*> Details;
    TMachine(){Details.reserve(15);}
};

struct DetailType
{
    int WorkTime; //  оличество тактов на каждый этап работы
    int pri; // ѕриоритет
    int MachCount;
    int stanok_i[10]; // »ндексы машин, которые нужны дл€ производства детали
};

// ќписание каждой конкретной детали
class TDetail
{
public:
    int id; //»дентификатор детали
    bool state; //текущее состо€ние детали (free - 0, busy - 1, finish - 2)
    int curtime;//текущее врем€ дл€ обработки
    DetailType Type;
    bool in_rab;
    int cur_mach; //текущий индекс станка дл€ обработки
    //int ctime, cstate;
    TDetail(DetailType curType):Type(curType),curtime(curType.WorkTime),in_rab(0),cur_mach(0),state(0){}
};

// —писок машин
std::vector <TMachine*> vecMachines;
// —писок потоков
std::vector <thread*> vecThreads;
// —писок деталей
std::vector<TDetail*> vecDetails;
// ќтсортированна€ очередь сообщений
std::vector<TDetail*> sortVecDetails;

HANDLE semaphore[30], semaphore_sync;

int cpu_time = 10;
int MaxP = 4;
int MaxT = 5;
int tick = 0;
int count = 0;

//int descr_c;

//int rab_det_cnt = 0;
struct cmp_detail {

   explicit cmp_detail(const TDetail*curGoal) : goal(curGoal) {}

   bool operator ()(const TDetail* Detail) const {
      return Detail == goal;
   }

private:
   const TDetail* goal;
};

char Ch(int s) { return s+'a'; }

bool process_det(TDetail* curDet, TMachine* curMachine, int time)
{
    curDet->curtime-=time;
    if(curDet->curtime>0) return 0;
    vector<TDetail*>::const_iterator it;
    it=find_if(curMachine->Details.begin(),curMachine->Details.end(),cmp_detail(curDet));
    int mach = curDet->cur_mach;
    if(curDet->id==24){
        int j=0;
    }
    if(it!=curMachine->Details.end()) curMachine->Details.erase(it);
    curMachine->readyfuture = 1;
    mach = curDet->cur_mach;
    return 1;
}

void lcfs(int i)
{
    //cout << "Thread is " << i << endl;

    while(vecDetails[i]->state != 2)//пока не обработана
    {
        // ∆дем начала кванта
        WaitForSingleObject(semaphore[i], INFINITE);
        //cout << "Kvant " << tick << "Thread " << i << endl;

        sortVecDetails.pop_back();

        int index_machine = vecDetails[i]->Type.stanok_i[vecDetails[i]->cur_mach];
        if(i==24){
            int id = vecDetails[i]->id;
            int state = vecDetails[i]->state;
            bool ready = vecMachines.at(index_machine)->ready;
            int j=0;
        }
        if(vecMachines.at(index_machine)->ready && !vecDetails[i]->state){
            vecMachines.at(index_machine)->Details.push_back(vecDetails[i]);
            vecDetails[i]->state = 1;
            if(vecMachines.at(index_machine)->Details.size()==vecMachines.at(index_machine)->rab_n)
               vecMachines.at(index_machine)->ready = false;
        }

        if(vecDetails[i]->state == 1){
            if(process_det(vecDetails[i],vecMachines.at(index_machine),MaxT))
            {
                char stanok = Ch(vecDetails[i]->Type.stanok_i[vecDetails[i]->cur_mach]);
                if(vecDetails[i]->cur_mach<(vecDetails[i]->Type.MachCount-1))
                {
                    int MachCount = vecDetails[i]->Type.MachCount;
                    int cur_mach = vecDetails[i]->cur_mach;

                    vecDetails[i]->cur_mach++;
                    vecDetails[i]->curtime = vecDetails[i]->Type.WorkTime;
                    vecDetails[i]->state = 0;
                }
                else
                {
                    int MachCount = vecDetails[i]->Type.MachCount;
                    int cur_mach = vecDetails[i]->cur_mach;
                    if(i==24)
                        int j=0;
                    int curcount = ::count--;
                    vecDetails[i]->cur_mach = 0;
                    vecDetails[i]->curtime = vecDetails[i]->Type.WorkTime;
                    vecDetails[i]->state = 2;
                }
                stanok = Ch(vecDetails[i]->Type.stanok_i[vecDetails[i]->cur_mach]);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(MaxT));
        //освобождаем бит дл€ синхронизации по окончании кванта времени
        ReleaseSemaphore(semaphore_sync,1,NULL);
        //освобождаем следующий поток на исполнение
        if(!sortVecDetails.empty()) ReleaseSemaphore(semaphore[sortVecDetails[sortVecDetails.size()-1]->id], 1, NULL);
    }
}
// ƒл€ сортировки по возрастанию приоритета
bool cmp_det_pri( TDetail *d1, TDetail *d2 )
{
  return d1->Type.pri < d2->Type.pri;
}

void rr(int i)
{
    ;
}

bool rfile(const string& name)
{
    ifstream file;
    file.open(name);

    //cout << file.is_open() << endl;
    string s1;

    int MachinesCount=0, TypeDetailsCount=0;
    int i, j;
    if(file.is_open()) {
        file >> MachinesCount >> TypeDetailsCount;//кол-во станков, деталей
        //cout << "Count mach is " << MachinesCount << " " << TypeDetailsCount <<endl;
        getline( file, s1 );
        getline( file, s1 );

        if (s1 == "lcfs")
            algorithm = lcfs;
        else if (s1 == "rr")
            algorithm = rr;
        else
        {
            cout << "Error in type\n";
            _getch ();
            return 0;
        }

        file >> cpu_time >> MaxT >> MaxP;//задержка, врем€ обработки, макс.приоритет
        //cout << "cpu_time " << cpu_time << "MaxT" << MaxT <<"MaxP "<< MaxP <<endl;
        // —писок машин
        for( i = 0; i < MachinesCount; i++ )
        {
            TMachine *m = new TMachine();

            getline( file, s1 );
            getline( file, s1 );
            file >> m->rab_n;

            vecMachines.push_back( m );
        }

        int Detal_c = 0, curTypeDetCnt=0;
        TMachine *m;

        for( i = 0; i < TypeDetailsCount; i++ )
        {
            DetailType *curDetailType = new DetailType();

            getline( file, s1 );
            //cout << s1 << endl;
            getline( file, s1 );
            //cout << s1 << endl;
            getline( file, s1 );
            file >> curTypeDetCnt >> curDetailType->MachCount >> curDetailType->WorkTime >> curDetailType->pri;

            //cout << "type is : " << curDetailType->MachCount << " " << curDetailType->WorkTime << " " << curDetailType->pri << endl;
            //cout << "arr mach is: ";
            for( j = 0; j < curDetailType->MachCount; j++ )
            {
                file >> curDetailType->stanok_i[j];
                curDetailType->stanok_i[j]--;
                //cout << curDetailType->stanok_i[j];
            }
            //cout << endl;
            //TDetail *d = new TDetail(*curDetailType);
            //d->in_rab = false;
            //d->state = 0;
            //d->cur_mach = 0;
            //d->count = curTypeDetCnt;
            //vecDetails.push_back( d );
            for(int j = 0; j < curTypeDetCnt; j++ )
            {
                TDetail *d = new TDetail(*curDetailType);
                d->id = ::count;
                vecDetails.push_back( d );
                ::count++;
                //cout << "detail is " << endl;
            }
        }

        file.close ();
        return 1;
    }
    else
    {
        cerr << "File not found" << endl;
        return 0;
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
  if(!rfile("g:\\WORK\\_QT\\_FREELANCE\\Planirovanie\\Lab1_14\\input.txt")) return 0;

  semaphore_sync = CreateSemaphore( NULL, 0, 4, NULL );
  //priority_queue<TDetail> query;

  for(int i=0;i<vecDetails.size();i++)
  {
    semaphore[i] = CreateSemaphore( NULL, 0, 1, NULL );
    thread *thr = new thread(algorithm,i);
    vecThreads.push_back(thr);
  }

  while (true)
  {

      sortVecDetails = vecDetails;
      std::sort( sortVecDetails.begin(), sortVecDetails.end(), cmp_det_pri );

      //провер€ем готовность станков
      for(int i=0;i<vecMachines.size();i++)
      {
          vecMachines.at(i)->ready = vecMachines.at(i)->readyfuture;
          vecMachines.at(i)->readyfuture = 0;
      }

//      for(int i=0;i<sortVecDetails.size();i++)
//      {
//        cout << "Current pri queue "<< sortVecDetails[i]->Type.pri << "Current id in queue "<< sortVecDetails[i]->id << endl;
//      }

      ReleaseSemaphore(semaphore[sortVecDetails[sortVecDetails.size()-1]->id], 1, NULL);

      for (int i = 0; i < vecThreads.size(); i++) WaitForSingleObject(semaphore_sync, INFINITE );


      cout << setw( 4 );
      cout << tick++ << "- ";

      for(int i = 0; i < vecDetails.size(); i++ )
      {
          if (vecDetails.at(i)->state==0)
              cout << "   w";
          else if (vecDetails.at(i)->state==2)
              printf( "    " );
          else
          {
              int cur_mach = vecDetails.at(i)->cur_mach;
              cout << setw( 3 );
              cout << vecDetails.at(i)->curtime << Ch(vecDetails.at(i)->Type.stanok_i[cur_mach]); // вывод станка и оставшегос€ времени
          }
      }

      cout << " # " ;
      for(int i = 0; i < vecMachines.size(); i++ )
          cout << " " << vecMachines[i]->Details.size();

      cout << " # " << ::count << endl;
      if(::count<=0) break;

  }


}



