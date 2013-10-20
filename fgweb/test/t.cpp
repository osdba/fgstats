#include <stdio.h>
#include <string.h>
#include "CAutoFreeMalloc.h"

int main()
{
    char * szSQL=NULL;

    AUTO_MALLOC(szSQL,256*1024);
    if(szSQL==NULL)
    {
        printf("szSQL is NULL\n");
    }
    else
    {
        memset(szSQL,0,256*1024);
    }
    return 0;
}

