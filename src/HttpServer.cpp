#include "HttpServer.h"
#include "Utils.h"

HttpServer::HttpServer()
    : srv(Server()) {
}

void HttpServer::Listen(ListenOptions opt, function<void(HttpServer&)> cb) {
  return this->srv.Listen(opt, [=](Server& srv) -> void {
    cb(*this);
    srv.OnAccpet(this->_acceptHandler);
  });
}

void HttpServer::Close() {
  return this->Close();
}

void HttpServer::Join() {
  return this->Join();
}

void HttpServer::_acceptHandler(Server&, Socket& socket) {
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
}