/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __CONNECTOR_H_
#define __CONNECTOR_H_

// 
// it's hard to have a connector use a reactor exclusively, or handle the completion event,
// because it must register an event for each new connection request, 
// and then it must manage all the pending requests, which will make the 
// implement too complex.
// 
// In our design, Connector uses the reactor which the Event Handler registers to.
// And if an event handler relays the connector to issue a connect, 
// it should handle the completion event.
// 
// So the connector will be really easy to implement.
// The only fault is that they MUST run in the same thread, while most of the time it's not a problem.
//

#include "SockUtil.h"

#include "InetAddr.h"
#include "EventHandler.h"
#include "Reactor.h"

namespace hpsl
{
	class CConnector
	{
	public:
		CConnector(){}
		virtual ~CConnector(){}
		// for normal operations, sync
		HL_SOCKET Connect(CInetAddr &svrAddr)
		{
			// create a socket
			HL_SOCKET handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			do
			{
				if(handle == INVALID_SOCKET) break;
				int res = connect(handle, svrAddr.GetAddr(), svrAddr.Size());
				if(res < 0) break;
				return handle;
			}while(0);
			if(handle != INVALID_SOCKET)
	        {
	            CSockUtil::CloseSocket(handle);
	        }
			return INVALID_SOCKET;
		}
		
	    //
	    // user needs to register a timer for timeout check, handle MUST already binded to a valid event handler.
	    //
		inline int Connect(CReactor *pReactor, HL_SOCKET handle, CInetAddr &svrAddr, struct timeval *ptv)
		{
			// connect, SYNC or ASYNC depends on the socket's property.
			if(pReactor== NULL)
	        {
	            return -1;
	        }
			// get the binded handler of handle
			IEventHandler *pHandler = pReactor->Handle2EventHandler(handle);
			if(pHandler == NULL)
			{
				return -1;
			}
	        int res = connect(handle, svrAddr.GetAddr(), svrAddr.Size());
			if((res<0) && CSockUtil::IoIsPending(SOCK_ERR()))
			{
				// when connect success, the socket will be set writable
				res = pReactor->RegisterEvent(handle, EV_WRITE);
	            if(res < 0)
	            {
	                return -1;
	            }
				if(ptv != NULL) // register timer event
				{
					res = pReactor->RegisterTimer(pHandler, ptv);
		            if(res < 0)
		            {
						pReactor->UnregisterEvent(handle, EV_WRITE);
		                return -1;
		            }
				}
				return 1; // return 1, operation doesn't complete yet.
			}
			if(res == 0) // connect success
			{
				pHandler->HandleConnectSvrSuccess(handle);
			}
			return res;
		}
	};
}

#endif // __CONNECTOR_H_
