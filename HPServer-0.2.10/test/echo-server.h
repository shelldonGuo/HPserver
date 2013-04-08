
#ifndef ECHO_SERVERS_H
#define ECHO_SERVERS_H

#include "Connector.h"
#include "SockAcceptor.h"
#include "Reactor.h"

#include <signal.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

using namespace hpsl;

#define BUF_SIZE 128

class CEchoServer;
class CWorker;

/// echo handlers////////////

extern int g_clients_total;
extern int g_clients_pass;

#include <list>

using namespace std;
#ifdef WIN32
#include <Windows.h>
#pragma comment(lib, "WS2_32.lib")

// on windows select can't operate on a pipe, use socketpair instead
int pipe(HL_SOCKET handle[])
{
    int res = CSockUtil::CreateSocketPair(AF_UNIX, SOCK_STREAM, 0, handle);
    return res;
}

int read(HL_SOCKET handle,char buf[], unsigned int count)
{
    return recv(handle, buf, count, 0);
}

int write(HL_SOCKET handle, char buf[], unsigned int count)
{
    return send(handle, buf, count, 0);

}

template <class T>
class CList
{
public:
	CList()
	{
		InitializeCriticalSection(&m_criticalSection);
	}
	~CList()
	{
		m_list.clear();
		DeleteCriticalSection(&m_criticalSection);
	}
	inline void PushBack(const T &t)
	{
		EnterCriticalSection(&m_criticalSection);
		m_list.push_back(t);
		LeaveCriticalSection(&m_criticalSection);
	}
	inline bool PopFront(T &t)
	{
		bool res = false;
		EnterCriticalSection(&m_criticalSection);
		if (m_list.size()>0)
		{
			t = m_list.front();
			m_list.pop_front();
			res = true;
		}
		LeaveCriticalSection(&m_criticalSection);
		return res;
	}
	inline bool Empty()
	{
		return m_list.empty();
	}
	inline size_t size()
	{
		return m_list.size();
	}
private:
	std::list<T> m_list;
	CRITICAL_SECTION m_criticalSection;
};
#else

#include <pthread.h>
template <class T>
class CList
{
public:
	CList()
	{
		pthread_mutex_init(&m_lock, NULL);
	}
	~CList() {m_list.clear();}
	inline void PushBack(const T &t)
	{
		pthread_mutex_lock(&m_lock);
		m_list.push_back(t);
		pthread_mutex_unlock(&m_lock);
	}
	inline bool PopFront(T &t)
	{
		bool res = false;
		pthread_mutex_lock(&m_lock);
		if(m_list.size() > 0)
		{
			t = m_list.front();
			m_list.pop_front();
			res = true;
		}
		pthread_mutex_unlock(&m_lock);
		return res;
	}
	inline bool Empty() {return m_list.empty();}
	inline size_t Size() {return m_list.size();}
private:
	list<T> m_list;

	pthread_mutex_t m_lock;
};
#endif


void send_to_worker(HL_SOCKET fd);

class CSigHandler : public IEventHandler
{
public:
	CSigHandler():IEventHandler(true){}
	virtual ~CSigHandler(){}
	virtual void HandleExit() {delete this;}
	virtual void HandleClose(){delete this;}

	virtual void HandleSignal()
	{
		getReactor()->UnregisterEvent(this, EV_SIGNAL);
		printf("INT signal occurred.\n");
	}
	virtual void HandleTimer()
	{
		exit(3);
	}
	
	virtual void HandleRead(HL_SOCKET handle){}
	virtual void HandleWrite(HL_SOCKET handle){}
};

// server echo worker handler, dynamically created
class CEchoHandler : public IEventHandler
{
public:
	CEchoHandler() : IEventHandler(false), m_lastActive(0), 
	m_rpos(0), m_wpos(0)
	{
		memset(m_buff, 0, sizeof(m_buff));
	}
	virtual ~CEchoHandler(){}
	virtual void HandleExit() {CloseHandle(); delete this;}
	virtual void HandleClose(){CloseHandle(); delete this;}
    virtual void HandleSignal(){}
    virtual void HandleRead(HL_SOCKET handle)
	{
		int res;
		res = recv(GetHandle(), m_buff+m_rpos, sizeof(m_buff)-m_rpos-1, 0);
		if(res > 0)
		{
			m_rpos += res;
			m_buff[m_rpos] = '\0';
			// prepare to send
			if(getReactor()->RegisterEvent(this, EV_WRITE) != 0)
			{
				CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), 
				"failed to register write event.");
				exit(1);
			}
	        m_lastActive = 1;
		}
		else if(res == 0) // remote closed gracefully
		{
			CLog::Log(LL_DEBUG, 0, "remote connection[%d] closed gracefully.\n", GetHandle());
			getReactor()->RemoveHandler(this);
		}
		else
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), 
			"Read[handle=%d] error.\n", GetHandle());
			exit(1);
		}
	}
    virtual void HandleWrite(HL_SOCKET handle)
	{
		int res;
	    res = send(GetHandle(), m_buff+m_wpos, m_rpos-m_wpos, 0);
		if(res < 0)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), 
			"Write[handle=%d] error.\n", GetHandle());
			exit(1);
		}
		m_wpos += res;
		if(m_buff[m_wpos-1] == '$') // send complete
		{
#ifdef WIN32
			shutdown(GetHandle(), SD_BOTH);
#else
			shutdown(GetHandle(), SHUT_RDWR);
#endif
			getReactor()->UnregisterEvent(this, EV_WRITE);
		}
		if(m_wpos >= m_rpos)
	    {
			getReactor()->UnregisterEvent(this, EV_WRITE);
		}
		m_lastActive = 1;
	}
    virtual void HandleTimer()
    {
        if(m_lastActive==0)
        {
			CLog::Log(LL_ERROR, __FILE__, __LINE__, 0,  
			"time out[handle=%d] error.\n", GetHandle());
            exit(5);
        }
        else
        {
            m_lastActive = 0; // clear
        }
    }
private:
	char m_buff[BUF_SIZE];
	int m_rpos, m_wpos;
    long m_lastActive;
};

// server acceptor handler
class CEchoAcceptHandler : public IEventHandler
{
public:
	CEchoAcceptHandler():IEventHandler(false)
	{
	}
	virtual ~CEchoAcceptHandler(){}

    inline int Open(CSockAcceptor *pAcceptor)
    {
		struct timeval tv;
		tv.tv_sec = 120;
		tv.tv_usec = 0;
        m_pAcceptor = pAcceptor;
        if(getReactor()->RegisterTimer(this, &tv) != 0)
        {
            CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "timer registered failed.\n");
			exit(1);
        }
        return getReactor()->RegisterEvent(this, EV_READ);
    }

	virtual void HandleExit() {CloseHandle();}
	virtual void HandleClose(){CloseHandle();}
    virtual void HandleSignal(){}
    virtual void HandleRead(HL_SOCKET handle)
	{
		HL_SOCKET client = m_pAcceptor->Accept(NULL);
		if(client != INVALID_SOCKET)
	    {
	        send_to_worker(client);
		}
	}
    virtual void HandleWrite(HL_SOCKET handle){}
    virtual void HandleTimer(){}
private:	
    CSockAcceptor *m_pAcceptor;
};

// notification handler for worker
class CNotifyHandler : public IEventHandler
{
public:
	CNotifyHandler() : IEventHandler(false), m_pWorker(NULL)
	{
	}
	virtual ~CNotifyHandler(){}
    int Initialize()
    {
		if(pipe(m_pipe) != 0)
		{
			exit(1);
		}
		AttachHandle((HL_SOCKET)m_pipe[0]); // m_pipe[0] is for read event
        return 0;
    }

	void SetWorker(CWorker* pWorker) {m_pWorker = pWorker;}
	void WriteNotification()
	{
		char buf[2];
		buf[0] = 'c', buf[1] = '\0';
		if(write(m_pipe[1], buf, 1) != 1)
		{
			exit(20);
		}
	}
    virtual void HandleClose()
	{
		CSockUtil::CloseSocket(m_pipe[0]);
		CSockUtil::CloseSocket(m_pipe[1]);
	}
    virtual void HandleExit()
	{
		CSockUtil::CloseSocket(m_pipe[0]);
		CSockUtil::CloseSocket(m_pipe[1]);
	}
    virtual void HandleSignal(){}
    virtual void HandleRead(HL_SOCKET handle);
    virtual void HandleWrite(HL_SOCKET handle){}
    virtual void HandleTimer(){}
private:
	CWorker *m_pWorker;
	HL_SOCKET m_pipe[2];
};

// echo server
class CEchoServer
{
public:
	CEchoServer(){}
	~CEchoServer(){}
	
	int Initialize(CInetAddr &addr)
	{
		IReactor_Imp* pReactorImp = NULL;
		do
		{
	        // initialize & register acceptor
	        if(m_sockAcceptor.Open(addr) < 0) break;
			// initialize reactor
#ifdef WIN32
	        pReactorImp = NewReactorImp(REACTOR_SELECT);
#else
	        pReactorImp = NewReactorImp(REACTOR_EPOLL);
#endif
			if(pReactorImp == NULL) break;
			if(m_reactor.Initialize(pReactorImp) != 0) break;
			// set to be the global reactor to handle events
			CReactor::SetSigReactor(&m_reactor);
			
			if(m_reactor.RegisterHandler(m_sockAcceptor.GetHandle(), &m_acceptor) != 0)
				break;
	        if(m_acceptor.Open(&m_sockAcceptor) != 0)
	        {
	            CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "Open Acceptor handler failed.\n");
	            break;
	        }
			
			// register signal
			struct timeval tv;
			tv.tv_sec = 10;
			if(m_reactor.RegisterHandler((HL_SOCKET)SIGINT, &m_sigHandler) != 0)
			{
				CLog::Log(LL_ERROR, 0, "failed to register signal handler, errno=%d\n", errno);
				exit(1);
			}
			m_reactor.RegisterEvent(&m_sigHandler, EV_SIGNAL);
	
	        CLog::Log(LL_NORMAL, 0, "Acceptor is running.,handle:%d\n", m_acceptor.GetHandle());
	        return 0;
		}while(0);

	    return -1;
	}
	int RunForever()
	{
		int res = m_reactor.RunForever();
		return res;
	}
	
	void Stop()
	{
		m_reactor.Stop();
	}
private:
    CSockAcceptor m_sockAcceptor;
	CEchoAcceptHandler m_acceptor;
	CSigHandler  m_sigHandler;
	CReactor m_reactor;
};

bool g_pass;


class CWorker
{
public:
	CWorker(){}
	~CWorker(){}

	int Initialize(IReactor_Imp* pImp)
	{
		if(m_reactor.Initialize(pImp) != 0)
		{
			exit(1);
		}
		// register notification event
        m_notifier.Initialize();
		if(m_reactor.RegisterHandler(&m_notifier) != 0)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, 
			"register notification handler failed.");
			exit(1);
		}
		m_notifier.SetWorker(this);
		m_reactor.RegisterEvent(&m_notifier, EV_READ);
		CLog::Log(LL_NORMAL, __FILE__, __LINE__, 0, "IO method:%s", m_reactor.GetMethod());
		return 0;
	}
	inline int NotifyNewConnection(HL_SOCKET fd)
	{
		m_waitList.PushBack(fd);
		m_notifier.WriteNotification();
		return 0;
	}
	
	inline void OnNotify()
	{
		// handle new connection 
		if(m_waitList.Empty()) 
		{
			return;
		}
		HL_SOCKET handle;
		m_waitList.PopFront(handle);
		if(CSockUtil::SockSetNonBlocking(handle) < 0)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "failed to set nonblocking[handle=%d]", handle);
			exit(21);
		}
		
		IEventHandler *pHandler = new CEchoHandler();
		if(m_reactor.RegisterHandler(handle, pHandler) == 0)
		{
			if(m_reactor.RegisterEvent(pHandler, EV_READ) != 0)
			{
				CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), 
				"failed to register read event!%d!\n", handle);
				exit(10);
			}
			struct timeval tv;
			tv.tv_sec = 120;
			tv.tv_usec = 0;
			if(m_reactor.RegisterTimer(pHandler, &tv) != 0)
			{
				CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), 
				"failed to register timer event~%d~\n", handle);
				exit(10);
			}
		}
		else
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "failed to register handler\n");
			exit(10);
        }
	}
	int RunForever()
	{
		int res = m_reactor.RunForever();
		return res;
	}
	void Stop()
	{
		m_reactor.Stop();
	}
	
private:
	CReactor m_reactor;
	CNotifyHandler m_notifier;
	CList<HL_SOCKET> m_waitList;
};

void CNotifyHandler::HandleRead(HL_SOCKET handle)
{
	char buf[2];
	if(read(m_pipe[0], buf, 1) != 1)
	{
		exit(20);
	}
	m_pWorker->OnNotify();
}
	
#endif
