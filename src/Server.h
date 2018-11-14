#include <winsock2.h>
#include <functional>
#include <string>
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

using namespace std;

// enum SocketError {
//   WinsockStartupError,
//   WinsockVersionError,
//   SocketCreateError,
//   SocketBindError,
//   SocketListenError,
// };

enum OpType {
  Accept,
  Recv
};

constexpr auto WinsockStartupError = 1;
constexpr auto WinsockVersionError = 2;
constexpr auto SocketCreateError = 3;
constexpr auto SocketBindError = 4;
constexpr auto SocketListenError = 5;
constexpr auto PostAcceptError = 6;
constexpr auto PostRecvError = 6;

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
  void Listen(ListenOptions, function<void(Socket)>);
  template <typename Any>
  void On(string, function<Any(Socket)>);
  void Close();

 private:
  HANDLE IOCompletionPort;  // Server IOCP handle
  SOCKET _srvSocketHandle;  // Server socket handle
  ListenOptions _listenOpt;
  sockaddr_in _srvAddr;
  LPFN_ACCEPTEX lpAcceptEx;                          // AcceptEx 函数指针
  LPFN_GETACCEPTEXSOCKADDRS lpGetAcceptExSockAddrs;  // GetAcceptExSockAddrs 函数指针
  void _LoadSocketLib();
  void _UnloadSocketLib();
  void _PostAccept(PerIOContext*);
  void _DoAccept(PerIOContext*);
  void _PostRecv(PerIOContext*);
  void _DoRecv(PerIOContext*);
};
