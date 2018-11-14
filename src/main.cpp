#include "Server.h"

int main() {
  Server* srv = new Server();
  srv->Listen({"127.0.0.1", 8080}, [](Socket) -> void {});

  return 0;
}