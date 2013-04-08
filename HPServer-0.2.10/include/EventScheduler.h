/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __I_EVENT_SCHEDULER_H_
#define __I_EVENT_SCHEDULER_H_

#include "defines.h"
#include "SockUtil.h"
#include "EventHandler.h"
#include <vector>

//
// event scheduler
//

namespace hpsl
{
	class CReactor;
	struct DTEV_ITEM;
	class IEventScheduler
	{
	public:
		IEventScheduler() : m_pReactor(NULL){}
		virtual ~IEventScheduler(){Finalize();}

		virtual int  Initialize(CReactor *pReactor) {m_pReactor = pReactor; return 0;}
        virtual void Finalize(){}
        // schedule all active events
        // stored in pVecActiveList[size], actually size is MAX_EV_PRIORITY
        // call CReactor::ScheduleActiveEvent() to run an active event
		virtual void ScheduleActiveEvents(std::vector<DTEV_ITEM *> *pVecActiveList, int size) = 0;
	protected:
		CReactor *m_pReactor;
    };

	// default scheduler
    // each schedule loop, the scheduler will only pickup the events from the queue with highest priority
    // 
	class CDefScheduler : public IEventScheduler
	{
	public:
		CDefScheduler(){}
		virtual ~CDefScheduler(){Finalize();}

        virtual int  Initialize(CReactor *pReactor) {return IEventScheduler::Initialize(pReactor);}
        virtual void Finalize()   {}
		virtual void ScheduleActiveEvents(std::vector<DTEV_ITEM *> *pVecActiveList, int size);
    };

}


#endif // __I_EVENT_SCHEDULER_H_

