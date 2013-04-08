/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef _REACTOR_H_
#define _REACTOR_H_

#include "defines.h"
#include "Reactor.h"
#include "Reactor_Imp.h"
#include "SockUtil.h"


//select on linux/windows
namespace hpsl
{
	class IEventHandler; 
	struct DTEV_ITEM;
    // dispatcher的Select实现
	class CReactor_Select:public IReactor_Imp
	{
	public:
        CReactor_Select():m_maxFds(-1),m_fdsz(0),
                        m_pReadset_in(NULL), m_pReadset_out(NULL), 
                        m_pWriteset_in(NULL), m_pWriteset_out(NULL), 
                        m_pEvents(NULL), m_pReactor(NULL){}
        virtual ~CReactor_Select(){}
        virtual int Initialize(CReactor *pReactor);
		virtual int Finalize();
		virtual int Dispatch(struct timeval *tv);
		
		//get kernel mechanism
		virtual const tchar *GetMethod(){return TEXT("select");} 
		
		//register event
		virtual int RegisterEvent(const DTEV_ITEM *pEvItem, short event);
		
		//unregister event	
		virtual int UnregisterEvent(const DTEV_ITEM *pEvItem, short event);
	private:
		int m_maxFds; //max file descriptor 
		int m_fdsz;
		CReactor* m_pReactor;
		fd_set*  m_pReadset_in;
		fd_set*  m_pWriteset_in;
		fd_set*  m_pReadset_out;
		fd_set*  m_pWriteset_out;
		struct DTEV_ITEM** m_pEvents; //store event has been registered 
		int reclac(int max);
	};
}
#endif
