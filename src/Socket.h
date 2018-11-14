#include <winsock2.h>

class Socket {
 private:
  SOCKET _sockHandler;

 public:
  Socket(SOCKET s);
  ~Socket();
};