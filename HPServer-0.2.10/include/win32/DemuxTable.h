/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __DEMUX_TABLE_H_
#define __DEMUX_TABLE_H_

// 
// demultiplex table, convert from handle to IEventHandler
// in linux HL_SOCKET is linear, so we can use an array directly
// in windows uing a map, for HL_SOCKET is a pointer
//

#include "defines.h"
#include "SockUtil.h"
#include "EventHandler.h"
#include "EventItem.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <map>
namespace hpsl
{
    typedef std::map<HL_SOCKET, DTEV_ITEM> EH_MAP;
    typedef std::pair<HL_SOCKET, DTEV_ITEM> EH_PAIR;
    class CDemuxTable
    {
    public:
        CDemuxTable(){}
        ~CDemuxTable() {}

        inline int  Initialize();
        inline void Finalize();

        inline int Insert(HL_SOCKET handle, const void *pHandler);
		inline int Remove(HL_SOCKET handle);

		inline DTEV_ITEM *GetAt(HL_SOCKET handle);
        inline DTEV_ITEM *GetFirst();
        inline DTEV_ITEM *GetNext();

        inline size_t GetSize() {return (int)m_mapHandlers.size();}
        inline size_t GetCapacity() {return m_mapHandlers.max_size();}
    private:
        EH_MAP       m_mapHandlers;
        EH_MAP::iterator m_itr; // used for iteration
    };
    int CDemuxTable::Initialize()
    {
        m_mapHandlers.clear();
        return 0;
    }
    void CDemuxTable::Finalize()
    {
        m_mapHandlers.clear();
    }
    inline int CDemuxTable::Insert(HL_SOCKET handle, const void *pHandler)
    {
        // insert into map, duplicate key insert will return false
        std::pair<EH_MAP::iterator, bool> pr;
        EH_PAIR pair;
        pair.first = handle;
        DTEV_ITEM *pEvItem = &(pair.second);

        memset(pEvItem, 0, sizeof(DTEV_ITEM));
        pEvItem->pHandler = pHandler;
		pEvItem->handle   = handle;
        pEvItem->heapIndex = -1;

        pr = m_mapHandlers.insert(pair);
        return (pr.second ? 0 : -1);
    }
    inline int CDemuxTable::Remove(HL_SOCKET handle)
    {
        m_mapHandlers.erase(handle); // 0: no such element, 1 removed
        return 0;
    }

    inline DTEV_ITEM *CDemuxTable::GetAt(HL_SOCKET handle)
    {
        EH_MAP::iterator itr = m_mapHandlers.find(handle);
        return &(itr->second);
    }
    inline DTEV_ITEM *CDemuxTable::GetFirst()
    {
        m_itr = m_mapHandlers.begin();
        if(m_itr != m_mapHandlers.end())
        {
            return &(m_itr->second);
        }
        return NULL;
    }
    inline DTEV_ITEM *CDemuxTable::GetNext()
    {
        ++m_itr;
        if(m_itr != m_mapHandlers.end())
        {
            return &(m_itr->second);
        }
        return NULL;
    }
}

#endif // __DEMUX_TABLE_H_
