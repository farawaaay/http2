#include <cstring>
#include "HttpServer.h"

int main() {
  try {
    HttpServer* srv = new HttpServer();
    srv->Listen({"127.0.0.1", 8080}, [](HttpServer& srv) -> void {
      srv.OnReq([](HttpReq& req, HttpRes& res) -> void {
        for (auto h : req.headers) {
          printf("%s -> %s\n", h.first.c_str(), h.second.c_str());
        }
        req.OnData([](char* buf, u_long len) -> void {
          // char* s = new char[len + 1];
          // snprintf(s, len + 1, "%s", buf);
          // printf(".");
        });

        req.OnEnd([&]() -> void {
          res.SetHeader("Content-Length", to_string(10));
          res.SetHeader("Server", "Faraway");
          res.Status(200, "OK");
          auto ended = new bool(false);
          res.Write(
              [=](u_long& len) -> char* {
                if (*ended == false) {
                  len = 10;
                  *ended = true;
                  return "HelloWorld";
                } else {
                  len = 0;
                  return "";
                }
              },
              []() -> void { printf("ended!"); });
          printf("Ended!!!");
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