/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __INET_ADDR_H_
#define __INET_ADDR_H_

//
// a wrapper class of socket addr
// support V4 & V6
//

#include "SockUtil.h"
#include <memory.h>

namespace hpsl
{
    enum IP_VER{
        IP_V4 = AF_INET, 
#ifdef IPV6_SUPPORT
        IP_V6 = AF_INET6,
#endif
    };


#ifdef IPV6_SUPPORT // support IP V6

    // a function to transfer (sockaddr, len) pair into sockaddr_in or sockaddr_in6

    class CInetAddr
    {
    public:
        CInetAddr():_ver(IP_V4) // default is IP v4 
        {
            memset(&_addr6, 0, sizeof(sockaddr_in6));
        }
        CInetAddr(const char *addr, u_short port, IP_VER ver) {Construct(addr, port, ver);}
        CInetAddr(const sockaddr &addr, IP_VER ver) 
        {
            switch(ver)
            {
            case IP_V4: Construct(addr); break;
            case IP_V6: Construct6(addr); break;
            }
        }
        void Construct(const char *addr, u_short port, IP_VER ver);
        inline void Construct(const sockaddr &addr);
        inline void Construct6(const sockaddr &addr6);

        inline int GetAF() {return _ver;}
        inline u_short GetPort();
        inline sockaddr *GetAddr();
        inline size_t Size();
    private:
        union
        {
            struct sockaddr_in  _addr4;
            struct sockaddr_in6 _addr6;
        };
        IP_VER _ver;
    };


    inline void CInetAddr::Construct6(const sockaddr &addr6)
    {
        _ver = IP_V6;
        memcpy(&_addr6, &addr6, sizeof(sockaddr_in6));
    }

    inline void CInetAddr::Construct(const sockaddr &addr)
    {
        _ver = IP_V4;
        memcpy(&_addr4, &addr, sizeof(sockaddr_in));
    }

    inline u_short CInetAddr::GetPort()
    {
        switch(_ver)
        {
        case IP_V4: return _addr4.sin_port;
        case IP_V6: return _addr6.sin6_port;
        }
        return 0; // should never goes here
    }

    inline size_t CInetAddr::Size()
    {
        switch(_ver)
        {
        case IP_V4: return sizeof(_addr4);
        case IP_V6: return sizeof(_addr6);
        }
        return 0; // should never goes here
    }

    inline sockaddr* CInetAddr::GetAddr()
    {
        switch(_ver)
        {
        case IP_V4: return (sockaddr*)(&_addr4);
        case IP_V6: return (sockaddr*)(&_addr6);
        }
        return 0; // should never goes here
    }

#else

    class CInetAddr
    {
    public:
        CInetAddr() {memset(&_addr4, 0, sizeof(sockaddr_in));}
        CInetAddr(const sockaddr &addr, IP_VER ver) {Construct(addr);}
        CInetAddr(const char *addr, u_short port, IP_VER ver) {Construct(addr, port, IP_V4);}

        void Construct(const char *addr, u_short port, IP_VER ver);
        inline void Construct(const sockaddr &addr);

        inline int GetAF() {return IP_V4;}
        inline u_short GetPort() {return _addr4.sin_port;}
        inline sockaddr *GetAddr() {return (sockaddr*)(&_addr4);}
        inline size_t Size() {return sizeof(_addr4);}
    private:
        struct sockaddr_in  _addr4;
    };


    inline void CInetAddr::Construct(const sockaddr &addr)
    {
        memcpy(&_addr4, &addr, sizeof(sockaddr_in));
    }

#endif

}
#endif // __INET_ADDR_H_
