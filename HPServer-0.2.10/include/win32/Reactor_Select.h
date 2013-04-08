/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef _REACTOR_SELECT_H_
#define _REACTOR_SELECT_H_

#include "defines.h"
#include "Reactor.h"
#include "Reactor_Imp.h"
#include "sockutil.h"
#include <map>

//select on windows
namespace hpsl	
{
	struct DTEV_ITEM;
	class CReactor_Select:public IReactor_Imp
	{
	public:
		CReactor_Select():m_pReactor(NULL){}
		virtual ~CReactor_Select(){}
		virtual int Initialize(CReactor *pReactor);
		virtual int Finalize();
		virtual int Dispatch(struct timeval *tv);
		
		//get kernel mechanism
		virtual const tchar* GetMethod(){return TEXT("select");}
		
		//register event
		virtual int RegisterEvent(const DTEV_ITEM *pEvItem,  short event);
		
		//unregister event
		virtual int UnregisterEvent(const DTEV_ITEM *pEvItem, short event);
	private:
		CReactor* m_pReactor;
		fd_set m_readSet, m_readSet_Cache;
		fd_set m_writeSet, m_writeSet_Cache;
		const DTEV_ITEM *m_pEvItems[FD_SETSIZE];
		int m_itemSize;
	};
}
#endif
