
#ifndef ECHO_CLIENT_H
#define ECHO_CLIENT_H

#include "Connector.h"
#include "Reactor.h"

#include <stdio.h>
#include <memory.h>
#include <string.h>

using namespace hpsl;

#ifdef WIN32
#define CLIENT_NUM 64 // on windows FD_SET of select is limited to 64

#else
#define CLIENT_NUM 512
#endif

#define BUF_SIZE 128
// string terminated by $
#define STR_FORMAT "#%d-->%d# the test strings[0x%0x] for multiple read/write in multi-thread.---?$"
/// echo handlers////////////

extern int g_clients_total;
extern int g_clients_pass;

// client handler
class CClientHandler: public IEventHandler
{
public:
	CClientHandler() : IEventHandler(false), m_active(false), m_itimeout(0), 
	m_rpos(0), m_wpos(0), m_connectOK(false)
	{
		memset(m_rbuff, 0, sizeof(m_rbuff));
		memset(m_wbuff, 0, sizeof(m_wbuff));
		snprintf(m_wbuff, sizeof(m_wbuff)-1, STR_FORMAT, 
		rand()%1000, rand()%50000, this);
	}
	
	virtual void HandleExit() {CloseHandle();}
	virtual void HandleClose(){CloseHandle();}
	
	virtual void HandleSignal(){}
	virtual void HandleConnectSvrSuccess()
	{
		m_connectOK = true;
		getReactor()->RegisterEvent(this, EV_WRITE|EV_READ);
		struct timeval tv;
		tv.tv_sec = rand()%5+5;
		tv.tv_usec = rand()%50000;
		if(getReactor()->RegisterTimer(this, &tv) != 0)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "register timer failed:%d\n", GetHandle());
			exit(10);
		}
        ++g_clients_total;
		m_active = true;
	}
	virtual void HandleWrite(HL_SOCKET handle)
	{
		m_active = true;
		if(!m_connectOK) // 
		{
			HandleConnectSvrSuccess();
		}
		int len = strlen(m_wbuff);
		int tmp = len - m_wpos;
		if(tmp >= 32) tmp = rand()%16+16; // set send size
		int n = send(GetHandle(), m_wbuff+m_wpos, tmp, 0);
		if(n < 0)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "send error[%d]", GetHandle());
			exit(1);
		}
		m_wpos += n;
		getReactor()->UnregisterEvent(this, EV_WRITE);
	}
	
	virtual void HandleRead(HL_SOCKET handle)
	{
		int n = recv(GetHandle(), m_rbuff+m_rpos, sizeof(m_rbuff)-m_rpos-1, 0);
		if(n < 0)
		{
			CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "read error[%d]", GetHandle());
			exit(1);
		}
		m_rpos += n;
		m_rbuff[m_rpos] = '\0';
		if(m_rbuff[m_rpos-1]=='$') // recv complete
		{
			getReactor()->UnregisterEvent(this, EV_READ|EV_TIMER);
			if(strncmp(m_rbuff, m_wbuff, m_rpos) != 0)
			{
				CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), 
				"[%d] r:[%s]<>[%s]\n", GetHandle(), m_rbuff, m_wbuff);
				exit(2);
			}
			g_clients_pass++;
			printf(".");
			if(g_clients_pass == g_clients_total)
			{
                printf("\ntest of events in multi-thread complete. total client number:%d\n", g_clients_total);
				exit(0); // test complete
			}
		}
		if(n == 0)
		{
			getReactor()->RemoveHandler(this);
		}
		m_active = true;
	}
	virtual void HandleTimer()
	{
		if(!m_active)
		{
			++m_itimeout;
			if(m_itimeout == 5)
			{
				CLog::Log(LL_ERROR, __FILE__, __LINE__, 0, "send timeout[%d]", GetHandle());			
				exit(1);
			}
		}
		m_active = false;
		if(m_rpos < strlen(m_wbuff))
		{
			if(getReactor()->RegisterEvent(this, EV_WRITE) != 0)
			{
				CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "reg write event error[%d]", GetHandle());
				exit(5);
			}
		}
		else
		{
			getReactor()->RemoveHandler(this);
		}
	}
private:
	void Release();
	char m_rbuff[BUF_SIZE], m_wbuff[BUF_SIZE];
	int m_rpos, m_wpos, m_itimeout;
    bool m_active, m_connectOK;
};

CClientHandler g_client[CLIENT_NUM];

class CClientSimulator
{
public:
	CClientSimulator(){}
	~CClientSimulator(){}

	int Initialize(short port)
	{
		IReactor_Imp *pReactorImp = NULL;
		do
		{
			// initialize reactor
#ifdef WIN32
	        pReactorImp = NewReactorImp(REACTOR_SELECT);
#else
	        pReactorImp = NewReactorImp(REACTOR_EPOLL);
#endif
			if(pReactorImp == NULL) break;
			if(m_reactor.Initialize(pReactorImp) != 0) break;
			
			CInetAddr svrAddr("127.0.0.1", port, IP_V4);
			createClients(svrAddr, CLIENT_NUM);
	        return 0;
		}while(0);

	    return -1;
	}
	int RunForever()
	{
		int res = m_reactor.RunForever();
		return res;
	}
	void Stop()
	{
		m_reactor.Stop();
	}	
private:
	void createClients(CInetAddr &svrAddr, int size)
	{
		for(int i = 0; i < size; i++)
		{
			// create socket
			HL_SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if(s == INVALID_SOCKET)
			{
				break; 
			}
			if(CSockUtil::SockSetNonBlocking(s) < 0)
			{
				CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "failed to set nonblocking[handle=%d]", s);
				exit(21);
			}
			if(m_reactor.RegisterHandler(s, &g_client[i]) != 0)
			{
				CLog::Log(LL_ERROR, __FILE__, __LINE__, ERRNO(), "Register client handler failed.\n");
	            exit(1);
			}
			struct timeval tv;
			tv.tv_sec = 20;
			tv.tv_usec = 0;
			int res = m_connector.Connect(&m_reactor, s, svrAddr, &tv);
			if(res = 0)
			{
				CLog::Log(LL_NORMAL, __FILE__, __LINE__, SOCK_ERR(), "connect success immediately.");
            }
            if(res < 0)
            {
				CLog::Log(LL_NORMAL, __FILE__, __LINE__, SOCK_ERR(), "connect to server failed.");
                break;
            }
		} 
	}
	CReactor m_reactor;
	CConnector m_connector;
};
#endif
