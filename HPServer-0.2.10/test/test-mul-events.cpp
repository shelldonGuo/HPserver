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

bool g_pass;

int g_irecv; // pairs test finished
int g_ipair; // total socket pairs in test

// string terminated by $
#define STR_FORMAT "#%d-->%d# simple test strings for multiple read/write test.---$"

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
		if(m_icatchs > 1)
		{
			exit(4);
		}
		getReactor()->UnregisterEvent(this, EV_TIMER);
		getReactor()->UnregisterEvent(this, EV_SIGNAL);
#ifndef WIN32
		alarm(5);
#endif
	}
	virtual void HandleTimer()
	{
		exit(3);
	}
	
	virtual void HandleRead(HL_SOCKET handle){}
	virtual void HandleWrite(HL_SOCKET handle){}
private:
	int m_icatchs;
};

class CReadHandler : public IEventHandler
{
public:
	CReadHandler(): IEventHandler(false), m_iread(0), m_active(false){}
	virtual ~CReadHandler(){}
	
	void SetSendStr(const char *str)
	{
		strncpy(m_send_str, str, sizeof(m_send_str)-1);
	}
	virtual void HandleExit() 
	{
		printf("error, read handler[%d] should be already removed\n", GetHandle());
	}
	virtual void HandleClose(){CloseHandle(); m_active = true; m_iread = 0;}
	
	virtual void HandleSignal(){}
	virtual void HandleWrite(HL_SOCKET handle){}
	
	virtual void HandleRead(HL_SOCKET handle)
	{
		int n = recv(GetHandle(), m_recvbuf+m_iread, 
		sizeof(m_recvbuf)-m_iread-1, 0);
		if(n < 0)
		{
			printf("handle[%d] read error:%d.\n", GetHandle(), errno);
			exit(1);
		}
		m_iread += n;
		//printf("recv:%d,%c\n", n, m_recvbuf[m_iread-1]);
		if(m_recvbuf[m_iread-1]=='$') // recv complete
		{			
			getReactor()->UnregisterEvent(this, EV_READ|EV_TIMER);
			getReactor()->RemoveHandler(this); 
			if(strncmp(m_recvbuf, m_send_str, m_iread) != 0)
			{
				printf("[%s]<>[%s]\n", m_recvbuf, m_send_str);
				exit(2);
			}
			g_irecv++;
			printf(".");
			if(g_irecv == g_ipair)
			{
				getReactor()->Stop();
			}
		}
		m_active = true;
	}
	virtual void HandleTimer()
	{
		// read, write event timeout
		if(!m_active)
		{
			printf("handle[%d] read time out.\n", GetHandle());
			exit(1);
		}
		m_active = false;
	}
private:
	int  m_iread;
	char m_send_str[256];
	bool m_active;
	char m_recvbuf[256];
};

// only call RemoveHandler() in HandleTimer(), so HandleExit() will be called for some handles
class CWriteHandler : public IEventHandler
{
public:
	CWriteHandler(): IEventHandler(false), 
	m_active(true), m_isend(0)
	{
	}
	virtual ~CWriteHandler()
	{
	}
	void SetSendStr(const char *str)
	{
		strncpy(m_sendbuff, str, sizeof(m_sendbuff)-1);
	}
	
	virtual void HandleExit() {CloseHandle(); m_active = true; m_isend = 0;}
	virtual void HandleClose(){CloseHandle(); m_active = true; m_isend = 0;}
	virtual void HandleSignal(){}
	
	virtual void HandleRead(HL_SOCKET handle)
	{
		exit(10);
	}
	
	virtual void HandleWrite(HL_SOCKET handle)
	{
		int len = strlen(m_sendbuff);
		int tmp = len - m_isend;
		if(tmp > 16) tmp = rand()%8+8; // set send size
		int n = send(GetHandle(), m_sendbuff+m_isend, tmp, 0);
		if(n < 0)
		{
			printf("handle[%d] send error:%d.\n", GetHandle(), errno);
			exit(1);
		}
		m_isend += n;
		getReactor()->UnregisterEvent(this, EV_WRITE);
		m_active = true;
	}
	virtual void HandleTimer()
	{
		if(!m_active)
		{
			printf("handle[%d] send time out.\n", GetHandle());
			exit(1);
		}
		m_active = false;
		if(m_isend < strlen(m_sendbuff))
		{
			if(getReactor()->RegisterEvent(this, EV_WRITE) != 0)
			{
				printf("handle[%d] register write event failed.\n", GetHandle());
				exit(5);
			}
		}
		else
		{
			getReactor()->UnregisterEvent(this, EV_TIMER);
			getReactor()->RemoveHandler(this);
		}
	}
private:
	bool m_active;
	int  m_isend;
	char m_sendbuff[256];
};

CReadHandler g_readHandler[PAIR_NUM];
CWriteHandler g_writeHandler[PAIR_NUM];

void TestSimpleEvent(CReactor *pReactor)
{
	char buff[256];
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
			printf("failed to register handler for pair[%d---(%d-%d)\n", ipair, pair[ipair][0], pair[ipair][1]);
			exit(1);
		}
		
		// set send string
		snprintf(buff, sizeof(buff)-1, STR_FORMAT, pair[ipair][1], pair[ipair][0]);
		g_writeHandler[ipair].SetSendStr(buff);
		g_readHandler[ipair].SetSendStr(buff);
		
		tv.tv_sec = 30;
		pReactor->RegisterEvent(&g_readHandler[ipair], EV_READ);
		pReactor->RegisterTimer(&g_readHandler[ipair], &tv);
			
		tv.tv_sec = rand_int(3)+1;
		if(pReactor->RegisterTimer(&g_writeHandler[ipair], &tv) != 0)
		{
			printf("register timer failed, handle=%d\n", pair[ipair][1]);
			exit(5);
		}
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

	printf("test handles %d\n", g_ipair);
	pReactor->RunForever();
	// no exit, test pass
	printf("\ntest complete.\n");
}

int main(int argc, char** argv)
{
	CLog::SetLogLevel(LL_DEBUG);
    CLog::SetLogFile(stdout);
	printf("-----------test mutiple events\n.-----------");
    CReactor reactor;
	CReactor::SetSigReactor(&reactor);
    
#ifdef WIN32
    WSADATA wsadata;
    memset(&wsadata, 0, sizeof(wsadata));
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

	//Initialize 
	IReactor_Imp* pSelectImp = NewReactorImp(REACTOR_SELECT);
	if(pSelectImp == NULL) exit(1);	
    if(reactor.Initialize(pSelectImp) != 0)
		exit(1);
	printf("\nusing select, multiple event test .\n");	
	TestSimpleEvent(&reactor);

#ifndef WIN32	
    IReactor_Imp* pEpollImp = NewReactorImp(REACTOR_EPOLL);
    if(pEpollImp == NULL) exit(1);
    if(reactor.Initialize(pEpollImp) != 0)
		exit(1);
	printf("\nusing epoll, multiple event test .\n");	
	TestSimpleEvent(&reactor);	
#endif    
	//exit(0);
    return 0;
}



