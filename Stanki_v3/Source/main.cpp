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
#include <QSettings>
#include <QDebug>
#include <QCoreApplication>
#include "mythread.h"

using namespace std;

class Error
{
public:
    QString descr;
    Error(QString str):descr(str){}
};

class TDetail;

// �������� �������
class TMachine
{
public:
    bool ready;
    int id;
    int count_det; // ����� ������������ ������������
    QVector<TDetail*> Details;
    TMachine(int curid,int count):id(curid),count_det(count){Details.reserve(15);}
};

// �������� ������ ���������� ������
class TDetail
{
public:
    int id; //������������� ������
    int count;//���-��
    int state; //������� ��������� ������ (free - 0, busy - 1, finish - 2)
    int curtime;//������� ����� ��� ���������
    int worktime;//����� ����� ��� ���������
    int pri; // ���������
    //
    //int MachCount;
    int stanok_i[10]; // ������� �����, ������� ����� ��� ������������ ������
    int cur_mach; //������� ������ ������ ��� ���������
    TDetail(int curid,int curcount,int curpri,int curworktime):id(curid),count(curcount),pri(curpri),worktime(curworktime),curtime(curworktime),cur_mach(0),state(0){
        for(int i=0;i<10;i++)
            stanok_i[i]=0;
    }
};

// ������ �����
QVector <TMachine*> vecMachines;
// ������ �������
QVector <MyThread*> vecThreads;
// ������ �������
QVector <TDetail*> vecDetails;
// ��������������� ������� ���������
QVector <TDetail*> sortVecDetails;

HANDLE semaphore[30], semaphore_sync;

//���������� ����������� ���������
int NR(0),NP(0),MaxP(4),MaxT(5),QT(0);
//������ ������������
QString PA;
//����� ���-�� �������
int count = 0;
//������� ���-�� ������� � ������
int curThreadCount = 0;
//��� ���������� �������������� ������� � ����. ������
QMutex PetriJump;
//��������� �� �������� ������� ��� �������
void (*algorithm)(int i);

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

//������� ������/������� ���� �����
int pnodes[2] =
{
  1,0
};

int petry_out[2][2] =
{
  { 1,0 },
  { 0,1 }
};

int petry_in[2][2] =
{
  { 0,1 },
  { 1,0 }
};

//������� � ���� ����� (������ �������� � ������ ������)
void move_petry( int t )
{
    bool fl = false;
    int i;

    while (!fl)
    {

        fl = true;
        for( i = 0; i < 2; i++ )
        {
            if (pnodes[i] < petry_in[t][i])
            {fl = false;break;}
        }

        if (fl)
        {
            PetriJump.lock();
            // �������� ����� �������
            for( i = 0; i < 2; i++ )
            {
                pnodes[i] -= petry_in[t][i];
                pnodes[i] += petry_out[t][i];
            }

#ifdef QT_DEBUG
            for( i = 0; i < 2; i++ )
            {
                qDebug() << pnodes[i] << ' ';
            }
#endif
            PetriJump.unlock();
        }

        if (!fl) Sleep(10);
    }
}

bool process_det(TDetail* curDet, TMachine* curMachine, int time)
{
    curDet->curtime-=time;
    if(curDet->curtime>0) return 0;
    QVector<TDetail*>::const_iterator it;
    it=find_if(curMachine->Details.begin(),curMachine->Details.end(),cmp_detail(curDet));
    if(curDet->id==24){
        int j=0;
    }
    //if(it!=curMachine->Details.end()) curMachine->Details.erase(it);
    //curMachine->readyfuture = 1;
    return 1;
}
void work(int i, int time)
{
    /*
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
    */
}

void SJF_n(int i)
{
    while(::count>0)//���� ���� ������
    {
        //������������� � ������ ������
        syncmutex.lock();
        synchronize.wait(&syncmutex);
        curThreadCount++;
        syncmutex.unlock();
        //���� ��������

        if(!sortVecDetails.empty()) sortVecDetails.pop_back();
        // �������� �������� ��������� ������
        work(i,MaxT);

        if(vecDetails[i]->state != 2) std::this_thread::sleep_for(std::chrono::milliseconds(MaxT));
        //����������� ������� ������������� �� ��������� ������ ������
        //ReleaseSemaphore(semaphore_sync,vecThreads.size(),NULL);

        //�������, ��� ������ ������ ��������� �� ������ ������, ��������� ���������� ���������� �������
        syncmutex.lock();
        curThreadCount--;
        syncmutex.unlock();

        if(vecDetails[i]->state != 2) {
            //������ ����.����� ������ �������
            QThread::currentThread()->sleep(0);
            sortVecDetails.push_back(vecDetails[i]); //������ ����� � �������
        }
    }

}
// ��� ���������� �� ����������� ����������
bool cmp_det_pri( TDetail *d1, TDetail *d2 )
{
  return d1->pri < d2->pri;
}

//�������� Short Job First preemptive
void SJF_p(int i)
{
    while(::count>0)//���� ���� ������
    {
        // ���� ��������������
        //WaitForSingleObject(semaphore[i], INFINITE);
        syncmutex.lock();
        synchronize.wait(&syncmutex);
        curThreadCount++;
        syncmutex.unlock();
        //���� ��������

        // ��������� ������� ����� �� �������� �� ������ ����
        if(!sortVecDetails.empty()) sortVecDetails.pop_back();
        // �������� �������� ��������� ������
        work(i,QT);

        // ���� ����� �������� ������, �� �������� ��� �� ����� ������
        if(vecDetails[i]->state != 2) std::this_thread::sleep_for(std::chrono::milliseconds(QT));

        //�������, ��� ������ ������ ��������� �� ������ ������, ��������� ���������� ���������� �������
        syncmutex.lock();
        curThreadCount--;
        syncmutex.unlock();
        //ReleaseSemaphore(semaphore_sync,1,NULL);

        //����������� ��������� ����� �� ����������
        if(!sortVecDetails.empty()) ReleaseSemaphore(semaphore[sortVecDetails[sortVecDetails.size()-1]->id], 1, NULL);
    }
}
//������ �� �����
void rfile(const string& name)
{
    QSettings settings("input.ini", QSettings::IniFormat);
    //�����
    settings.beginGroup("common");
    const QStringList childKeys = settings.childKeys();
    if(childKeys.empty()) throw Error("Something wrong with input file");

    PA = settings.value("PA").toString();
    QT = settings.value("QT").toInt();
    MaxT = settings.value("MaxT").toInt();
    MaxP = settings.value("MaxP").toInt();
    NR = settings.value("NR").toInt();
    NP = settings.value("NP").toInt();

    settings.endGroup();

    qDebug() << PA << QT << MaxT << MaxP << NR << NP;

    //����� �� ��������:
    for(int i=1;i<=NR;i++){
        settings.beginGroup("Resource_"+QString::number(i));
        vecMachines.push_back(new TMachine(i,settings.value("count_det").toInt()));
        settings.endGroup();
    }
    //����� �� ���������(�������):
    for(int i=1;i<=NP;i++){
        int it(0);
        settings.beginGroup("Process_"+QString::number(i));
        vecDetails.push_back(new TDetail(i,settings.value("count").toInt(),settings.value("pri").toInt(),settings.value("worktime").toInt()));
        QStringList query = settings.value("query").toString().split(" ",QString::SkipEmptyParts);
        //QString query = settings.value("query").toString();
        qDebug() << query;
        while(!query.isEmpty()){
            vecDetails.last()->stanok_i[it]=query.takeFirst().toInt();
            it++;
        }

        settings.endGroup();
    }
    if(PA=="SJF_n") algorithm=&SJF_n;
    if(PA=="SJF_p") algorithm=&SJF_p;
#ifdef QT_DEBUG
    //���������� �����
    //����� �� ��������:
    foreach (TMachine* curmach, vecMachines) {
        qDebug() << "Index mach: " << curmach->id  << " det: " << curmach->count_det;
    }
    foreach (TDetail* curdet, vecDetails) {
        qDebug() << "Index det: " << curdet->id  << " count det: " << curdet->count << " worktime:" << curdet->worktime << "pri:" << curdet->pri
                 << "query: " << QString::number(curdet->stanok_i[0]) << QString::number(curdet->stanok_i[1]) << QString::number(curdet->stanok_i[2])
                              << QString::number(curdet->stanok_i[3]) << QString::number(curdet->stanok_i[4]);
    }
#endif
}
//����� ����������� �� �����
void print()
{

    cout << setw( 4 );
    cout << QT++ << "- ";

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
            cout << vecDetails.at(i)->curtime << Ch(vecDetails.at(i)->stanok_i[cur_mach]); // ����� ������ � ����������� �������
        }
    }

    cout << " # " ;
    for(int i = 0; i < vecMachines.size(); i++ )
        cout << " " << vecMachines[i]->Details.size();

    cout << " # " << ::count << endl;

}

int main(int argc, _TCHAR* argv[])
{
  QCoreApplication  app(argc, argv);
  //��������� �� �������, ������� ����� ��������� �����

  try{
        rfile("input.txt");
/*
  //������� ������
  for(int i=0;i<vecDetails.size();i++)
  {
    semaphore[i] = CreateSemaphore( NULL, 0, 1, NULL );
    thread *thr = new thread(algorithm,i);
    vecThreads.push_back(thr);
  }
*/
        //������� ������
        for(int i=0;i<vecDetails.size();i++)
        {
          MyThread *thr = new MyThread(algorithm,vecDetails.at(i)->id);
          thr->start();
          vecThreads.push_back(thr);
        }


  //������� ��� �������������
  //semaphore_sync = CreateSemaphore( NULL, 0, vecThreads.size(), NULL );
  //sortVecDetails = vecDetails; //������ ��� �������

  while (::count>0)
  {
      /*
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
      */
      //���� ������ �� ����������� ���
      while(curThreadCount>0);
      //����� �� �����
      print();
      //����� ������
      synchronize.wakeAll();
  }
  _getch();

  }
    catch(Error err){
        qDebug() << "Error: "<< err.descr;
    }
  return app.exec();
}


