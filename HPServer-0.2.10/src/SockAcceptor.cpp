/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#include "SockAcceptor.h"
#include <stdio.h>

using namespace hpsl;

HL_SOCKET CSockAcceptor::Open(const CInetAddr &addr)
{
    int on = 1, r;
    CInetAddr inetAddr(addr);
    do
    {
        // creat listen socket
        m_handle = socket(inetAddr.GetAF(), SOCK_STREAM, 0);

        if(m_handle == INVALID_SOCKET)
        {
            break;
        }
        // set non blocking
        if(CSockUtil::SockSetNonBlocking(m_handle) < 0) break;
        if(CSockUtil::SockSetFD(m_handle, 1) < 0) break;

        setsockopt(m_handle, SOL_SOCKET, SO_KEEPALIVE, (const char *)&on, sizeof(on));
        // bind
        r = bind(m_handle, inetAddr.GetAddr(), inetAddr.Size());
        if(r == -1) break;

        // listen
        r = listen(m_handle, 8);
        if(r == -1) break;

        return (m_handle);
    }while(0);
    
    CLOSE_SOCKET(m_handle);
    // error occurred
    return INVALID_SOCKET;
}

HL_SOCKET CSockAcceptor::Accept(CInetAddr *addr)
{
	// IPV4 version
    struct sockaddr sa;
	HL_SOCK_LEN_T len = sizeof(struct sockaddr);
	HL_SOCKET handle;
	do
	{
		// accept
		if((handle = accept(m_handle, &sa, &len)) == INVALID_SOCKET)
		{
			break;
		}
		// set nonblocking
		if(CSockUtil::SockSetNonBlocking(handle) < 0) break;
		
		if(addr != NULL)
		{
			addr->Construct(sa);
		}
        return handle;
	}while(0);

    CLOSE_SOCKET(handle);
    return INVALID_SOCKET;
}
