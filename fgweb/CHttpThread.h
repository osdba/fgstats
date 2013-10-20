#ifndef _CHTTPTHREAD_H_INCLUDED
#define _CHTTPTHREAD_H_INCLUDED
#include <unistd.h>
#include <pthread.h>

#include "CServiceData.h"
#include "CHttpSession.h"
#include "simplefunc.h"
#include "httpparse.h"
#include "CReqQueue.h"
#include <libpq-fe.h>


class CHttpThread
{
public:
    CHttpThread(int id);
    ~CHttpThread();

    pthread_attr_t m_Attr;
    pthread_t m_Thread;
	int m_id;

    virtual void Run();

    CServiceData * m_pSvcData;

    int m_sockfd;

	char m_szHttpHead[MAXHTTPHEADLEN];
	HTTPHEAD m_HttpHead;

	char m_szHttpContent[MAXHTTPPACKETLEN];

	char m_buff[MAXSENDHTTPPACKETLEN];

	int ReadHttpReq();

	void ReplyHttpReq(const char * info,const char * sessionid=NULL);


    PGconn * m_pq;

    //执行DML的SQL，无结果集的
    int ExecuteSQL(char * szSQL,char * errmsg);

    //执行查询的SQL，要求结果集的只有一行且只有一列
    int QuerySQL(const char * szSQL,char * szRes,char * errmsg);

    int CallService(int argc,char ** urlargv);

    int PgExecuteSQL( PGconn *conn,
                      PGresult * &dataset,
                      const char * szSQL,
                      int nParams,
                      const char * const *paramValues,
                      char * szErrMsg);


    //int SendDBData(char * szSQL);
    int SendFileData(const char * szFileName);

	int Url2FileName(const char * szUrl,char * szFile);


    static void RunCtrl(CHttpThread *pthread)
    {
        pthread->Run();
        return;
    }

    int GetPassAuth(char * pass);

public:

    static int m_bExitFlag;

    static CHttpSession m_session;
    static int m_ThreadCnt;
    static CReqQueue m_Reqque;

    static int m_dbtype; //数据库类型,1为mysql,2为PostgreSQL，默认为PostgreSQL

    static char m_webroot_dir[256];

    static char m_session_dir[256];
    static char m_loginhtml[256];
    static char m_reloginhtml[256];



    static char m_dbip[128];
    static int m_dbport;
    static char m_dbname[128];
    static char m_dbuser[64];
    static char m_dbpass[128];

    static char * m_adminpass[512];
    static int m_adminpasscnt;
    static int m_adminauth[512];

    static int  m_httpport;
    static int  m_session_timeouts;

private:
    //struct linger m_optval;
};
#endif

