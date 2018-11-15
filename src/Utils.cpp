#include "Utils.h"

u_long GetNumberOfProcessors() {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwNumberOfProcessors;
}