/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __SIGNAL_SET_H_
#define __SIGNAL_SET_H_

// 
// signal set, used to store all signal event handlers
//

#include "EventItem.h"

#include <map>

namespace hpsl
{
    typedef std::map<IEventHandler*, DTEV_ITEM> SEH_MAP;
    typedef std::pair<IEventHandler*, DTEV_ITEM> SEH_PAIR;
    typedef SEH_MAP::iterator SEH_ITR;
    class CSignalSet
    {
    public:
        CSignalSet(){}
        ~CSignalSet() {}
        int  Initialize(){m_mapHandlers.clear(); return 0;}
        void Finalize(){m_mapHandlers.clear();}

        inline int Insert(IEventHandler *pHandler);
        inline void Remove(IEventHandler *pHandler);
        inline DTEV_ITEM *GetAt(IEventHandler *pHandler);
        inline DTEV_ITEM *GetFirst();
        inline DTEV_ITEM *GetNext();
        inline size_t GetSize() {return m_mapHandlers.size();}
    private:
        SEH_MAP  m_mapHandlers;
        SEH_ITR  m_itr;
    };
    inline int CSignalSet::Insert(IEventHandler *pHandler)
    {
        // insert into map
        SEH_PAIR prItem;
        prItem.first = pHandler;
        DTEV_ITEM &evItem = prItem.second;
        memset(&evItem, 0, sizeof(DTEV_ITEM));
        // initialize index in heap to -1
        evItem.heapIndex = -1;
        evItem.pHandler = pHandler;

        std::pair<SEH_MAP::iterator, bool> pr;
        pr = m_mapHandlers.insert(prItem);
        if(pr.second) return 0;
        return -1;
    }

    inline void CSignalSet::Remove(IEventHandler* pHandler)
    {
        if(pHandler != NULL)
        {
            m_mapHandlers.erase(pHandler);
        }
    }

    inline DTEV_ITEM *CSignalSet::GetAt(IEventHandler* pHandler)
    {
        if(pHandler != NULL)
        {
             SEH_ITR itr = m_mapHandlers.find(pHandler);
             if(itr != m_mapHandlers.end())
             {
                 return &(itr->second);
             }
        }
        return NULL;
    }

    inline DTEV_ITEM *CSignalSet::GetFirst()
    {
        m_itr = m_mapHandlers.begin();
        if(m_itr != m_mapHandlers.end())
        {
            return &(m_itr->second);
        }
        return NULL;
    }

    inline DTEV_ITEM *CSignalSet::GetNext()
    {
        if(m_itr != m_mapHandlers.end())
        {
            ++m_itr;
            if(m_itr != m_mapHandlers.end())
            {
                return &(m_itr->second);
            }
        }
        return NULL;
    }
}

#endif // __SIGNAL_SET_H_
