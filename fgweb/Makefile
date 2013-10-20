#############make file for tangtest###################
PLATFORM=$(shell uname)

PG_INCLUDE=-I$(shell pg_config --includedir)
PG_LIBS= -L$(shell pg_config --libdir) -lpq


#Notice: in AIX, large file support is -D_LARGE_FILE，in Linux is -D_FILE_OFFSET_BITS=64
ifeq ($(PLATFORM),AIX)
    XLCEXISTS=$(shell if which xlC >/dev/null 2>&1 ;then echo 1 ;else echo 0 ;fi;)

    #xlC not exists,use g++
    ifeq ($(XLCEXISTS),0)
        CXX=g++ -static -static-libgcc -D_REENTRANT -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE
        CXXFLAGS=-g -pthread -D$(PLATFORM)
        LIBS=$(PG_LIBS) $(PG_LIBS) -lpthread -lrt
        #LIBS= -static-libgcc -Wl,-bstatic,-bI:/lib/aio.exp,-bI:/lib/crypt.exp -L$(ORACLE_HOME)/lib32 -lclntsh -lm -lpthread /opt/freeware/lib/gcc/powerpc-ibm-aix5.3.0.0/4.2.0/ppc64/libgcc_s.a /lib/libc.a /lib/libcrypt.a /lib/libxcrypt.a /lib/libodm.a
        #LIBS=-maix64 -static-libgcc -lm -lpthread /opt/freeware/lib/gcc/powerpc-ibm-aix5.3.0.0/4.2.0/ppc64/libstdc++.a /opt/freeware/lib/gcc/powerpc-ibm-aix5.3.0.0/4.2.0/ppc64/libgcc_s.a /usr/lib/libcrypt.a
        #CXXFLAGS=-g -maix64 -pthread -D_REENTRANT -Dthread_creds_t='' -D$(PLATFORM)
    else
        #CXX="xlC_r7 -g -O -qthreaded -qstaticinline -qrtti=all"
        CXX=xlc++ -q64 -D_THREAD_SAFE -g -qthreaded -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE
        CXXFLAGS= -D_REENTRANT -D$(PLATFORM)
        LIBS=$(PG_LIBS) -lpthread -lrt
    endif
endif

ifeq ($(PLATFORM),Linux)
	CXX=g++ -g -pthread -D$(PLATFORM) -D_REENTRANT -D_FILE_OFFSET_BITS=64
	MAKE=make
	LIBS= $(PG_LIBS) -lpthread  -L/usr/lib
	CXXFLAGS=
endif

ifeq ($(PLATFORM),SunOS)
	CXX="CC -g -D_SUNWS -D_POSIX_PTHREAD_SEMANTICS -D__EXTENSIONS__  -DUSE_LOG_HEADER"
	CXXFLAGS=$(DEBUG) -D$(PLATFORM)
	LIBS=$(PG_LIBS) -lpthread -lrt -lsocket -lxnet -lposix4 -ldl
	MAKE="gmake -j5"
endif


ifeq ($(PLATFORM),HP)
	CXX=aCC
	CXXFLAGS=-D_AGENT -D_REENTRANT -w -g +p  -DUSE_LOG_HEADER -D$(PLATFORM)
	LIBS=$(PG_LIBS) $(MYSQL_LIBS) -lpthread
	MAKE=make
endif

OBJS_DIR=./objs


#application directory####
APP_DIR:=$(shell pwd)

VPATH=$(APP_DIR)


INCLUDES=\
	-I. \
	$(PG_INCLUDE)

APP_OBJS = fgweb.o \
	CHttpSession.o \
	CHttpThread.o \
	CRdWrtLock.o \
	CReqQueue.o \
	CServiceData.o \
	httpparse.o \
	simplefunc.o



ALL_OBJS=$(APP_OBJS)

all: fgweb
#生成应用#
fgweb:$(ALL_OBJS)
	echo OBJS_DIR=$(OBJS_DIR);cd $(OBJS_DIR);$(CXX) -o $(APP_DIR)/fgweb  $(ALL_OBJS) -L$(OBJS_DIR) $(LIBS)

clean:
	rm $(OBJS_DIR)/*.o; rm fgweb
	
install:
	cp fgweb ../bin/.


#################################################################
.SUFFIXES: .cpp .c .o
.cpp.o:
	@echo Compile file $<,`more $<|wc -l` lines ....
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $(OBJS_DIR)/$*.o

.c.o:
	@echo Compile file $<,`more $<|wc -l` lines ....
	$(CXX) $(INCLUDES) -c $< -o $(OBJS_DIR)/$*.o
################################################################
#################makefile end################################
