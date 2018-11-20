#include "Utils.h"

u_long GetNumberOfProcessors() {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwNumberOfProcessors;
}

std::vector<std::string> split(const std::string& text, const std::string& delims) {
  std::vector<std::string> tokens;
  std::size_t start = text.find_first_not_of(delims), end = 0;

  while ((end = text.find_first_of(delims, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = text.find_first_not_of(delims, end);
  }
  if (start != std::string::npos)
    tokens.push_back(text.substr(start));

  return tokens;
}

bool hasEnding(std::string const& fullString, std::string const& ending) {
  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(fullString.length() - ending.length(),
                                    ending.length(), ending));
  } else {
    return false;
  }
}

bool mimeTypeMapInit = false;
map<string, string> mimeTypeMap;

std::string mimeType(string ext) {
  if (!mimeTypeMapInit) {
    mimeTypeMap[".aac"] = "audio/aac";
    mimeTypeMap[".abw"] = "application/x-abiword";
    mimeTypeMap[".arc"] = "application/octet-stream";
    mimeTypeMap[".avi"] = "video/x-msvideo";
    mimeTypeMap[".azw"] = "application/vnd.amazon.ebook";
    mimeTypeMap[".bin"] = "application/octet-stream";
    mimeTypeMap[".bz"] = "application/x-bzip";
    mimeTypeMap[".bz2"] = "application/x-bzip2";
    mimeTypeMap[".csh"] = "application/x-csh";
    mimeTypeMap[".css"] = "text/css";
    mimeTypeMap[".csv"] = "text/csv";
    mimeTypeMap[".doc"] = "application/msword";
    mimeTypeMap[".epub"] = "application/epubzip";
    mimeTypeMap[".gif"] = "image/gif";
    mimeTypeMap[".htm"] = "text/html";
    mimeTypeMap[".html"] = "text/html";
    mimeTypeMap[".ico"] = "image/x-icon";
    mimeTypeMap[".ics"] = "text/calendar";
    mimeTypeMap[".jar"] = "application/java-archive";
    mimeTypeMap[".jpg"] = "image/jpeg";
    mimeTypeMap[".jpeg"] = "image/jpeg";
    mimeTypeMap[".js"] = "application/javascript";
    mimeTypeMap[".json"] = "application/json";
    mimeTypeMap[".json"] = "application/javascript";
    mimeTypeMap[".mid"] = "audio/midi";
    mimeTypeMap[".midi"] = "audio/midi";
    mimeTypeMap[".mpeg"] = "video/mpeg";
    mimeTypeMap[".mpkg"] = "application/vnd.apple.installerxml";
    mimeTypeMap[".odp"] = "application/vnd.oasis.opendocument.presentation";
    mimeTypeMap[".ods"] = "application/vnd.oasis.opendocument.spreadsheet";
    mimeTypeMap[".odt"] = "application/vnd.oasis.opendocument.text";
    mimeTypeMap[".oga"] = "audio/ogg";
    mimeTypeMap[".ogv"] = "video/ogg";
    mimeTypeMap[".ogx"] = "application/ogg";
    mimeTypeMap[".pdf"] = "application/pdf";
    mimeTypeMap[".ppt"] = "application/vnd.ms-powerpoint";
    mimeTypeMap[".rar"] = "application/x-rar-compressed";
    mimeTypeMap[".rtf"] = "application/rtf";
    mimeTypeMap[".sh"] = "application/x-sh";
    mimeTypeMap[".svg"] = "image/svgxml";
    mimeTypeMap[".swf"] = "application/x-shockwave-flash";
    mimeTypeMap[".tar"] = "application/x-tar";
    mimeTypeMap[".tiff"] = "image/tiff";
    mimeTypeMap[".tif"] = "image/tiff";
    mimeTypeMap[".ttf"] = "application/x-font-ttf";
    mimeTypeMap[".txt"] = "text/plain";
    mimeTypeMap[".vsd"] = "application/vnd.visio";
    mimeTypeMap[".wav"] = "audio/x-wav";
    mimeTypeMap[".weba"] = "audio/webm";
    mimeTypeMap[".webm"] = "video/webm";
    mimeTypeMap[".webp"] = "image/webp";
    mimeTypeMap[".woff"] = "application/x-font-woff";
    mimeTypeMap[".xhtml"] = "application/xhtmlxml";
    mimeTypeMap[".xls"] = "application/vnd.ms-excel";
    mimeTypeMap[".xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    mimeTypeMap[".xml"] = "application/xml";
    mimeTypeMap[".xul"] = "application/vnd.mozilla.xulxml";
    mimeTypeMap[".zip"] = "application/zip";
    mimeTypeMap[".3gp"] = "video/3gpp";
    mimeTypeMap[".3g2"] = "video/3gpp2";
    mimeTypeMap[".7z"] = "application/x-7z-compressed";
    mimeTypeMapInit = true;
  }
  try {
    return mimeTypeMap.at(ext);
  } catch (const std::out_of_range) {
    return "application/octet-stream";
  }
}
