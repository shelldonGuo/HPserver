/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __TIME_UTIL_H_
#define __TIME_UTIL_H_

#include "HP_Config.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <sys/timeb.h>
#else

#include <sys/time.h>
#endif

namespace hpsl
{
    class CTimeUtil
    {
        static bool m_bMonotonicTime;
    public:
		// check whether it's monotonic time
		static void DetectMonotonic();
		static inline bool IsMonotonicTime() {return m_bMonotonicTime;}
		// time operations
        static inline void TimeClear(struct timeval *ptv)
        {
            ptv->tv_sec = ptv->tv_usec = 0;
        }

        static inline void TimeSet(struct timeval *ptv, int sec, int usec)
        {
            ptv->tv_sec = sec;
            ptv->tv_usec = usec;
        }

        static inline bool TimeGreater(const struct timeval *ptv1, const struct timeval *ptv2)
        {
            if(ptv1->tv_sec == ptv2->tv_sec) return (ptv1->tv_usec > ptv2->tv_usec);
            return (ptv1->tv_sec > ptv2->tv_sec);
        }

        static inline bool TimeLess(const struct timeval *ptv1, const struct timeval *ptv2)
        {
            if(ptv1->tv_sec == ptv2->tv_sec) return (ptv1->tv_usec < ptv2->tv_usec);
            return (ptv1->tv_sec < ptv2->tv_sec);
        }

        static inline bool TimeEuqal(const struct timeval *ptv1, const struct timeval *ptv2)
        {
            return ((ptv1->tv_sec==ptv2->tv_sec) && (ptv1->tv_usec==ptv2->tv_usec));
        }

        static inline void TimeAdd(const struct timeval *ptv1, const struct timeval *ptv2, struct timeval *vvp)
        {
            vvp->tv_sec = ptv1->tv_sec + ptv2->tv_sec;
            vvp->tv_usec = ptv1->tv_usec + ptv2->tv_usec;
            if(vvp->tv_usec >= 1000000)
            {
                vvp->tv_sec++;
                vvp->tv_usec -= 1000000;
            }
        }

        static inline void TimeSub(const struct timeval *ptv1, const struct timeval *ptv2, struct timeval *vvp)
        {
            vvp->tv_sec = ptv1->tv_sec - ptv2->tv_sec;
            vvp->tv_usec = ptv1->tv_usec - ptv2->tv_usec;
            if(vvp->tv_usec < 0)
            {
                vvp->tv_sec--;
                vvp->tv_usec += 1000000;
            }
        }
        // get system time
        static int GetSysTime(struct timeval *ptv);
    };
}
#endif // __TIME_UTIL_H_
