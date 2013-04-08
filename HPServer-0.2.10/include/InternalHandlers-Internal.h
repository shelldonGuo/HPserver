/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __I_HANDLERS_INTERNAL_H_
#define __I_HANDLERS_INTERNAL_H_

#include "EventHandler.h"
#include "Log.h"
#include <memory.h>


namespace hpsl
{
	// handler used to monitor signal events
    class CMonitorSignalHandler: public IEventHandler
    {
    public:
        CMonitorSignalHandler(bool isSignal = false):IEventHandler(isSignal)
        {
            memset(m_signal, 0, sizeof(m_signal));
        }
        virtual ~CMonitorSignalHandler(){}
		virtual void HandleExit() {delete this;}
        virtual void HandleClose()
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, "MonitorSignalHandler is closed!");
			delete this;
		}

        virtual void HandleSignal(){}
        virtual void HandleRead(HL_SOCKET handle)
        {
			int n = recv(GetHandle(), m_signal, 1, 0);
			if(n == -1)
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "MonitorSignalHandler read socket error!");
        }
        virtual void HandleWrite(HL_SOCKET handle){}
        virtual void HandleTimer(){}
    private:
        char   m_signal[2];	
    };
}


#endif // __I_HANDLERS_INTERNAL_H_

