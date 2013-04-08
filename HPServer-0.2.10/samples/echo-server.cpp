
#include "echo-server.h"

#define WORKER_NUM 4
CEchoServer g_echoServer;
CWorker g_worker[WORKER_NUM];


int g_worker_id;
void send_to_worker(HL_SOCKET fd)
{
	int workerid = (g_worker_id+1)%WORKER_NUM;
	g_worker[workerid].NotifyNewConnection(fd);
	g_worker_id = workerid;
	printf("%d\n", fd);
}

void workerThread(void *arg)
{
	if(arg == NULL) return;
	CWorker *pworker = (CWorker*)arg;
	printf("Worker Server: I'm running.\n");
	pworker->RunForever();
}


#ifdef WIN32
#include <windows.h>
typedef HANDLE THREAD_HANDLE;
typedef DWORD THREAD_ID;
typedef DWORD WINAPI(*THREAD_CALLBACK)(void *pArg);

#else
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
typedef pthread_t THREAD_HANDLE;
typedef pthread_t THREAD_ID;
typedef void* (*THREAD_CALLBACK)(void *pArg);
#endif

class CThreadManager
{
public:
	// spawn a thread
	static int Spawn(THREAD_HANDLE *pHandle, THREAD_CALLBACK pCallBack, void *pArg)
	{
#ifdef WIN32 // windows
	
#else
	int res;
	THREAD_HANDLE handle;
	res = pthread_create(&handle, NULL, pCallBack, pArg);
	if(pHandle != NULL)
	{
		*pHandle = handle;
	}
	return res;
#endif
	}
};

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        printf("usage: .exe  <my port>\n");
        return -1;
    }
    //CLog::SetLogLevel(LL_DETAIL);
    CLog::SetLogFile(stdout);
	
    // initialize all, acceptor, worker, and client simulator
    CInetAddr addr("", atoi(argv[1]), IP_V4);
    if(g_echoServer.Initialize(addr) != 0)
	{
		CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "initialize echo server failed.");
		exit(-1);
	}
    printf("server running on port[%s]\n", argv[1]);

	
	// create work threads
	for(int i = 0; i < WORKER_NUM; i++)
	{
		IReactor_Imp *pImp;
		if(i < WORKER_NUM/2)
		{
	        pImp = NewReactorImp(REACTOR_EPOLL);
		}
		else
		{
#ifdef WIN32
	        pImp = NewReactorImp(REACTOR_SELECT);
#else
	        pImp = NewReactorImp(REACTOR_EPOLL);
#endif
		}
		if(g_worker[i].Initialize(pImp) != 0) exit(-1);
		CThreadManager::Spawn(NULL, (THREAD_CALLBACK)workerThread, (void*)&g_worker[i]);
	}
	
    // accept connections
    g_echoServer.RunForever();
	
    return 0;
}



