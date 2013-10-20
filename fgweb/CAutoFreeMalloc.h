#ifndef _CAUTOFREEMALLOC_H_INCLUDED
#define _CAUTOFREEMALLOC_H_INCLUDED
#include <stdlib.h>

#define AUTO_MALLOC(ptrvar,size) CAutoFreeMalloc __auto_free_##ptrvar((void **)(&ptrvar),size )

class CAutoFreeMalloc
{
public:

    CAutoFreeMalloc(void **ptrvar,int size)
    {
        ptr=malloc(size);
        *ptrvar=ptr;
    }

    ~CAutoFreeMalloc()
    {
        free(ptr);
        ptr=NULL;
    }
    void * ptr;
};


#endif

