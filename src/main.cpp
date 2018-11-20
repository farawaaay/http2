#include <cstring>
#include "HttpServer.h"

int main() {
  try {
    HttpServer* srv = new HttpServer();
    srv->Listen({"127.0.0.1", 8080}, [](HttpServer& srv) -> void {
      printf("Server is up!\n");
      srv.OnReq([](HttpReq& req, HttpRes& res) -> void {
        /*
          req.OnData([](char* buf, u_long len) -> void {
            // char* s = new char[len + 1];
            // snprintf(s, len + 1, "%s", buf);
            // printf(".");
          });
        */
        printf("%s %s\n", req.method.c_str(), req.path.c_str());

        req.OnEnd([&]() -> void {
          res.Status(200, "OK")
              .SetHeader("Content-Length", to_string(10))
              .SetHeader("Connection", "keep-alive")
              .SetHeader("Server", "Faraway")
              .Write(
                  [=](u_long& len, bool& hasMore) -> const char* {
                    len = 10;
                    hasMore = false;
                    return "HelloWorld";
                  },
                  []() -> void {});
        });
      });
    });
    printf("Press 'Enter' key to close the server elegantly.\n");
    getchar();
    srv->Close();
  } catch (int n) {
    printf("Error: %d, MS Error Code: %d", n, WSAGetLastError());
    getchar();
  }

  return 0;
}
