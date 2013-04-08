#include "Reactor.h"
#include <signal.h>

using namespace hpsl;

#define EH_NUM 1024*32
#ifdef WIN32
#pragma comment(lib, "WS2_32.lib")
#endif

bool g_pass;
int  g_expired_timers, g_max_precise, g_min_precise;

// beacause timer handler must consume  a handle or signo
// so here we register timer in many SIGALRM handlers
// but doen't register EV_SIGNAL at all
class CSigHandler : public IEventHandler
{
public:
	CSigHandler():IEventHandler(true)
	{}
	virtual ~CSigHandler(){}
	
	void StartTimer(){CTimeUtil::GetSysTime(&m_tv);}
	void SetPeriod(struct timeval *ptv) 
	{
		m_period = ptv->tv_sec*1000 + ptv->tv_usec/1000;
	}
	
	virtual void HandleClose(){delete this;}
	virtual void HandleExit(){delete this;}
	
	virtual void HandleSignal(){}
	virtual void HandleTimer()
	{
		struct timeval tv, tmp;
		CTimeUtil::GetSysTime(&tv);
		CTimeUtil::TimeSub(&tv, &m_tv, &tmp);
		
		int diff = tmp.tv_sec*1000 + tmp.tv_usec/1000 - m_period;
		if(diff < 0) diff = -diff;
		
		if(diff >= 200)
		{
			printf("timer precision too large.%d\n", diff);
			exit(4);
		}
		if(g_max_precise < diff) 
		{
			g_max_precise = diff;
		}
		if(g_min_precise > diff) 
		{
			g_min_precise = diff;
		}
		int t = rand()%1000;		
		if(t<50)
		{
			struct timeval tv;
			tv.tv_sec = rand()%5+1;
			tv.tv_usec = rand()%900000;
			SetPeriod(&tv);
			StartTimer();
			getReactor()->RegisterTimer(this, &tv);
		}
		else
		{
			getReactor()->UnregisterEvent(this, EV_TIMER);
			g_expired_timers++;
			if(g_expired_timers%100 == 0) printf(".");
			if(g_expired_timers == EH_NUM)
			{
				getReactor()->Stop();
			}
		}
	}
	
	virtual void HandleRead(HL_SOCKET handle){}
	virtual void HandleWrite(HL_SOCKET handle){}
private:
	struct timeval m_tv;
	int m_period;
};

void TestSimpleEvent(CReactor *pReactor)
{
	g_pass = false;
	g_expired_timers = 0;
	g_max_precise = 0;
    g_min_precise = 1000000;
	// creat socket pairs
	for(int i = 0; i < EH_NUM; i++)
	{
		struct timeval tv;
		tv.tv_sec = rand()%5+1;
		tv.tv_usec = rand()%500000;
		// register signal
		CSigHandler *pSigHandler = new CSigHandler();
		pReactor->RegisterHandler(SIGINT, pSigHandler);
		
		pSigHandler->StartTimer();
		pSigHandler->SetPeriod(&tv);
		pReactor->RegisterTimer(pSigHandler, &tv);
	}
	pReactor->RunForever();
	// no exit, test pass
	printf("\ntimer test pass, timer precision range[%d--%d]ms.\n", 
        g_min_precise, g_max_precise);
}

int main(int argc, char** argv)
{
    CReactor reactor;
    printf("-----------test timer\n.-----------");
#ifdef WIN32
    WSADATA wsadata;
    memset(&wsadata, 0, sizeof(wsadata));
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

	CLog::SetLogLevel(LL_NORMAL);
	CLog::SetLogFile(stdout);
    //Initialize 
	CReactor::SetSigReactor(&reactor);
	
	IReactor_Imp* pSelectImp = NewReactorImp(REACTOR_SELECT);
	if(pSelectImp == NULL) exit(1);	
    if(reactor.Initialize(pSelectImp) != 0)
		exit(1);
	printf("timer test, using select, timer number[%d].\n", EH_NUM);
	TestSimpleEvent(&reactor);	
	
#ifndef WIN32	
    IReactor_Imp* pEpollImp = NewReactorImp(REACTOR_EPOLL);
    if(pEpollImp == NULL) exit(1);
    if(reactor.Initialize(pEpollImp) != 0)
		exit(1);
	printf("timer test, using epoll, timer number[%d].\n", EH_NUM);
	TestSimpleEvent(&reactor);	
#endif
    return 0;
}



