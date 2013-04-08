/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */
 
#include "Reactor_Select.h"
#include "EventItem.h"
#include <errno.h>

using namespace hpsl;

int CReactor_Select::Initialize(CReactor *pReactor)
{
	if (pReactor==NULL)
	{
		return -1;
	}
	m_pReactor = pReactor;
	FD_ZERO(&m_readSet_Cache);
	FD_ZERO(&m_writeSet_Cache);
	memset(m_pEvItems, 0, sizeof(m_pEvItems));
	m_itemSize = 0;

	return 0;
}

int CReactor_Select::Finalize()
{
	FD_ZERO(&m_readSet_Cache);
	FD_ZERO(&m_writeSet_Cache);

	return 0;
}

int CReactor_Select::RegisterEvent(const DTEV_ITEM *pEvItem, short event)
{
	if (pEvItem == NULL)
	{
		return -1;
	}
	if ((event&(EV_READ|EV_WRITE)) == 0)
	{
		CLog::Log(LL_WARN, __FILE__, __LINE__, 0,
			"Can not register such Event[%d]", event);
		return -1;
	}
	HL_SOCKET handle = pEvItem->handle;
	
	int i = 0, idx = 0;
	for(i = 0; i < FD_SETSIZE;  ++i)
	{
		if(m_pEvItems[i] == pEvItem) break;
		if((m_pEvItems[i]==NULL) && (idx==0)) idx = i;
	}
	if ( (i==FD_SETSIZE) && (m_itemSize==FD_SETSIZE))
	{
		CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, 
			"Failed to register select event of handle[%d], FD size exceeded.", handle);	
		return -1;
	}
	if (event&EV_READ)
	{
		FD_SET(handle, &m_readSet_Cache);
	}
	if (event&EV_WRITE)
	{
		FD_SET(handle, &m_writeSet_Cache);
	}
	if(i == FD_SETSIZE) // it's a new handle
	{
		for(idx; idx < FD_SETSIZE; ++idx)
		{
			if(m_pEvItems[idx] == NULL)
			{
				m_pEvItems[idx] = pEvItem;
				break;
			}
		}
		++m_itemSize;
	}
	return 0;
}

int CReactor_Select::UnregisterEvent(const DTEV_ITEM *pEvItem, short event)
{
	if (pEvItem == NULL)
	{
		return -1;
	}

	HL_SOCKET handle = pEvItem->handle;
	short events = pEvItem->events&(EV_READ|EV_WRITE); //existing read/write event
	
	if ((event&(EV_READ|EV_WRITE))==0)
	{
		CLog::Log(LL_WARN, __FILE__, __LINE__, 0, 
			"unregister event, no such Event[%d] of handle[%d]", event, handle);
		return -1;
	}
	int i = 0;
	for(i = 0; i < FD_SETSIZE;  ++i)
	{
		if(m_pEvItems[i] == pEvItem) break;
	}
	if (i == FD_SETSIZE)
	{
		CLog::Log(LL_WARN, __FILE__, __LINE__, ERRNO(),  
			"Failed to unregister select event. handle[%d] did not registered", handle);	
		return -1;
	}
	if (event&EV_READ)
	{
		FD_CLR(handle, &m_readSet_Cache);
	}
	if (event&EV_WRITE)
	{
		FD_CLR(handle, &m_writeSet_Cache);
	}
	//decide whether to remove it
	if ((events&(~event)) == 0)
	{
		m_pEvItems[i] = NULL;
		--m_itemSize;
	}
	return 0;
}
int CReactor_Select::Dispatch(timeval *tv)
{
    memcpy(&m_readSet, &m_readSet_Cache, sizeof(m_readSet));
    memcpy(&m_writeSet, &m_writeSet_Cache, sizeof(m_writeSet));

	int res = select(0, &m_readSet, &m_writeSet, NULL, tv);
	if (res == -1)
	{
		if (ERRNO() != EINTR)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "select dispatch failed.");
			return -1;
		}
		return 0;
	}
	m_pReactor->ProcessSignals();
	m_pReactor->ExpireTimerEvents();
	//active I/O events
	for (int i = 0; i < FD_SETSIZE; ++i)
	{
		if(m_pEvItems[i] == NULL) continue;

		HL_SOCKET handle = m_pEvItems[i]->handle;
		short events=0;
		const struct DTEV_ITEM* r_ev = NULL, *w_ev = NULL;
		if (FD_ISSET(handle, &m_readSet))
		{
			events |= EV_READ;
			r_ev = m_pEvItems[i];
		}
		if (FD_ISSET(handle, &m_writeSet))
		{
			events |= EV_WRITE;
			w_ev = m_pEvItems[i];
		}
		if ((r_ev) && (events & r_ev->events))
		{
			m_pReactor->ActiveEvent(const_cast<DTEV_ITEM*>(r_ev), EV_READ);
		}
		if ((w_ev) && (events & w_ev->events))
		{
			m_pReactor->ActiveEvent(const_cast<DTEV_ITEM*>(w_ev), EV_WRITE);
		}
	}
	return 0;
}

