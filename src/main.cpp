#include <cstring>
#include <fstream>
#include "HttpServer.h"
#include "Utils.h"
#define CHUNK_L 8192

std::string fullPath(string relative) {
  char* buffer = new char[2048];
  _fullpath(buffer, relative.c_str(), 2048);
  return string(buffer);
}

int main(size_t argc, char** argv) {
  string rootpath = fullPath(argv[3]);
  try {
    HttpServer* srv = new HttpServer();
    srv->Listen({"127.0.0.1", (u_short)stoi(argv[2])}, [&](HttpServer& srv) -> void {
      printf("Server is up!\n");
      srv.OnReq([&](HttpReq& req, HttpRes& res) -> void {
        printf("%s %s\n", req.method.c_str(), req.path.c_str());
        string fpath = hasEnding(rootpath, "\\")
                           ? rootpath + string(req.path.c_str() + 1)
                           : rootpath + req.path;

        if (req.method == "POST") {
          auto outfile = new ofstream(fpath);

          req.OnData([=](char* buf, u_long len) -> void {
            outfile->write(buf, len);
          });
          req.OnEnd([=]() -> void {
            outfile->close();
            delete outfile;
          });

          req.OnEnd([&]() {
            res.Status(201, "Created")
                .SetHeader("Content-Length", to_string(13))
                .SetHeader("Connection", "keep-alive")
                .SetHeader("Server", "Faraway")
                .Write(
                    [=](u_long& len, bool& hasMore) -> const char* {
                      len = 13;
                      hasMore = false;
                      return "File Created!";
                    },
                    []() -> void {});
          });
        } else if (req.method == "GET") {
          req.OnEnd([&]() -> void {
            ifstream in(fpath, ifstream::ate | ifstream::binary);
            if (!in) {
              res.Status(404, "Not Found")
                  .SetHeader("Content-Length", to_string(10))
                  .SetHeader("Connection", "keep-alive")
                  .SetHeader("Server", "Faraway")
                  .Write(
                      [=](u_long& len, bool& hasMore) -> const char* {
                        len = 10;
                        hasMore = false;
                        return "NotFound!!";
                      },
                      []() -> void {});
              return;
            }
            streampos fsize = in.tellg();
            in.clear();
            in.seekg(0, ios::beg);
            res.Status(200, "OK")
                .SetHeader("Content-Length", to_string(fsize))
                .SetHeader("Connection", "keep-alive")
                .SetHeader("Server", "Faraway")
                .Write(
                    [&](u_long& len, bool& hasMore) -> const char* {
                      char buffer[CHUNK_L];
                      in.read(buffer, CHUNK_L);
                      len = (u_long)in.gcount();
                      hasMore = !in.eof();
                      return buffer;
                    },
                    []() -> void {});
          });
        }
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
