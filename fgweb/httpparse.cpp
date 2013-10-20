#include "httpparse.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "simplefunc.h"

//Linux下没有strcmpi
#define strcmpi strcasecmp



//解析请求报文
int ParseHttpHead(char * szHttpHead,HTTPHEAD * pHead)
{
    //请求报文格式
    /*
    GET /somedir/page.html HTTP/1.1
　　Host:www.cnpaf.net
　　Connection:close
　　User-agent:Mozilla/4.0
　　Accept-language:zh-cn
    Content-Type: text/html; charset=gb2312
	Content-Length: 32
   */
    int i,iLen;

	pHead->m_ContentLength=0;
    pHead->m_Method=ENUM_UNKNOW;
    pHead->m_Url=NULL;
    pHead->m_ExtName=NULL;
    pHead->m_Host=NULL;

    pHead->m_Connection=NULL;
    pHead->m_User_Agent=NULL;
    pHead->m_Accept=NULL;
    pHead->m_Accept_Encoding=NULL;
    pHead->m_Accept_Language=NULL;
    pHead->m_HttpVersion=NULL;
	pHead->m_Cookie=NULL;

	pHead->m_ContentType=NULL;

    if (szHttpHead==NULL)
    {
    	return -1;
    }
    if (strlen(szHttpHead)==0)
    {
   	    return -1;
   	}

    char * pCurr;

    int iPos,iPrePos;
    int iSubPos;
    iPos=strchr(szHttpHead,' ') - szHttpHead;
    if (!iPos)
    {
    	return -1;
    }
    szHttpHead[iPos]='\0';
    if (!strcmpi(szHttpHead,"GET"))
    {
    	pHead->m_Method=ENUM_GET;
    }
    else if(!strcmpi(szHttpHead,"HEAD"))
    {
    	pHead->m_Method=ENUM_HEAD;
    }
    else if(!strcmpi(szHttpHead,"POST"))
    {
    	pHead->m_Method=ENUM_POST;
    }
    else
    {
    	return -1;
    }

    //解析url
    iPrePos=iPos+1;
    iPos=strchr(&szHttpHead[iPrePos],' ') - szHttpHead;
    if (!iPos)
    {
    	return -1;
    }
    szHttpHead[iPos]='\0';
    pHead->m_Url=&szHttpHead[iPrePos];


    iLen=strlen(pHead->m_Url);
    for(i=iLen-1;i>=0;i--)
    {
    	if (pHead->m_Url[i]=='.')
    	{
    		pHead->m_ExtName=&pHead->m_Url[i+1];
    		break;
    	}
    }

   //解析HTTP VERSION
    iPrePos=iPos+1;
    iPos=strchr(&szHttpHead[iPrePos],'\r') - szHttpHead;
    if (!iPos)
    {
    	return -1;
    }
    szHttpHead[iPos]='\0';
	iPos++;
    szHttpHead[iPos]='\0';
    pHead->m_HttpVersion=&szHttpHead[iPrePos];

    //解析每行一个变量的内容
    //没有遇到两个换行回车时
	iPos++;
    while(!(szHttpHead[iPos]=='\r' && szHttpHead[iPos+1]=='\n' ))
    {
        iPrePos=iPos;
        pCurr=strstr(&szHttpHead[iPrePos],"\r\n");
        if (pCurr==NULL)
        {
        	return -1;
        }
        iPos=pCurr - szHttpHead;
        szHttpHead[iPos]='\0';
        iPos++;
        szHttpHead[iPos]='\0';
        iPos++;

        //找冒号
        iSubPos = strchr(&szHttpHead[iPrePos],':') - szHttpHead;
        if (!iSubPos)
        {
        	return -1;
        }

        szHttpHead[iSubPos]='\0';
		iSubPos++;

        if (!strcmpi(&szHttpHead[iPrePos],"Host"))
        {
        	pHead->m_Host= &szHttpHead[iSubPos];
        }
        else if (!strcmpi(&szHttpHead[iPrePos],"Connection"))
        {
        	pHead->m_Connection= &szHttpHead[iSubPos];
        }
        else if (!strcmpi(&szHttpHead[iPrePos],"User-Agent"))
        {
        	pHead->m_User_Agent= &szHttpHead[iSubPos];
        }
        else if (!strcmpi(&szHttpHead[iPrePos],"Accept-Language"))
        {
        	pHead->m_Accept_Language= &szHttpHead[iSubPos];
        }
        else if (!strcmpi(&szHttpHead[iPrePos],"Accept"))
        {
        	pHead->m_Accept= &szHttpHead[iSubPos];
        }
        else if (!strcmpi(&szHttpHead[iPrePos],"Accept-Encoding"))
        {
        	pHead->m_Accept_Encoding= &szHttpHead[iSubPos];
        }
		else if(!strcmpi(&szHttpHead[iPrePos],"Content-Type"))
		{
		    pHead->m_ContentType=&szHttpHead[iSubPos];
		}
		else if(!strcmpi(&szHttpHead[iPrePos],"Content-Length"))
		{
		    pHead->m_ContentLength=atoi(&szHttpHead[iSubPos]);
		}
		else if(!strcmpi(&szHttpHead[iPrePos],"Cookie"))
		{
		    pHead->m_Cookie=&szHttpHead[iSubPos];
		}
        else
        {
        	;
        }
    }
    return 0;
}

int ParseHttpPara(char * url,int &argc,char * * urlargv)
{
    char * p;
	char * p1,*p2;
    int i,pos;
    int n;
    int len;

    argc=0;

	urlargv[0]=url;
	argc=1;
	for(i=0;;i++)
	{
	    if(url[i]=='&' || url[i]=='?' )
        {
            url[i]=0;
            urlargv[argc]=&url[i+1];
            argc++;
        }
		else if(url[i]==' ' || (url[i]=='\r' && url[i+1]=='\n') || url[i]==0)
        {
		    //printf("i=%d exit\n",i);
			url[i]=0;
			i++;
			break;
		}
	}
	pos=i;

	char * szTmpBuf=NULL;
	int nTmpBufSize=0;
	for(i=0;i<argc;i++)
	{
	    len=strlen(urlargv[i]);
	    if(len +1> nTmpBufSize)
        {
            if(szTmpBuf != NULL)
            {
                delete []szTmpBuf;
            }
            szTmpBuf = new char[len+1];
            nTmpBufSize =  len+1;
        }
        strcpy(szTmpBuf,urlargv[i]);
        unescape(szTmpBuf,strlen(szTmpBuf),urlargv[i],len);
		printf("para %d=%s\n",i,urlargv[i]);
	}
	if(szTmpBuf!=NULL)
	{
	    delete []szTmpBuf;
	}

    return 0;
}

int GetSessionId(char * szCookie,char * szSessionId)
{
    char * p;
    int i;
	if(szCookie==NULL)
	{
	    return -1;
	}
	p=strstr(szCookie,"OSDBASESSIONID=");
    if(p==NULL)
    {
        szSessionId[0]=0;
		return -1;
    }
	p+=strlen("OSDBASESSIONID=");

	i=0;
	for(i=0; ((p[i]!=';') && (p[i]!='\r' || p[i+1]!='\n')) && i<63;i++)
    {
	    szSessionId[i]=p[i];
    }
	szSessionId[i]=0;
	printf("sessionid=%s\n",szSessionId);
	return 0;
}

