/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#include "Log.h"
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

using namespace hpsl;

const char *gStrLevelString[] = 
{
    "ERROR", 
    "WARNING",
    "NORMAL",
    "DEBUG",
    "DETAIL",
};

LOG_LEVEL CLog::m_level = LL_NORMAL;
FILE     *CLog::m_pLog  = NULL;
string    CLog::m_strLogFile = "";

const char* CLog::GetLevelString(LOG_LEVEL level)
{
    int i = level;
    if((i>=0) && (i<=LL_DETAIL))
    {
        return gStrLevelString[i];
    }
    return "UNKNOWN";
}

void CLog::Close()
{
    if(m_pLog)
    {
        if((m_pLog!=stdout) && (m_pLog!=stderr))
        {
            fclose(m_pLog);
        }
        m_pLog = NULL;
    }
}

int CLog::SetLogFile(const string &file)
{
	FILE *pLog = fopen(file.c_str(), "a+");
	int res = SetLogFile(pLog);
	if(res == 0)
	{
		m_strLogFile = file;
	}
	return res;
}

int CLog::SetLogFile(FILE *pF)
{
	if(pF == NULL)
	{
		return -1;
	}
    Close();
	m_pLog = pF;
	m_strLogFile = "";

	return 0;
}

void CLog::Log(LOG_LEVEL level, const char *file, int line, int error, char *fmt, ...)
{
    if(m_pLog == NULL) return; // no log file specified
	if(level > m_level) return; // too detail
	// format time
	char strtime[128];
	time_t t = time(NULL);
	struct tm *tt = gmtime(&t);
	snprintf(strtime, sizeof(strtime)-1, "%d-%d-%d %d:%d:%d", 1900+tt->tm_year, tt->tm_mon, 
		tt->tm_yday, tt->tm_hour, tt->tm_min, tt->tm_sec);

	
	// format message
	char buf[64*1024]; // max error message size in Windows
	va_list maker;
	va_start(maker, fmt);
	vsnprintf(buf, sizeof(buf)-1, fmt, maker);
	va_end(maker);
	fprintf(m_pLog, "[%s][%s][%s][line:%d]%s[errno=%d]", strtime, GetLevelString(level), file, line, buf, error);
	// format error strings
	buf[0] = '\0';
	if(error != 0)
	{
#ifdef WIN32 // Windows
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, DWORD(error), 0, buf, sizeof(buf)-1, NULL);
		fprintf(m_pLog, "%s.\n", buf);
#else // Linux
		fprintf(m_pLog, "%s.\n", strerror(error));
#endif
	}
	else
	{
		fprintf(m_pLog, ".\n");
	}
	// flush file for error message
	if(level == LL_ERROR)
	{
		fflush(m_pLog);
	}
}

void CLog::Log(LOG_LEVEL level, int error, char *fmt, ...)
{
    if(m_pLog == NULL) return; // no log file specified
	if(level > m_level) return; // too detail
	// format time
	char strtime[128];
	time_t t = time(NULL);
	struct tm *tt = gmtime(&t);
	snprintf(strtime, sizeof(strtime)-1, "%d-%d-%d %d:%d:%d", 1900+tt->tm_year, tt->tm_mon, 
		tt->tm_yday, tt->tm_hour, tt->tm_min, tt->tm_sec);

	
	// format message
	char buf[64*1024]; // max error message size in Windows
	va_list maker;
	va_start(maker, fmt);
	vsnprintf(buf, sizeof(buf)-1, fmt, maker);
	va_end(maker);
	fprintf(m_pLog, "[%s][%s]%s[errno=%d]", strtime, GetLevelString(level), buf, error);
	// format error strings
	buf[0] = '\0';
	if(error != 0)
	{
#ifdef WIN32 // Windows
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, DWORD(error), 0, buf, sizeof(buf)-1, NULL);
		fprintf(m_pLog, "%s.\n", buf);
#else // Linux
		fprintf(m_pLog, "%s.\n", strerror(error));
#endif
	}
	else
	{
		fprintf(m_pLog, ".\n");
	}
	// flush file for error message
	if(level == LL_ERROR)
	{
		fflush(m_pLog);
	}
}
