/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#include "EventScheduler.h"
#include "Reactor.h"

using namespace hpsl;

void CDefScheduler::ScheduleActiveEvents(std::vector<DTEV_ITEM *> *pVecActiveList, int size)
{
    // iterate all active lists and process based on scheduler
    for(int i = size-1; i >= 0; --i)
    {
        if(!pVecActiveList[i].empty())
        {
            for(int n = 0; n < pVecActiveList[i].size(); ++n)
            {
                m_pReactor->ScheduleActiveEvent(pVecActiveList[i][n]); 
            }
            pVecActiveList[i].clear();
        }
    }
}

