#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "Server.h"

using namespace std;

// struct ListenOptions {
//   string host;
//   uint16_t port;
// };
enum HttpResError {
  NoHeader
};

struct HttpReq {
  vector<function<void(char*, u_long)>> dataCb;
  vector<function<void(void)>> endCb;

  string method;
  string path;
  string clientIp;
  u_short clientPort;
  map<string, string> headers;
  void OnData(function<void(char*, u_long)>);
  void OnEnd(function<void(void)>);
};

struct HttpRes {
  uint16_t statusCode;
  string statusText;
  map<string, string> headers;
  bool headerSent = false;
  bool ended = false;
  Socket* socket;
  HttpRes& Status(uint16_t, string);
  HttpRes& SetHeader(string, string);
  HttpRes& Write(function<const char*(u_long&, bool&)>, function<void(void)>);
};

class HttpServer {
 private:
  Server srv;
  vector<function<void(HttpReq&, HttpRes&)>> reqCb;
  void _acceptHandler(Server&, Socket&);

 public:
  HttpServer();
  ~HttpServer();
  void Listen(ListenOptions, function<void(HttpServer&)>);
  void OnReq(function<void(HttpReq&, HttpRes&)>);
  void Close();
  void Join();
};
