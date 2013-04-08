/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __MIN_HEAP_H_
#define __MIN_HEAP_H_

#include "defines.h"
#include "TimeUtil.h"
#include "EventItem.h"

#include <stdio.h>

//
// a min-heap, used to manager timeout events
// the element is a pointer to DTEV_ITEM struct
//

namespace hpsl
{
 	class CMinHeap
	{
	public:
        CMinHeap():m_pHeap(NULL), m_iSize(0), m_iCapacity(0){}
		~CMinHeap(){Finalize();}
        inline int  Initialize() {return Reserve(1024);} // default reserve 1024
        inline void Finalize();
        inline int Reserve(int capacity);
        inline int Push(DTEV_ITEM *pEvItem); // push item
        inline DTEV_ITEM *Top(); // get top
        inline DTEV_ITEM *Pop(); // pop
        inline void Update(DTEV_ITEM *pEvItem);
        inline void Erase(DTEV_ITEM *pEvItem);
		// subtract tv from timeout value of all of each timer
		inline void AdjustTimer(const struct timeval &tv);
        inline bool IsEmpty()     {return (m_iSize==0);}
        inline int  GetSize()     {return m_iSize;} // get current used size
        inline int  GetCapacity() {return m_iCapacity;}
    private:
        inline void shiftUp(int holeIdx, DTEV_ITEM *pEvItem);
        inline void shiftDown(int holeIdx, DTEV_ITEM *pEvItem);
        DTEV_ITEM **m_pHeap;
        int  m_iSize, m_iCapacity;
	};

    inline void CMinHeap::Finalize()
    {
        __Free(m_pHeap);
        m_iCapacity = m_iSize = 0;
    }
    // reserve vacancies, if still has enough space return directly
    inline int CMinHeap::Reserve(int capacity)
    {
        if(capacity > m_iCapacity)
        {
            DTEV_ITEM **pHeap = (DTEV_ITEM**)realloc(m_pHeap, capacity*sizeof(DTEV_ITEM*));
            if(pHeap == NULL)
            {
                return -1;
            }
            m_pHeap = pHeap;
            memset(pHeap+m_iCapacity, 0, (capacity-m_iCapacity)*sizeof(DTEV_ITEM*));
            m_iCapacity = capacity;
        }
        return 0;
    }
    inline DTEV_ITEM *CMinHeap::Top()
    {
        if(m_iSize > 0)
        {
            return m_pHeap[0];
        }
        return NULL;
    }
    inline DTEV_ITEM *CMinHeap::Pop()
    {
        if(m_iSize > 0)
        {
            DTEV_ITEM *pEvItem = m_pHeap[0];
            // reserve a vacancy at top for the last element & shift it down
            shiftDown(0, m_pHeap[m_iSize-1]);
            m_iSize--;
            // clear related index in min-heap
            pEvItem->heapIndex = -1;
            return pEvItem;
        }
        return NULL;
    }
    inline int CMinHeap::Push(DTEV_ITEM *pEvItem)
    {
        if(pEvItem != NULL)
        {
            if(Reserve(m_iSize+1) == 0)
            {
                // reserve a vacancy at last for the new element & shift it up
                shiftUp(m_iSize, pEvItem);
                m_iSize++;
                return 0;
            }
        }
        return -1;
    }   
    
    inline void CMinHeap::shiftDown(int holeIdx, DTEV_ITEM *pEvItem)
    {
        int lc = (holeIdx<<1)+1;
        while(lc < m_iSize)
        {
            int rc = lc+1;
            int parent = holeIdx; // parent points to the current hole
            // select next node (left or right child) as hole
            if((rc<m_iSize) && CTimeUtil::TimeLess(&m_pHeap[rc]->timeout, &m_pHeap[lc]->timeout))
            {
                holeIdx = rc;
            }
            else
            {
                holeIdx = lc;
            }
            if(!CTimeUtil::TimeGreater(&pEvItem->timeout, &m_pHeap[holeIdx]->timeout))
            {
                holeIdx = parent; // place at current hole, item <= next hole's time out
                break;
            }
            // exchange, shift holeIdx up
			m_pHeap[holeIdx]->heapIndex = parent;
            m_pHeap[parent] = m_pHeap[holeIdx];
            
            lc = (holeIdx<<1)+1;
        }
        // set heap index
        pEvItem->heapIndex = holeIdx;
        m_pHeap[holeIdx] = pEvItem;
    }

    inline void CMinHeap::shiftUp(int holeIdx, DTEV_ITEM *pEvItem)
    {
        int parent;
        DTEV_ITEM *p;
        while(holeIdx > 0)
        {
            parent = ((holeIdx-1)>>1);
            p = m_pHeap[parent];
            if(CTimeUtil::TimeLess(&pEvItem->timeout, &p->timeout)) // exchange
            {
                m_pHeap[holeIdx] = p; // shift down parent
                // update index in heap
                p->heapIndex = holeIdx;
                holeIdx = parent;
            }
            else
            {
                break;
            }
        }
        // set heap index
        pEvItem->heapIndex = holeIdx;
        m_pHeap[holeIdx] = pEvItem;
    }
    inline void CMinHeap::Update(DTEV_ITEM *pEvItem)
    {     
        int holeIdx = pEvItem->heapIndex;
        if(holeIdx >= 0)
        {
            int parent = ((holeIdx-1)>>1);
            if((holeIdx>0) && CTimeUtil::TimeGreater(&m_pHeap[parent]->timeout, &pEvItem->timeout))
            {
                // item's timeout decreaed, and <= parent, so shift it up
                shiftUp(holeIdx, pEvItem);
            }
            else
            {
                // just try to shift it down, if position not changed
                // pEvItem will reset to holeIdx, which is just it's current position
                shiftDown(holeIdx, pEvItem);
            }
        }
    }
    inline void CMinHeap::Erase(DTEV_ITEM *pEvItem)
    {
        int holeIdx = pEvItem->heapIndex;
        if(holeIdx >= 0)
        {
            // reserve a vacancy at the position of pEvItem for the last element
            DTEV_ITEM *last = m_pHeap[m_iSize-1];
            int parent = ((holeIdx-1)>>1);
            if((holeIdx>0) && CTimeUtil::TimeGreater(&m_pHeap[parent]->timeout, &last->timeout))
            {
                // last <= parent, so shift last up (from hole)
                shiftUp(holeIdx, last);
            }
            else
            {
                // last > parent, so shift last down (from hole)
                shiftDown(holeIdx, last);
            }
            m_iSize--;
            // clear related index in min-heap
            pEvItem->heapIndex = -1;
        }
    }
	inline void CMinHeap::AdjustTimer(const struct timeval &tv)
	{
		for(int i = 0; i < m_iSize; i++)
		{
			struct timeval *ptv = &m_pHeap[i]->timeout;
			CTimeUtil::TimeSub(ptv, &tv, ptv);
		}
	}
}


#endif // __MIN_HEAP_H_
