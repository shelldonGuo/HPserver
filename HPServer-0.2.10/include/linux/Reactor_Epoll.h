/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __REACTOR_EPOLL_H_
#define __REACTOR_EPOLL_H_

#include "defines.h"
#include "SockUtil.h"
#include "Reactor_Imp.h"
#include "Reactor.h"

#include <sys/resource.h>
#include <sys/queue.h>
#include <sys/epoll.h>
#include <stdio.h>
// 
// epoll on Linux
// 
//
// Reactor_Imp MUST implement:
// Initialize()
// Finalize()
// GetMethod()
// RegisterEvent()
// UnregisterEvent()
// Dispatch()

namespace hpsl
{
    class IEventHandler;
	struct DTEV_ITEM;
    class CReactor_Epoll : public IReactor_Imp
	{
	public:
		CReactor_Epoll();
		virtual ~CReactor_Epoll() {}

		// initialize I/O method and set event factory
		virtual int Initialize(CReactor *pReactor);
		virtual int Finalize();
		virtual int Dispatch(struct timeval *tv);
		// get kernel mechanism
		virtual const tchar *GetMethod() {return TEXT("Epoll");}

		// register event read/write
        // @event: EV_READ or EV_WRITE
        virtual int RegisterEvent(const DTEV_ITEM *pEvItem, short event);
        // unregister event read/write
        // @event: EV_READ or EV_WRITE
        virtual int UnregisterEvent(const DTEV_ITEM *pEvItem, short event);
	private:
		CReactor *m_pReactor;
		int m_epollFD;
		struct epoll_event *m_pEvents;
		int m_iEvents;
	};
}


#endif // __REACTOR_EPOLL_H_
