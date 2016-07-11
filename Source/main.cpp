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

// �������� �������
class TMachine
{
public:
    bool ready;
    bool readyfuture; //������ ����� �� ����. ����� � ���������
    int rab_n; // ����� ������������ ������������
    std::vector<TDetail*> Details;
    TMachine(){Details.reserve(15);}
};

struct DetailType
{
    int WorkTime; // ���������� ������ �� ������ ���� ������
    int pri; // ���������
    int MachCount;
    int stanok_i[10]; // ������� �����, ������� ����� ��� ������������ ������
};

// �������� ������ ���������� ������
class TDetail
{
public:
    int id; //������������� ������
    int state; //������� ��������� ������ (free - 0, busy - 1, finish - 2)
    int curtime;//������� ����� ��� ���������
    DetailType Type;
    bool in_rab;
    int cur_mach; //������� ������ ������ ��� ���������
    TDetail(DetailType curType):Type(curType),curtime(curType.WorkTime),in_rab(0),cur_mach(0),state(0){}
};

// ������ �����
std::vector <TMachine*> vecMachines;
// ������ �������
std::vector <thread*> vecThreads;
// ������ �������
std::vector<TDetail*> vecDetails;
// ��������������� ������� ���������
std::vector<TDetail*> sortVecDetails;

HANDLE semaphore[30], semaphore_sync;

int cpu_time = 10;
int MaxP = 4;
int MaxT = 5;
int tick = 0;
int count = 0;

//������� ������������ ��� ��������� ���������� �����������
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
    if(curDet->id==24){
        int j=0;
    }
    if(it!=curMachine->Details.end()) curMachine->Details.erase(it);
    curMachine->readyfuture = 1;
    return 1;
}
void work(int i, int time)
{
    int index_machine = vecDetails[i]->Type.stanok_i[vecDetails[i]->cur_mach];

    if(vecMachines.at(index_machine)->ready && (vecDetails[i]->state==0)){
        vecMachines.at(index_machine)->Details.push_back(vecDetails[i]);
        vecDetails[i]->state = 1;
        if(vecMachines.at(index_machine)->Details.size()==vecMachines.at(index_machine)->rab_n)
           vecMachines.at(index_machine)->ready = false;
    }

    if(vecDetails[i]->state == 1){
        if(process_det(vecDetails[i],vecMachines.at(index_machine),time))
        {
            if(vecDetails[i]->cur_mach<(vecDetails[i]->Type.MachCount-1))
            {
                vecDetails[i]->cur_mach++;
                vecDetails[i]->curtime = vecDetails[i]->Type.WorkTime;
                vecDetails[i]->state = 0;
            }
            else
            {
                ::count--;
                vecDetails[i]->cur_mach = 0;
                vecDetails[i]->curtime = vecDetails[i]->Type.WorkTime;
                vecDetails[i]->state = 2;
            }
        }
    }
}

void lcfs(int i)
{
    while(::count>0)//���� ���� ������
    {
        // ���� ������ ������
        WaitForSingleObject(semaphore[i], INFINITE);

        if(!sortVecDetails.empty()) sortVecDetails.pop_back();
        // �������� �������� ��������� ������
        work(i,MaxT);

        if(vecDetails[i]->state != 2) std::this_thread::sleep_for(std::chrono::milliseconds(MaxT));
        //����������� ������� ������������� �� ��������� ������ ������
        ReleaseSemaphore(semaphore_sync,vecThreads.size(),NULL);

        if(vecDetails[i]->state != 2) {
            std::this_thread::sleep_for(std::chrono::milliseconds(0));//������� ������� ����� ������ �������
            sortVecDetails.push_back(vecDetails[i]); //������ ����� � �������
        }
    }

}
// ��� ���������� �� ����������� ����������
bool cmp_det_pri( TDetail *d1, TDetail *d2 )
{
  return d1->Type.pri < d2->Type.pri;
}

//�������� Round Robin
void rr(int i)
{
    while(::count>0)//���� ���� ������
    {
        // ���� ������ ������
        WaitForSingleObject(semaphore[i], INFINITE);
        // ��������� ������� ����� �� �������� �� ������ ����
        if(!sortVecDetails.empty()) sortVecDetails.pop_back();
        // �������� �������� ��������� ������
        work(i,cpu_time);

        // ���� ����� �������� ������, �� �������� ��� �� ����� ������
        if(vecDetails[i]->state != 2) std::this_thread::sleep_for(std::chrono::milliseconds(cpu_time));

        //����������� ��� ��� ������������� �� ��������� ������ �������
        ReleaseSemaphore(semaphore_sync,1,NULL);

        //����������� ��������� ����� �� ����������
        if(!sortVecDetails.empty()) ReleaseSemaphore(semaphore[sortVecDetails[sortVecDetails.size()-1]->id], 1, NULL);
    }
}
//������ �� �����
bool rfile(const string& name)
{
    ifstream file;
    file.open(name);

    //cout << file.is_open() << endl;
    string s1;

    int MachinesCount=0, TypeDetailsCount=0;
    int i, j;
    if(file.is_open()) {
        file >> MachinesCount >> TypeDetailsCount;//���-�� �������, �������
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

        file >> cpu_time >> MaxT >> MaxP;//�����, ����.����� CPU_burst, ����.���������
        //cout << "cpu_time " << cpu_time << "MaxT" << MaxT <<"MaxP "<< MaxP <<endl;
        // ������ �����
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
//����� ����������� �� �����
void print()
{

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
            cout << vecDetails.at(i)->curtime << Ch(vecDetails.at(i)->Type.stanok_i[cur_mach]); // ����� ������ � ����������� �������
        }
    }

    cout << " # " ;
    for(int i = 0; i < vecMachines.size(); i++ )
        cout << " " << vecMachines[i]->Details.size();

    cout << " # " << ::count << endl;

}

int _tmain(int argc, _TCHAR* argv[])
{
  if(!rfile("g:\\WORK\\_QT\\_FREELANCE\\Planirovanie\\Lab1_14\\input.txt")) return 0;

  //������� ������
  for(int i=0;i<vecDetails.size();i++)
  {
    semaphore[i] = CreateSemaphore( NULL, 0, 1, NULL );
    thread *thr = new thread(algorithm,i);
    vecThreads.push_back(thr);
  }

  //������� ��� �������������
  semaphore_sync = CreateSemaphore( NULL, 0, vecThreads.size(), NULL );
  sortVecDetails = vecDetails; //������ ��� �������

  while (::count>0)
  {
      //��� RR ��������� �� ���������� ������ ����� ���� ��������
      if(algorithm == rr) {
          sortVecDetails = vecDetails;
      }
      std::sort( sortVecDetails.begin(), sortVecDetails.end(), cmp_det_pri );

      //��������� ���������� �������
      for(int i=0;i<vecMachines.size();i++)
      {
          vecMachines.at(i)->ready |= vecMachines.at(i)->readyfuture;
          vecMachines.at(i)->readyfuture = 0;
      }
      //����������� ������� ��� ������ ������������� ������
      ReleaseSemaphore(semaphore[sortVecDetails[sortVecDetails.size()-1]->id], 1, NULL);
      //���� ���� ��� �������� �� ������� ���� (RR) ��� ���� ����� �� ����������(LCFS)
      for (int i = 0; i < vecThreads.size(); i++) WaitForSingleObject(semaphore_sync, INFINITE );

      //����� �� �����
      print();

  }
}



