#include "Server.h"

int main() {
  Server* srv = new Server();
  try {
    srv->Listen({"127.0.0.1", 8080}, [](Server& srv) -> void {
      printf("Server Listen OK!\n");

      srv.OnAccpet([](Server&, Socket& socket) -> void {
        printf("Accepted!!\n");
        socket.OnRecv([](Socket& socket, WSABUF buf) -> void {
          printf("Recv!\n%s", buf.buf);
        });
        socket.OnClose([](Socket&) -> void {
          printf("Closed!");
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