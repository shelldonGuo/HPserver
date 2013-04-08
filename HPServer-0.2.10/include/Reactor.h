/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __REACTOR_H_
#define __REACTOR_H_

#include "HP_Config.h"
#include "defines.h"
#include "SockUtil.h"
#include "TimeUtil.h"
#include "Log.h"
#include "EventHandler.h"
#include "EventScheduler.h"
#include "Reactor_Imp.h"

#include <stdio.h>
#include <vector>

//
// MUST first register the event handler, next register read/write/timer/signal events of the handler
// Reactor takes charges of Timer & Signal event, and leaves Read/Write event to Reactor_Imp, 
// which is the real I/O demultiplex method implementation class
//

#define LOOP_INFINITE -1

namespace hpsl
{
	class CDemuxTable;
	class CSignalSet;
	class CMinHeap;
	class CSignalInfo;
	struct DTEV_ITEM;
    // CReactor 是dispatcher
    // 用于注册、删除handler
    // 根据事件分发handler进行处理
	class CReactor
	{
	public:
		CReactor() : m_pImp(NULL), m_pScheduler(&m_defScheduler), 
			m_bRunning(false), m_bNeedStop(false)
        {
            m_defScheduler.Initialize(this); // set default scheduler
        }
		~CReactor(){} // do not inherent from the class

		// set the global reactor to handle signal events
		static void SetSigReactor(CReactor *pReactor);
		
		// initialize I/O method and set event factory
		int Initialize(IReactor_Imp *pImp);
		int Finalize();
		inline int SetEventScheduler(IEventScheduler *pScheduler);

        // 循环检测需要处理的事件
        // 调用 eventLoopOnce
		int RunForever(int loops = LOOP_INFINITE);
		// the Stop can only take effect in an event handler
		inline void Stop()		{m_bNeedStop = true;}
		inline bool IsRunning() {return m_bRunning;}
		// get kernel mechanism
		inline const tchar *GetMethod() {return m_pImp->GetMethod();}

		// get handlers number
		size_t GetHandlerNumber();

        // register a new event handler , and bind @handle to @pHandler, for signal handler, signum = (int)handle; 
		// @handle : file descripter or signum
		// @pHandler : the handler to register
		// NOTE:
		// an I/O event handler can be associated with multiple handles
		//     While, a handle can't be associated with multiple event handlers
		// a signal event handler can be associated with ONLY ONE handle(signum)
		//     While, a signum can be associated with multiple signal event handlers
		// if register success and pHandler's handle < 0, then pHandler->AttachHandle(handle) is invoked
        // 参数应该是一个Event_Handler和一个Event_Type
		int RegisterHandler(HL_SOCKET handle, IEventHandler *pHandler);

		// register a new event handler, and bind @pHandler->GetHandle()  to @pHandler
		// @pHandler: the handler to register
		inline int RegisterHandler(IEventHandler *pHandler);
		
		// remove the I/O or signal event handler,  this will remove all events of @pHandler->GetHandle() too
		// the handler's ref number will be decreased, if decreased to 0, then IEventHandler::HandleClose() will be invoked.
		inline int RemoveHandler(IEventHandler* pHandler);

		// remove the I/O event handler associated by handle,  this will remove all events of handle too.
		// the handler's ref number will be decreased, if decreased to 0, then IEventHandler::HandleClose() will be invoked.
		int	       RemoveHandler(HL_SOCKET handle);

		// register I/O event of handle
		// @event: EV_READ, EV_WRITE
		int RegisterEvent(HL_SOCKET handle, short event);

		// register I/O or signal event, 
		// @event: 1 EV_READ, EV_WRITE
		//              2 EV_SIGNAL
		inline int RegisterEvent(IEventHandler* pHandler, short event);

		// register timer event of an event handler
        int RegisterTimer(IEventHandler *pHandler, struct timeval *ptv);
		
		// unregister I/O events of handle
        // @event: can be EV_READ, EV_WRITE, EV_TIMER
		int UnregisterEvent(HL_SOCKET handle, short event);

		// unregister I/O or signal event
		// @event: 1 EV_READ, EV_WRITE, EV_TIMER
		//              2 EV_SIGNAL, EV_TIMER
		inline int UnregisterEvent(IEventHandler* pHandler, short event);
		
		// get I/O event handler from handle
		IEventHandler* Handle2EventHandler(HL_SOCKET handle);
		
		static IEventHandler* ToEventHandler(DTEV_ITEM* pEvItem);
        // get I/O events of handle
        int GetEvents(HL_SOCKET handle, short &event, short &evActive);
		// get events of handler
        int GetEvents(IEventHandler* pHandler, short &event, short &evActive); 

		// active timer, and called by SignalInfo, IReactor_Imp to active events
		int ActiveEvent(DTEV_ITEM *pEvItem, short event);

        // called by IReactor_Imp to active expired timer events
        // SHOULD call this before I/O events to process timer events ASAP.
        void ExpireTimerEvents();
        // called by IReactor_Imp to active signal events
        // SHOULD call this before I/O events to process signal events
        int ProcessSignals();
		// called by Scheduler to schedule an active event
		void ScheduleActiveEvent(DTEV_ITEM *pEvItem);
		
		// called by CSignalInfo
		CSignalInfo* GetSigInfo();
	private:
		int removeSignalHandler(IEventHandler* pHandler);		
		int removeHandler(IEventHandler *pHandler);
		
		int registerSignal(IEventHandler* pHandler);
		int unregisterSignal(IEventHandler* pHandler, short event);
        
		DTEV_ITEM *handler2DtevItem(IEventHandler *pHandler);
		int  createInternalHandlers();
        // event loop once
        // 实际执行事件检测
        // 一直循环执行
		int eventLoopOnce();
        
        // correct timer, user may change system's time manually
		// @now: current time, if time is corrected, will be updated to the latest time
        void correctTimer(struct timeval &now);
		
		// get time
		void getTime(struct timeval &t);
		
		// child-class can't access these variables directly
        // 实际的dispatcher, 可以是select或epoll实现
		IReactor_Imp *m_pImp; // real implementation
        // current active events in each loop
        std::vector<DTEV_ITEM *> m_vecActiveList[MAX_EV_PRIORITY];
        int m_activeHandlers;
		// event scheduler
		IEventScheduler *m_pScheduler;
        CDefScheduler    m_defScheduler;
		// all handlers
        // 相当于保存所有event handle的表
        // 这里分别使用了几个不同的表来保存handle
		CDemuxTable   *m_tabIoHandlers;
        CSignalSet    *m_setSigHandlers;
        CMinHeap      *m_timerHeap;
        CSignalInfo   *m_sigInfo;

        struct timeval m_time; // last dispatch return time
        struct timeval m_timeCache; // caches dispatch return time

        volatile bool m_bRunning, m_bNeedStop;
	};
    
	inline int CReactor::SetEventScheduler(IEventScheduler *pScheduler)
	{
		if(pScheduler != NULL)
		{
			m_pScheduler = pScheduler;
			return 0;
		}
		return -1;
    }
	inline int CReactor::RegisterHandler(IEventHandler *pHandler)
	{
		if(pHandler != NULL)
		{
			return RegisterHandler(pHandler->GetHandle(), pHandler);
		}
        return -1;
	}
	inline int CReactor::RemoveHandler(IEventHandler* pHandler)
	{
		if(pHandler != NULL)
		{
			if(pHandler->IsSignalHandler()) //  is signal handler
			{
				return removeSignalHandler(pHandler);
			}
			// I/O event handler
			return RemoveHandler(pHandler->GetHandle());
		}
        return -1;
	}
	inline int CReactor::RegisterEvent(IEventHandler* pHandler, short event)
	{
		if(pHandler != NULL)
		{
			if(pHandler->IsSignalHandler()) //  is signal handler
			{
				if(event&EV_SIGNAL) return registerSignal(pHandler);
				return -1;
			}
			// I/O event handler
			return RegisterEvent(pHandler->GetHandle(), event);
		}
		return -1;
	}
	inline int CReactor::UnregisterEvent(IEventHandler* pHandler, short event)
	{
		if(pHandler != NULL)
		{
			if(pHandler->IsSignalHandler()) //  is signal handler
			{
				return unregisterSignal(pHandler, event);
			}
			// I/O event handler
			return UnregisterEvent(pHandler->GetHandle(), event);
		}
		return -1;
	}

}


#endif // __REACTOR_H_
