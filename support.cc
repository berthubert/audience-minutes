#include "support.hh"

using namespace std;

// 19/Mar/2023:00:00:10 +0100
time_t getTimeFromLog(const string& in)
{
  struct tm tm {};
  tm.tm_isdst = -1;
  strptime(in.c_str(), "%d/%b/%Y:%H:%M:%S %z", &tm);

  // this gets the timezone wrong - it assumes the time is local to us
  return mktime(&tm);
}
