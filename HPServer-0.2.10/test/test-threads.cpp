//
// test for multiple threads, and multiple read/write events
//
// the test simulate  a C/S model
// server:
// 1 listen at a port, and wait for connection from client
// 2 creat several worker threads to receive messages from client, and echo to client
// 3 use a seperate thread to accept new connections
// client:
// 1 the client thead issues connections
// 2 send messages to server
// 3 recv messages from server,  when receive the whole message, check;
// check method:
// client check the message send & received are identical
//

#include "echo-server.h"
#include "echo-client.h"

#define WORKER_NUM 4

CEchoServer g_echoServer;
CWorker g_worker[WORKER_NUM];
CClientSimulator      g_simulator;

int g_clients_total;
int g_clients_pass;

int g_worker_id;
void send_to_worker(HL_SOCKET fd)
{
	int workerid = (g_worker_id+1)%WORKER_NUM;
	g_worker[workerid].NotifyNewConnection(fd);
	g_worker_id = workerid;
}

void workerThread(void *arg)
{
	if(arg == NULL) return;
	CWorker *pworker = (CWorker*)arg;
	printf("Worker Server: I'm running.\n");
	pworker->RunForever();
}

void clientThread(void *arg)
{
	if(arg == NULL) return;
	CClientSimulator *pSimulator = (CClientSimulator*)arg;
	printf("CClient Simulator: I'm running.\n");
	pSimulator->RunForever();
}

#ifdef WIN32
#include <windows.h>
#pragma comment(lib,"WS2_32.lib")

typedef HANDLE THREAD_HANDLE;
typedef DWORD THREAD_ID;
typedef DWORD (WINAPI*THREAD_CALLBACK)(void *pArg);
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
	DWORD threadId = 0;
	HANDLE handle = CreateThread(NULL, 0, pCallBack, pArg, 0, &threadId);
	if (pHandle!=NULL)
	{
		*pHandle = handle;
	}
	if (handle==NULL)
	{
		return -1;
	}
	else
	{
		return 0;
	}
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
    printf("-----------test multiple events in multiple threads\n.-----------");
    //CLog::SetLogLevel(LL_DEBUG);
    CLog::SetLogFile(stdout);
#ifdef WIN32
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	int err = WSAStartup(wVersionRequested,&wsaData);
	if (err != 0)
	{
		CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "invoke WSAStartup failed.");
		exit(-1);
	}
#endif
	g_clients_pass = 0;
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
#ifdef WIN32 
		pImp = NewReactorImp(REACTOR_SELECT);
#else
		if(i < WORKER_NUM/2)
		{
			pImp = NewReactorImp(REACTOR_EPOLL);
		}
		else
		{
			pImp = NewReactorImp(REACTOR_SELECT);
		}
#endif
		if(g_worker[i].Initialize(pImp) != 0)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "initialize worker thread failed.");
			exit(-1);
		}
		CThreadManager::Spawn(NULL, (THREAD_CALLBACK)workerThread, (void*)&g_worker[i]);
	}
	// create client thread
    if(g_simulator.Initialize(atoi(argv[1])) != 0)
	{
		CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "initialize client simulator failed.");
		exit(-1);
	}
	CThreadManager::Spawn(NULL, (THREAD_CALLBACK)clientThread, (void*)&g_simulator);
	
    // accept connections
    g_echoServer.RunForever();
#ifdef WIN32
	WSACleanup();
#endif
    return 0;
}



