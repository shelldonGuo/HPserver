/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#include "TimeUtil.h"
#include <stdio.h>

using namespace hpsl;

bool CTimeUtil::m_bMonotonicTime = 0;

// functions not implemented in windows
#ifdef WIN32

static int gettimeofday(struct timeval *ptv, struct timezone *ptz)
{
	struct _timeb tb;
	if(ptv == NULL)
	{
		return -1;
	}
	_ftime_s(&tb);
	ptv->tv_sec = (long)tb.time;
	ptv->tv_usec = (long)tb.millitm*1000;
	return 0;
}

#endif

void CTimeUtil::DetectMonotonic()
{
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)

	struct timespec	ts;
	if(!m_bMonotonicTime) // detect only when not set
	{
		if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
		{
			m_bMonotonicTime = true;
		}
	}
#endif
}

int CTimeUtil::GetSysTime(struct timeval *ptv)
{
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
	if(m_bMonotonicTime) {
		struct timespec	ts;
		if(ptv == NULL)
		{
			return -1;
		}
		if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
		{
			return (-1);
		}
		ptv->tv_sec = ts.tv_sec;
		ptv->tv_usec = ts.tv_nsec / 1000;
		return (0);
	}
#endif

	gettimeofday(ptv, NULL);
	return 0;
}
