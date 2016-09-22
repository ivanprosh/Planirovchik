#include "stdafx.h"
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <locale.h>
//#include <vector>
//#include <algorithm>
//#include <iostream>
//#include <vector>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <thread>
#include <queue>
#include <QSettings>
#include <QDebug>
#include <QCoreApplication>
#include <QSemaphore>
#include <QFile>
#include "mythread.h"

using namespace std;

class Error
{
public:
    QString descr;
    Error(QString str):descr(str){}
};

class TDetail;

// Описание станков
class TMachine
{
public:
    bool ready,readyNextCycle;
    int id;
    int count_det; // может обрабатывать одновременно
    QVector<TDetail*> Details;
    TMachine(int curid,int count):id(curid),count_det(count),ready(1),readyNextCycle(0){Details.reserve(15);}
};

// Описание каждой конкретной детали
class TDetail
{
public:
    int id; //Идентификатор детали
    //int count;//кол-во
    int state; //текущее состояние детали (free - 0, busy - 1, finish - 2)
    int curtime;//текущее время для обработки
    int worktime;//общее время для обработки
    int pri; // Приоритет
    //
    int MachCount;
    int stanok_i[10]; // Индексы машин, которые нужны для производства детали
    int cur_mach; //текущий индекс станка для обработки
    TDetail(int curid,int curpri,int curworktime):id(curid),pri(curpri),worktime(curworktime),curtime(curworktime),cur_mach(0),state(0){
        for(int i=0;i<10;i++)
            stanok_i[i]=0;
    }
};
//объявление
void SJF_p(int i);

// Список машин
QVector <TMachine*> vecMachines;
// Список потоков
QVector <MyThread*> vecThreads;
// Список деталей
QVector <TDetail*> vecDetails;
// Отсортированная очередь
QVector <TDetail*> sortVecDetails;

QSemaphore semaphore_sync;

//Глобальные настроечные параметры
int NR(0),NP(0),MaxP(4),MaxT(5),QT(0),kvant(0);
//способ планирования
QString PA;
//общее кол-во деталей
int count = 0;
//текущее кол-во потоков в работе
int curThreadCount = 0;
//для блокировки одновременного доступа к крит. секции
QMutex PetriJump;
int countThreadOnCycle = 0;
//указатель на ключевую функцию для потоков
void (*algorithm)(int i);
//Строковый поток для записи в файл
QTextStream* out;

/*
//функтор используется для алгоритма поиска
struct cmp_detail {

   explicit cmp_detail(const TDetail*curGoal) : goal(curGoal) {}

   bool operator ()(const TDetail* Detail) const {
      return Detail == goal;
   }

private:
   const TDetail* goal;
};
*/

//предикат для сортировки по убыванию
bool moreCPU_BurstThen(const TDetail* det1,const TDetail* det2){
    return det1->worktime > det2->worktime;
}
//предикат для сортировки по убыванию приоритета процесса
bool morePriorThen(const TDetail* det1,const TDetail* det2){
    if(det1->pri==det2->pri) return (det1->pri > det2->pri);
    return (det1->worktime > det2->worktime);
}

char Ch(int s) { return s+'a'; }

//матрица входов/выходов сети Петри
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

//переход в сети Петри (аналог семафора в данной работе)
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
            // Движение можно сделать
            for( i = 0; i < 2; i++ )
            {
                pnodes[i] -= petry_in[t][i];
                pnodes[i] += petry_out[t][i];
            }

//#ifdef QT_DEBUG
            for( i = 0; i < 2; i++ )
            {
                //qDebug() << pnodes[i] << ' ';
            }
//#endif
            PetriJump.unlock();
        }

        if (!fl) QThread::currentThread()->msleep(100);
    }
}
//проверка, что деталь в работе
bool process_det(TDetail* curDet, TMachine* curMachine, int time)
{
    curDet->curtime-=time;
    if(curDet->curtime>0)
        return 0;
    for(int it=0;it<curMachine->Details.size();it++) {
        if(curMachine->Details.at(it)==curDet) {
            curMachine->Details.remove(it);
            break;
        }
    }

    curMachine->readyNextCycle = 1;
    return 1;
}
void work(int i, int time)
{
    QVector<TMachine*>* temp = &vecMachines;
    //индекс станка для текущей детали
    int index_machine = (vecDetails.at(i)->stanok_i[vecDetails.at(i)->cur_mach]) - 1;
    Q_ASSERT(index_machine<vecMachines.size() && index_machine>=0);

    if(vecDetails.at(i)->state==0){
        if(vecMachines.at(index_machine)->ready)
        {
            vecMachines.at(index_machine)->Details.push_back(vecDetails[i]);
            vecDetails[i]->state = 1;
            if(vecMachines.at(index_machine)->Details.size()==vecMachines.at(index_machine)->count_det)
                vecMachines.at(index_machine)->ready = false;
        } else if((algorithm==SJF_p) && vecMachines.at(index_machine)->Details.size()!=0) {
            for(int j=0;j<vecMachines.at(index_machine)->Details.size();j++){
                if(vecDetails[i]->pri < vecMachines.at(index_machine)->Details.at(j)->pri){
                    vecMachines.at(index_machine)->Details.at(j)->state=0;
                    vecMachines.at(index_machine)->Details[j] = vecDetails[i];
                    vecMachines.at(index_machine)->Details.at(j)->state=1;
                }
            }
        }
    }

    if(vecDetails[i]->state == 1){
        if(process_det(vecDetails[i],vecMachines[index_machine],time))
        {
            if(vecDetails[i]->cur_mach<(vecDetails.at(i)->MachCount-1))
            {
                vecDetails[i]->cur_mach++;
                int mach = vecDetails[i]->cur_mach;
                vecDetails[i]->curtime = vecDetails.at(i)->worktime;
                vecDetails[i]->state = 0;
            }
            else
            {
                ::count--;
                vecDetails[i]->cur_mach = 0;
                vecDetails[i]->curtime = vecDetails[i]->worktime;
                vecDetails[i]->state = 2;
            }
        }
    }

}

void SJF_n(int i)
{
    QVector<TDetail*>* debug = &sortVecDetails;
    syncmutex.lock();
    while(::count>0 && (vecDetails.at(i)->state != 2))//пока есть детали
    {
        //синхронизация в начале кванта
        semaphore_sync.acquire();
        synchronize.wait(&syncmutex);
        curThreadCount++;
        countThreadOnCycle++;
        syncmutex.unlock();
        semaphore_sync.release();


        //пока не подойдет очередь согласно SJF для данного потока - ждем
        while(sortVecDetails.back()->id!=i);

        if(!sortVecDetails.empty()){
            // блокировка сетью Петри (взять фишку)
            move_petry(1);
            //извлекаем детали в порядке очереди
            sortVecDetails.pop_back();
            // Основной алгоритм обработки детали
            work(i,MaxT);
            //отдать фишку
            move_petry(0);
            //Имитируем длительность кванта
            if(vecDetails.at(i)->state != 2) QThread::currentThread()->msleep(MaxT);

            //считаем, что работа потока завершена на данном кванте, уменьшаем количество запущенных потоков
            syncmutex.lock();
            curThreadCount--;
        }
    }
    syncmutex.unlock();

}
// Для сортировки по возрастанию приоритета
bool cmp_det_pri( TDetail *d1, TDetail *d2 )
{
  return d1->pri < d2->pri;
}

//алгоритм Short Job First preemptive
void SJF_p(int i)
{
    QVector<TDetail*>* debug = &sortVecDetails;
    while(::count>0 && (vecDetails.at(i)->state != 2))//пока есть детали
    {
        //синхронизация в начале кванта
        syncmutex.lock();
        semaphore_sync.acquire();
        synchronize.wait(&syncmutex);
        curThreadCount++;
        countThreadOnCycle++;
        semaphore_sync.release();
        syncmutex.unlock();

        //пока не подойдет очередь согласно SJF для данного потока - ждем
        while(sortVecDetails.back()->id!=i);

        if(!sortVecDetails.empty()){
            // блокировка сетью Петри (взять фишку)
            move_petry(1);
            //извлекаем детали в порядке очереди
            sortVecDetails.pop_back();
            // Основной алгоритм обработки детали
            work(i,QT);
            //отдать фишку
            move_petry(0);
            //Имитируем длительность кванта
            if(vecDetails.at(i)->state != 2) QThread::currentThread()->msleep(QT);

            //считаем, что работа потока завершена на данном кванте, уменьшаем количество запущенных потоков
            syncmutex.lock();
            curThreadCount--;
            syncmutex.unlock();

        }
    }
}
//Чтение из файла
void rfile(const string& name)
{
    QSettings settings("input.ini", QSettings::IniFormat);
    //общие
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

    //Обход по ресурсам:
    for(int i=1;i<=NR;i++){
        settings.beginGroup("Resource_"+QString::number(i));
        vecMachines.push_back(new TMachine(i-1,settings.value("count_det").toInt()));
        settings.endGroup();
    }
    //Обход по процессам(деталям):
    for(int i=1;i<=NP;i++){
        int it(0);
        settings.beginGroup("Process_"+QString::number(i));
        vecDetails.push_back(new TDetail(i-1,settings.value("pri").toInt(),settings.value("worktime").toInt()));
        QStringList query = settings.value("query").toString().split(" ",QString::SkipEmptyParts);
        //QString query = settings.value("query").toString();
        qDebug() << query;
        while(!query.isEmpty()){
            vecDetails.back()->stanok_i[it]=query.takeFirst().toInt();
            it++;
        }
        vecDetails.back()->MachCount = it;
        //прибавляем к общему количеству деталей
        ::count++;
        settings.endGroup();
    }
    if(PA=="SJF_n"){
        algorithm=&SJF_n; *out << "Method SJB nonpreemtive\n";
    } else if(PA=="SJF_p") {
        algorithm=&SJF_p; *out << "Method SJB preemtive\n";
    } else throw Error("Cann't understand PA method!");
//#ifdef QT_DEBUG
    //отладочный вывод
    //Обход по ресурсам:
    foreach (TMachine* curmach, vecMachines) {
        qDebug() << "Index mach: " << curmach->id  << " det: " << curmach->count_det;
    }
    foreach (TDetail* curdet, vecDetails) {
        qDebug() << "Index det: " << curdet->id  << " worktime:" << curdet->worktime << "pri:" << curdet->pri
                 << "MachCount: " << curdet->MachCount  << "query: " << QString::number(curdet->stanok_i[0]) << QString::number(curdet->stanok_i[1]) << QString::number(curdet->stanok_i[2])
                              << QString::number(curdet->stanok_i[3]) << QString::number(curdet->stanok_i[4]);
    }
    qDebug() << "*******************Finish read input file********************************";
//#endif
}
//Вывод результатов в файл
void print(QTextStream* out)
{
    syncmutex.lock();

    *out << qSetFieldWidth(4) << kvant++ << "- ";

    for(int i = 0; i < vecDetails.size(); i++ )
    {
        if (vecDetails.at(i)->state==0)
            *out << "   w";
        else if (vecDetails.at(i)->state==2)
            *out << "    ";
        else
        {
            int cur_mach = vecDetails.at(i)->cur_mach;
            //cout << setw( 3 );
            //cout << vecDetails.at(i)->curtime << Ch(vecDetails.at(i)->stanok_i[cur_mach]-1);
            // вывод станка и оставшегося времени
            *out << qSetFieldWidth(4) << QString::number(vecDetails.at(i)->curtime) + Ch(vecDetails.at(i)->stanok_i[cur_mach]-1);
        }
    }

    *out << " # " ;
    for(int i = 0; i < vecMachines.size(); i++ )
        *out << " " << vecMachines[i]->Details.size();

    *out << " # " << ::count << endl;

    syncmutex.unlock();
}

int main(int argc, _TCHAR* argv[])
{
  QVector<TDetail*>* debug = &sortVecDetails;
  QCoreApplication  app(argc, argv);
  QFile* outputFile = new QFile("output.txt");
  out = new QTextStream(outputFile);

  if (!outputFile->open(QIODevice::WriteOnly | QIODevice::Text ))
      qWarning() << "Cann't open output file!";

  outputFile->seek(outputFile->size());

  try{
        rfile("input.txt");

        //Создаем потоки
        for(int i=0;i<vecDetails.size();i++)
        {
          MyThread *thr = new MyThread(algorithm,vecDetails.at(i)->id);
          thr->start();
          vecThreads.push_back(thr);
        }

  //Семафор для синхронизации
  semaphore_sync.release(vecDetails.size());

  bool firstStart = true;

  while (::count>0)
  {
      for(int i=0;i<vecDetails.size();i++){
        if(vecDetails.at(i)->state!=2) sortVecDetails.push_back(vecDetails.at(i)); //вектор для очереди деталей
      }
      int sizeDet = sortVecDetails.size();

      if(algorithm==SJF_n){
        //сортируем по убыванию длительности CPU_burst
        qSort( sortVecDetails.begin(), sortVecDetails.end(),moreCPU_BurstThen);
      } else if (algorithm==SJF_p) {
        qSort( sortVecDetails.begin(), sortVecDetails.end(),morePriorThen);
      } else throw Error("Algorithm sorting is undefined for current method PA");

      //проверяем готовность станков
      for(int i=0;i<vecMachines.size();i++)
      {
          vecMachines.at(i)->ready |= vecMachines.at(i)->readyNextCycle;
          vecMachines.at(i)->readyNextCycle = 0;
      }

      //будим потоки

      while(semaphore_sync.available()>(vecDetails.size()-sizeDet));
      synchronize.wakeAll();

      //пока количество потоков не стало максимальным
      while(countThreadOnCycle!=sizeDet);
      //пока потоки не завершились все
      while(curThreadCount>0);

      //вывод на экран
      print(out);

      countThreadOnCycle = 0;
      //for(int i=0;i<100000;i++);
  }
  *out << "Finish!";
  outputFile->close();
  //_getch();

  }
  catch(Error err){
      qDebug() << "Error: "<< err.descr;
      outputFile->close();
  }
  return app.exec();
}



