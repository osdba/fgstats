#include "simplefunc.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <sys/time.h>
#include <errno.h>


#include <sys/types.h>
#include <sys/socket.h>


/************************************************
  *   把字符串进行SQL编码，主要是把单个单引号换成两个单引号,&换成\&，\换成\\
  *   输入：
  *     str:           要编码的字符串
  *     result:         结果缓冲区的地址
  *     resultSize:结果地址的缓冲区大小
  *   返回值：
  *     >0:     result中实际有效的字符长度，
  *     -1:     编码失败，原因是结果缓冲区result的长度太小
  ************************************************/
int SQLEncode(const char * szSrcStr,char * szDstStr,const int resultSize)
{
    int i,j;
	int strSize;

	if(resultSize<=0)
	{
	    return -1;
	}
	if(szSrcStr==NULL)
	{
	    szDstStr[0]='\0';
		return 0;
	}

	strSize=strlen(szSrcStr);
    for(i=0,j=0;i<strSize;i++)
    {
		if(szSrcStr[i]=='\\')
		{
            if(j+2>=resultSize-1)
		    {
		        return -1;
		    }
		    szDstStr[j] = '\\';
		    j++;
		    szDstStr[j] = '\\';
		    j++;
		}
		else if(szSrcStr[i]=='\'')
		{
            if(j+2>=resultSize-1)
		    {
		        return -1;
		    }
		    szDstStr[j] = '\'';
		    j++;
		    szDstStr[j] = '\'';
		    j++;
		}
        else if(szSrcStr[i]=='&')
        {
            if(j+2>=resultSize-1)
		    {
		        return -1;
		    }
			if(szSrcStr[i]=='&')
			{
			    szDstStr[j] = '\\';
				j++;
				szDstStr[j] = '&';
				j++;
			}
        }
		else
		{
		    if(j+1>=resultSize-1)
		    {
		        return -1;
		    }

			szDstStr[j] = szSrcStr[i];
            j++;
		}
    }
    szDstStr[j]='\0';
	return j;
}


/*
  U-00000000 - U-0000007F: 0xxxxxxx
  U-00000080 - U-000007FF: 110xxxxx 10xxxxxx
  U-00000800 - U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
  U-00010000 - U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
  U-00200000 - U-03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
  U-04000000 - U-7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
int GetUtf8ByteNumForWord(char firstCh)
{
    unsigned char temp = 0x80;
    int num = 0;

    while (temp & (unsigned char )firstCh)
    {
        num++;
        temp = (temp >> 1);
    }
    printf("the num is: %d\n", num);
    return num;
}


bool IsHexChar(char ch)
{
    if(ch>='0' && ch<='9')return true;
    if(ch>='a' && ch<='f')return true;
    if(ch>='A' && ch<='F')return true;
    return false;
}



char Char2Num(char   ch)
{
    if(ch>='0' && ch<='9')return (char)(ch-'0');
    if(ch>='a' && ch<='f')return (char)(ch-'a'+10);
    if(ch>='A' && ch<='F')return (char)(ch-'A'+10);
    return NON_NUM;
}

/************************************************
  *   把字符串进行与javascript中escape相同的编码。
  *   输入：
  *     str:               要编码的字符串
  *     strSize:       字符串的长度。这样str中可以是二进制数据
  *     result:         结果缓冲区的地址
  *     resultSize:结果地址的缓冲区大小(如果str所有字符都编码，该值为strSize*3)
  *   返回值：
  *     >=0:     result中实际有效的字符长度，
  *      <0:     编码失败，原因是结果缓冲区result的长度太小
  ************************************************/
int escape(const char* argstr, const int strSize, char* argresult, const int resultSize)
{
    int   i;
    int   j   =   0;   /* for   result   index   */
    unsigned char ch;
    unsigned char widechar[4];
	int charlen;
	const unsigned char * str=(const unsigned char *)argstr;
	unsigned char *result=(unsigned char *)argresult;


    if( (result == NULL) || (strSize < 0) || (resultSize <= 0))
    {
        return -1;
    }

	if((str == NULL) || (strSize == 0))
    {
        result[0]=0;
		return 0;
    }

    for(i=0; (i<strSize) && (j<resultSize); )
	{
        ch   =   str[i];
		if( (ch & 0x80) == 0) // 00000000 01111111的字符
        {
			/*  *，+，-，.，/，@，_，0-9，a-z，A-Z  */
            if ((ch   >=   'A')   &&   (ch   <=   'Z'))
			{
                result[j++]   =   ch;
            }
			else if((ch   >=   'a')   &&   (ch   <=   'z'))
			{
                result[j++]   =   ch;
            }
			else if((ch   >=   '0')   &&   (ch   <=   '9'))
			{
                result[j++]   =   ch;
            }
			else if(ch == '*' || ch == '+' || ch == '-' || ch=='.' || ch == '/'|| ch=='@' || ch=='_')
			{
                result[j++]   =  ch;
            }
			else
			{
                if(j + 3 < resultSize)
				{
                    sprintf((char *)&result[j], "%%%02X", (unsigned char)ch);
                    j += 3;
                }
				else
				{
                    return -1;
                }
            }
			i++;
		}
		else
		{
		    charlen=GetUtf8ByteNumForWord(ch);
			if(charlen==2) //U-00000080 - U-000007FF: 110xxxxx 10xxxxxx
			{
			    if(j + 6 >= resultSize)
				{
				    return -1;
				}
				//           (str[i] & 00011111)<<3+ (str[i+1] & 00111000) >>3 & 11111000
				widechar[0]= ((str[i] & 0x1F)<<3) + ((str[i+1] & 0x38)>>3);
				widechar[1]= (str[i+1] & 0x7) <<5;
				sprintf((char *)&result[j],"%%u%02X%02X",widechar[0],widechar[1]);
				j+=6;
				i+=2;
			}
			else if(charlen==3) //U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
			{
			    if(j + 6 >= resultSize)
				{
				    return -1;
				}
				//           (str[i] & 00011111)<<3+ (str[i+1] & 00111100) >>2
				widechar[0]= ((str[i] & 0xF)<<4) + ((str[i+1] & 0x3C)>>2);
				widechar[1]= ((str[i+1] & 0x3)<<6) + (str[i+2] & 0x3F) ;
				sprintf((char *)&result[j],"%%u%02X%02X",widechar[0],widechar[1]);
				j+=6;
				i+=3;
			}
			else if(charlen==4) //U-00010000 - U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			{
			    if(j + 8 >= resultSize)
				{
				    return -1;
				}
				widechar[0]= ((str[i] & 0x7)<<5) + ((str[i+1] & 0x3E)>>1);
			    widechar[1]= ((str[i+1] & 0x1)<<7) + ((str[i+2] & 0x3F)<<1) + ((str[i+3] & 0x20) >>5);
			    widechar[2]= (str[i+3] & 0x1F)<<3;
				sprintf((char *)&result[j],"%%u%02X%02X%2X",widechar[0],widechar[1],widechar[2]);
				j+=8;
				i+=4;
			}
			else if(charlen==5) //U-00010000 - U-001FFFFF: 111110xx 10xxxxxx, 10xxxxxx 10xx,xxxx 10xxxx,xx
			{
			    if(j + 8 >= resultSize)
				{
				    return -1;
				}
			    widechar[0]= ((str[i] & 0x3)<<6) + ((str[i+1] & 0x3E));
			    widechar[1]= ((str[i+2] & 0x3F)<<2) + ((str[i+3] & 0x30)>>4);
			    widechar[2]= ((str[i+3] & 0xF)<<4)+((str[i+4] & 0x3C)>>2);
				widechar[3]= (str[i+4] & 0x3)<<6;
				sprintf((char *)&result[j],"%%u%02X%02X%2X%02X",widechar[0],widechar[1],widechar[2],widechar[3]);
				j+=8;
				i+=5;
			}
			else if(charlen==6) //U-00010000 - U-001FFFFF: 1111110x 10xxxxxx 10x,xxxxx 10xxx,xxx 10xxxxx,x 10xxxxxx
			{
			    if(j + 8 >= resultSize)
				{
				    return -1;
				}
			    widechar[0]= ((str[i] & 0x1)<<7) + ((str[i+1] & 0x3F)<<2)+((str[i+2] & 0x20)<<2);
			    widechar[1]= ((str[i+2] & 0x1F)<<3) + ((str[i+3] & 0x38)>>3);
			    widechar[2]= ((str[i+3] & 0x7)<<5)+((str[i+4] & 0x3E)>>1);
				widechar[3]= ((str[i+4] & 0x1)<<7) + ((str[i+4] & 0x3F)<<1);
				sprintf((char *)&result[j],"%%u%02X%02X%2X%02X",widechar[0],widechar[1],widechar[2],widechar[3]);
				j+=8;
				i+=6;
			}
			else
			{
			    return -2; //错误的utf-8编码
			}
		}
    }

    result[j] = '\0';
    return j;
}

/************************************************
  *   把字符串进行unescape解码。
  *   输入：
  *     str:               要解码的字符串
  *     strSize:       字符串的长度。
  *     result:         结果缓冲区的地址
  *     resultSize:结果地址的缓冲区大小，可以<=strSize
  *   返回值：
  *     >=0:     result中实际有效的字符长度，
  *     <0:     解码失败，原因是结果缓冲区result的长度太小
  ************************************************/
int unescape(const char * argstr, const int strSize, char * argresult, const int resultSize)
{
    unsigned char   ch,   ch1,   ch2;
    int   i;
    int   j   =   0;   /*   for   result   index   */

	const unsigned char * str=(const unsigned char *)argstr;
	unsigned char *result=(unsigned char *)argresult;


    unsigned char widechar[4];
	int charlen;

	if((str == NULL) || (strSize==0))
	{
	    result[0]=0;
		return 0;
	}

    if ( (result == NULL) || (strSize < 0) || (resultSize <= 0))
	{
        return -1;
    }

    for (i=0; (i<strSize) && (j<resultSize); i++)
	{
        ch   =   str[i];
        switch   (ch)
		{
        case '%':
		    if( i+1 > strSize)
			{
			    result[j++]  =   ch;
				break;
			}
			if(str[i+1]=='u'|| str[i+1]=='U') //Unicode编码 %u7FFB
			{
				if(  (!IsHexChar(str[i+2]))
    			  || (!IsHexChar(str[i+3]))
				  ||(!IsHexChar(str[i+4]))
				  ||(!IsHexChar(str[i+5]))
				  )
				{
				    result[j++]   =   ch;
					break;
				}
				if( i+5 > strSize)
				{
				    result[j++]   =   ch;
					break;
				}

				//是unicode编码   U-00000800 - U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
				widechar[0] = (Char2Num(str[i+2])<<4) + Char2Num(str[i+3]);
				widechar[1] = (Char2Num(str[i+4])<<4) + Char2Num(str[i+5]);
                result[j]= 0xe0 | (widechar[0] >>4);
				j++;
                result[j]= 0x80 | (((widechar[0]) & 0x0F) <<2) + ((widechar[1] & 0xc0)>>6);
				j++;
                result[j]= 0x80 | ((widechar[1]) & 0x3F);
				j++;
				i+=5;
                break;
			}

            if(i+2 < strSize)
			{
                ch1   =   Char2Num(str[i+1]);
                ch2   =   Char2Num(str[i+2]);
                if   ((ch1 != NON_NUM) && (ch2 != NON_NUM))
			    {
                    result[j++]   =   (char)((ch1<<4)   |   ch2);
                    i += 2;
                    break;
                }
            }
			else
			{
			    return -1;
			}

        /* goto default */
        default:
            result[j++]  = ch;
            break;
        }
    }
    result[j] = '\0';
    return j;
}


/************************************************
  *   把字符串进行URL编码。
  *   输入：
  *     str:               要编码的字符串
  *     strSize:       字符串的长度。这样str中可以是二进制数据
  *     result:         结果缓冲区的地址
  *     resultSize:结果地址的缓冲区大小(如果str所有字符都编码，该值为strSize*3)
  *   返回值：
  *     >=0:     result中实际有效的字符长度，
  *      <0:     编码失败，原因是结果缓冲区result的长度太小
  ************************************************/
int URLEncode(const char* str, const int strSize, char* result, const int resultSize)
{
    int   i;
    int   j   =   0;   /*   for   result   index   */
    char   ch;

    if( (result == NULL) || (strSize < 0) || (resultSize <= 0))
    {
        return -1;
    }

	if((str == NULL) || (strSize == 0))
    {
        result[0]=0;
		return 0;
    }


    for(i=0; (i<strSize) && (j<resultSize); i++)
	{
        ch   =   str[i];
        if   ((ch   >=   'A')   &&   (ch   <=   'Z'))
		{
            result[j++]   =   ch;
        }
		else if((ch   >=   'a')   &&   (ch   <=   'z'))
		{
            result[j++]   =   ch;
        }
		else if((ch   >=   '0')   &&   (ch   <=   '9'))
		{
            result[j++]   =   ch;
        }
		else if(ch == ' ')
		{
            result[j++]   =   '+';
        }
		else
		{
            if(j + 3 < resultSize)
			{
                sprintf(result+j, "%%%02X", (unsigned   char)ch);
                j += 3;
            }
			else
			{
                return -1;
            }
        }
    }

    result[j]   =   '\0';
    return j;
}


/************************************************
  *   把字符串进行URL解码。
  *   输入：
  *     str:               要解码的字符串
  *     strSize:       字符串的长度。
  *     result:         结果缓冲区的地址
  *     resultSize:结果地址的缓冲区大小，可以<=strSize
  *   返回值：
  *     >=0:     result中实际有效的字符长度，
  *     <0:     解码失败，原因是结果缓冲区result的长度太小
  ************************************************/
int URLDecode(const   char*   str,   const   int   strSize,   char*   result,   const   int   resultSize)
{
    char   ch,   ch1,   ch2;
    int   i;
    int   j   =   0;   /*   for   result   index   */

	if((str == NULL) || (strSize==0))
	{
	    result[0]=0;
		return 0;
	}

    if ( (result == NULL) || (strSize < 0) || (resultSize <= 0))
	{
        return -1;
    }

    for (i=0;   (i<strSize)   &&   (j<resultSize);   i++)
	{
        ch   =   str[i];
        switch   (ch)
		{
        case '+':
            result[j++]=' ';
            break;
        case '%':
            if(i+2 < strSize)
			{
                ch1   =   Char2Num(str[i+1]);
                ch2   =   Char2Num(str[i+2]);
                if   ((ch1   !=   NON_NUM)   &&   (ch2   !=   NON_NUM))
			    {
                    result[j++]   =   (char)((ch1<<4)   |   ch2);
                    i += 2;
                    break;
                }
            }

        /*   goto   default   */
        default:
            result[j++]   =   ch;
            break;
        }
    }
    result[j] = '\0';
    return j;
}



double time_so_far()
{
#if defined(SysV)
    int        val;
    struct tms tms;

    if ((val = times(&tms)) == -1)
    {
        printf("Call times() error\n");
    }
    return ((double) val) / ((double) sysconf(_SC_CLK_TCK));

#else

    struct timeval tp;

    if (gettimeofday(&tp, (struct timezone *) NULL) == -1)
    {
        printf("Call gettyimeofday error\n");
    }
    return ((double) (tp.tv_sec)) +
           (((double) tp.tv_usec) / 1000000.0);
#endif
}


int SocketRead(int sockfd,char * p,int len)
{
    int n=0;
    int readnums;

    while(n<len)
    {
        readnums = recv(sockfd,&p[n],len -n,0);
        if(readnums < 0)
        {
#ifndef WIN32
            if(errno==EINTR)
            {
                continue;
            }
#endif
            return -1;
        }

        if(readnums==0)
        {
            return 0;
        }

        n+=readnums;
    }
    return n;
}

char * trim(char * s)
{
    if (s==NULL)
    {
        return NULL;
    }

    int i=0;
    int start;
    int iLen=strlen(s);

    for(i=0;i<iLen;i++)
    {
        if(s[i]!=' ' && s[i]!='\t' && s[i]!='\r' && s[i]!='\n')
        {
            break;
        }
    }
    start=i;
    i=iLen-1;
    while((s[i]==' ' || s[i]=='\t' || s[i]=='\r' || s[i]=='\n')
         && i>=0 && i>=start)
    {
        s[i]=0;
        i--;
    }
    return &s[start];
}

void StrToLower(char * s)
{
    int i;
    i=0;
    for(i=0;s[i]!=0;i++)
    {
        s[i] = tolower(s[i]);
    }
}


//获得SQL语句中绑定变量的最大序号
int GetMaxBindIdx(char * szSQL,char pre)
{
    int i;
    int len=strlen(szSQL);
    char szBind[64];
    int BindStart=0;
    int BindIdx;
    int MaxBindIdx=0;
    int n;
    int IsInStr=0;

    for(i=0;i<len;i++)
    {
        if(IsInStr) //在引号中
        {
            if(szSQL[i]=='\'') //遇到结束的引号
            {
                IsInStr=0;
                continue;
            }
            else
            {
                continue;
            }
        }
        else
        {
            if(szSQL[i]=='\'') //遇到结束的引号
            {
                IsInStr=1;
                continue;
            }
        }


        if(BindStart)
        {
            if(szSQL[i]>='0' && szSQL[i]<='9')
            {
                szBind[n]=szSQL[i];
                n++;
            }
            else
            {
                szBind[n]='\0';
                BindIdx=atoi(szBind);
                if(BindIdx>MaxBindIdx)
                {
                    MaxBindIdx=BindIdx;
                }
                BindStart=0;
            }
        }
        if(szSQL[i]==pre)
        {
            BindStart=1;
            n=0;
        }
    }
    return MaxBindIdx;
}


