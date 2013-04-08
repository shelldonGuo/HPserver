/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __EVENT_ITEM_H_
#define __EVENT_ITEM_H_

// 
// core data structure of HPServer.
// store registered handle and its binded event handler,  its registered events, and its timer index in timer heap
// flags, and other information
//

#include "defines.h"
#include "TimeUtil.h"
#include "EventHandler.h"

#define __TO_HANDLER(pEv) ((IEventHandler*)((pEv)->pHandler))
namespace hpsl
{
	struct DTEV_ITEM // event item structure
	{
        // 通过_TO_HANDLER转化为一个IEventHandler指针
		const void *pHandler; // event handler/completion handler the handle binded to
		HL_SOCKET      handle; // hande of the event item, in linux it's the same as the index in DTEV_ITEM array
		short events, evActive;
        short nsigs; // for signals 
		short flags; // event flags
        // for timer event management
        int   heapIndex; // index in timer min-heap
		struct timeval timeout; // next timeout value
        struct timeval period; // period value
	};
}

#endif // __EVENT_ITEM_H_
