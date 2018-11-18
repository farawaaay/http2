#include <functional>
#include <string>
#include <vector>
#include "Server.h"

using namespace std;

struct ListenOptions {
  string host;
  uint16_t port;
};

class HttpReq {
 private:
 public:
  string method;
  string path;
  vector<pair<string, string>> headers;
  void onData(function<void(char*, u_long)>);
  void onEnd(function<void(void)>);
};

class HttpRes {
 public:
  HttpRes& Status(uint16_t, string);
  HttpRes& SetHeader(string name, string value);
  HttpRes& Write(char*, u_long);
  HttpRes& End();
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
  void onReq(function<void(HttpReq&, HttpRes&)>);
  void Close();
  void Join();
};
