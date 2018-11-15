#include <winsock2.h>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include "Socket.h"

#define MAJOR_VERSION 2
#define MINOR_VERSION 2
#define WSAID_ACCEPTEX                                                             \
  {                                                                                \
    0xb5367df1, 0xcbac, 0x11cf, { 0x95, 0xca, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92 } \
  }
#define WSAID_GETACCEPTEXSOCKADDRS                                                 \
  {                                                                                \
    0xb5367df2, 0xcbac, 0x11cf, { 0x95, 0xca, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92 } \
  }
#define MAX_POST_ACCEPT 10
#define MAX_BUFFER_LEN 8192

class Server;
using namespace std;
using Callback = function<void(Server&, Socket&)>;
using CallbackList = vector<Callback>;
using PThreadList = vector<thread*>;

enum ServerError {
  WinsockStartupError,
  WinsockVersionError,
  SocketCreateError,
  SocketBindError,
  SocketListenError,
  PostAcceptError,
  PostRecvError,
  PostSendError
};

enum OpType {
  Exit,
  Accept,
  Recv,
};

struct ListenOptions {
  string host;
  u_short port;
};

struct PerIOContext {
  OVERLAPPED overlapped;
  WSABUF wsaBuf;
  u_short opType;
  SOCKET acceptSocket;
  SOCKADDR_IN clientAddr;
};

typedef BOOL(PASCAL FAR* LPFN_ACCEPTEX)(
    _In_ SOCKET sListenSocket,
    _In_ SOCKET sAcceptSocket,
    _Out_writes_bytes_(dwReceiveDataLength + dwLocalAddressLength + dwRemoteAddressLength) PVOID lpOutputBuffer,
    _In_ DWORD dwReceiveDataLength,
    _In_ DWORD dwLocalAddressLength,
    _In_ DWORD dwRemoteAddressLength,
    _Out_ LPDWORD lpdwBytesReceived,
    _Inout_ LPOVERLAPPED lpOverlapped);

typedef VOID(PASCAL FAR* LPFN_GETACCEPTEXSOCKADDRS)(
    _In_reads_bytes_(dwReceiveDataLength + dwLocalAddressLength + dwRemoteAddressLength) PVOID lpOutputBuffer,
    _In_ DWORD dwReceiveDataLength,
    _In_ DWORD dwLocalAddressLength,
    _In_ DWORD dwRemoteAddressLength,
    _Outptr_result_bytebuffer_(*LocalSockaddrLength) struct sockaddr** LocalSockaddr,
    _Out_ LPINT LocalSockaddrLength,
    _Outptr_result_bytebuffer_(*RemoteSockaddrLength) struct sockaddr** RemoteSockaddr,
    _Out_ LPINT RemoteSockaddrLength);

class Server {
 public:
  Server();
  ~Server();
  void Listen(ListenOptions, Callback);
  void OnAccpet(Callback);
  void OnClose(Callback);
  void Close();
  void Join();

 private:
  HANDLE IOCompletionPort;  // Server IOCP handle
  SOCKET _srvSocketHandle;  // Server socket handle
  ListenOptions _listenOpt;
  sockaddr_in _srvAddr;
  LPFN_ACCEPTEX lpAcceptEx;                          // AcceptEx 函数指针
  LPFN_GETACCEPTEXSOCKADDRS lpGetAcceptExSockAddrs;  // GetAcceptExSockAddrs 函数指针
  PThreadList pThreads;
  CallbackList acceptCallbacks;
  CallbackList closeCallbacks;
  void _LoadSocketLib();
  void _UnloadSocketLib();
  u_long _GetNumOfProcessors();
  void _PostAccept(PerIOContext*);
  void _DoAccept(PerIOContext*);
  void _PostRecv(PerIOContext*);
  void _DoRecv(PerIOContext*);
  void _PostSend(PerIOContext*, WSABUF);
  void _DoSend(PerIOContext*);
  void _DoClose(PerIOContext*);
};
