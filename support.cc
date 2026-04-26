#include "support.hh"
#include <iostream>

using namespace std;

// 19/Mar/2023:00:00:10 +0100
time_t getTimeFromLog(const string& in)
{
  struct tm tm {};
  tm.tm_isdst = -1;
  const char* tzptr = strptime(in.c_str(), "%d/%b/%Y:%H:%M:%S ", &tm);

  time_t utc = timegm(&tm);
  int hoffset=0;
  int minoffset=0;
  char dir=1;
  int secondoffset=0;
  if(tzptr) {
    int ret = sscanf(tzptr, "%c%02d%02d", &dir, &hoffset, &minoffset);
    if(ret != EOF) {
      secondoffset = hoffset*3600 + minoffset*60;
      if(dir == '+')
	secondoffset = -secondoffset;
      //      cout<<"dir "<<dir<<" hoffset "<<hoffset<<" minoffset "<<minoffset<<endl;
    }
  }
  // 
  return utc + secondoffset;
}
