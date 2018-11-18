#include <cstring>
#include "Server.h"
#include "Utils.h"

int main() {
  Server* srv = new Server();
  try {
    srv->Listen({"127.0.0.1", 8080}, [](Server& srv) -> void {
      printf("Server Listen OK!\n");

      srv.OnAccpet([](Server&, Socket& socket) -> void {
        auto totalLen = new u_long(0);
        auto bodyBuffers = new vector<pair<char*, u_long>>();
        auto headStr = new string("");
        auto headEnded = new bool(false);
        auto firstChunk = new bool(true);
        socket.OnRecv([=](Socket& socket, WSABUF buf, u_long len) -> void {
          if (*headEnded) {
            char* copy = new char[len];
            memcpy(copy, buf.buf, len);
            bodyBuffers->push_back({copy, len});
            return;
          }
          string chunk(buf.buf);
          if (*firstChunk) {
            // first chunk,
            *firstChunk = false;
            size_t pos = chunk.find("\r\n");
            if (pos == string::npos) {
              socket.End();  // not http
              return;
            }
          }

          size_t pos = chunk.find("\r\n\r\n");

          if (pos != string::npos) {
            *headStr += chunk.substr(0, pos);
            *headEnded = true;
            // parse header
            auto headerLines = split(*headStr, "\r\n");
            for (auto line : headerLines) {
              printf("%s\n", line.c_str());
            }
            size_t headLen = pos + 4;
            if (len > headLen) {
              char* copy = new char[len - headLen];
              memcpy(copy, buf.buf + headLen, len - headLen);
              bodyBuffers->push_back({copy, len - headLen});
            }
          } else
            *headStr += chunk;

          // buffers->push_back({buf, len});
        });
        socket.OnClose([=](Socket&) -> void {
          printf("Closed!\n");
          delete totalLen;
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