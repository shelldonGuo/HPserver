/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __I_REACTOR_IMP_H_
#define __I_REACTOR_IMP_H_

#include "defines.h"
#include "TimeUtil.h"
#include <stdio.h>

//
// Reactor_Imp is the real I/O demultiplex method implementation class
// It only takes charge of Read/Write event
//
// each Reactor Implementation class MUST implement:
// Initialize()
// Finalize()
// GetMethod()
// RegisterEvent()
// UnregisterEvent()
// Dispatch()

// reactor types
enum REACTOR_TYPE{
    REACTOR_SELECT = 1, 
#ifdef WIN32 // types for win32
#else // types for linux
    REACTOR_EPOLL, 
#endif 
};

#define DeleteReactorImp(pImp) do{  \
    if(pImp != NULL){   \
	pImp->Finalize();	\
    delete pImp;    \
    pImp = NULL;    \
    }                   \
    }while(0)

namespace hpsl
{
    class CReactor;
	struct DTEV_ITEM;
    // dispatcher接口
    // 实际的dispatch有Select和epoll两种实现
    class IReactor_Imp
    {
    public:
        IReactor_Imp(){}
        virtual ~IReactor_Imp(){}

        // initialize I/O method and set event factory
        virtual int Initialize(CReactor *pReactor) = 0;
        virtual int Finalize() = 0;
        virtual int Dispatch(struct timeval *tv) = 0;
        // get kernel mechanism
        virtual const tchar *GetMethod() = 0;

        // register event read/write
        // @event: EV_READ or EV_WRITE
        virtual int RegisterEvent(const DTEV_ITEM *pEvItem, short event) = 0;
        // unregister event read/write
        // @event: EV_READ or EV_WRITE
        virtual int UnregisterEvent(const DTEV_ITEM *pEvItem, short event) = 0;
    };
    IReactor_Imp *NewReactorImp(int type);
}


#endif // __I_REACTOR_IMP_H_
