#include "CHttpThread.h"
#include "simplefunc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "CAutoFreeMalloc.h"


CHttpSession CHttpThread::m_session;
CReqQueue CHttpThread::m_Reqque;

char CHttpThread::m_dbip[128];
int  CHttpThread::m_dbport;
char CHttpThread::m_dbname[128];
char CHttpThread::m_dbuser[64];
char CHttpThread::m_dbpass[128];

char * CHttpThread::m_adminpass[512];
int CHttpThread::m_adminpasscnt=0;
int CHttpThread::m_adminauth[512];


int  CHttpThread::m_session_timeouts=300;

char CHttpThread::m_webroot_dir[256];
char CHttpThread::m_session_dir[256];
char CHttpThread::m_loginhtml[256];
char CHttpThread::m_reloginhtml[256];

int CHttpThread::m_bExitFlag=0;

const char * szReplyBadReq="HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 48\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>";


CHttpThread::CHttpThread(int id)
{
    m_id=id;
    m_Thread=0;

    m_pSvcData=NULL;

	pthread_attr_init(&m_Attr);
}

CHttpThread::~CHttpThread()
{
	pthread_attr_destroy(&m_Attr);
    if(m_pSvcData!=NULL)
    {
        delete m_pSvcData;
    }
}

int CHttpThread::ReadHttpReq()
{
    int iRet;
	int pos;
	int len;
	char * p;
	int contentpos;
	int i,j;

	m_szHttpHead[0]='\0';
    pos=0;
	p=strstr(m_szHttpHead,"\r\n\r\n");
    while (p==NULL)
    {
        len=recv(m_sockfd,&m_szHttpHead[pos],1023,0);
		if(len<=0)
		{
		    return -1;
		}
        //printf("len=%d\n",len);
        pos+=len;
		m_szHttpHead[pos]=0;
		p=strstr(m_szHttpHead,"\r\n\r\n");
    }
	if(pos==0)
	{
	    return -1;
	}
	m_szHttpHead[pos]=0;
	contentpos=p - m_szHttpHead + 4;
	p[2]=0;

	printf("========================http head begin =================================\n");
	printf("%s",m_szHttpHead);
	printf("========================http head end =================================\n");

	m_szHttpContent[0]=0;
	for(j=0,i=contentpos;i<pos;i++)
	{
	    m_szHttpContent[j]=m_szHttpHead[i];
		j++;
	}
	m_szHttpContent[j]=0;
	iRet= ParseHttpHead(m_szHttpHead,&m_HttpHead);

	if(m_HttpHead.m_ContentLength<0 || m_HttpHead.m_ContentLength>MAXHTTPPACKETLEN)
	{
	    return -1;
	}

	if(m_HttpHead.m_ContentLength==0)
	{
	    return 0;
	}

	if(m_HttpHead.m_ContentLength - j>0)
	{
		iRet=SocketRead(m_sockfd,&m_szHttpContent[j],m_HttpHead.m_ContentLength - j);
		if(iRet<=0)
		{
			return -1;
		}
		m_szHttpContent[iRet+j]=0;
	}

	printf("============================http content begin ===============================\n");
	printf("%s\n",m_szHttpContent);
	printf("============================http content end   ===============================\n");

	return 0;
}

void CHttpThread::ReplyHttpReq(const char * info,const char * sessionid)
{
	if(sessionid==NULL)
	{
		sprintf(m_buff,
		"HTTP/1.1 200 OK\r\n"\
		"Server: TangCheng Data Server1.0\r\n"\
		"Connection: close\r\n"\
		"Cache-Control: no-cache\r\n"\
		"Content-Type: text/html; charset=UTF-8\r\n"\
		"Content-Length: %d\r\n\r\n%s",(int)strlen(info),info);
	}
	else
	{
		sprintf(m_buff,
		"HTTP/1.1 200 OK\r\n"\
		"Server: TangCheng Data Server1.0\r\n"\
		"Connection: close\r\n"\
		"Cache-Control: no-cache\r\n"\
		"Set-Cookie: OSDBASESSIONID=%s\r\n"\
		"Content-Type: text/html; charset=gb2312\r\n"\
		"Content-Length: %d\r\n\r\n%s",sessionid,(int)strlen(info),info);
	}
	send(m_sockfd,m_buff,strlen(m_buff),0);
}


void CHttpThread::Run()
{
    //char content[1024];
    char szSQL[2048];
	char szFile[512];

    char * svcname;
    char * urlargv[256];
    int argc;

    unsigned long bindparalen[256];
    char sql_in[2048];

    int pos;
    int len;
    int start_time;
    int stat_id;
    int i;
	int iRet;

	char sessionid[64];
	char errinfo[256];
    int svcauthkey; //服务的authkey
    int authpassed;
    int loginauthkey; //login的authkey


    printf("Thread %d is starting...\n",m_id);

    m_pSvcData = new CServiceData;

	ConnStatusType pgstatus;
	char connstr[1024];
	sprintf(connstr,"hostaddr=%s dbname=%s port=%d user=%s password=%s",
			m_dbip,m_dbname,m_dbport,m_dbuser,m_dbpass);
	m_pq=PQconnectdb(connstr);
	pgstatus=PQstatus(m_pq);
	if(pgstatus==CONNECTION_OK)
	{
		printf("Thread %d connect database success!\n",m_id);
	}
	else
	{
		printf("Thread %d connect database fail:%s\n",m_id,PQerrorMessage(m_pq));
		return;
	}

	m_pSvcData->Load(m_pq);


    while(!m_bExitFlag)
    {
        //m_optval.l_onoff  = 1;
        //m_optval.l_linger = 10;
        //setsockopt(m_sockfd,SOL_SOCKET,SO_LINGER, (char*)&m_optval,sizeof(m_optval));
        m_sockfd=m_Reqque.getq();
	    printf("thread %d start to process job ...\n",m_id);

		if(m_sockfd==-1)
		{
		    break;
		}

        //读http请求
		iRet=ReadHttpReq();
		if(iRet<0)
		{
			send(m_sockfd,szReplyBadReq,strlen(szReplyBadReq),0);
            goto next_flag;
		}

		//只处理GET和POST的请求
		if(m_HttpHead.m_Method!=ENUM_GET && m_HttpHead.m_Method!=ENUM_POST)
		{
			send(m_sockfd,szReplyBadReq,strlen(szReplyBadReq),0);
            goto next_flag;
		}

		if(m_HttpHead.m_ContentLength<0)
		{
			printf("Content-length:%d can not less than zero!\n",m_HttpHead.m_ContentLength);
			send(m_sockfd,szReplyBadReq,strlen(szReplyBadReq),0);
            goto next_flag;
		}

		Url2FileName(m_HttpHead.m_Url,szFile);

		if(m_HttpHead.m_Method==ENUM_GET)
        {
			//是需要登陆验证的网页
            if(   strstr(szFile,m_session_dir)!=NULL
               && strcmp(szFile,m_loginhtml)!=0
               && strcmp(szFile,m_reloginhtml)!=0)
			{
				authpassed=0;
				iRet=GetSessionId(m_HttpHead.m_Cookie,sessionid);
				if(iRet==0) //cookie中有session id
				{
					loginauthkey=m_session.UpdateSession(sessionid,m_session_timeouts);
					if(loginauthkey)
					{
					    authpassed=1;
					}
				}
            }
            else
            {
                authpassed=1;
            }
            if(authpassed)
            {
            	iRet=SendFileData(szFile);
            }
            else //没有验证通过，把relogin的html发过去
            {
                iRet=SendFileData(m_reloginhtml);
            }
            if(iRet>0)
            {
                send(m_sockfd,szReplyBadReq,strlen(szReplyBadReq),0);
            }
            goto next_flag;
        }
        else //POST请求，都认为是服务请求
        {
			iRet=ParseHttpPara(m_szHttpContent,argc,urlargv);
			if(iRet<0 || argc<1 )
			{
				ReplyHttpReq("1\tPOST验证请求中的参数不正确!");
				goto next_flag;
			}
            svcname=urlargv[0];
            if(!strcmp(urlargv[0],"httpdbauth")) //登录
            {
                loginauthkey=GetPassAuth(urlargv[1]);
                if(loginauthkey>0) //找到密码
                {
                    iRet=m_session.CreateSession(sessionid,loginauthkey,m_session_timeouts);
                    ReplyHttpReq("0",sessionid);
                    goto next_flag;
                }
                else
                {
                    //密码错误
				    ReplyHttpReq("1");
				    goto next_flag;
                }
            }

            authpassed=0;
            svcauthkey=m_pSvcData->GetSvcAuthKey(svcname);
            if(svcauthkey ==0 )//为0，不需要验证
            {
                authpassed=1;
            }
            else
            {
				iRet=GetSessionId(m_HttpHead.m_Cookie,sessionid);
				if(iRet==0) //获得了sessionid
				{
					loginauthkey=m_session.UpdateSession(sessionid,m_session_timeouts);
					if(svcauthkey & loginauthkey)
					{
					    authpassed=1;
					}
					else
					{
					    if(loginauthkey)
					    {
                            ReplyHttpReq("1\t没有权限做此操作，请与管理员联系!");
                            goto next_flag;
					    }
					}
				}
            }
            if(! authpassed) //没有验证通过
            {
                ReplyHttpReq("9\t没有登陆，或登陆超时,请重新登陆!");
            }
            else
            {
                CallService(argc,urlargv);
            }
            goto next_flag;
        }


next_flag:
        shutdown(m_sockfd,SHUT_RDWR);
        close(m_sockfd);
		printf("thread %d wait job ...\n",m_id);
        //pthread_cond_wait(&m_cond,&m_mutex); //等待任务
    }
    //pthread_mutex_unlock(&m_mutex);

    PQfinish(m_pq);
	printf("Thread %d is stop.\n",m_id);
}

int CHttpThread::GetPassAuth(char * pass)
{
    int i;
    for(i=0;i<m_adminpasscnt;i++)
    {
        if(!strcmp(pass,m_adminpass[i]))
        {
            return m_adminauth[i];
        }
    }
    return 0;
}


int CHttpThread::PgExecuteSQL(PGconn *conn,
                              PGresult * &dataset,
                              const char * szSQL,
                              int nParams,
                              const char * const *paramValues,
                              char * szErrMsg)
{
    int retrycnt=0;

sqlexecretry:
    dataset = PQexecParams(m_pq,
                       szSQL,
                       nParams,       /* 参数个数 */
                       NULL,    /* 让后端推出参数类型 */
                       paramValues,
                       NULL,    /* 因为是文本，所以必须要参数长度 */
                       NULL,    /* 缺省是全部文本参数 */
                       0);      /* 是否是二进制结果 */

    if(  (PQresultStatus(dataset) == PGRES_COMMAND_OK ) ||(PQresultStatus(dataset) == PGRES_TUPLES_OK))
    {
         printf("Successfully execute SQL : %s\n",szSQL);
         return 0;
    }
    else
    {
        sprintf(szErrMsg,"%s",PQerrorMessage(m_pq));
        printf("%s\n",szErrMsg);

        PQclear(dataset);
        if(PQstatus(m_pq) != CONNECTION_OK)
        {
            if(retrycnt > 3)
            {
                return -1;
            }
            sleep(1);
            PQreset(m_pq);
            retrycnt++;

            if(PQstatus(m_pq)!=CONNECTION_OK)
            {
                printf("Thread %d reconnect database fail!\n",m_id);
                PQclear(dataset);
                goto sqlexecretry;
            }
            else
            {
                printf("Thread %d reconnect database success!\n",m_id);
            }
        }
        else //非连接性错误，可能是SQL语句错误等原因
        {
            return -1;
        }
    }

}

int CHttpThread::CallService(int argc,char ** urlargv)
{
    char *svcname=urlargv[0];
    SERVICEDATA * pSvcData;
    char **paramValues;
    int paramcnt;
    char * szSQL=NULL;

    int rowcount;
    int fieldcnt;
    int len;
    int pos;
    char * szEscapeBuf;
	char szChunkHead[64];

	char httpbuf[1024];
	char szErrMsg[1024];

	//由于chunk包的格式为: "十六进制长度字符串"加 "\r\n"再加实际的内容，所以用startpos表示实际的内容开始的位置。
	//chunkheadlen表示 "十六进制长度字符串"加"\r\n"的长度，所以chunk实际的开始位置为: startpos - chunkheadlen
    char * szSendBuf;
	const int startpos=64;
	int chunkheadlen;

	int escape_result_len;

	int collen;

    int i,j;
    int isfirstrow=1;
    int iRet=0;

    PGresult   *dataset;

    char httphead[1024];

    AUTO_MALLOC(szSendBuf,MAXSENDHTTPPACKETLEN);
    AUTO_MALLOC(szEscapeBuf,MAXSENDHTTPPACKETLEN);
    AUTO_MALLOC(szSQL,256*1024);
    AUTO_MALLOC(paramValues,sizeof(char *)*argc);

    sprintf(httphead,
        "HTTP/1.1 200 OK\r\n"\
        "Server: TangCheng Data Server1.0\r\n"\
        "Transfer-Encoding: chunked\r\n"\
        "Connection: close\r\n"\
        "Cache-Control: no-cache\r\n"\
        "Content-Type: text/html; charset=UTF-8\r\n\r\n"
        );
    send(m_sockfd,httphead,strlen(httphead),0);


    pSvcData = m_pSvcData->GetService(svcname);
    if(pSvcData==NULL)
    {
        len=sprintf(&szSendBuf[64],"1\tUnknown Service: %s\r\n0\r\n\r\n",svcname);
        chunkheadlen=sprintf(szChunkHead,"%x\r\n",len-7);
        memcpy(&szSendBuf[64-chunkheadlen],szChunkHead,chunkheadlen);
        send(m_sockfd,&szSendBuf[64-chunkheadlen],len+chunkheadlen,0);
        return -1;
    }

    if(pSvcData->svctype ==101 || pSvcData->svctype ==201 || pSvcData->svctype==150  )
    {
        paramValues=&urlargv[1];
        szSQL=pSvcData->sql;
        paramcnt=argc-1;
    }
    else if(pSvcData->svctype==102) //SQL中有1个不定绑定变量数目列表的查询
    {
        char * liststr=strstr(pSvcData->sql,"$list");
        int pos=0;
        int len=0;
        if(liststr==NULL)
        {
            len=sprintf(&szSendBuf[64],"1\tService sql can not find $list: %s\r\n0\r\n\r\n",svcname);
            chunkheadlen=sprintf(szChunkHead,"%x\r\n",len-7);
            memcpy(&szSendBuf[64-chunkheadlen],szChunkHead,chunkheadlen);
            send(m_sockfd,&szSendBuf[64-chunkheadlen],len+chunkheadlen,0);
            return -1;
        }
        int listpos = liststr - pSvcData->sql;
        int bindidx=GetMaxBindIdx(pSvcData->sql);
        if(bindidx+1>argc-1)
        {
            len=sprintf(&szSendBuf[64],"1\tParameter count must more than%d : %s\r\n0\r\n\r\n",bindidx,svcname);
            chunkheadlen=sprintf(szChunkHead,"%x\r\n",len-7);
            memcpy(&szSendBuf[64-chunkheadlen],szChunkHead,chunkheadlen);
            send(m_sockfd,&szSendBuf[64-chunkheadlen],len+chunkheadlen,0);
            return -1;
        }
        pos=listpos;
        memcpy(szSQL,pSvcData->sql,pos);
        szSQL[pos]='\0';
        for(i=bindidx+1;i<argc;i++)
        {
            len=sprintf(&szSQL[pos],"$%d",i);
            pos+=len;
            if(i!=argc-1)
            {
                strcat(&szSQL[pos],",");
                pos++;
            }
        }
        szSQL[pos]='\0';
        strcat(szSQL,&liststr[5]);
        paramValues=&urlargv[1];
        paramcnt=argc-1;
    }
    else if (pSvcData->svctype ==202) //update语句，需要拼SQL
    {
        //这时传入的参数格式为：<服务名><where子句中绑定变量的个数><where bind1><where bind2>...<col name><col value>....
        char tablename[32];
        char * szWhere;

        len=strlen(pSvcData->sql);
        for(i=0;pSvcData->sql[i]!=' ' && pSvcData->sql[i]!='\t' && i<len;i++)
        {
            tablename[i]=pSvcData->sql[i];
        }
        tablename[i]='\0';
        szWhere = &pSvcData->sql[i+1];
        int NextBindIdx=GetMaxBindIdx(szWhere,'$')+1;
        pos=sprintf(szSQL,"update %s set ",tablename);

        int WhereStart=0;
        paramcnt=0;

        int WhereParamCnt=atoi(urlargv[1]);

        for(i=2;i<WhereParamCnt+2;i++)
        {
            paramValues[paramcnt]=urlargv[i];
            paramcnt++;
        }
        for( ;i<argc;i++)
        {

            len=sprintf(&szSQL[pos],"%s=$%d,",urlargv[i],paramcnt+1);
            pos+=len;
            i++;
            paramValues[paramcnt]=urlargv[i];
            paramcnt++;
        }
        pos--;
        len=sprintf(&szSQL[pos]," %s",szWhere);
        pos+=len;
    }

    printf("Run SQL: %s\n",szSQL);
    iRet=PgExecuteSQL(m_pq,dataset,szSQL,paramcnt,paramValues,szErrMsg);
    if(iRet != 0)
    {
        char szErrorTmp1[2048];
		char szErrorTmp2[2048];
		len = sprintf(szErrorTmp1,"%s\n%s",szSQL,szErrMsg);
		escape(szErrorTmp1, len, szErrorTmp2, 2047);
		len=sprintf(&szSendBuf[64],"1\tExecute SQL failed: %s\r\n0\r\n\r\n",szErrorTmp2);
        chunkheadlen=sprintf(szChunkHead,"%x\r\n",len-7);
        memcpy(&szSendBuf[64-chunkheadlen],szChunkHead,chunkheadlen);
        send(m_sockfd,&szSendBuf[64-chunkheadlen],len+chunkheadlen,0);
        return -1;
    }

    if(pSvcData->svctype >200) //大于200是无结果集的SQL，一般是ddl语句
    {
        PQclear(dataset);
		len=sprintf(&szSendBuf[64],"\n0\tsuccess\r\n0\r\n\r\n");
        chunkheadlen=sprintf(szChunkHead,"%x\r\n",len-7);
        memcpy(&szSendBuf[64-chunkheadlen],szChunkHead,chunkheadlen);
        send(m_sockfd,&szSendBuf[64-chunkheadlen],len+chunkheadlen,0);
        return 0;
    }

    //后面处理的是有结果集的SQL
    pos=startpos;
    isfirstrow=1;

    int rowcnt=PQntuples(dataset);
    const char * cell;

    for (i = 0; i < rowcnt; i++)
    {
       if(!isfirstrow)
       {
           szSendBuf[pos]='\n';
           pos++;
       }
       else
       {
           isfirstrow=0;
       }

       for (j = 0; j < PQnfields(dataset); j++)
       {
           cell = PQgetvalue(dataset, i, j);
           if(cell!=NULL)
           {
               collen=strlen(cell);
           }
           else
           {
               collen=0;
           }
           if(collen>0)
           {
               escape_result_len=escape(cell, collen, szEscapeBuf, MAXSENDHTTPPACKETLEN);
               if(escape_result_len==-1)
               {
                   printf("error in escape!!!!!\n");
                   PQclear(dataset);
                   return -1;
               }
           }
           else
           {
               escape_result_len=0;
               szEscapeBuf[0]=0;
           }
           //printf("escape_result_len=%d\n",escape_result_len);

           //当buf中剩余的空闲空间不足以放下此字段的内容时，先把前面的内容发送出去。
           if( MAXSENDHTTPPACKETLEN - pos < escape_result_len +1+2 ) //每个字段以\t分隔，所以要+1，另每个chunk以\r\n结尾，所以还要加2
           {
               chunkheadlen=sprintf(szChunkHead,"%x\r\n",pos - startpos);
               memcpy(&szSendBuf[startpos-chunkheadlen],szChunkHead,chunkheadlen);
               szSendBuf[pos]='\r';
               szSendBuf[pos+1]='\n';
               pos+=2;
               send(m_sockfd,&szSendBuf[startpos-chunkheadlen],pos -(startpos-chunkheadlen),0);
               //printf("send data,pos=%d,chunkheadlen=%d,len=%d: %s\n",pos,chunkheadlen,pos -(startpos-chunkheadlen),szChunkHead);
               pos=startpos;
           }

           if(j==0)
           {
               len=sprintf(&szSendBuf[pos],"%s",szEscapeBuf);
               printf("%s",szEscapeBuf);
           }
           else
           {
               len=sprintf(&szSendBuf[pos],"\t%s",szEscapeBuf);
           }
           pos+=len;
        }
        printf("\n");
    }
    PQclear(dataset);

    if(pos>startpos)
    {
        chunkheadlen=sprintf(szChunkHead,"%x\r\n",pos - startpos);
		memcpy(&szSendBuf[startpos-chunkheadlen],szChunkHead,chunkheadlen);
		szSendBuf[pos]='\r';
		szSendBuf[pos+1]='\n';
		pos+=2;
        send(m_sockfd,&szSendBuf[startpos-chunkheadlen],pos -(startpos-chunkheadlen),0);
    }

    //sprintf(m_buff,"0\r\n\r\n");
    //send(m_sockfd,m_buff,strlen(m_buff),0);
	
	len=sprintf(&szSendBuf[64],"\n0\tsuccess\r\n0\r\n\r\n");
	chunkheadlen=sprintf(szChunkHead,"%x\r\n",len-7);
	memcpy(&szSendBuf[64-chunkheadlen],szChunkHead,chunkheadlen);
	send(m_sockfd,&szSendBuf[64-chunkheadlen],len+chunkheadlen,0);
    return 0;
}


//执行DML的SQL，无结果集的
int CHttpThread::ExecuteSQL(char * szSQL,char * errmsg)
{
    int iRet;
	PGresult * dataset;
	dataset = PQexec(m_pq,szSQL);
	if (PQresultStatus(dataset) != PGRES_COMMAND_OK)
	{
		strcpy(errmsg,PQerrorMessage(m_pq));
		PQclear(dataset);
		return -1;
	}
	return 0;
}

//执行查询的SQL，结果集的只有一行且只有一列
int CHttpThread::QuerySQL(const char * szSQL,char * szRes,char * errmsg)
{
    int iRet;

	PGresult * dataset;
	dataset = PQexec(m_pq,szSQL);
	if (PQresultStatus(dataset) != PGRES_TUPLES_OK)
	{
		strcpy(errmsg,PQerrorMessage(m_pq));
		PQclear(dataset);
		return -1;
	}
	int rowcnt=PQntuples(dataset);
	if(rowcnt>=1)
	{
		strcpy(szRes,PQgetvalue(dataset,0,0));
	}
	else
	{
		 szRes[0]=0;
	}
	return 0;
}

/*
int CHttpThread::SendDBData(char * szSQL)
{
    int rowcount;
	
    int fieldcnt;
    int len;
    int pos;
    //char szBuf[MAXSENDHTTPPACKETLEN];
    char szEscapeBuf[MAXSENDHTTPPACKETLEN];
	char szChunkHead[64];

	//由于chunk包的格式为: "十六进制长度字符串"加 "\r\n"再加实际的内容，所以用startpos表示实际的内容开始的位置。
	//chunkheadlen表示 "十六进制长度字符串"加"\r\n"的长度，所以chunk实际的开始位置为: startpos - chunkheadlen
    char szSendBuf[MAXSENDHTTPPACKETLEN];
	const int startpos=64;
	int chunkheadlen;

	int escape_result_len;

	int collen;

    int i,j;
    int isfirstrow=1;
    //int ret;
    int iRet;

    printf("Execute SQL: %s\n",szSQL);


	PGresult * dataset=NULL;
	dataset =  PQexec(m_pq,szSQL);
	if (PQresultStatus(dataset) != PGRES_TUPLES_OK)
	{
		printf("Execute SQL failed:%s\n%s",PQerrorMessage(m_pq),szSQL);
		PQclear(dataset);
		if(PQstatus(m_pq)!=CONNECTION_OK)
		{
			printf("Thread %d reconnect to PostgreSQL ...\n",m_id);
			PQreset(m_pq);

			if(PQstatus(m_pq)!=CONNECTION_OK)
			{
				printf("Thread %d reconnect database fail!\n",m_id);
				PQclear(dataset);
				return -1;
			}
			else
			{
				printf("Thread %d reconnect database success!\n",m_id);
				dataset =  PQexec(m_pq,szSQL);
				if (PQresultStatus(dataset) != PGRES_COMMAND_OK)
				{
					printf("retry execute SQL still failed:%s",PQerrorMessage(m_pq));
					PQclear(dataset);
					return -1;
				}
			}
		}
		else
		{
			return -1;
		}
	}

	pos=startpos;
	isfirstrow=1;

	int rowcnt=PQntuples(dataset);
	const char * cell;

	for (i = 0; i < rowcnt; i++)
	{
	   if(!isfirstrow)
	   {
		   szSendBuf[pos]='\n';
		   pos++;
	   }
	   else
	   {
		   isfirstrow=0;
	   }

	   for (j = 0; j < PQnfields(dataset); j++)
	   {
		   cell = PQgetvalue(dataset, i, j);
		   if(cell!=NULL)
		   {
			   collen=strlen(cell);
		   }
		   else
		   {
			   collen=0;
		   }
		   if(collen>0)
		   {
			   escape_result_len=escape(cell, collen, szEscapeBuf,MAXSENDHTTPPACKETLEN);
			   if(escape_result_len==-1)
			   {
				   printf("error in escape!!!!!\n");
				   PQclear(dataset);
				   return -1;
			   }
		   }
		   else
		   {
			   escape_result_len=0;
			   szEscapeBuf[0]=0;
		   }
		   //printf("escape_result_len=%d\n",escape_result_len);

		   //当buf中剩余的空闲空间不足以放下此字段的内容时，先把前面的内容发送出去。
		   if( MAXSENDHTTPPACKETLEN - pos < escape_result_len +1+2 ) //每个字段以\t分隔，所以要+1，另每个chunk以\r\n结尾，所以还要加2
		   {
			   chunkheadlen=sprintf(szChunkHead,"%x\r\n",pos - startpos);
			   memcpy(&szSendBuf[startpos-chunkheadlen],szChunkHead,chunkheadlen);
			   szSendBuf[pos]='\r';
			   szSendBuf[pos+1]='\n';
			   pos+=2;
			   send(m_sockfd,&szSendBuf[startpos-chunkheadlen],pos -(startpos-chunkheadlen),0);
			   //printf("send data,pos=%d,chunkheadlen=%d,len=%d: %s\n",pos,chunkheadlen,pos -(startpos-chunkheadlen),szChunkHead);
			   pos=startpos;
		   }

		   if(j==0)
		   {
			   len=sprintf(&szSendBuf[pos],"%s",szEscapeBuf);
			   printf("%s",szEscapeBuf);
		   }
		   else
		   {
			   len=sprintf(&szSendBuf[pos],"\t%s",szEscapeBuf);
		   }
		   pos+=len;
		}
		printf("\n");
	}
	PQclear(dataset);

    if(pos>startpos)
    {
        chunkheadlen=sprintf(szChunkHead,"%x\r\n",pos - startpos);
		memcpy(&szSendBuf[startpos-chunkheadlen],szChunkHead,chunkheadlen);
		szSendBuf[pos]='\r';
		szSendBuf[pos+1]='\n';
		pos+=2;
        send(m_sockfd,&szSendBuf[startpos-chunkheadlen],pos -(startpos-chunkheadlen),0);
		//printf("send data,pos=%d,chunkheadlen=%d,len=%d: %s\n",pos,chunkheadlen,pos -(startpos-chunkheadlen),szChunkHead);
    }
}
*/

int CHttpThread::Url2FileName(const char * szUrl,char * szFile)
{
    strcpy(szFile,m_webroot_dir);
    strcat(szFile,"/");
    if(!strcmp(szUrl,"/"))
    {
        strcat(szFile,"index.htm");
    }
    else
    {
        if(szUrl[0]=='/')
		{
		    strcat(szFile,&szUrl[1]);
		}
		else
		{
		    strcat(szFile,szUrl);
		}
    }
}

int CHttpThread::SendFileData(const char * szFile)
{
    //char szFile[512];
    char szContentType[40];
    const char * szExtName;
    char * p;
    char buf[4100];
    char szTemp[256];
    int iReadFileSize;
    int fd;
	int iRet;

    p=rindex((char *)szFile,'.');
    if(p==NULL)
    {
        szExtName=NULL;
    }
    else
    {
        szExtName=&p[1];
    }

    //Content-Type:image/gif
    if ( (!strcasecmp(szExtName,"jpg"))
      || (!strcasecmp(szExtName,"jpeg"))
      || (!strcasecmp(szExtName,"bmp"))
       )
    {
        strcpy(szContentType,"image/jpeg");
    }
	else if(!strcasecmp(szExtName,"jpg"))
	{
	    strcpy(szContentType,"image/gif");
	}
    else if( (!strcasecmp(szExtName,"htm"))
      || (!strcasecmp(szExtName,"html"))
      || (!strcasecmp(szExtName,"txt"))
       )
    {
        strcpy(szContentType,"text/html");
    }
	else if (!strcasecmp(szExtName,"css"))
	{
        strcpy(szContentType,"text/css");
	}
    else
    {
        strcpy(szContentType,"application/octet-stream");
    }

    fd=open(szFile,O_RDONLY);
    if (fd==-1)
    {
        printf("Can not open %s\n",szFile);
		return 1;
    }

    sprintf(buf,
            "HTTP/1.1 200 OK\r\n"\
            "Server: TangCheng Data Server1.0\r\n"\
            "Transfer-Encoding: chunked\r\n"\
            "Connection: close\r\n"\
            "Cache-Control: no-cache\r\n"\
            "Content-Type: %s; charset=UTF8\r\n\r\n",
            szContentType
            );
    iRet=send(m_sockfd,buf,strlen(buf),0);
	if(iRet<=0)
	{
	    return -1;
	}

    while((iReadFileSize=read(fd,buf,4096))>0)
    {
        sprintf(szTemp,"%x\r\n",iReadFileSize);
        iRet=send(m_sockfd,szTemp,strlen(szTemp),0);
	    if(iRet<=0)
	    {
	        close(fd);
			return -1;
	    }
        strcpy(&buf[iReadFileSize],"\r\n");
        iRet=send(m_sockfd,buf,iReadFileSize+2,0);
		if(iRet<=0)
	    {
	        close(fd);
			return -1;
	    }
    }
    close(fd);
    sprintf(szTemp,"0\r\n\r\n");
    iRet=send(m_sockfd,szTemp,strlen(szTemp),0);
	if(iRet>0)
	{
	    return 0;
	}
	else
	{
	    return -1;
	}
}


