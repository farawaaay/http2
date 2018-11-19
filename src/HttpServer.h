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

struct HttpReq {
  vector<function<void(char*, u_long)>> dataCb;
  vector<function<void(void)>> endCb;

  string method;
  string path;
  map<string, string> headers;
  void OnData(function<void(char*, u_long)>);
  void OnEnd(function<void(void)>);
};

struct HttpRes {
  HttpRes& Status(uint16_t, string);
  HttpRes& SetHeader(string name, string value);
  HttpRes& Write(function<char*(u_long&)>, function<void(void)>);
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
  void OnReq(function<void(HttpReq&, HttpRes&)>);
  void Close();
  void Join();
};
