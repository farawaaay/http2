#include "Server.h"
#include <winsock2.h>
#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std;

Server::Server() {}
Server::~Server() {}

void Server::_LoadSocketLib() {
  WSADATA wsaData;
  int rtn;
  rtn = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (NO_ERROR != rtn)
    throw WinsockStartupError;

  if (LOBYTE(wsaData.wVersion) != MAJOR_VERSION ||
      HIBYTE(wsaData.wVersion) != MINOR_VERSION) {
    WSACleanup();
    throw WinsockVersionError;
  }
}

void Server::_UnloadSocketLib() {
  WSACleanup();
}

void Server::Listen(ListenOptions opt, function<void(Socket)> callback) {
  this->_LoadSocketLib();
  this->_listenOpt = opt;

  this->IOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

  if ((this->_srvSocketHandle = WSASocket(AF_INET,
                                          SOCK_STREAM,
                                          0,
                                          NULL,
                                          0,
                                          WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
    WSACleanup();
    throw SocketCreateError;
  }

  thread* t = new thread(
      [](Server* srv) -> void {
        OVERLAPPED* pOverlapped = NULL;
        Server* __srv = NULL;
        DWORD dwBytesTransfered = 0;
        while (true) {
          bool ret = GetQueuedCompletionStatus(
              srv->IOCompletionPort,
              &dwBytesTransfered,
              (PULONG_PTR)&__srv,
              &pOverlapped,
              INFINITE);

          if (!ret) {
            continue;
          }

          PerIOContext ctx = *CONTAINING_RECORD(pOverlapped, PerIOContext, overlapped);
          switch (ctx.opType) {
            case 0: {  // ACCEPT
              SOCKADDR_IN* ClientAddr = NULL;
              SOCKADDR_IN* LocalAddr = NULL;
              int remoteLen = sizeof(SOCKADDR_IN);
              int localLen = sizeof(SOCKADDR_IN);
              srv->lpGetAcceptExSockAddrs(ctx.wsaBuf.buf,
                                          ctx.wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
                                          sizeof(SOCKADDR_IN) + 16,
                                          sizeof(SOCKADDR_IN) + 16,
                                          (LPSOCKADDR*)&LocalAddr,
                                          &localLen,
                                          (LPSOCKADDR*)&ClientAddr,
                                          &remoteLen);
            }
            case 1: {  // RECV
            }
          }
        }
      },
      this);

  CreateIoCompletionPort((HANDLE)this->_srvSocketHandle, this->IOCompletionPort, (ULONG_PTR)this, 0);

  this->_srvAddr.sin_family = AF_INET;
  this->_srvAddr.sin_port = htons(opt.port);
  this->_srvAddr.sin_addr.S_un.S_addr = htonl(0x00000000);

  if (::bind(this->_srvSocketHandle,
             (sockaddr*)&this->_srvAddr,
             sizeof(this->_srvAddr)) == SOCKET_ERROR) {
    closesocket(this->_srvSocketHandle);
    this->_UnloadSocketLib();
    throw SocketBindError;
  }

  if (listen(this->_srvSocketHandle, SOMAXCONN) == SOCKET_ERROR) {
    closesocket(this->_srvSocketHandle);
    this->_UnloadSocketLib();
    throw SocketListenError;
  }

  GUID GuidAcceptEx = WSAID_ACCEPTEX;              // GUID，这个是识别 AcceptEx 函数必须的
  GUID GuidGetAcceptExSockAddrs = WSAID_ACCEPTEX;  // GUID，这个是识别 GetAcceptExSockAddrs 函数必须的
  DWORD dwBytes = 0;

  WSAIoctl(
      // m_pListenContext->m_Socket,
      this->_srvSocketHandle,
      SIO_GET_EXTENSION_FUNCTION_POINTER,
      &GuidAcceptEx,
      sizeof(GuidAcceptEx),
      &this->lpAcceptEx,
      sizeof(this->lpAcceptEx),
      &dwBytes,
      NULL,
      NULL);

  WSAIoctl(
      // m_pListenContext->m_Socket,
      this->_srvSocketHandle,
      SIO_GET_EXTENSION_FUNCTION_POINTER,
      &GuidGetAcceptExSockAddrs,
      sizeof(GuidGetAcceptExSockAddrs),
      &this->lpGetAcceptExSockAddrs,
      sizeof(this->lpGetAcceptExSockAddrs),
      &dwBytes,
      NULL,
      NULL);

  // 为AcceptEx 准备参数，然后投递AcceptEx I/O请求
  for (int i = 0; i < MAX_POST_ACCEPT; i++) {
    // 新建一个IO_CONTEXT
    DWORD dwBytes = 0;

    PerIOContext* IOCtx = new PerIOContext();

    IOCtx->opType = 0;
    IOCtx->wsaBuf.buf = new char[MAX_BUFFER_LEN];
    IOCtx->wsaBuf.len = MAX_BUFFER_LEN;

    OVERLAPPED* p_ol = &IOCtx->overlapped;

    memset(p_ol, 0, sizeof(OVERLAPPED));
    memset(IOCtx->wsaBuf.buf, 0, MAX_BUFFER_LEN);

    SOCKET acceptSocket = WSASocket(AF_INET,
                                    SOCK_STREAM,
                                    IPPROTO_TCP,
                                    NULL,
                                    0,
                                    WSA_FLAG_OVERLAPPED);

    lpAcceptEx(this->_srvSocketHandle,
               acceptSocket,
               IOCtx->wsaBuf.buf,
               IOCtx->wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
               sizeof(SOCKADDR_IN) + 16,
               sizeof(SOCKADDR_IN) + 16,
               &dwBytes,
               p_ol);
  }
  t->join();

  return callback(Socket(this->_srvSocketHandle));
}