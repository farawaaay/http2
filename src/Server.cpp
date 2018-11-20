#include "Server.h"
#include <winsock2.h>
#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "Utils.h"

using namespace std;

// Socket::Socket()
//     : closing(false), closed(false) {}
// Socket::~Socket() {}

size_t Socket::Write(Buffer buf, function<void(Socket&, u_long)> cb) {
  // // if (closing) {
  // //   throw SocketError::Closing;
  // // }
  // DWORD dwBytes = 0;

  // // memset(&IOCtx->wsaBuf.buf, 0, sizeof(WSABUF));
  // memset(&overlapped, 0, sizeof(OVERLAPPED));

  // opType = OpType::Sent;
  // writeCb = cb;

  // int nBytesSend = WSASend(acceptSocket,
  //                          &buf,
  //                          1,
  //                          &dwBytes,
  //                          0,
  //                          &overlapped,
  //                          //[](DWORD, DWORD, LPWSAOVERLAPPED, DWORD) -> void {
  //                          //  printf("!!!!Send!!!!");
  //                          //}
  //                          NULL);
  // int lastError = 0;
  // if ((nBytesSend == SOCKET_ERROR) && ((lastError = WSAGetLastError()) != WSA_IO_PENDING)) {
  //   throw ServerError::PostSendError;
  // }
  // return dwBytes;
  size_t r = send(acceptSocket, buf.buf, buf.len, 0);
  cb(*this, (u_long)r);
  return r;
}

void Socket::OnRecv(function<void(Socket&, WSABUF, u_long)> cb) {
  recvCb.push_back(cb);
}

void Socket::OnClose(function<void(Socket&)> cb) {
  closeCb.push_back(cb);
}

void Socket::End() {
  shutdown(this->acceptSocket, SD_SEND);
}

Server::Server() {}
Server::~Server() {}

void Server::_LoadSocketLib() {
  WSADATA wsaData;
  int rtn;
  rtn = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (NO_ERROR != rtn)
    throw ServerError::WinsockStartupError;

  if (LOBYTE(wsaData.wVersion) != MAJOR_VERSION ||
      HIBYTE(wsaData.wVersion) != MINOR_VERSION) {
    WSACleanup();
    throw ServerError::WinsockVersionError;
  }
}

void Server::_UnloadSocketLib() {
  WSACleanup();
}

u_long Server::_GetNumOfProcessors() {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwNumberOfProcessors;
}

void Server::Listen(ListenOptions opt, function<void(Server&)> callback) {
  this->_LoadSocketLib();
  this->listenOpt = opt;

  this->IOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

  if ((this->srvSocketHandle = WSASocket(AF_INET,
                                         SOCK_STREAM,
                                         0,
                                         NULL,
                                         0,
                                         WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
    WSACleanup();
    throw ServerError::SocketCreateError;
  }

  for (uint16_t i = 0; i < 2 * GetNumberOfProcessors(); i++) {
    this->pThreads.push_back(new thread(
        [&](Server* srv) -> void {
          OVERLAPPED* pOverlapped = NULL;
          Server* key;
          u_long bytesTransfered = 0;
          while (true) {
            if (!GetQueuedCompletionStatus(srv->IOCompletionPort,
                                           &bytesTransfered,
                                           (PULONG_PTR)&key,
                                           &pOverlapped,
                                           INFINITE)) {
              continue;
            }

            if ((ULONGLONG)key == OpType::Exit) {
              break;
            }

            Socket* pCtx = (Socket*)pOverlapped;
            if (pCtx->opType != OpType::Accept && bytesTransfered == 0) {
              for (auto cb : pCtx->closeCb) {
                cb(*pCtx);
              }
              delete pCtx;
              continue;
            }
            switch (pCtx->opType) {
              case OpType::Accept: {  // ACCEPT

                SOCKADDR_IN* pClientAddr = NULL;
                SOCKADDR_IN* pLocalAddr = NULL;
                int remoteLen = sizeof(SOCKADDR_IN);
                int localLen = sizeof(SOCKADDR_IN);
                this->lpGetAcceptExSockAddrs(pCtx->wsaBuf.buf,
                                             pCtx->wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
                                             sizeof(SOCKADDR_IN) + 16,
                                             sizeof(SOCKADDR_IN) + 16,
                                             (LPSOCKADDR*)&pLocalAddr,
                                             &localLen,
                                             (LPSOCKADDR*)&pClientAddr,
                                             &remoteLen);

                pCtx->clientIp = inet_ntoa(pClientAddr->sin_addr);
                pCtx->clientPort = ntohs(pClientAddr->sin_port);

                for (auto cb : this->acceptCallbacks)
                  cb(*this, *pCtx);
                for (auto cb : pCtx->recvCb)
                  cb(*pCtx, pCtx->wsaBuf, bytesTransfered);

                CreateIoCompletionPort(
                    (HANDLE)pCtx->acceptSocket,
                    this->IOCompletionPort,
                    (ULONG_PTR)this,
                    0);

                this->_PostRecv(pCtx);
                this->_PostAccept(new Socket());

                break;
              }
              case OpType::Recv: {  // RECV
                for (auto cb : pCtx->recvCb)
                  cb(*pCtx, pCtx->wsaBuf, bytesTransfered);

                srv->_DoRecv(pCtx);
                break;
              }
              case OpType::Sent: {
                pCtx->writeCb(*pCtx, bytesTransfered);
                break;
              }
            }
          }
        },
        this));
  }

  CreateIoCompletionPort((HANDLE)this->srvSocketHandle, this->IOCompletionPort, (ULONG_PTR)this, 0);

  this->srvAddr.sin_family = AF_INET;
  this->srvAddr.sin_port = htons(opt.port);
  this->srvAddr.sin_addr.S_un.S_addr = htonl(0x00000000);

  if (::bind(this->srvSocketHandle,
             (sockaddr*)&this->srvAddr,
             sizeof(this->srvAddr)) == SOCKET_ERROR) {
    closesocket(this->srvSocketHandle);
    this->_UnloadSocketLib();
    throw ServerError::SocketBindError;
  }

  if (listen(this->srvSocketHandle, SOMAXCONN) == SOCKET_ERROR) {
    closesocket(this->srvSocketHandle);
    this->_UnloadSocketLib();
    throw ServerError::SocketListenError;
  }

  GUID GuidAcceptEx = WSAID_ACCEPTEX;
  GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
  DWORD dwBytes = 0;

  WSAIoctl(
      this->srvSocketHandle,
      SIO_GET_EXTENSION_FUNCTION_POINTER,
      &GuidAcceptEx,
      sizeof(GuidAcceptEx),
      &this->lpAcceptEx,
      sizeof(this->lpAcceptEx),
      &dwBytes,
      NULL,
      NULL);

  WSAIoctl(
      this->srvSocketHandle,
      SIO_GET_EXTENSION_FUNCTION_POINTER,
      &GuidGetAcceptExSockAddrs,
      sizeof(GuidGetAcceptExSockAddrs),
      &this->lpGetAcceptExSockAddrs,
      sizeof(this->lpGetAcceptExSockAddrs),
      &dwBytes,
      NULL,
      NULL);

  callback(*this);

  for (int i = 0; i < MAX_POST_ACCEPT; i++) {
    // Socket* IOCtx = ;
    this->_PostAccept(new Socket());
  }

  return;
}

void Server::Close() {
  for (auto _ : this->pThreads) {
    PostQueuedCompletionStatus(this->IOCompletionPort, 0, (ULONGLONG)OpType::Exit, NULL);
  }

  this->Join();
  for (auto cb : this->closeCallbacks) {
    cb(*this);
  }
  return;
}

void Server::Join() {
  for (auto t : this->pThreads) {
    t->join();
  }
}

void Server::OnAccpet(function<void(Server&, Socket&)> cb) {
  this->acceptCallbacks.push_back(cb);
}

void Server::OnClose(function<void(Server&)> cb) {
  this->closeCallbacks.push_back(cb);
}

void Server::_PostAccept(Socket* IOCtx) {
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
    throw ServerError::SocketCreateError;
  }

  this->lpAcceptEx(this->srvSocketHandle,
                   IOCtx->acceptSocket,
                   IOCtx->wsaBuf.buf,
                   IOCtx->wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
                   sizeof(SOCKADDR_IN) + 16,
                   sizeof(SOCKADDR_IN) + 16,
                   NULL,
                   &IOCtx->overlapped);

  return;
}

void Server::_DoAccept(Socket* IOCtx) {
  // SOCKADDR_IN* pClientAddr = NULL;
  // SOCKADDR_IN* pLocalAddr = NULL;
  // int remoteLen = sizeof(SOCKADDR_IN);
  // int localLen = sizeof(SOCKADDR_IN);
  // this->lpGetAcceptExSockAddrs(IOCtx->wsaBuf.buf,
  //                              IOCtx->wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
  //                              sizeof(SOCKADDR_IN) + 16,
  //                              sizeof(SOCKADDR_IN) + 16,
  //                              (LPSOCKADDR*)&pLocalAddr,
  //                              &localLen,
  //                              (LPSOCKADDR*)&pClientAddr,
  //                              &remoteLen);

  // IOCtx->clientIp = inet_ntoa(pClientAddr->sin_addr);
  // IOCtx->clientPort = ntohs(pClientAddr->sin_port);

  // CreateIoCompletionPort(
  //     (HANDLE)IOCtx->acceptSocket,
  //     this->IOCompletionPort,
  //     (ULONG_PTR)this,
  //     0);

  // this->_PostRecv(IOCtx);
  // this->_PostAccept(new Socket());
}

void Server::_PostRecv(Socket* IOCtx) {
  DWORD dwFlags = 0;
  DWORD dwBytes = 0;

  // memset(&IOCtx->wsaBuf.buf, 0, sizeof(WSABUF));
  memset(&IOCtx->overlapped, 0, sizeof(OVERLAPPED));

  IOCtx->opType = OpType::Recv;

  int nBytesRecv = WSARecv(IOCtx->acceptSocket,
                           &IOCtx->wsaBuf,
                           1,
                           &dwBytes,
                           &dwFlags,
                           &IOCtx->overlapped,
                           NULL);

  int lastError = 0;
  if ((nBytesRecv == SOCKET_ERROR) && ((lastError = WSAGetLastError()) != WSA_IO_PENDING)) {
    throw ServerError::PostRecvError;
  }
  return;
}

void Server::_DoRecv(Socket* IOCtx) {
  return this->_PostRecv(IOCtx);
}

void Server::_PostSend(Socket* IOCtx, WSABUF wsaBuf) {
}

void Server::_DoSend(Socket* IOCtx) {}
void Server::_DoClose(Socket* IOCtx) {}
