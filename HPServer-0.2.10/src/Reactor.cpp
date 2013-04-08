/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#include "Reactor.h"
#include "InternalHandlers-Internal.h"
#include "DemuxTable.h"
#include "SignalSet.h"
#include "MinHeap.h"
#include "SignalInfo.h"

using namespace hpsl;

#define __Finalize(p, bInitialized) do{	\
	if(p != NULL)						\
	{									\
		if(bInitialized) p->Finalize();	\
		delete p; p = NULL;				\
	}									\
}while(0)

// the global Reactor for signals
extern CReactor *g_sigReactor;

// initialize I/O method and set event factory
int CReactor::Initialize(IReactor_Imp *pImp)
{
    // set implementation & event handler factory
    if(pImp == NULL)
    {
        return -1;
    }
    // 用 Reactor_Imp来初始化本类
    // 后边还会用本类来初始华Reactor_Imp，两个类交叉
    m_pImp = pImp;

	// check whether it's monotonic time
	CTimeUtil::DetectMonotonic();
    bool bIoOk = false, bSigOk = false, bTimerOk = false;
	bool bImpOk = false, bSigInfoOk = false;
	do
	{
		// initialize IO handler tables
		m_tabIoHandlers = new CDemuxTable();
		if((m_tabIoHandlers==NULL) || (m_tabIoHandlers->Initialize()!=0))
		{
			break;
		}
		bIoOk = true;
		// initialize signal handler tables
		m_setSigHandlers = new CSignalSet();
		if((m_setSigHandlers==NULL) || (m_setSigHandlers->Initialize()!=0))
		{
			break;
		}
		bSigOk = true;
		// initialize timer heap
		m_timerHeap = new CMinHeap();
		if((m_timerHeap==NULL) || (m_timerHeap->Initialize()!=0))
		{
			break;
		}
		bTimerOk = true;
        // Reactor_Imp又通过本类来进行初始化
        // 两个类交叉严重啊
		// initialize i/o method
		if(m_pImp->Initialize(this) != 0)
		{
			break;
		}
		bImpOk = true;
		//initialize signal handler method
		m_sigInfo = new CSignalInfo();
		if((m_sigInfo==NULL) || (m_sigInfo->Initialize(this)!=0))
		{
			break;
		}
		bSigInfoOk = true;
		// create internal used handlers
		if(createInternalHandlers() != 0)
		{
			break;
		}
		
		for(int i = 0; i < MAX_EV_PRIORITY; i++)
		{
			m_vecActiveList[i].clear();
			m_vecActiveList[i].reserve(16);
		}
		m_vecActiveList[MAX_EV_PRIORITY>>1].reserve(128); // it's the default priority

		m_bRunning = m_bNeedStop = false;
		CTimeUtil::GetSysTime(&m_time);
		CTimeUtil::TimeClear(&m_timeCache);
		return 0;
	}while(0);

	// failed, release
	__Finalize(m_tabIoHandlers, bIoOk);
	__Finalize(m_setSigHandlers, bSigOk);
	__Finalize(m_timerHeap, bTimerOk);
	__Finalize(m_sigInfo, bSigInfoOk);
	__Finalize(m_pImp, bImpOk);

    return -1;
}

int CReactor::RunForever(int loops)
{
    int res = 0;
    m_bRunning = true;		
	m_activeHandlers = 0;
    // 如果 loops < 0就会一直循环
    // 利用了loops >=0的值
    while(!m_bNeedStop)
    {
		// check loop
		if(loops == 0)
		{
			m_bNeedStop = true;
			break;
		}
        res = eventLoopOnce();
		if(loops > 0) --loops;
    }
    Finalize();
    return res;
}

int CReactor::Finalize()
{
    m_bRunning = m_bNeedStop = false;
    m_activeHandlers = 0;

    __Finalize(m_pImp, true);
    // clear all registered signal events
    __Finalize(m_sigInfo, true);
    // clear all active events
    for(int i = 0; i < MAX_EV_PRIORITY; i++)
    {
        m_vecActiveList[i].clear();
    }
    // release all event handlers
    DTEV_ITEM *pEvItem = NULL;
	if(m_tabIoHandlers != NULL)
    {
		pEvItem = m_tabIoHandlers->GetFirst();
	}
    while(pEvItem != NULL)
    {
        IEventHandler *pHandler = __TO_HANDLER(pEvItem);
        if((pHandler!=NULL) && (pHandler->DecreaseRef() == 0))
        {
            // doesn't need to unregister events, because Reactor_Imp already closed
            //invoke handler's exit method
			pHandler->HandleExit();
        }
        pEvItem = m_tabIoHandlers->GetNext();
    }
	__Finalize(m_tabIoHandlers, true);
    // release registered signals
	pEvItem = NULL;
	if(m_setSigHandlers != NULL)
    {
		pEvItem = m_setSigHandlers->GetFirst();
	}
    while(pEvItem != NULL)
    {
        IEventHandler *pHandler = __TO_HANDLER(pEvItem);
        if((pHandler!=NULL) && (pHandler->DecreaseRef() == 0))
        {
            //invoke handler's exit method
            pHandler->HandleExit();
        }
        pEvItem = m_setSigHandlers->GetNext();
    }
	__Finalize(m_setSigHandlers, true);
	__Finalize(m_timerHeap, true);

    return 0;
}

int CReactor::eventLoopOnce()
{
    int res = 0;
    struct timeval *ptv, now, tv_tmp;

	getTime(now);
	correctTimer(now);
    // if there are still active events wait to scheudle, no wait
    if(m_activeHandlers > 0)
    {
        ptv = &tv_tmp;
		CTimeUtil::TimeClear(ptv);
    }
    else // calc maximum wait time
    {
        DTEV_ITEM *pEvTimer = m_timerHeap->Top();
        if(pEvTimer == NULL)
        {
            ptv = NULL; // no timer waiting
        }
        else
        {
            //计算下一轮计算需要等待的时间
            //保存在ptv中
            ptv = &tv_tmp;
            if(CTimeUtil::TimeGreater(&pEvTimer->timeout, &now))
            {
                CTimeUtil::TimeSub(&pEvTimer->timeout, &now, ptv);
            }
            else
            {
                CTimeUtil::TimeClear(ptv); // timer expires
            }
        }
    }
	getTime(m_time); // update last old time
    CTimeUtil::TimeClear(&m_timeCache); // clear time cache
    // 在dispatch中设置超时时间为ptv，即等待时间
    // 在dispatch中会处理信号
    // 处理超时事件
    // 激活可读可写事件
    // 如下：
    // m_pReactor->ProcessSignals();
    // m_pReactor->ExpireTimerEvents();
    // m_pReactor->ActiveEvent(r_ev, EV_READ);
    // dispatch I/O events
    res = m_pImp->Dispatch(ptv);
	getTime(m_timeCache);
    // process all active events
    if(res == 0)
    {
	    CLog::Log(LL_DETAIL, 0,	"Active events this loop:[%d].", m_activeHandlers);
        if(m_activeHandlers > 0)
        {
            //调度激活的事件
            //调回Reactor中
            // 实际执行了:
            // void CReactor::ScheduleActiveEvent(DTEV_ITEM *pEvItem)
            // pHandler->HandleRead(handle);
            //   调用到CMonitorSignalHandler,执行recv
            // pHandler->HandleWrite(handle);
            //   无操作
            m_pScheduler->ScheduleActiveEvents(m_vecActiveList, MAX_EV_PRIORITY);
        }
    }

    return res;
}

IEventHandler* CReactor::ToEventHandler(DTEV_ITEM* pEvItem)
{
	if(pEvItem != NULL) return __TO_HANDLER(pEvItem);
	return NULL;
}

// register a new event handler
int CReactor::RegisterHandler(HL_SOCKET handle, IEventHandler *pHandler)
{
	if(pHandler == NULL)
	{
		return -1;
	}
	// insert into table
	if(pHandler->IsSignalHandler()) // signal handler
	{
		int signum = (int)handle;
		int ownersig = (int)pHandler->GetHandle();
		if((signum < 0) || (signum >= NSIG))
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), 
			"Insert signal handler, sig num[%d] is invalid.", signum);
			return -1;
		}
		if((ownersig>=0) && (signum!=ownersig))
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, 
			"Register signal handler, it's signum[%d]  != [%d].", ownersig, signum);
			return -1;
		}
		if(g_sigReactor != this)
		{
			CLog::Log(LL_ERROR, 0, "Reactor[0x%0x] isn't the global reactor to handle signals.", this);
			return -1;
		}
		if(m_setSigHandlers->Insert(pHandler) != 0)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), 
			"Insert Handler[0x%0x][%d] failed.", pHandler, signum);
			return -1;
		}
		if(ownersig < 0)
		{
			pHandler->AttachHandle(handle);
		}
	}
	else // I/O handler
	{	
		if(m_tabIoHandlers->Insert(handle, pHandler) != 0)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, 
			"Insert handler[0x%0x][%d] failed.", pHandler, handle);
			return -1;
		}
		if(pHandler->GetHandle() == INVALID_SOCKET)
		{
			pHandler->AttachHandle(handle);
		}
	}
	// set reactor
	pHandler->SetReactor(this);
	// increase ref number
	pHandler->IncreaseRef();
	
    return 0;
}

// remove the I/O event handler associated with handle
int CReactor::RemoveHandler(HL_SOCKET handle)
{
    int res = 0;
    DTEV_ITEM *pEvItem = m_tabIoHandlers->GetAt(handle);
	if(pEvItem == NULL)
	{
		CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, "Remove I/O handler, invalid handle[%d]", handle);
		return -1;
	}
	IEventHandler* pHandler = __TO_HANDLER(pEvItem);
	if(pHandler == NULL)
	{
		return -1;
	}
	// unregister all existing events
	if(pEvItem->events != 0)
	{
		res = m_pImp->UnregisterEvent(pEvItem, pEvItem->events);
		if(res != 0)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "failed to remove I/O handler[%d]", handle);
			return -1;
		}
        if(pEvItem->events&EV_TIMER)
        {
            m_timerHeap->Erase(pEvItem);
        }
		pEvItem->events = 0;
	}
	// still has active events, delay removing this event handler in ScheduleActiveEvent() method
	if(pEvItem->evActive != 0)
	{
		pEvItem->flags |= EVFLAG_TOBE_DEL;
		CLog::Log(LL_DETAIL, 0, "delay remove handler, handler[%d]", handle);
		return 0;
	}
	
	// remove records from I/O table
	m_tabIoHandlers->Remove(handle);
	if(pHandler->DecreaseRef() == 0)
	{
		pHandler->HandleClose();
	}
	return 0;
}

// remove the signal event handler
int CReactor::removeSignalHandler(IEventHandler* pHandler)
{
    int res = 0;
    DTEV_ITEM *pEvItem = m_setSigHandlers->GetAt(pHandler);
	if(pEvItem == NULL)
	{
		CLog::Log(LL_ERROR, 0, "remove signal handler, invalid signum[%d]", pHandler->GetHandle());
		return -1;
	}
    if(__TO_HANDLER(pEvItem) == pHandler)
	{
		if(unregisterSignal(pHandler, pEvItem->events) != 0)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), 
			"failed to remove signal handler[%d]", pHandler->GetHandle());
			return -1;
		}
		if(pEvItem->events&EV_TIMER)
		{
			m_timerHeap->Erase(pEvItem);
		}
        pEvItem->events = 0;
		// still has active events, delay removing this event handler in ScheduleActiveEvent() method
		if(pEvItem->evActive != 0)
		{
			pEvItem->flags |= EVFLAG_TOBE_DEL;
			CLog::Log(LL_DETAIL, 0, "delay remove handler, handler[%d]", pHandler->GetHandle());
			return 0;
		}
		// try to delete the handler
		m_setSigHandlers->Remove(pHandler);
		if(pHandler->DecreaseRef() == 0)
		{
			pHandler->HandleClose();
		}
		return 0;
	}
	// error
	CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, 
		"Failed to remove signal, handler[%d:0x%0X] and item[0x%0X]  (handler is [0x%0X]) unmatch", 
		pHandler->GetHandle(), pHandler, pEvItem, __TO_HANDLER(pEvItem));
	return -1;
}

// register I/O event
int CReactor::RegisterEvent(HL_SOCKET handle, short event)
{
	DTEV_ITEM *pEvItem = m_tabIoHandlers->GetAt(handle);
	if(pEvItem == NULL)
	{
		CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, "Register Event failed, invalid handle[%d]", handle);
		return -1;
	}
	if(__TO_HANDLER(pEvItem) != NULL)
	{
		if(__TO_HANDLER(pEvItem)->IsSignalHandler())
		{
			CLog::Log(LL_ERROR, 0, "RegisterEvent::[%d:0x%0x] is signal handler", handle, __TO_HANDLER(pEvItem));
			return -1;
		}
		int res = 0;
		short &events = pEvItem->events;
		event &= (EV_READ|EV_WRITE);
		if((events|event) != events) // has new event to register, need to update
		{
			if((res=m_pImp->RegisterEvent(pEvItem, event)) == 0)
			{
				events |= event;
			}
		}
		return res;
	}
	// error
	CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, 
		"Failed to register event of handle[%d], whose item[0x%0X]->handler is NULL", 
		handle, pEvItem);
	return -1;
}

// register signal event
int CReactor::registerSignal(IEventHandler *pHandler)
{
	DTEV_ITEM *pEvItem = m_setSigHandlers->GetAt(pHandler);
	if(pEvItem == NULL)
	{
		CLog::Log(LL_ERROR, 0, "Register signal event, invalid signum[%d]", pHandler->GetHandle());
		return -1;
	}
	if(__TO_HANDLER(pEvItem) == pHandler)
	{
		int res = 0;
		short &events = pEvItem->events;
		if((events&EV_SIGNAL)==0) // if already registered skip
		{
			if((res=m_sigInfo->RegisterSignalEvent(pEvItem))==0)
			{
				events |= EV_SIGNAL;
			}
		}
		return res;
	}
	// error
	CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, 
		"Failed to register signal event of handler[%d:0x%0X] and item[0x%0X] (handler is [0x%0X]) unmatch", 
		pHandler->GetHandle(), pHandler, pEvItem, __TO_HANDLER(pEvItem));
	return -1;
}

// register timer event
int CReactor::RegisterTimer(IEventHandler *pHandler, struct timeval *ptv)
{
	if((ptv == NULL) || (pHandler == NULL))
	{
		return -1;
	}

	DTEV_ITEM *pEvItem = handler2DtevItem(pHandler);
	if(pEvItem == NULL)
	{
		CLog::Log(LL_ERROR, 0, "Register timer, invalid handle[%d]", pHandler->GetHandle());
		return -1;
	}
	if(__TO_HANDLER(pEvItem) == pHandler)
	{
		struct timeval now;
		// set next expire time
		getTime(now);
		pEvItem->period = *ptv;
		CTimeUtil::TimeAdd(ptv, &now, &(pEvItem->timeout));

		short &events = pEvItem->events;
		// update the old one
		if(events&EV_TIMER)
		{
			// if currently not actived, need to update timer heap 
			if(!(pEvItem->evActive&EV_TIMER))
			{
				m_timerHeap->Update(pEvItem);
			}
		}
		else // register new timer event
		{
			if(m_timerHeap->Push(pEvItem) != 0)
			{
				return -1;
			}
			events |= EV_TIMER;
		}
		return 0;
	}
	CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, 
		"Failed to register timer, handler[%d:0x%0X] and item[0x%0X]  (handler is [0x%0X]) unmatch", 
		pHandler->GetHandle(), pHandler, pEvItem, __TO_HANDLER(pEvItem));
	return -1;
}

// unregister I/O event, if the specified event isn't registered, just ignore 
int CReactor::UnregisterEvent(HL_SOCKET handle, short event)
{
	DTEV_ITEM *pEvItem = m_tabIoHandlers->GetAt(handle);
	if((pEvItem==NULL) || (__TO_HANDLER(pEvItem)==NULL))
	{
		CLog::Log(LL_ERROR, 0, "Unegister event failed, invalid handle[%d]", handle);
		return -1;
	}
	IEventHandler* pHandler = __TO_HANDLER(pEvItem);
	if(pHandler->IsSignalHandler())
	{
		CLog::Log(LL_ERROR, 0, "UnregisterEvent::[0x%0x] is signal handler", pHandler);
		return -1;
	}
	
	short &events = pEvItem->events;
	event &= events; // filter invalid event types
	if(event == 0) // no specified event
	{
		return 0;
	}
	short evtimer = event&EV_TIMER;
	event &= (EV_READ|EV_WRITE);
	if(event != 0) // I/O events
	{
		if(m_pImp->UnregisterEvent(pEvItem, event) != 0)
			return -1;
		events &= (~event);
	}	
	if(evtimer) // timer
	{
		m_timerHeap->Erase(pEvItem);
		events &= (~EV_TIMER);
	}
	return 0;
}

int CReactor::unregisterSignal(IEventHandler* pHandler, short event)
{
	DTEV_ITEM *pEvItem = m_setSigHandlers->GetAt(pHandler);
	if(pEvItem == NULL)
	{
		CLog::Log(LL_ERROR, 0, "Unregister signal event, invalid signum[%d]", pHandler->GetHandle());
		return -1;
	}
	if(__TO_HANDLER(pEvItem) == pHandler)
	{
		short &events = pEvItem->events;
		event &= events;
		if(event & EV_SIGNAL) // unregister signal event
		{
			if(m_sigInfo->UnregisterSignalEvent(pEvItem)!=0)
			{
				return -1;
			}
			events &= (~EV_SIGNAL);
		}			
		if(events&EV_TIMER) // timer
		{
			m_timerHeap->Erase(pEvItem);
			events &= (~EV_TIMER);
		}
		return 0;
	}
	// error
	CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, 
		"Failed to unregister signal event, handler[%d:0x%0X] and item[0x%0X] (handler is [0x%0X]) unmatch", 
		pHandler->GetHandle(), pHandler, pEvItem, __TO_HANDLER(pEvItem));
	return -1;
}

// 设置当前活跃事件
int CReactor::ActiveEvent(DTEV_ITEM *pEvItem, short event)
{
    if(pEvItem == NULL)
    {
        return -1;
    }
	if(pEvItem->flags&EVFLAG_TOBE_DEL)
	{
		return 0;
	}
    short eventsNew = (event&pEvItem->events); 
    short &pEvActive = pEvItem->evActive;
    if(eventsNew == 0)
    {
        CLog::Log(LL_WARN, __FILE__, __LINE__, 0, 
		"Active Event failed, no such event[%d] of handler[0x%0X, events:%d]", event, __TO_HANDLER(pEvItem), pEvItem->events);
        return -1;
    }
    // if already has active events, just append current flag
	if(pEvActive != 0)
    {
        pEvActive |= eventsNew;
        return 0;
    }
    // add to active list according to it's priority
    pEvActive = eventsNew;
    m_vecActiveList[__TO_HANDLER(pEvItem)->GetPriority()].push_back(pEvItem);
    m_activeHandlers++;

    return 0;
}

void CReactor::ExpireTimerEvents()
{
    struct timeval now;
    // calc maximum wait time 
    DTEV_ITEM *pEvTimer = m_timerHeap->Top();
    // check timer events
    getTime(now);
    while(pEvTimer != NULL)
    {
        if(!CTimeUtil::TimeLess(&pEvTimer->timeout, &now)) break;
        // timer event is always inner
        ActiveEvent(pEvTimer, EV_TIMER);
        m_timerHeap->Pop();
        pEvTimer = m_timerHeap->Top();
    }
}

void CReactor::ScheduleActiveEvent(DTEV_ITEM *pEvItem)
{
    if(pEvItem == NULL)
    {
        return;
    }
    IEventHandler *pHandler = __TO_HANDLER(pEvItem);
    if(pHandler == NULL)
    {
        return;
    }
    m_activeHandlers--;
	HL_SOCKET handle = pEvItem->handle;
	short &event = pEvItem->evActive;
    do
    {
		short &flag = pEvItem->flags;
        if(flag & EVFLAG_TOBE_DEL) break; // need to remove
        // process timer event first
        if(event&EV_TIMER)
        {
            pHandler->HandleTimer();
            if(flag & EVFLAG_TOBE_DEL) break; // need to remove
			if(pEvItem->events&EV_TIMER) // reschedule timer
            {
				struct timeval *ptv = &(pEvItem->period);
				CTimeUtil::TimeAdd(&m_timeCache, ptv, &(pEvItem->timeout));
				m_timerHeap->Push(pEvItem); 
			}
        }
        if(event&EV_SIGNAL)
        {
			int nsigs = pEvItem->nsigs;
			pEvItem->nsigs = 0;
            while(nsigs-- > 0)
            {
                pHandler->HandleSignal(); // handle signal
                if(flag & EVFLAG_TOBE_DEL) break; // need to remove
            }
        }
        else
        {
            if(event&EV_READ)
			{
				pHandler->HandleRead(handle);
				if(flag & EVFLAG_TOBE_DEL) break; // need to remove
			}			
            if(event&EV_WRITE)
			{
				pHandler->HandleWrite(handle);
				if(flag & EVFLAG_TOBE_DEL) break; // need to remove
			}
        }
        event = 0; // clear active event flag

        return;
    }while(0);
	
    // the handler was marked as deleted
    CLog::Log(LL_DETAIL, 0, "Schedule Event::Handler[%d:0x%0X] is marked as remove, delete.", 
        handle, pHandler);
    // delete
	if(pHandler->IsSignalHandler())
	{
		m_setSigHandlers->Remove(pHandler);
	}
	else
	{
		m_tabIoHandlers->Remove(handle);
	}
	if(pHandler->DecreaseRef() == 0)
	{
		pHandler->HandleClose();
	}
}

void CReactor::correctTimer(struct timeval &tv)
{
	if(CTimeUtil::IsMonotonicTime())
	{
		return;
	}

	// time is running backwards, and needs to correct
	if(CTimeUtil::TimeLess(&tv, &m_time))
    {
        struct timeval tsub;
        CLog::Log(LL_DEBUG, __FILE__, __LINE__, 0, 
			"Time is running backwards, start to correct[%d,%d]--[%d,%d].", 
            m_time.tv_sec, m_time.tv_usec, tv.tv_sec, tv.tv_usec);

		CTimeUtil::TimeSub(&m_time, &tv, &tsub);
		m_timerHeap->AdjustTimer(tsub);
        m_time = tv; // update last time
	}
}

int CReactor::createInternalHandlers()
{
	//register read event of CMonitorSignalHandler, to monitor signal events
    IEventHandler *pHandler = new CMonitorSignalHandler();
	if(pHandler == NULL) return -1;
	do
	{
		// insert into table, IO map should guarantinee that the handle doen't insert with multiple handlers
		pHandler->AttachHandle(m_sigInfo->GetRecvHandle());
		if(m_tabIoHandlers->Insert(m_sigInfo->GetRecvHandle(), pHandler) != 0)
		{
			break;
		}
		// set reactor
		pHandler->SetReactor(this);		
		if(RegisterEvent(pHandler, EV_READ) != 0)
		{
			 break;
		}
		DTEV_ITEM* pEvItem = m_tabIoHandlers->GetAt(pHandler->GetHandle());
		pEvItem->flags |= EVFLAG_INTERNAL; // tag internal flag
		
		return 0;
	}while(0);
	
	delete pHandler;
	return -1;
}

void CReactor::getTime(struct timeval &t)
{
	if(m_timeCache.tv_sec)
	{
		t = m_timeCache;
	}
	else
	{
		CTimeUtil::GetSysTime(&t);
	}
}

DTEV_ITEM *CReactor::handler2DtevItem(IEventHandler *pHandler)
{
	DTEV_ITEM *pEvItem = NULL;
	if(pHandler->IsSignalHandler()) // is signal handler
	{
		pEvItem = m_setSigHandlers->GetAt(pHandler);
	}
	else // I/O event handler
	{
		pEvItem = m_tabIoHandlers->GetAt(pHandler->GetHandle());
	}
	return pEvItem;
}

IEventHandler* CReactor::Handle2EventHandler(HL_SOCKET handle)
{
	DTEV_ITEM *pEvItem = m_tabIoHandlers->GetAt(handle);
	if(pEvItem != NULL)
	{
		return __TO_HANDLER(pEvItem);
	}
	return NULL;
}

int CReactor::ProcessSignals() 
{
	return m_sigInfo->ProcessSignal();
}
size_t CReactor::GetHandlerNumber()
{
	return m_tabIoHandlers->GetSize();
}

void CReactor::SetSigReactor(CReactor *pReactor)
{
	g_sigReactor = pReactor;
}

CSignalInfo* CReactor::GetSigInfo()
{
	return m_sigInfo;
}

int CReactor::GetEvents(IEventHandler *pHandler, short &event, short &evActive)
{
	DTEV_ITEM *pEvItem = handler2DtevItem(pHandler);
	if((pEvItem!=NULL) && (__TO_HANDLER(pEvItem)==pHandler))
	{
		event = pEvItem->events;
		evActive = pEvItem->evActive;
		return 0;
	}
	return -1;
}

int CReactor::GetEvents(HL_SOCKET handle, short &event, short &evActive)
{
	DTEV_ITEM *pEvItem = m_tabIoHandlers->GetAt(handle);
	if(pEvItem != NULL)
	{
		event = pEvItem->events;
		evActive = pEvItem->evActive;
		return 0;
	}
	return -1;
}
