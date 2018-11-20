#pragma once
#include <winsock2.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace std;

u_long GetNumberOfProcessors();
std::vector<std::string> split(const std::string&, const std::string&);
bool hasEnding(std::string const& fullString, std::string const& ending);
std::string mimeType(string);
