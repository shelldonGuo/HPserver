/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __SOCK_ACCEPTOR_H_
#define __SOCK_ACCEPTOR_H_

// 
// a wraper class for socket as an acceptor
//

#include "InetAddr.h"
#include "SockUtil.h"

namespace hpsl
{
    class CSockAcceptor
    {
    public:
        CSockAcceptor():m_handle(INVALID_SOCKET) {}
        ~CSockAcceptor() {CLOSE_SOCKET(m_handle);}

        HL_SOCKET Open(const CInetAddr &addr);
        HL_SOCKET Accept(CInetAddr *addr);
        inline void Close() {CLOSE_SOCKET(m_handle);}
        HL_SOCKET GetHandle() {return m_handle;}

    private:
        HL_SOCKET m_handle;
    };
}

#endif // __SOCK_ACCEPTOR_H_
