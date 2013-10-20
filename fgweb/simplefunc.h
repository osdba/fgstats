#ifndef _SIMPLEFUNC_H_INCLUDED
#define _SIMPLEFUNC_H_INCLUDED

//Linux下没有strcmpi
#define strcmpi strcasecmp

#define   NON_NUM   '0'

int SQLEncode(const char * szSrcStr,char * szDstStr,const int resultSize);
int GetUtf8ByteNumForWord(char firstCh);
bool IsHexChar(char ch);
char Char2Num(char   ch);

char * trim(char * s);
void StrToLower(char * s);

int escape(const char* argstr, const int strSize, char* argresult, const int resultSize);
int unescape(const char * argstr, const int strSize, char * argresult, const int resultSize);
int URLEncode(const char* str, const int strSize, char* result, const int resultSize);
int URLDecode(const   char*   str,   const   int   strSize,   char*   result,   const   int   resultSize);

double time_so_far();

int SocketRead(int sockfd,char * p,int len);

//获得SQL语句中绑定变量的最大序号
int GetMaxBindIdx(char * szSQL,char pre='$');


#endif

