#include <cstring>
#include "HttpServer.h"

int main() {
  try {
    HttpServer* srv = new HttpServer();
    srv->Listen({"127.0.0.1", 8080}, [](HttpServer& srv) -> void {
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
          res.SetHeader("Content-Length", to_string(10));
          res.SetHeader("Connection", "keep-alive");
          res.SetHeader("Server", "Faraway");
          res.Status(200, "OK");
          auto ended = new bool(false);
          res.Write(
              [=](u_long& len, bool& hasMore) -> char* {
                len = 10;
                hasMore = false;
                string _s("HelloWorld");
                char* s = new char[11];
                strcpy(s, _s.c_str());
                return s;
              },
              []() -> void {});
          // printf("Ended!!!");
        });
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
