/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#include "EventItem.h"
#include "Reactor_Epoll.h"

using namespace hpsl;

#define MAX_NEVENTS 4096

#ifdef HAVE_SETFD
#define FD_CLOSEONEXEC(x) do{if(fcntl(x, F_SETFD, 1)==-1) printf("fcntl(%d, F_SETFD)", x);}while(0)
#else
#define FD_CLOSEONEXEC(x)
#endif

// @: from lib event
// On Linux kernels at least up to 2.6.24.4, Epoll can't handle timeout
// values bigger than (LONG_MAX - 999ULL)/HZ.  HZ in the wild can be
// as big as 1000, and LONG_MAX can be as small as (1<<31)-1, so the
// largest number of msec we can support here is 2147482.  Let's
// round that down by 47 seconds.
#define MAX_EPOLL_TIMEOUT_MSEC (35*60*1000)

CReactor_Epoll::CReactor_Epoll() : m_epollFD(-1), m_pReactor(NULL), m_pEvents(NULL)
{
}

int CReactor_Epoll::Initialize(CReactor *pReactor)
{
	struct rlimit rl;
    int nfiles = 32000;
	if((getrlimit(RLIMIT_NOFILE, &rl)==0) && (rl.rlim_cur!=RLIM_INFINITY))
	{
		// @: from lib event
		// Solaris is somewhat retarded - 
		// it's important to drop backwards compatibility when making changes.
		// So, don't dare to put rl.rlim_cur here.
		nfiles = rl.rlim_cur - 1;
	}	
	do
	{
		if(pReactor == NULL) break;
		m_pReactor = pReactor;

		// Initialize the kernel queue
		if((m_epollFD = epoll_create(nfiles)) == -1) 
		{
            CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "Failed create epoll.");
            break;
		}

		FD_CLOSEONEXEC(m_epollFD);

		// allocate space for epoll event
		m_pEvents = (struct epoll_event*)malloc(MAX_NEVENTS * sizeof(struct epoll_event));
		if(m_pEvents == NULL) break;
		m_iEvents = MAX_NEVENTS;

		return 0;
	} while(0);

	// initialize failed
	Finalize();

	return -1;
}

int CReactor_Epoll::Finalize()
{
	__Free(m_pEvents);
	if(m_epollFD >= 0)
    {
        close(m_epollFD);
    }
}

int CReactor_Epoll::Dispatch(struct timeval *tv)
{
	int timeout = -1;

	if(tv != NULL) timeout = tv->tv_sec*1000 + (tv->tv_usec+999)/1000;

	if(timeout > MAX_EPOLL_TIMEOUT_MSEC) timeout = MAX_EPOLL_TIMEOUT_MSEC;
	
	// wait for epoll events
	int res = epoll_wait(m_epollFD, m_pEvents, m_iEvents, timeout);

	if(res == -1)
	{
		if(!CSockUtil::ErrTryAgain(errno)) // it's an error
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "epoll wait failed.");
			return (-1);
		}
        res = 0;
    }
    m_pReactor->ProcessSignals();
    //printf("%s: epoll_wait reports %d", __func__, res);

    // active timer event first
    m_pReactor->ExpireTimerEvents();

    // active I/O events
	for(int i = 0; i < res; i++)
	{
		int what = m_pEvents[i].events;
		int events = 0;
		DTEV_ITEM* pEvItem = (DTEV_ITEM*)m_pEvents[i].data.ptr;

		if(what & (EPOLLHUP|EPOLLERR))
		{
			events = EV_READ|EV_WRITE;
		}
		else
		{
			if(what & EPOLLIN) events |= EV_READ;
			if(what & EPOLLOUT) events |= EV_WRITE;
		}
		if(events != 0)
		{
            m_pReactor->ActiveEvent(pEvItem, events);
		}
	}

	return (0);
}

// register event
int CReactor_Epoll::RegisterEvent(const DTEV_ITEM *pEvItem, short event)
{
	int events = 0, op;
	if(pEvItem == NULL)
	{
		return -1;
	}
	HL_SOCKET handle = pEvItem->handle;
	short curevent = pEvItem->events;
	if(curevent&(EV_READ|EV_WRITE))
	{
		op = EPOLL_CTL_MOD;
		if(curevent&EV_READ) events |= EPOLLIN;
		if(curevent&EV_WRITE) events |= EPOLLOUT;
	}
	else
	{
		op = EPOLL_CTL_ADD;
		events = 0;
	}

	if(event & EV_READ) events |= EPOLLIN;
	if(event & EV_WRITE) events |= EPOLLOUT;

	struct epoll_event epev = {0, {0}};
	epev.data.ptr = const_cast<DTEV_ITEM*>(pEvItem);
	epev.events = events;
	if(epoll_ctl(m_epollFD, op, handle, &epev) == -1)
	{
		switch(errno)
		{
			case EEXIST: // already exist
			return 0;
		}
        CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "epoll_ctl [%s] handle[%d], events[%d] failed", 
            (op==EPOLL_CTL_ADD ? "add":"mod"), handle, events);
		return (-1);
    }
	return (0);
}

// unregister event
int CReactor_Epoll::UnregisterEvent(const DTEV_ITEM *pEvItem, short event)
{
	if(pEvItem == NULL)
	{
		return -1;
	}
	HL_SOCKET handle = pEvItem->handle;
	short curevent = pEvItem->events;
	event &= curevent; // filter event, reserve only legitimate events.
	if(event == 0) // specified event isn't registered
    {
        return 0;
    }
	int op = EPOLL_CTL_DEL, events = 0;
	if(event != curevent)
	{
		curevent &= (~event);
		if(curevent&EV_WRITE)
		{
			op = EPOLL_CTL_MOD;
			events = EPOLLOUT; // still need to reserve WRITE event 
		}
		else if(curevent&EV_READ)
		{
			op = EPOLL_CTL_MOD;
			events = EPOLLIN; // still need to reserve READ event
		}
	}

	struct epoll_event epev = {0, {0}};
	epev.data.ptr = const_cast<DTEV_ITEM*>(pEvItem);
    epev.events = events;
    if(epoll_ctl(m_epollFD, op, handle, &epev) == -1)
    {
		if(op == EPOLL_CTL_DEL)
		{
			switch(errno)
			{
				case EBADF: // bad file descriptor
				case ENOENT: // the fd is already removed
				return 0;
			}
		}
        CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "epoll_ctl [%s] handle[%d], events[%d] failed(will ignore error2&9)", 
            (op==EPOLL_CTL_ADD ? "add":"mod"), handle, events);        
         return (-1); 
	}
	return (0);
}


