#include "Reactor.h"
#include <signal.h>
#include <string.h>

using namespace hpsl;

#ifdef WIN32
#pragma comment(lib, "WS2_32.lib")
#define PAIR_NUM 30

#else

#define PAIR_NUM 512
#endif

// 1. wait for write timer expire,
// 2. then register write event
// 3. write complete, unregister write event
// 4. read complete, test finish
// 5. test finish

bool g_pass;

int g_irecv;
int g_ipair;

static int rand_int(int n)
{
	return (int)(rand() % n);
}

class CSigHandler : public IEventHandler
{
public:
	CSigHandler():IEventHandler(true), m_icatchs(0){}
	virtual ~CSigHandler(){}
	
	virtual void HandleExit() {delete this;}
	virtual void HandleClose(){delete this;}

	virtual void HandleSignal()
	{
		++m_icatchs;
		if(m_icatchs == 2)
		{
			exit(3);
		}
		getReactor()->UnregisterEvent(this, EV_TIMER|EV_SIGNAL);
#ifndef WIN32
		alarm(5);
#endif
	}
	virtual void HandleTimer()
	{
		exit(4);
	}
	
	virtual void HandleRead(HL_SOCKET handle){}
	virtual void HandleWrite(HL_SOCKET handle){}
private:
	int m_icatchs;
};

class CReadHandler : public IEventHandler
{
public:
	CReadHandler(): IEventHandler(false)
	{}
	virtual ~CReadHandler()
	{}
	virtual void HandleExit() 
	{
		printf("error, handler[%d] should be already removed\n", GetHandle());
	}
	virtual void HandleClose(){CloseHandle();}
	virtual void HandleSignal(){}
	virtual void HandleWrite(HL_SOCKET handle){}
	virtual void HandleRead(HL_SOCKET handle)
	{
		char buff[256];
		int n = recv(GetHandle(), buff, sizeof(buff)-1, 0);
		if(n < 0)
		{
			printf("handle[%d] read error:%d.\n", GetHandle(), errno);
			exit(100);
		}
		g_irecv++;
		printf(".");
		getReactor()->RemoveHandler(this); 
		if(g_irecv == g_ipair)
		{
			getReactor()->Stop();
		}
	}
	virtual void HandleTimer()
	{
		// read, write event timeout
		printf("handle[%d] time out.\n", GetHandle());
		exit(10);
	}
};


class CWriteHandler : public IEventHandler
{
public:
	CWriteHandler(): IEventHandler(false)
	{
		m_willwrite = 0;
	}
	virtual ~CWriteHandler()
	{
	}
	virtual void HandleExit()
	{
		printf("error, handler[%d] should be already removed\n", GetHandle());
	}
	virtual void HandleClose(){CloseHandle(); m_willwrite = 0;}
	virtual void HandleSignal(){}
	
	virtual void HandleRead(HL_SOCKET handle){}
	virtual void HandleWrite(HL_SOCKET handle)
	{
		char buff[256];
		snprintf(buff, sizeof(buff)-1, "#%d# simple test string.", GetHandle());
		int n = send(GetHandle(), buff, strlen(buff), 0);
		if(n < 0)
		{
			printf("handle[%d] send error:%d.\n", GetHandle(), errno);
			exit(101);
		}
		getReactor()->RemoveHandler(this);
	}
	virtual void HandleTimer()
	{
		if(m_willwrite==0)
		{
			m_willwrite=1;
			getReactor()->RegisterEvent(this, EV_WRITE);
		}
		else
		{
			// read, write event timeout
			printf("handle[%d] time out.\n", GetHandle());
			exit(11);
		}
	}
private:
	int m_willwrite;
};

CReadHandler g_readHandler[PAIR_NUM];
CWriteHandler g_writeHandler[PAIR_NUM];

void TestSimpleEvent(CReactor *pReactor)
{
	HL_SOCKET pair[PAIR_NUM][2];
    int ipair;
	struct timeval tv;
	tv.tv_usec = rand_int(900)+100;

	g_pass = false;
	// creat socket pairs
	for(ipair = 0; ipair < PAIR_NUM; ipair++)
	{
		if(CSockUtil::CreateSocketPair(AF_UNIX, SOCK_STREAM, 0, pair[ipair]) == -1)
		{
			break;
		}
		if(CSockUtil::SockSetNonBlocking(pair[ipair][0]) != 0) exit(6);
		if(CSockUtil::SockSetNonBlocking(pair[ipair][1]) != 0) exit(6);

		int res = 0;
		// se handle & register handler
		res += pReactor->RegisterHandler(pair[ipair][0], &g_readHandler[ipair]);
		res += pReactor->RegisterHandler(pair[ipair][1], &g_writeHandler[ipair]);
		if(res != 0)
		{
			printf("failed to register handler for pair[%d]---(%d-%d)\n", ipair, pair[ipair][0], pair[ipair][1]);
			exit(1);
		}
		// register read event of read handler
		tv.tv_sec = 120;
		pReactor->RegisterEvent(&g_readHandler[ipair], EV_READ);
		pReactor->RegisterTimer(&g_readHandler[ipair], &tv);
		// register timer event of write handler
		tv.tv_sec = rand_int(10)+5;
		pReactor->RegisterTimer(&g_writeHandler[ipair], &tv);
	}
	g_ipair = ipair;
	g_irecv = 0;
	
#ifndef WIN32
	// register signal		
	tv.tv_sec = 10;
	CSigHandler *pSigHandler = new CSigHandler();
	pSigHandler->AttachHandle((HL_SOCKET)SIGALRM);	
	if(pReactor->RegisterHandler(pSigHandler) != 0)
	{
		printf("failed to register signal handler, errno=%d\n", errno);
		exit(1);
	}
	pReactor->RegisterEvent(pSigHandler, EV_SIGNAL);
	pReactor->RegisterTimer(pSigHandler, &tv);
	
	alarm(5);
#endif

    printf("test pairs: %d\n", g_ipair);
	pReactor->RunForever();
	// no exit, test pass
	printf("\ntest complete.\n");
}

int main(int argc, char** argv)
{
	//CLog::SetLogLevel(LL_DETAIL);
    CLog::SetLogFile(stdout);
    printf("-----------test simple events\n.-----------");
#ifdef WIN32
    WSADATA wsadata;
    memset(&wsadata, 0, sizeof(wsadata));
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

    CReactor reactor;
	CReactor::SetSigReactor(&reactor);

	//Initialize 
	IReactor_Imp* pSelectImp = NewReactorImp(REACTOR_SELECT);
	if(pSelectImp == NULL) exit(1);	
    if(reactor.Initialize(pSelectImp) != 0)
		exit(1);
	printf("\nusing select, simple event test .\n");	
	TestSimpleEvent(&reactor);
	
#ifndef WIN32
    IReactor_Imp* pEpollImp = NewReactorImp(REACTOR_EPOLL);
    if(pEpollImp == NULL) exit(1);
    if(reactor.Initialize(pEpollImp) != 0)
		exit(1);
	printf("\nusing epoll, simple event test .\n");	
	TestSimpleEvent(&reactor);	
#endif
	//exit(0);
    return 0;
}



