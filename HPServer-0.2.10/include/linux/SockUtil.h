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


// file headers
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#define ERR_WOULDBLOCK EWOULDBLOCK
#define ERR_INPROGRESS EINPROGRESS
// socket utility functions
#define SOCK_ERR() (errno)
// close a socket
#define CLOSE_SOCKET(s) do{if(s != INVALID_SOCKET){CSockUtil::CloseSocket(s);s = INVALID_SOCKET;}}while(0)

namespace hpsl
{
	class CSockUtil
	{
	public:
		static inline int CloseSocket(HL_SOCKET s){return close(s);}
		static inline int CloseHandle(HL_HANDLE h){return close(h);}
		// when encounter such error, APP should try again
		static inline bool ErrTryAgain(int err)
		{
			return ((err==EINTR)||(err==EAGAIN));
		}

		static inline bool IoIsPending(int err)
		{
			return ((err==ERR_WOULDBLOCK) || (err==ERR_INPROGRESS));
		}

		// set file descriptor
		static inline int SockSetFD(HL_SOCKET sock, int value)
		{
			return fcntl(sock, F_SETFD, value);
		}

		// set non blocking
		static inline int SockSetNonBlocking(HL_SOCKET sock)
		{
			int res;
			res = fcntl(sock, F_SETFL, O_NONBLOCK);
			return res;
		}
		static inline int CreateSocketPair(int family, int type, int protocol, HL_SOCKET fd[2])
		{
			return socketpair(family, type, protocol, fd);
		}
	};
}
#endif // __SOCK_UTILITY_H_
