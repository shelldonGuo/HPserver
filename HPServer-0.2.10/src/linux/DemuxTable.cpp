/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */


#include "DemuxTable.h"
#include "Log.h"

using namespace hpsl;

CDemuxTable::CDemuxTable():m_iSize(0), m_iCapacity(0)
{
	for(int i = 0; i < ELE_SET_ARRAY_NUM; i++)
	{
		m_pTable[i].item_array = NULL;
	}
}

int CDemuxTable::Initialize()
{
	for(int i = 0; i < ELE_SET_ARRAY_NUM; i++)
	{
		m_pTable[i].item_array = NULL;
	}
	// allocate memory for the first element
    m_pTable[0].item_array = (DTEV_ITEM*)calloc(ELE_SET_SIZE, sizeof(DTEV_ITEM));
    if(m_pTable[0].item_array == NULL)
    {
        return -1;
    }
    m_iSize = 0;
	m_iCapacity = ELE_SET_SIZE;
    return 0;
}

void CDemuxTable::Finalize()
{
    m_iSize = m_iCapacity = 0;
    for(int i = 0; i < ELE_SET_ARRAY_NUM; i++)
	{
		__Free(m_pTable[i].item_array);
	}
}

int CDemuxTable::recalc(int max)
{
	if(max >= (ELE_SET_ARRAY_NUM<<ELE_SET_SIZE_SHIFT))
	{
		return -1;
	}
	while(m_iCapacity <= max)
	{
		// allocate memory, and initialized to zero
		int idx = (m_iCapacity>>ELE_SET_SIZE_SHIFT);		
		m_pTable[idx].item_array = (DTEV_ITEM*)calloc(ELE_SET_SIZE, sizeof(DTEV_ITEM)); 
		if(m_pTable[idx].item_array == NULL)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "Failed to expand demux table space.");
			return (-1);
		}
		m_iCapacity += ELE_SET_SIZE;
		CLog::Log(LL_DETAIL, __FILE__, __LINE__, 0, "Current demux table space:%u.", m_iCapacity);
	}
    return 0;
}

