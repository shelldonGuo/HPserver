/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */
 
#ifndef __DEMUX_TABLE_H_
#define __DEMUX_TABLE_H_

// 
// demultiplex table, convert from handle to {IEventHandler, event, current event}
// in Linux HL_SOCKET is linear, so we can use an array directly
//

#include "defines.h"
#include "SockUtil.h"
#include "EventHandler.h"
#include "EventItem.h"


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define ELE_SET_SIZE 4096
#define ELE_SET_SIZE_SHIFT 12
#define ELE_SET_ARRAY_NUM 256

namespace hpsl
{
    // Event Handler table
    // 保存event handler的table
    // 可以通过dispather(Reactor)的registe handle和remove handle创建和删除
    class CDemuxTable
    {
    public:
        CDemuxTable();
        ~CDemuxTable() {}

        int  Initialize();
        void Finalize();

        inline int Insert(HL_SOCKET handle, const void *pHandler);
		inline int Remove(HL_SOCKET handle);

		inline DTEV_ITEM *GetAt(HL_SOCKET handle);
        inline DTEV_ITEM *GetFirst();
        inline DTEV_ITEM *GetNext();
		inline size_t GetSize()     {return m_iSize;}
		inline size_t GetCapacity() {return m_iCapacity;}
	private:
        int recalc(int max);
		inline DTEV_ITEM* getEvItem(HL_SOCKET handle);
		struct ELE_SET
		{
			DTEV_ITEM *item_array;
		};
		ELE_SET m_pTable[ELE_SET_ARRAY_NUM];
        size_t m_iSize, m_iCapacity;
        size_t m_itr; // iterator
    };
	
	inline DTEV_ITEM* CDemuxTable::getEvItem(HL_SOCKET handle)
	{
		int idx = (handle>>ELE_SET_SIZE_SHIFT);
		int idx_item = handle - (idx<<ELE_SET_SIZE_SHIFT);
		return (&(m_pTable[idx].item_array[idx_item]));
	}
    inline int CDemuxTable::Insert(HL_SOCKET handle, const void *pHandler)
    {
        if(handle < 0)
        {
            return -1;
        }
        if(handle >= m_iCapacity)
        {
            if(recalc(handle) < 0)
            {
                return -1;
            }
        }
		DTEV_ITEM *pEvItem = getEvItem(handle);
		if(pEvItem->pHandler != NULL)
		{
			return -1;
        }
		memset(pEvItem, 0, sizeof(DTEV_ITEM));
         // initialize index in heap to -1
        pEvItem->heapIndex = -1;
        pEvItem->pHandler = pHandler;
		pEvItem->handle = handle;
		++m_iSize;
        return 0;
    }
	inline int CDemuxTable::Remove(HL_SOCKET handle)
	{
		if((handle>=0) && (handle<m_iCapacity))
		{
			DTEV_ITEM *pEvItem = getEvItem(handle);
			pEvItem->pHandler = NULL;
			--m_iSize;
			return 0;
		}
		return -1;
	}
	inline DTEV_ITEM *CDemuxTable::GetAt(HL_SOCKET handle) 
	{
		if((handle>=0) && (handle<m_iCapacity))
		{
			return getEvItem(handle);
		}
		return NULL;
	}
	inline DTEV_ITEM *CDemuxTable::GetFirst() 
	{
		m_itr = 0;
		if(m_iCapacity > 0)
        {
			return getEvItem(0);
		}
		return NULL;
	}
	inline DTEV_ITEM *CDemuxTable::GetNext() 
	{
        if((++m_itr) < m_iCapacity)
        {
            return getEvItem(m_itr);
        }
        return NULL;
	}
}

#endif // __DEMUX_TABLE_H_
