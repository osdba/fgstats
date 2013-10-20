#ifndef _HTTPPARSE_H_INCLUDED
#define _HTTPPARSE_H_INCLUDED

#define MAXHTTPHEADLEN 8192
#define MAXHTTPPACKETLEN 32000
#define MAXSENDHTTPPACKETLEN 32000
#define MAXDBCOLLEN 16000


enum HTTPMETHOD {ENUM_GET,ENUM_POST,ENUM_HEAD,ENUM_UNKNOW};
typedef struct
{
    enum HTTPMETHOD m_Method;  //请求的类型,如GET,POST
    char * m_Url;        //请求中的url 如somedir/page.html
    char * m_ExtName;    //请求中的url的扩展名，如.htm,.html,.jpg
    char * m_Host;       //

    char * m_Connection; //
    char * m_User_Agent;
    char * m_Accept;
    char * m_Accept_Encoding;
    char * m_Accept_Language;
    char * m_HttpVersion;
	char * m_Cookie;

	char * m_ContentType;
    int m_ContentLength;

}HTTPHEAD;

//解析请求报文
int ParseHttpHead(char * szHttpHead,HTTPHEAD * pHead);

int ParseHttpPara(char * url,int &argc,char * * urlargv);
int GetSessionId(char * szCookie,char * szSessionId);

#endif

