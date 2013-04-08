/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#include "EventItem.h"
#include "Reactor_Select.h"
#include <string.h>

#ifndef howmany
#define howmany(x,y)  (((x)+((y)-1))/(y))
#endif


using namespace hpsl;

int CReactor_Select::Initialize(CReactor *pReactor)
{
    if(pReactor == NULL)
    {
        return -1;
    }
    m_pReactor = pReactor;

    int fdsz = howmany(32+1, NFDBITS) * sizeof(fd_mask);
    int ret = reclac(fdsz);
    if(ret != 0)
    {
        Finalize();
    }
    return ret;
}

int CReactor_Select::reclac(int fdsz)
{
    int nEvents = (fdsz/sizeof(fd_mask)) * NFDBITS;
    int nEvents_old = (m_fdsz/sizeof(fd_mask)) * NFDBITS; 

	fd_set *readset_in = NULL;
	fd_set *readset_out = NULL;
	fd_set *writeset_in = NULL;
	fd_set *writeset_out = NULL;
	struct DTEV_ITEM** pEvents = NULL;
    if((readset_in=(fd_set*)realloc(m_pReadset_in, fdsz)) == NULL)
        return -1;
	m_pReadset_in = readset_in;
    memset((char*)m_pReadset_in+m_fdsz,  0, (fdsz-m_fdsz));
    if((readset_out=(fd_set*)realloc(m_pReadset_out,fdsz)) == NULL)
        return -1;
	m_pReadset_out = readset_out;
	
    if((writeset_in=(fd_set*)realloc(m_pWriteset_in, fdsz)) == NULL)
        return -1;
	m_pWriteset_in = writeset_in;
    memset((char*)m_pWriteset_in+m_fdsz, 0, (fdsz-m_fdsz));
    if((writeset_out=(fd_set*)realloc(m_pWriteset_out,fdsz)) == NULL)
        return -1;
	m_pWriteset_out = writeset_out;
    if((pEvents=(struct DTEV_ITEM**)realloc(m_pEvents, nEvents * sizeof(struct DTEV_ITEM*))) == NULL)
        return -1;
	m_pEvents = pEvents;
	
	memset(m_pEvents+nEvents_old, 0, (nEvents-nEvents_old)*sizeof(struct DTEV_ITEM*));
    m_fdsz = fdsz;
	
    return 0;
}

int CReactor_Select::Finalize()
{
    m_maxFds = -1;
    m_fdsz   = 0;
    __Free(m_pReadset_in);
    __Free(m_pReadset_out);
    __Free(m_pWriteset_in);
    __Free(m_pWriteset_out);
    __Free(m_pEvents);
    return 0;
}

int CReactor_Select::RegisterEvent(const DTEV_ITEM* pEvItem, short event)
{
    if(pEvItem == NULL)
	{
		return -1;
	}
	HL_SOCKET handle = pEvItem->handle;
    if(m_maxFds < handle)
    {
        int fdsz = m_fdsz;
        if(fdsz < sizeof(fd_mask))
            fdsz = sizeof(fd_mask);
        while(fdsz < (howmany(handle+1,NFDBITS)*sizeof(fd_mask)))
        {
            fdsz <<= 1;
        }
        if(fdsz != m_fdsz)
        {
            if(reclac(fdsz) != 0)
                return -1;
        }
        m_maxFds = handle;
    }
    struct DTEV_ITEM* evep = m_pEvents[handle];
    if((evep!=NULL) && (evep!=pEvItem)) //the handle is already registered with an event handler
    {
        CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(),  
            "Failed to register select event. event itme[0x%0X] mismatch handle[%d]'s existing item[0x%0X].", 
            pEvItem, handle, evep);
        return -1;
    }
    if(event & EV_READ)
    {
        FD_SET(handle, m_pReadset_in);
    }
    if(event & EV_WRITE)
    {
        FD_SET(handle, m_pWriteset_in);
    }
	// update event pointer
	if(m_pEvents[handle] == NULL)
	{
		m_pEvents[handle] = const_cast<DTEV_ITEM*>(pEvItem);
	}
    return 0;
}

int CReactor_Select::UnregisterEvent(const DTEV_ITEM* pEvItem, short event)
{
    if(pEvItem == NULL)
	{
		return -1;
	}
	HL_SOCKET handle = pEvItem->handle;
    int events = pEvItem->events;; // existing read/write event
    if(handle > m_maxFds)
    {
        return 0;
    }
    //check whether event to be unregietered is legitimate event
    struct DTEV_ITEM* evep = m_pEvents[handle];
    if(evep != pEvItem)
    {
        CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(),  
            "Failed to unregister select event. event itme[0x%0X] mismatch handle[%d]'s existing item[0x%0X].", 
            pEvItem, handle, evep);
        return -1;
    }
    if(event & EV_READ)
    {
        FD_CLR(handle, m_pReadset_in);
    }
    if(event & EV_WRITE)
    {
        FD_CLR(handle, m_pWriteset_in);
    }
    // check whether needs to clear event set.
    if((events&event) == events)
    {
        m_pEvents[handle] = NULL;
    }
    return 0;
}
// 利用select作为demultiplexer
// Dispatch是dispatcher的实现
int CReactor_Select::Dispatch(struct timeval* tv)
{

    /************************************************************************/
    /* before invoke select,should clear fd_set                            */
    /************************************************************************/
    memcpy(m_pReadset_out, m_pReadset_in, m_fdsz);
    memcpy(m_pWriteset_out, m_pWriteset_in, m_fdsz);
    int res = select(m_maxFds+1, m_pReadset_out, m_pWriteset_out, NULL, tv);
    if(res == -1)
    {
        if(errno != EINTR)
        {
            CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "select dispatch failed.");
            return -1;
        }
        //TODO: should process single
        return 0;
    }
    m_pReactor->ProcessSignals();
    // active timer event first
    m_pReactor->ExpireTimerEvents();

    // active I/O events
    for(int i=0; i <= m_maxFds; ++i)
    {
        struct DTEV_ITEM* r_ev = NULL, *w_ev = NULL;
        res = 0;
        short event = 0;
        if(FD_ISSET(i, m_pReadset_out))
        {
            r_ev = m_pEvents[i];
            res |= EV_READ;
        }
        if(FD_ISSET(i, m_pWriteset_out))
        {
            w_ev = m_pEvents[i];
            res |= EV_WRITE;
        }
        if((r_ev) && (res & r_ev->events))
        {
            m_pReactor->ActiveEvent(r_ev, EV_READ);
        }
        if((w_ev) && (res & w_ev->events))
        {
            m_pReactor->ActiveEvent(w_ev, EV_WRITE);
        }
    }
    return 0;
}
