#include "CHttpSession.h"
#include "simplefunc.h"
#include <stdio.h>
#include <string.h>

CHttpSession::CHttpSession()
{
    int i;
	for(i=0;i<MAXSESSIONNUMS;i++)
	{
	    m_session[i].expired_time=0;
	}
}

CHttpSession::~CHttpSession()
{
    ;
}


int CHttpSession::CreateSession(char * sessionid,int authkey,int seconds)
{
    int i;
	time_t currtime;
	time(&currtime);

	m_Lock.lockwrite();
	for(i=0;i<MAXSESSIONNUMS;i++)
	{
	    if(m_session[i].expired_time<currtime)
		{
		    break;
		}
	}
	if(i==MAXSESSIONNUMS) //session池已满，无法创建新的session了。
	{
	    m_Lock.unlock();
		return -1;
	}

	int num1,num2;
	GetRandom(num1,num2);
	sprintf(m_session[i].id,"osdba%16.0f%u%u",time_so_far()*1000000,num1,num2);
	m_session[i].expired_time=currtime+seconds;
	strcpy(sessionid,m_session[i].id);
	m_session[i].authkey=authkey;
	m_Lock.unlock();
	return 0;
}

int CHttpSession::GetRandom(int &num1,int &num2)
{
   FILE * fp;
   fp=fopen("/dev/urandom","r");
   if(fp==NULL)
   {
       return -1;
   }
   fread(&num1,4,1,fp);
   fread(&num2,4,1,fp);
   fclose(fp);
   return 0;
}

int CHttpSession::DeleteSession(char * sessionid)
{
	int i;
	time_t currtime;
	time(&currtime);

	m_Lock.lockwrite();
	for(i=0;i<MAXSESSIONNUMS;i++)
	{
	    if(m_session[i].expired_time>currtime && strcmp(sessionid,m_session[i].id)==0)
		{
		    m_session[i].expired_time=0;
			m_Lock.unlock();
            return 0;
		}
	}
	if(i==MAXSESSIONNUMS) //没有找到sessionid
	{
	    m_Lock.unlock();
		return 1;
	}
	m_Lock.unlock();
	return 0;
}

int CHttpSession::UpdateSession(char * sessionid,int seconds)
{
	int i;
	time_t currtime;
	time(&currtime);
	int authkey;

	m_Lock.lockwrite();
	for(i=0;i<MAXSESSIONNUMS;i++)
	{
	    if(m_session[i].expired_time>currtime && strcmp(sessionid,m_session[i].id)==0)
		{
		    m_session[i].expired_time=currtime+seconds;
		    authkey=m_session[i].authkey;
			m_Lock.unlock();
            return authkey;
		}
	}
	if(i==MAXSESSIONNUMS) //没有找到sessionid
	{
	    m_Lock.unlock();
		return 0;
	}
	m_Lock.unlock();
	return 0;
}




