/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#include "SockUtil.h"

using namespace hpsl;
#define SET_SOCK_ERR(errcode) do{WSASetLastError(errcode);}while(0)
	
int CSockUtil::CreateSocketPair(int family, int type, int protocol, HL_SOCKET fd[])
{
	HL_SOCKET listener = INVALID_SOCKET;
	HL_SOCKET connector = INVALID_SOCKET;
	HL_SOCKET acceptor = INVALID_SOCKET;	
	int size = -1, ret = -1, saved_errno = -1;
	struct sockaddr_in listen_addr;
	struct sockaddr_in connect_addr;
	//memset(&listen_addr, 0, sizeof(struct sockaddr_in));
	//memset(&connect_addr, 0, sizeof(struct sockaddr_in));
	
	if (protocol
#ifdef AF_UNIX
		|| family!=AF_UNIX
#endif
		)
	{
		SET_SOCK_ERR(WSAEAFNOSUPPORT);
		return -1;
	}
	if (!fd)
	{
		SET_SOCK_ERR(WSAEINVAL);
		return -1;
	}
	do 
	{
		listener = socket(AF_INET, type, IPPROTO_TCP);	
		if (listener < 0)
			break;
		listen_addr.sin_family = AF_INET;
		//set ip as 127.0.0.1
		listen_addr.sin_addr.S_un.S_addr = htonl(INADDR_LOOPBACK);
		//assigned by OS?
		listen_addr.sin_port = 0; 
		ret = bind(listener, (struct sockaddr *) &listen_addr, sizeof(listen_addr));
		if (ret == -1) 
			break;
		ret = listen(listener,1);
		connector = socket(AF_INET, type, 0);
		if (connector < 0)
			break;
		//find out the port number to connect to
		size = sizeof(connect_addr);
		ret = getsockname(listener,(struct sockaddr *) &connect_addr, &size);
		if (ret ==-1)
			break;
		if (size != sizeof (connect_addr))
		{
			saved_errno = WSAECONNABORTED;
			break;
		}	
		
		ret = connect(connector, (struct sockaddr *) &connect_addr, sizeof(connect_addr));
		if (ret == -1)
			break;
		size = sizeof(listen_addr);
		acceptor = accept(listener, (struct sockaddr *) &listen_addr, &size);
		if (acceptor < 0)
			break;
		if (size != sizeof(listen_addr))
		{
			saved_errno = WSAECONNABORTED;
			break;
		}
		CloseSocket(listener);
		/* Now check we are talking to ourself by matching port and host on the
			two sockets.*/
		ret = getsockname(connector, (struct sockaddr*) &connect_addr, &size);
		if (ret == -1)
			break;
		if (size != sizeof (connect_addr)
			|| listen_addr.sin_family != connect_addr.sin_family
			|| listen_addr.sin_addr.S_un.S_addr != connect_addr.sin_addr.S_un.S_addr
			|| listen_addr.sin_port != connect_addr.sin_port)
		{
			saved_errno = WSAECONNABORTED;
			break;
		}
		fd[0] = connector;
		fd[1] = acceptor;
		return 0;
	} while (0);
	if (saved_errno < 0)
		saved_errno = SOCK_ERR();
	if (listener !=-1)
		CloseSocket(listener);
	if (connector != -1)
		CloseSocket(connector);
	if (acceptor != -1)
		CloseSocket(acceptor);
	return -1;
}
