#include "Server.h"

int main() {
  Server* srv = new Server();
  try {
    srv->Listen({"127.0.0.1", 8080}, [](Socket) -> void {
      printf("Server Listen OK!\n");
    });
  } catch (int n) {
    printf("Error: %d, MS Error Code: %d", n, WSAGetLastError());
    getchar();
  }

  return 0;
}