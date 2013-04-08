/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#include "SignalInfo.h"
#include "Reactor.h"
#include "InternalHandlers-Internal.h"

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>


#ifdef HAVE_SETFD
#define FD_CLOSEONEXEC(x) do{if(fcntl(x,F_SETFD,1)==-1)printf("fcntl(%d,F_SETFD,1)",x);}while(0)
#else
#define FD_CLOSEONEXEC(x)
#endif

using namespace hpsl;

CReactor *g_sigReactor = NULL;

int CSignalInfo::Initialize(CReactor* pReactor)
{
	if(pReactor==NULL)
	{
		return -1;
	}
	m_pReactor = pReactor;
	do
	{
		for (int i = 0; i < NSIG; ++i)
		{
			m_signalHandler[i].clear();
		}
		
		memset(m_isigCaught, 0, NSIG * sizeof(sig_atomic_t));

#ifdef HAVE_SIGACTION
		memset(m_sigHandler_old, 0, NSIG*sizeof(struct sigaction*));
#else
		memset(m_sigHandler_old, 0, NSIG*sizeof(sighandler_t));
#endif	
		
		int res = CSockUtil::CreateSocketPair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
		if(res)
		{
			break;
		}
		FD_CLOSEONEXEC(m_socketPair[0]);
		FD_CLOSEONEXEC(m_socketPair[1]);
		if(CSockUtil::SockSetNonBlocking(m_socketPair[0]) < 0)
		{
			CLog::Log(LL_ERROR,__FILE__,__LINE__, ERRNO(), 
			"fcnlt(%d, F_SETFL, O_NONBLOCK) failed",m_socketPair[0]);
			break;
		}		
		return 0;
	}while(0);
	Finalize();
	return -1;
}

int CSignalInfo::Finalize()
{
	CSockUtil::CloseSocket(m_socketPair[0]);
	m_socketPair[0] = -1;
	CSockUtil::CloseSocket(m_socketPair[1]);
	m_socketPair[1] = -1;
	memset(m_isigCaught, 0, NSIG * sizeof(sig_atomic_t));
	//unregister all registered event
	for(int i = 0; i < NSIG; ++i)
	{
		while(!m_signalHandler[i].empty())
		{
			UnregisterSignalEvent(*(m_signalHandler[i].begin()));
		}
	}

#ifdef HAVE_SIGACTION
	memset(m_sigHandler_old, 0, NSIG*sizeof(struct sigaction*));
#else
	memset(m_sigHandler_old, 0, NSIG*sizeof(sighandler_t));
#endif
	return 0;
}

int CSignalInfo::RegisterSignalEvent(const DTEV_ITEM* pEvItem)
{
	if (pEvItem->events &(EV_READ|EV_WRITE))
	{
		CLog::Log(LL_WARN, __FILE__, __LINE__, ERRNO(), "Can not register EV_READ|EV_WRITE event");
		return -1;
	}
    HL_SOCKET handle = __TO_HANDLER(pEvItem)->GetHandle();
	if ( (handle<0)||(handle>=NSIG))return -1;
	if (setSignalHandler(handle))
	{
		return -1;
	}
	m_signalHandler[handle].insert(m_signalHandler[handle].end(), const_cast<DTEV_ITEM*>(pEvItem));
	return 0;
}

int CSignalInfo::UnregisterSignalEvent(DTEV_ITEM* pEvItem)
{
	if (pEvItem->events &(EV_READ|EV_WRITE))
	{
		CLog::Log(LL_WARN, __FILE__, __LINE__, ERRNO(), "Cant not unregister EV_READ|EV_WRITE event");
	}
	HL_SOCKET handle = __TO_HANDLER(pEvItem)->GetHandle();
	if ((handle<0)||(handle>=NSIG)) return -1;
	if (restoreSignalHandler(handle))
		return -1;
	m_signalHandler[handle].remove(pEvItem);
	return 0;
}
int CSignalInfo::ProcessSignal()
{
	HL_SOCKET handle = INVALID_SOCKET;
	for(int i = 0; i < NSIG; ++i)
	{
		if(m_isigCaught[i] > 0)
		{
			std::list<DTEV_ITEM *>::iterator iter = m_signalHandler[i].begin();
			for(iter; iter!=m_signalHandler[i].end(); ++iter)
			{
				handle = __TO_HANDLER(*iter)->GetHandle();
				(*iter)->nsigs = m_isigCaught[i];
				m_pReactor->ActiveEvent(*iter, EV_SIGNAL);
			}
			m_isigCaught[i] = 0;
		}
	}
	return 0;
}

int CSignalInfo::setSignalHandler(int signum)
{
#ifdef HAVE_SIGACTION
	struct sigaction sa;
	struct sigaction sa_old;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_handler;
	sa.sa_flags|=SA_RESTART;
	sigfillset(&sa.sa_mask);
	if (sigaction(signum, &sa, &sa_old)==-1)
	{
		CLog::Log(LL_WARN,__FILE__,__LINE__,ERRNO(),"invoke sigaction to set signal handler error!");
		return -1;
	}
	//m_sigHandler_old[signum] should store originial handler 
	if (m_sigHandler_old[signum]==NULL) 
	{
		m_sigHandler_old[signum] = (struct sigaction*)malloc(sizeof(struct sigaction));
		if (m_sigHandler_old[signum]==NULL)
		{
			CLog::Log(LL_ERROR,__FILE__,__LINE__,ERRNO(),"malloc error!");
			return -1;
		}
		memcpy(m_sigHandler_old[signum], &sa_old, sizeof(struct sigaction));
	}
#else
	sighandler_t sig_old = signal(signum, sig_handler);	
	if (sig_old==SIG_ERR)
		return -1;
	//m_sigHandler_old should store original handler.
	if (m_sigHandler_old[signum]==NULL)
	{	
		m_sigHandler_old[signum] = sig_old;
	}
#endif
	return 0;
}

int CSignalInfo::restoreSignalHandler(int signum)
{
	if(m_signalHandler[signum].empty() && (m_sigHandler_old[signum]!=NULL))
	{
#ifdef HAVE_SIGACTION
		if(sigaction(signum, m_sigHandler_old[signum], NULL)==-1)
		{
			CLog::Log(LL_WARN,__FILE__,__LINE__,"sigaction");
			return -1;
		}
		free(m_sigHandler_old[signum]);
#else
		sighandler_t sh = signal(signum, m_sigHandler_old[signum]);
		if(sh==SIG_ERR)
		{
			CLog::Log(LL_WARN, __FILE__, __LINE__, ERRNO(),"signal");
			return -1;
		}			
#endif
		m_sigHandler_old[signum] = NULL;
	}
	return 0;
}

void CSignalInfo::sig_handler(int signum)
{
    //record occur counts of signum
	if(signum>=0 && signum<NSIG)
	{
		if(g_sigReactor != NULL)
		{
			CSignalInfo *sigInfo = g_sigReactor->GetSigInfo();
			if(sigInfo != NULL)
			{
				sigInfo->m_isigCaught[signum]++;
				char buf[2] = "s";
				send(sigInfo->m_socketPair[0], buf, 1, 0);
			}
			else
			{
				CLog::Log(LL_ERROR,__FILE__,__LINE__, ERRNO(), 
				"signal caught, global signal reactor isn't initialized!");
			}
		}
		else
		{
			CLog::Log(LL_ERROR,__FILE__,__LINE__, ERRNO(), 
			"signal caught, global signal reactor is NULL!");
		}
	}
}
