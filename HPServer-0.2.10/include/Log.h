/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __LOG_H_
#define __LOG_H_

//
// log
//
// common log format:
// <[TIME] [ERROR LEVEL]> [CPP] [LINE] [FUNCTION] [MESSGE] <[ERROR NO]> <[ERROR STRING]>
// NOTE: record in <> is auto added by log class
//
#include <string>
using namespace std;

#ifdef WIN32
#include <windows.h>

#define ERRNO() GetLastError()
#define snprintf(buf, n, ...) _snprintf_s((buf), (n), _TRUNCATE, __VA_ARGS__)

#else
#define ERRNO() errno
#endif

namespace hpsl
{
	enum LOG_LEVEL
	{
		LL_ERROR, // it's an error
		LL_WARN,  // it's a warning, but doesn't affect running
		LL_NORMAL, // normal information
		LL_DEBUG,  // for debug
		LL_DETAIL, // most detail
	};
	class CLog
	{
	public:
		CLog(){}
        ~CLog() {Close();}

		static inline void SetLogLevel(LOG_LEVEL level){m_level = level;}
		static inline int  GetLogLevel() {return m_level;}
		static inline void GetLogFile(string &file) {file = m_strLogFile;}
		static int SetLogFile(const string &file);
		static int SetLogFile(FILE *pF);

		static void Log(LOG_LEVEL level, const char *file, int line, int error, char *fmt, ...);        
		static void Log(LOG_LEVEL level, int error, char *fmt, ...);
        static void Close();

        static const char* GetLevelString(LOG_LEVEL level);
	private:
		static LOG_LEVEL m_level;
		static FILE *m_pLog;
		static string m_strLogFile;
	};
}

#endif // __DEFINES_H_
