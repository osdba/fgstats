#ifndef _CSERVICEDATA_H_INCLUDED
#define _CSERVICEDATA_H_INCLUDED

#include "libpq-fe.h"


typedef struct
{
 int id;
 char svcname[40];
 int  auth;
 char * sql;
 int svctype;
}SERVICEDATA;

class CServiceData
{
public:
    CServiceData();
    ~CServiceData();
    int Load(PGconn * pg);
    //int Load(MYSQL mysql);
    SERVICEDATA * m_pData;
    int m_datacnt;

    int GetSvcAuthKey(char * svcname);
    SERVICEDATA * GetService(char * svcname);

};

#endif

