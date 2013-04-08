/* 
 * Copyright (C) 2010. sparkling.liang@hotmail.com, barbecue.jb@gmail.com.
 * All rights reserved. 
 */

#ifndef __DEFINES_H_
#define __DEFINES_H_

typedef unsigned short u_short;
typedef unsigned long  u_long;
typedef unsigned int   u_int;

#if defined WIN32 // windows
#if defined UNICODE
typedef wchar_t tchar;
#else 
typedef char tchar;
#endif
#else // Linux
typedef char    tchar;
#ifndef TEXT
#define TEXT(x) x
#endif

#endif

// socket & handle values
#if defined WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
typedef SOCKET HL_SOCKET;
typedef HANDLE HL_HANDLE;
typedef int HL_SOCK_LEN_T;
#define INVALID_HANDLE INVALID_HANDLE_VALUE

#else

typedef int HL_SOCKET; 
typedef int HL_HANDLE;
#ifndef HL_SOCK_LEN_T
#define HL_SOCK_LEN_T socklen_t 
#endif 
#define INVALID_SOCKET -1
#define INVALID_HANDLE -1

#endif


// event types
#define EV_READ   (1<<0)
#define EV_WRITE  (1<<1)
#define EV_TIMER  (1<<2)
#define EV_SIGNAL (1<<3)

// internal use only
#define EVFLAG_INTERNAL (1<<14) // it's a handler internally use
#define EVFLAG_TOBE_DEL (1<<15) // to be deleted

// max priority
#define MAX_EV_PRIORITY 64

// this flag is only enabled for read/write event
enum EVRW_SIDE {INNER = 1, OUTER = 2};

#define __Free(p) do{if(p != NULL){free(p); p = NULL;}}while(0)

#endif // __DEFINES_H_
