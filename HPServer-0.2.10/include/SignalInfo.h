/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */
 
#ifndef _SIGNAL_INFO_H_
#define _SIGNAL_INFO_H_

#include "SockUtil.h"
#include "defines.h"
#include "EventItem.h"
#include "Log.h"
#include <signal.h>
#include <string.h>
#include <list>

namespace hpsl
{
	typedef void (*sighandler_t) (int);
	class CReactor;
	class CSignalInfo
	{
	public:
		CSignalInfo():m_pReactor(NULL){}
		~CSignalInfo(){}
		int Finalize();
		int Initialize(CReactor* pReactor);
		int RegisterSignalEvent (const DTEV_ITEM* pEvItem);
		int UnregisterSignalEvent(DTEV_ITEM* pEvItem);
		int ProcessSignal();
		inline HL_SOCKET GetRecvHandle() {return m_socketPair[1];}
		inline HL_SOCKET GetSendHandle() {return m_socketPair[0];}
		
	private:
		void catchSignalOccur(int signum);
		int setSignalHandler(int signum);
		int restoreSignalHandler(int signum);

		static void sig_handler(int signum);
		
		CReactor* m_pReactor;
	#ifdef HAVE_SIGACTION
		struct sigaction* m_sigHandler_old[NSIG];	
	#else
		sighandler_t m_sigHandler_old[NSIG];
	#endif
		std::list<DTEV_ITEM*> m_signalHandler[NSIG];
		sig_atomic_t  m_isigCaught[NSIG];
		HL_SOCKET m_socketPair[2];
	};
}
#endif

