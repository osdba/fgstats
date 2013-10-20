#include "CRdWrtLock.h"

CRdWrtLock::CRdWrtLock()
{
    pthread_rwlock_init(&m_rwlock, NULL);
}

CRdWrtLock::~CRdWrtLock()
{
    pthread_rwlock_destroy(&m_rwlock);
}



/**
* lockRead:一个线程如果多次调用了lockread加锁，必须调用同样多次的unlock()解锁
* @see unlock()
* @see lockwrite()
*/
void CRdWrtLock::lockread()
{
    pthread_rwlock_rdlock(&m_rwlock);
}


/**
* lockwrite:一个线程如果多次调用了lockwrite加锁，必须调用同样多次的unlock()解锁
* @see unlock()
* @see lockread()
*/
void CRdWrtLock::lockwrite()
{
    pthread_rwlock_wrlock(&m_rwlock);
}

void CRdWrtLock::unlock()
{
    pthread_rwlock_unlock(&m_rwlock);
}

