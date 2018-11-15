#include "Server.h"

int main() {
  Server* srv = new Server();
  try {
    srv->Listen({"127.0.0.1", 8080}, [](Server& srv, Socket& s) -> void {
      printf("Server Listen OK!\n");

      srv.OnAccpet([](Server, Socket) -> void {
        printf("Accepted!!");
      });

      srv.OnClose([](Server, Socket) -> void {
        printf("!!!!");
      });
    });
    getchar();
    srv->Close();
  } catch (int n) {
    printf("Error: %d, MS Error Code: %d", n, WSAGetLastError());
    getchar();
  }

  return 0;
}