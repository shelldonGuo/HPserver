/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __I_EVENT_HANDLER_H_
#define __I_EVENT_HANDLER_H_

#include "defines.h"
#include "SockUtil.h"
#include <stdio.h>
#include <stdlib.h>

//
// event handler
//  you can register R/W events for multiple handlers on the same handler
// event type can be: R/W, Timer, Signal.
// 
// If it's a signal event handler, then it can't register R/W events any more,
//    ------ And m_handle is the interested signal number, not a file descriptor/HL_SOCKET 
// If it's a I/O event handler, then it can't register signal events any more.
//

namespace hpsl
{
	class CReactor;

	class IEventHandler
	{
	public:
		IEventHandler(bool isSignal = false):m_pReactor(NULL), 
			m_handle(INVALID_SOCKET), m_ref(0), m_isSignal(isSignal), 
            m_priority(MAX_EV_PRIORITY>>1) // by default priority is in MAX_EV_PRIORITY/2
		{}
		virtual ~IEventHandler(){}

		inline void SetReactor(CReactor *pReactor) 		{if(m_pReactor == NULL) m_pReactor = pReactor;}
        // CReactor manages each Event Handler by it's handle, so you can't change the handle once set.
		inline void AttachHandle(HL_SOCKET handle)	   
		{	
			if((m_handle==INVALID_SOCKET) || (m_handle<0))
			{
				m_handle = handle;
			}
		}
		inline void ResetHandle() {m_handle = INVALID_SOCKET;}
		
        inline void SetPriority(short prio) {if((prio>=0) && (prio<MAX_EV_PRIORITY)) m_priority = prio;}

		inline HL_SOCKET GetHandle() 		{return m_handle;}
        inline short     GetPriority()      {return m_priority;}
		
		inline int DecreaseRef() {return --m_ref;}
		inline int IncreaseRef() {return ++m_ref;}
		
		inline bool IsSignalHandler() {return m_isSignal;}
        // close handle - and set socket to INVALID_SOCKET.
		// NOTE:
        // Reactor manages EventHandler by it's m_handle, if carelessly call CloseHandle() before RemoveHandler(this), 
        // then m_handle = INVALID_SOCKET, Reactor can't find it, and can't remove it.
        inline void CloseHandle() {if(!IsSignalHandler()) CLOSE_SOCKET(m_handle);}

		// invoked by reactor, when reactor is finalized
		virtual void HandleExit() = 0;
        // invoked by reactor, when the handler's ref number is 0 and is removed from reactor
		virtual void HandleClose() = 0;
        virtual void HandleSignal() = 0;
		virtual void HandleRead(HL_SOCKET handle) = 0;
		virtual void HandleWrite(HL_SOCKET handle) = 0;
        virtual void HandleTimer() = 0;
		// invoked when connect to remote server successfully
        virtual void HandleConnectSvrSuccess(HL_SOCKET handle){}
    protected:
		inline CReactor *getReactor() {return m_pReactor;}
	private:
		short m_ref;
        short m_priority;
		bool m_isSignal;
		CReactor *m_pReactor;
        HL_SOCKET m_handle;
    };
}


#endif // __I_EVENT_HANDLER_H_

