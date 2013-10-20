#include "CServiceData.h"
#include <stdlib.h>
#include <string.h>

CServiceData::CServiceData()
{
    m_pData!=NULL;

}

CServiceData::~CServiceData()
{
    int i;
    if(m_pData !=NULL)
    {
        for(i=0;i<m_datacnt;i++)
        {
            delete []m_pData[i].sql;
        }
        delete []m_pData;
    }
}

int CServiceData::Load(PGconn * pg)
{
    int i;
    int len;
    char szRowCnt[32];
    PGresult * dataset;

    dataset = PQexec(pg,"select id,svcname,auth,sql,svctype from httpdbservice");
    if (PQresultStatus(dataset) != PGRES_TUPLES_OK)
    {
        printf("Error: %s",PQerrorMessage(pg));
        PQclear(dataset);
        return -1;
    }

    m_datacnt=PQntuples(dataset);
    if(m_datacnt>=1)
    {
        m_pData = new SERVICEDATA[m_datacnt];
        for(i=0;i<m_datacnt;i++)
        {
            m_pData[i].id=atoi(PQgetvalue(dataset,i,0));
            strcpy(m_pData[i].svcname,PQgetvalue(dataset,i,1));
            m_pData[i].auth=atoi(PQgetvalue(dataset,i,2));

            len=strlen(PQgetvalue(dataset,i,3));
            m_pData[i].sql=new char[len+1];
            strcpy(m_pData[i].sql,PQgetvalue(dataset,i,3));

            m_pData[i].svctype=atoi(PQgetvalue(dataset,i,4));
        }
    }
    else
    {
         return 100;
    }

    return 0;
}

int CServiceData::GetSvcAuthKey(char * svcname)
{
    int i;
    for(i=0;i<m_datacnt;i++)
    {
        if(strcmp(m_pData[i].svcname,svcname)==0)
        {
            return m_pData[i].auth;
        }
    }
    return 0;
}

SERVICEDATA * CServiceData::GetService(char * svcname)
{
    int i;
    for(i=0;i<m_datacnt;i++)
    {
        if(strcmp(m_pData[i].svcname,svcname)==0)
        {
            return &m_pData[i];
        }
    }
    return NULL;
}

