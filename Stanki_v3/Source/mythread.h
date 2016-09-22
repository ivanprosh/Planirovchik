#ifndef MYTHREAD_H
#define MYTHREAD_H
#include <QThread>
#include <QWaitCondition>

QMutex syncmutex;
QWaitCondition synchronize;

class MyThread : public QThread
{
    void (*func)(int i);
public:
    int id;
    MyThread(void (*fptr)(int i),int detid):func(fptr),id(detid){}
    void run(){
        //запуск функции, привязанной к потоку
        if(func) func(id);
    }
};

#endif // MYTHREAD_H
