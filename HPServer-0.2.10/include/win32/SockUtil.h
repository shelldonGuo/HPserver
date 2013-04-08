/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __SOCK_UTIL_H_
#define __SOCK_UTIL_H_


#include "defines.h"


#ifdef IPV6_SUPPORT
#ifndef INADDR_ANY6 
#define INADDR_ANY6 in6addr_any
#endif
#endif


#if (_WIN32_WINNT >= 0x0600) // Longhorn 
#include <in_addr.h> // for ipv6
#elseif (_WIN32_WINNT >= 0x0500) // 2000, xp or 2003 server
#include <Ws2tcpip.h> // for ipv6
#endif

// socket utility functions

#define ERR_WOULDBLOCK WSAEWOULDBLOCK
#define ERR_INPROGRESS WSAEINPROGRESS

#define SOCK_ERR() GetLastError()
// close a socket
#define CLOSE_SOCKET(s) do{if(s != INVALID_SOCKET){CSockUtil::CloseSocket(s);s = INVALID_SOCKET;}}while(0)

namespace hpsl
{
	class CSockUtil
	{
	public:
		static inline int CloseSocket(HL_SOCKET s){return closesocket(s);}
		static inline int CloseHandle(HL_HANDLE h){return ::CloseHandle(h);}
		// when encounter such error, APP should try again
		static inline bool ErrTryAgain(int err)
		{
			return (err==WSAEINTR);
		}

		static inline bool IoIsPending(int err)
		{
			return ((err==ERR_WOULDBLOCK) || (err==ERR_INPROGRESS));
		}

		// set file descriptor
		static inline int SockSetFD(HL_SOCKET sock, int value)
		{
			return 0;
		}

		// set non blocking
		static inline int SockSetNonBlocking(HL_SOCKET sock)
		{
			int res;
			unsigned long nonblocking = 1;
			res = ioctlsocket(sock, FIONBIO, (unsigned long*) &nonblocking);
			return res;
		}
		static int CreateSocketPair(int family, int type, int protocol, HL_SOCKET fd[2]); 
	};
}
#endif // __SOCK_UTILITY_H_
