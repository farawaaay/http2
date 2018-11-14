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
        u_long bytesTransfered = 0;
        while (true) {
          bool ret = GetQueuedCompletionStatus(
              srv->IOCompletionPort,
              &bytesTransfered,
              (PULONG_PTR)&__srv,
              &pOverlapped,
              INFINITE);

          if (!ret) {
            continue;
          }

          PerIOContext ctx = *CONTAINING_RECORD(pOverlapped, PerIOContext, overlapped);
          printf("OpType: %d\n", ctx.opType);
          switch (ctx.opType) {
            case OpType::Accept: {  // ACCEPT
              srv->_DoAccept(&ctx);
              break;
            }
            case OpType::Recv: {  // RECV
              srv->_DoRecv(&ctx);
              break;
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

  GUID GuidAcceptEx = WSAID_ACCEPTEX;                          // GUID，这个是识别 AcceptEx 函数必须的
  GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;  // GUID，这个是识别 GetAcceptExSockAddrs 函数必须的
  DWORD dwBytes = 0;

  WSAIoctl(
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
      this->_srvSocketHandle,
      SIO_GET_EXTENSION_FUNCTION_POINTER,
      &GuidGetAcceptExSockAddrs,
      sizeof(GuidGetAcceptExSockAddrs),
      &this->lpGetAcceptExSockAddrs,
      sizeof(this->lpGetAcceptExSockAddrs),
      &dwBytes,
      NULL,
      NULL);

  for (int i = 0; i < MAX_POST_ACCEPT; i++) {
    PerIOContext* IOCtx = new PerIOContext();
    this->_PostAccept(IOCtx);
  }

  callback(Socket(this->_srvSocketHandle));
  t->join();
  return;
}

void Server::_PostAccept(PerIOContext* IOCtx) {
  IOCtx->opType = OpType::Accept;
  IOCtx->wsaBuf.buf = new char[MAX_BUFFER_LEN];
  IOCtx->wsaBuf.len = MAX_BUFFER_LEN;

  memset(&IOCtx->overlapped, 0, sizeof(OVERLAPPED));
  memset(IOCtx->wsaBuf.buf, 0, MAX_BUFFER_LEN);

  if (INVALID_SOCKET == (IOCtx->acceptSocket = WSASocket(
                             AF_INET,
                             SOCK_STREAM,
                             IPPROTO_TCP,
                             NULL,
                             0,
                             WSA_FLAG_OVERLAPPED))) {
    throw SocketCreateError;
  }

  this->lpAcceptEx(this->_srvSocketHandle,
                   IOCtx->acceptSocket,
                   IOCtx->wsaBuf.buf,
                   IOCtx->wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
                   sizeof(SOCKADDR_IN) + 16,
                   sizeof(SOCKADDR_IN) + 16,
                   NULL,
                   &IOCtx->overlapped);

  return;
}

void Server::_DoAccept(PerIOContext* IOCtx) {
  SOCKADDR_IN* pClientAddr = NULL;
  SOCKADDR_IN* pLocalAddr = NULL;
  int remoteLen = sizeof(SOCKADDR_IN);
  int localLen = sizeof(SOCKADDR_IN);
  this->lpGetAcceptExSockAddrs(IOCtx->wsaBuf.buf,
                               IOCtx->wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
                               sizeof(SOCKADDR_IN) + 16,
                               sizeof(SOCKADDR_IN) + 16,
                               (LPSOCKADDR*)&pLocalAddr,
                               &localLen,
                               (LPSOCKADDR*)&pClientAddr,
                               &remoteLen);

  IOCtx->clientAddr = *pClientAddr;
  // inet_ntoa(ClientAddr->sin_addr), ntohs(ClientAddr->sin_port)
  // printf("%s:%d", inet_ntoa(pClientAddr->sin_addr), ntohs(pClientAddr->sin_port));

  CreateIoCompletionPort(
      (HANDLE)IOCtx->acceptSocket,
      this->IOCompletionPort,
      (ULONG_PTR)this,
      0);

  this->_PostRecv(IOCtx);
}

void Server::_PostRecv(PerIOContext* IOCtx) {
  DWORD dwFlags = 0;
  DWORD dwBytes = 0;

  // memset(&IOCtx->wsaBuf.buf, 0, sizeof(WSABUF));
  // memset(&IOCtx->overlapped, 0, sizeof(OVERLAPPED));

  IOCtx->opType = OpType::Recv;

  int nBytesRecv = WSARecv(IOCtx->acceptSocket,
                           &IOCtx->wsaBuf,
                           1,
                           &dwBytes,
                           &dwFlags,
                           &IOCtx->overlapped,
                           NULL);

  // 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
  int lastError = 0;
  if ((nBytesRecv == SOCKET_ERROR) && ((lastError = WSAGetLastError()) != WSA_IO_PENDING)) {
    throw PostRecvError;
  }
  return;
}

void Server::_DoRecv(PerIOContext* IOCtx) {
  return this->_PostRecv(IOCtx);
}