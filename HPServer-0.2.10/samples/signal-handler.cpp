#include "Reactor.h"
#include <signal.h>
#include <unistd.h>

using namespace hpsl;

class CSingalHandler: public IEventHandler
{
public:
    CSingalHandler():IEventHandler(true)
    {
        m_count = 0;
    }
    virtual ~CSingalHandler(){}
	
	virtual void HandleExit() {delete this;}
	virtual void HandleClose(){delete this;}
	
    virtual void HandleSignal();
    virtual void HandleRead(HL_SOCKET handle){}
    virtual void HandleWrite(HL_SOCKET handle){}
    virtual void HandleTimer(){}
private:
    int m_count;
};

void CSingalHandler::HandleSignal()
{
    m_count++;
    CLog::Log(LL_NORMAL, 0, "signal[%d] captured %d times", GetHandle(), m_count);
	if(GetHandle() == SIGINT)
	{
		CLog::Log(LL_NORMAL, 0, "INT signal occurred, exit.");
		exit(0);
	}
#ifndef WIN32
	if(GetHandle() == SIGALRM)
	{
		alarm(rand()%5+1);
	}
#endif
}

int main(int argc, char** argv)
{
    CReactor reactor;
	
    CLog::SetLogFile(stdout);
	CLog::Log(LL_NORMAL, 0, "reactor initialized ok.");
    
	//Initialize 
    IReactor_Imp* pReactorImp = NewReactorImp(REACTOR_SELECT);
    if (pReactorImp==NULL)exit(1);
	
    if (reactor.Initialize(pReactorImp))
        exit(1);
    
	CReactor::SetSigReactor(&reactor);

    CSingalHandler* intHandler = new CSingalHandler();
	reactor.RegisterHandler((HL_SOCKET)SIGINT, intHandler);
    reactor.RegisterEvent(intHandler, EV_SIGNAL);
	
#ifndef WIN32
    CSingalHandler* alarmHandler = new CSingalHandler();
	reactor.RegisterHandler((HL_SOCKET)SIGALRM, alarmHandler);
    alarm(2);
    reactor.RegisterEvent(alarmHandler, EV_SIGNAL);
#endif

    reactor.RunForever();
    return 0;
}



