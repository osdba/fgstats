#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <libpq-fe.h>

#include "CHttpThread.h"
#include "CAutoFreeMalloc.h"


#define CONFIG_FILE "../conf/fgstats.conf"

int g_ThreadCnt;
int sockfd;

char g_szExePath[256];
char g_szCfgFile[256];


int g_port;


CHttpThread * * pThreadArray;
void closeclear();
void sig_func(int no);
void clearup();



void clearup()
{
    close(sockfd);
    for(int i=0;i<g_ThreadCnt;i++)
    {
        //printf("ThreadArray[%d]\n",i);
        delete pThreadArray[i];
    }
    //printf("all thread end!\n");
    delete []pThreadArray;
}

void NoticeProgramWillExit()
{
    CHttpThread::m_bExitFlag=1;
    /*
	for(int i=0;i<g_ThreadCnt;i++)
    {
	    //唤醒线程
		pThreadArray[i]->wakeup();
    }
    */
}

void sig_func(int no)
{
    switch (no)
    {
    case SIGHUP:
        //printf("Recive SIGHUP.\n");
        break;
    case 2:
        NoticeProgramWillExit();
        printf("Recive SIGINT\n");
        break;
    case SIGQUIT:
        printf("Recive SIGQUIT \n");
        NoticeProgramWillExit();;
        exit(0);

    case SIGTERM:
        printf("Recive SIGTERM \n");
        NoticeProgramWillExit();
        break;
    case SIGABRT:
        printf("Recive SIGABRT \n");
        break;

    case SIGILL:
        printf("Recive SIGILL 非法指令异常!\n");
        NoticeProgramWillExit();
		exit(1);
        break;
    case SIGSEGV:
        printf("Recive SIGSEGV,发生了非法内存访问!\n");
        //NoticeProgramWillExit();
		exit(1);
        break;
    case SIGPIPE:
        printf("Recive SIGPIPE \n");
		break;

    default:
        printf("Recive sigial %d !\n",no);
    break;
    }
}

int main(int argc,char * argv[])
{

    if(argc!=1 && argc!=2)
    {
        printf("Example: %s [config_file]\n", argv[0]);
        return 1;
    }


    char * p;
    FILE * fp;
    char buf[1024];
    char * line;
    char * paraname;
    char * paravalue;
    int linenum;

    strcpy(g_szExePath,argv[0]);
    p=rindex(g_szExePath,'/');
    if(p==0)
    {
        g_szExePath[0]=0;
    }
    else
    {
        p[1]=0;
    }

    if(argc==1)
    {
        if(g_szExePath[0]==0)
        {
            strcpy(g_szCfgFile, CONFIG_FILE);
        }
        else
        {
            strcpy(g_szCfgFile, g_szExePath);
            strcat(g_szCfgFile, CONFIG_FILE);
        }
    }

    if(argc==2)
    {
        strcpy(g_szCfgFile,argv[1]);
    }

    fp=fopen(g_szCfgFile,"r");
    if(fp==NULL)
    {
        printf("Can not open config file %s\n",g_szCfgFile);
        return -1;
    }

    CHttpThread::m_webroot_dir[0]='\0';
    char * adminpassbuf;
    AUTO_MALLOC(adminpassbuf,16384);

    linenum=1;
    while((line=fgets(buf,1023,fp))!=NULL)
    {
        line=trim(line);
        if(line[0]=='#')
        {
            continue;
        }
        if(strlen(line)<1)
        {
            continue;
        }
        p=strstr(line,"=");
        if(p==NULL)
        {
            printf("config file %s format error: %d Line,can not find \'=\'\n%s\n",
                      g_szCfgFile,linenum,line);
            fclose(fp);
            return -2;
        }
        p[0]=0;
        paraname=trim(line);
        paravalue=trim(&p[1]);
        StrToLower(paraname);

		if(strcmp(paraname,"dbip")==0)
        {
            strncpy(CHttpThread::m_dbip,paravalue,127);
            CHttpThread::m_dbip[126]=0;
        }
        else if(strcmp(paraname,"dbport")==0)
        {
            CHttpThread::m_dbport=atoi(paravalue);
        }
        else if(strcmp(paraname,"dbname")==0)
        {
            strncpy(CHttpThread::m_dbname,paravalue,127);
            CHttpThread::m_dbname[126]=0;
        }
        else if(strcmp(paraname,"dbuser")==0)
        {
            strncpy(CHttpThread::m_dbuser,paravalue,127);
            CHttpThread::m_dbuser[126]=0;
        }
        else if(strcmp(paraname,"dbpass")==0)
        {
            strncpy(CHttpThread::m_dbpass,paravalue,127);
            CHttpThread::m_dbpass[126]=0;
        }

        if(strcmp(paraname,"httpport")==0)
        {
            g_port=atoi(paravalue);
        }

        if(strcmp(paraname,"threadcnt")==0)
        {
            g_ThreadCnt=atoi(paravalue);
        }
        if(strcmp(paraname,"adminpass")==0)
        {
			strncpy(adminpassbuf,paravalue,16383);
			adminpassbuf[16383]=0;
        }
		if(strcmp(paraname,"session_timeouts")==0)
		{
		    CHttpThread::m_session_timeouts=atoi(paravalue);
        }
		if(strcmp(paraname,"session.dir")==0)
		{
		    strncpy(CHttpThread::m_session_dir,paravalue,256);
			CHttpThread::m_session_dir[255]=0;
        }
		if(strcmp(paraname,"session.loginhtml")==0)
		{
		    strncpy(CHttpThread::m_loginhtml,paravalue,256);
			CHttpThread::m_loginhtml[255]=0;
        }
		if(strcmp(paraname,"session.reloginhtml")==0)
		{
		    strncpy(CHttpThread::m_reloginhtml,paravalue,256);
			CHttpThread::m_reloginhtml[255]=0;
        }
		if(strcmp(paraname,"webroot_dir")==0)
		{
		    strncpy(CHttpThread::m_webroot_dir,paravalue,256);
			CHttpThread::m_webroot_dir[255]=0;
        }

        linenum++;
    }
    fclose(fp);

    //static char * m_adminpass[512];
    CHttpThread::m_adminpasscnt=0;
    if(adminpassbuf[0]=='\0')
    {
        printf("must setup adminpass in config file!\n");
        return -1;
    }

    int i=0;
    int len=strlen(adminpassbuf);
    char * chldstr[1024];
    int cnt=0;
    chldstr[0]=adminpassbuf;
    cnt++;
    while(i<len)
    {
        if(adminpassbuf[i]==',')
        {
            adminpassbuf[i]='\0';
            chldstr[cnt]=&adminpassbuf[i+1];
            cnt++;
        }
        i++;
    }

    if(cnt%2) //权限数字与密码应该是成对出现的
    {
        printf("Invalid adminpass format in config file!\n");
        return -1;
    }

    CHttpThread::m_adminpasscnt=cnt/2;
    int n=0;
    for(i=0;i<CHttpThread::m_adminpasscnt;i++)
    {
        CHttpThread::m_adminauth[i]=atoi(chldstr[n]);
        n++;
        CHttpThread::m_adminpass[i]=chldstr[n];
        n++;
    }


    if(CHttpThread::m_webroot_dir[0]=='\0')
    {
        strcpy(CHttpThread::m_webroot_dir,"../htdocs");
    }

    if(g_ThreadCnt<=0)
    {
        printf("Thread count must more than zero: %d\n",g_ThreadCnt);
        return -1;
    }


    pThreadArray = new CHttpThread * [g_ThreadCnt];

    printf("Thread count is %d, start...\n",g_ThreadCnt);
    for(i=0;i<g_ThreadCnt;i++)
    {
        pThreadArray[i]=new CHttpThread(i);
		pthread_create (&pThreadArray[i]->m_Thread,
		                &pThreadArray[i]->m_Attr,
						reinterpret_cast<void* (*)(void *)>(&CHttpThread::RunCtrl),
						(void*) pThreadArray[i]);
    }

    signal(SIGHUP, sig_func);
    signal(SIGINT, sig_func); //Ctrl+C
    signal(SIGQUIT, sig_func);
    signal(SIGBUS, SIG_DFL);

    signal(SIGURG,sig_func);  //软件中断。如当Alarm Clock超时（SIGURG），

    //此程序必须忽略此事件,因为在socket写数据时,客户端可能把这个通信管道关闭.
    signal(SIGPIPE,sig_func); //当Reader中止之后又向管道写数据（SIGPIPE），

    //signal(SIGABRT,sig_func); //由调用abort函数产生，进程非正常退出

    //signal(SIGILL,sig_func); //非法指令异常
    //signal(SIGSEGV,sig_func); //非法内存访问

    int acceptfd;
    //char ipaddr[40];

    //printf("port=%d\n",port);


    struct sockaddr_in my_addr;
    struct sockaddr_in from;

//#if _AIX || _LINUX
    socklen_t fromlen=sizeof(from);
//#else
//    int fromlen=sizeof(from);
//#endif

    //0.init
    memset ((char *)&my_addr, 0, sizeof(struct sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(g_port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //my_addr.sin_addr.s_addr = inet_addr(ipaddr);

    //1.create
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd==-1)
    {
        clearup();
		return false;
    }

	int option=1;
	setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option));
    //2.bind
    int ret = bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    if (ret==-1)
    {
        perror("bind error:");
		clearup();
        return false;
    }
    //3.listen
    ret = listen(sockfd, 10);
    if (ret==-1)
    {
        perror("listen error:");
		clearup();
        return false;
    }

    struct linger optval;
    optval.l_onoff  = 1;
    optval.l_linger = 10;

    if(fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1)
	{
        perror("fcntl");
		clearup();
        return errno;
    }


    i=0;
    while(!CHttpThread::m_bExitFlag)
    {
        acceptfd = accept(sockfd,(struct sockaddr*)&from,&fromlen);
        if (acceptfd==-1)
        {
            if(errno==EAGAIN)
			{
			    usleep(1000);
				continue;
			}
			printf("Accept Error!\n");
            return -1;
        }

        printf("accept new connect from %s!\n",inet_ntoa(from.sin_addr));
		setsockopt(acceptfd,SOL_SOCKET,SO_LINGER, (char*)&optval,sizeof(optval));
		CHttpThread::m_Reqque.putq(acceptfd);
    }
    close(sockfd);
	for(i=0;i<g_ThreadCnt;i++)
	{
	    CHttpThread::m_Reqque.putq(-1);
	}
	//printf("Waiting thread exit...\n");
	for(i=0;i<g_ThreadCnt;i++)
    {
		pthread_join(pThreadArray[i]->m_Thread,NULL);
    }
    clearup();
    return 0;
}



