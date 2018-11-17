#include "Server.h"

int main() {
  Server* srv = new Server();
  try {
    srv->Listen({"127.0.0.1", 8080}, [](Server& srv) -> void {
      printf("Server Listen OK!\n");

      srv.OnAccpet([](Server&, Socket& socket) -> void {
        printf("Accepted!!\n");
        u_long* totalLen = new u_long(0);
        socket.OnRecv([=](Socket& socket, WSABUF buf, u_long len) -> void {
          *totalLen += len;
          printf("Recv! %d\n", *totalLen);
        });
        socket.OnClose([=](Socket&) -> void {
          printf("Closed!\n");
        });
      });

      srv.OnClose([](Server) -> void {
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