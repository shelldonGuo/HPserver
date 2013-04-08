#include "Reactor.h"
#include <signal.h>

#ifdef WIN32
#pragma comment(lib, "WS2_32.lib")
#else
#include <unistd.h>
#endif

using namespace hpsl;

int main(int argc, char** argv)
{
    CReactor reactor;
#ifdef WIN32
    WSADATA wsadata;
    memset(&wsadata, 0, sizeof(wsadata));
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif
    printf("-----------test init\n.-----------");
    //Initialize 
	IReactor_Imp* pEpollImp = NULL;
	IReactor_Imp* pSelectImp = NULL;
	pSelectImp = NewReactorImp(REACTOR_SELECT);
	if(pSelectImp == NULL) goto __exit;
	
    if(reactor.Initialize(pSelectImp) != 0)
		goto __exit;
	reactor.RunForever(0);

#ifndef WIN32
	pEpollImp = NewReactorImp(REACTOR_EPOLL);
	if(pEpollImp == NULL) goto __exit;
    if(reactor.Initialize(pEpollImp) != 0)
		goto __exit;
	reactor.RunForever(0);
#endif
	goto __suc;
__exit:
	printf("failed to setup reactor.\n");
	exit(1);
__suc:
	printf("reactor setup & fini test pass.\n");
    return 0;
}



