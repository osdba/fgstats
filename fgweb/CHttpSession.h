#ifndef _CHTTPSESSION_H_INCLUDED
#define _CHTTPSESSION_H_INCLUDED

#include "CRdWrtLock.h"


typedef struct
{
    time_t expired_time;
	char id[64];
	int authkey;
}SESSIONDATA;

#define MAXSESSIONNUMS 512

class CHttpSession
{
public:
    CHttpSession();

    virtual ~CHttpSession();

    SESSIONDATA m_session[MAXSESSIONNUMS];
	int CreateSession(char * sessionid,int authkey,int seconds);
	int GetRandom(int &num1,int &num2);

	int DeleteSession(char * sessionid);

    //�ҵ�session������session�ĳ�ʱʱ�䣬Ȼ�󷵻�session��authkey,���򷵻�0
	int UpdateSession(char * sessionid,int seconds);

private:
    CRdWrtLock m_Lock;
};

#endif



