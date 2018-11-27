#include <cstdio>
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
    srv->Listen({argv[1], (u_short)stoi(argv[2])}, [&](HttpServer& srv) -> void {
      printf("Server is up!\n");
      srv.OnReq([&](HttpReq& req, HttpRes& res) -> void {
        if (req.method == "POST") {
          string fpath = hasEnding(rootpath, "\\")
                             ? rootpath + string(req.path.c_str() + 1)
                             : rootpath + req.path;
          auto outfile = new ofstream(fpath, std::ios_base::binary | std::ios_base::out);

          req.OnData([=](char* buf, u_long len) -> void {
            outfile->write(buf, len);
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

          req.OnEnd([=]() -> void {
            outfile->close();
            delete outfile;
          });
        } else if (req.method == "GET") {
          req.OnEnd([&]() -> void {
            string fpath = hasEnding(rootpath, "\\")
                               ? rootpath + string(req.path.c_str() + 1)
                               : rootpath + req.path;
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
            char* buffer = new char[CHUNK_L];
            in.clear();
            in.seekg(0, ios::beg);
            res.Status(200, "OK")
                .SetHeader("Content-Length", to_string(fsize))
                .SetHeader("Content-Type", mimeType("." + split(fpath, ".").back()))
                .SetHeader("Connection", "keep-alive")
                .SetHeader("Server", "Faraway")
                .Write(
                    [&](u_long& len, bool& hasMore) -> const char* {
                      in.read(buffer, CHUNK_L);
                      len = (u_long)in.gcount();
                      hasMore = !in.eof();
                      return buffer;
                    },
                    [&]() -> void { delete buffer; });
          });
        } else if (req.method == "DELETE") {
          string fpath = hasEnding(rootpath, "\\")
                             ? rootpath + string(req.path.c_str() + 1)
                             : rootpath + req.path;
          if (::remove(fpath.c_str()) == 0) {
            res.Status(200, "OK")
                .SetHeader("Content-Length", "13")
                .SetHeader("Connection", "keep-alive")
                .SetHeader("Server", "Faraway")
                .Write(
                    [&](u_long& len, bool& hasMore) -> const char* {
                      len = 13;
                      hasMore = false;
                      return "File deleted!";
                    },
                    []() -> void {});
          } else {
            res.Status(400, "Bad Request")
                .SetHeader("Content-Length", "39")
                .SetHeader("Connection", "keep-alive")
                .SetHeader("Server", "Faraway")
                .Write(
                    [&](u_long& len, bool& hasMore) -> const char* {
                      len = 39;
                      hasMore = false;
                      return "Fail to delete file, check you request!";
                    },
                    []() -> void {});
          }

        } else {
          req.OnEnd([&]() -> void {
            res.Status(405, "Method Not Allowed")
                .SetHeader("Content-Length", "18")
                .SetHeader("Connection", "keep-alive")
                .SetHeader("Server", "Faraway")
                .Write(
                    [&](u_long& len, bool& hasMore) -> const char* {
                      len = 18;
                      hasMore = false;
                      return "Method Not Allowed";
                    },
                    []() -> void {});
          });
        }

        req.OnEnd([&]() -> void {
          printf("%-6s %-20s %15s:%d -> %d %s\n",
                 req.method.c_str(),
                 req.path.c_str(),
                 req.clientIp.c_str(),
                 req.clientPort,
                 res.statusCode,
                 res.statusText.c_str());
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
