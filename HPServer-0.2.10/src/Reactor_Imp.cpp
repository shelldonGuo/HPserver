/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#include "Reactor_Imp.h"
#include "Reactor_Select.h"

#ifdef WIN32
namespace hpsl
{
    IReactor_Imp *NewReactorImp(int type)
    {
        switch(type)
        {
        case REACTOR_SELECT: return new CReactor_Select();
        }
        return NULL;
    }
}

#else

#include "Reactor_Epoll.h"

namespace hpsl
{
    IReactor_Imp *NewReactorImp(int type)
    {
        switch(type)
        {
        case REACTOR_EPOLL: return new CReactor_Epoll();
        case REACTOR_SELECT: return new CReactor_Select();
        }
        return NULL;
    }
}

#endif
