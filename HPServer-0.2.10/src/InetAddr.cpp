/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#include "InetAddr.h"

using namespace hpsl;

inline void __constructAddr4(struct sockaddr_in &_addr4, const char *addr, u_short port, IP_VER ver)
{
	_addr4.sin_family = AF_INET;
	_addr4.sin_port = htons(port);
	if((addr == NULL) || (addr[0] == '\0'))
	{
		_addr4.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		_addr4.sin_addr.s_addr = inet_addr(addr);
	}
}


#ifdef IPV6_SUPPORT

void CInetAddr::Construct(const char *addr, u_short port, IP_VER ver)
{
    _ver = ver;
    switch(ver)
    {
    case IP_V6: // IPV6
        _addr6.sin6_family = AF_INET6;
        _addr6.sin6_port = htons(port);
        if((addr == NULL) || (addr[0] == '\0'))
        {
            _addr6.sin6_addr = INADDR_ANY6;
        }
        else
        {
            inet_pton(AF_INET6, addr, &_addr6.sin6_addr);
        }
        break;
    case IP_V4: // IPV4
		__constructAddr4(_addr4, addr, port, ver);
        break;
    }
}


#else

void CInetAddr::Construct(const char *addr, u_short port, IP_VER ver)
{
	__constructAddr4(_addr4, addr, port, ver);
}

#endif
