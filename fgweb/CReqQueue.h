#ifndef _CREQQUEUE_H_INCLUDED
#define _CREQQUEUE_H_INCLUDED

#include <unistd.h>
#include <pthread.h>


/* 把http请求放到这个队列中 */
class CReqQueue
{

public:
    CReqQueue(int queueSize=1024);
    virtual ~CReqQueue();

    int getq();

    void putq(int data);

	int GetQueueSize();

private:
    pthread_mutex_t m_mutex;
    pthread_cond_t condGet;
    pthread_cond_t condPut;

    int * buffer;      //循环消息队列
    int sizeQueue;     //队列大小
    int lput;          //location put  放数据的指针偏移
    int lget;          //location get  取数据的指针偏移
    int nFullThread;   //队列满，阻塞在putq处的线程数
    int nEmptyThread;  //队列空，阻塞在getq处的线程数
    int nData;         //队列中的消息个数，主要用来判断队列空还是满
};

#endif

