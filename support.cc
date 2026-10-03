#include "support.hh"
#include <iostream>
#include "comboaddress.hh"
#include <fmt/printf.h>
using namespace std;

static unsigned __int128 to128(const ComboAddress& in)
{
  if(in.sin4.sin_family == AF_INET)
    return htonl(in.sin4.sin_addr.s_addr);
  else if(in.sin4.sin_family == AF_INET6) {
    unsigned __int128 ret=0;
    uint8_t* dptr = (uint8_t*) &ret;
    const uint8_t* sptr= (const uint8_t*) in.sin6.sin6_addr.s6_addr;
    
    for(int n=0; n < 16; ++n)
      dptr[n] = sptr[15-n];

    return ret;
  }
  throw std::runtime_error("Impossible ComboAddres");
}
/*
static string p128(unsigned __int128 t)
{
  string reg;
  uint8_t* ptr = (uint8_t*)&t;
  for(int n=15; n>=0; --n) {
    reg += fmt::sprintf("%02x", (unsigned int)ptr[n]);
  }
  return reg;
}
*/
CountryDB::CountryDB(const std::string& fname)
{
  FILE* fp = fopen(fname.c_str(), "r");
  if(!fp)
    throw std::runtime_error("Unable to open "+fname+" for reading IP addresses: "+string(strerror(errno)));
  
  shared_ptr<FILE> rfp(fp, fclose);
  char line[256];
  ComboAddress start, stop;
  
  while(fgets(line, sizeof(line), rfp.get())) {
    // 0.0.0.0,0.255.255.255,ZZ
    char* ptr = strchr(line, ',');
    if(!ptr)
      continue;
    *ptr = 0;
    start = ComboAddress(line);
    ptr++;
    char *ptr2 = strchr(ptr, ',');
    if(!ptr2)
      continue;
    *ptr2=0;
    ptr2++;
    stop = ComboAddress(ptr);
    
    char* ptr3 = strchr(ptr2, '\n');
    if(!ptr3)
      continue;
    *ptr3=0;
    string country = ptr2;
    if(country == "ZZ")
      continue;
    //      cout << ptr2 << endl;
    if(!(start < stop) && start != stop) {
      cout<<"Oops, "<<start.toString()<< " >= "<< stop.toString() <<endl;
    }
    //      if(start.sin4.sin_family != AF_INET)
    //	continue;
    //      cout<<start.toString() << " - "<< stop.toString()<<": "<<country<<endl;
    
    d_tree.add(to128(start), to128(stop)+1, country);
  }
  d_tree.index();
}

string CountryDB::getCountry(const std::string& ip)
{
  ComboAddress ca(ip);
  auto s = to128(ca);
  vector<size_t> result;
  d_tree.overlap(s, s+1, result);
  
  if(result.empty())
    return "??";
  
  return d_tree.data(result[0]);
}

string CountryDB::getASName(const std::string& ip)
{
  string val = getCountry(ip);
  // 13335,"Cloudflare, Inc."
  // 4134,Chinanet
  
  auto pos = val.find(",");
  if(pos == string::npos)
    return "??";
  
  string ret = val.substr(pos + 1);
  if(ret.size()>=2 && ret[0] == '"')
    return ret.substr(1, ret.size()-2);
  return ret;
}

uint32_t CountryDB::getAS(const std::string& ip)
{
  return (uint32_t)atol(getCountry(ip).c_str()); 
}

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
