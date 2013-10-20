#ifndef _CRDWRTLOCK_H_INCLUDED
#define _CRDWRTLOCK_H_INCLUDED
#include <unistd.h>
#include <pthread.h>


//读写锁的类,当加读锁时，还可以读，但不能写，加写锁时，不能写也不能读。
class CRdWrtLock
{
public:
    CRdWrtLock();

	~CRdWrtLock();

    /**
    * lockRead:一个线程如果多次调用了lockread加锁，必须调用同样多次的unlock()解锁
    * @see unlock()
    * @see lockwrite()
    */
    void lockread();

    /**
    * lockwrite:一个线程如果多次调用了lockwrite加锁，必须调用同样多次的unlock()解锁
    * @see unlock()
    * @see lockread()
    */
    void lockwrite();

    void unlock();


protected:
    pthread_rwlock_t m_rwlock;

};

#endif

