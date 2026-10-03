#include "comboaddress.hh"

void ComboAddress::truncate(unsigned int bits)
{
  uint8_t* start;
  int len=4;
  if(sin4.sin_family==AF_INET) {
    if(bits >= 32)
      return;
    start = (uint8_t*)&sin4.sin_addr.s_addr;
    len=4;
  }
  else {
    if(bits >= 128)
      return;
    start = (uint8_t*)&sin6.sin6_addr.s6_addr;
    len=16;
  }

  auto tozero= len*8 - bits; // if set to 22, this will clear 1 byte, as it should

  memset(start + len - tozero/8, 0, tozero/8); // blot out the whole bytes on the right
  
  auto bitsleft=tozero % 8; // 2 bits left to clear

  // a b c d, to truncate to 22 bits, we just zeroed 'd' and need to zero 2 bits from c
  // so and by '11111100', which is ~((1<<2)-1)  = ~3
  uint8_t* place = start + len - 1 - tozero/8; 
  *place &= (~((1<<bitsleft)-1));
}

int makeIPv6sockaddr(const std::string& addr, struct sockaddr_in6* ret)
{
  if(addr.empty())
    return -1;
  std::string ourAddr(addr);
  int port = -1;
  if(addr[0]=='[') { // [::]:53 style address
    auto pos = addr.find(']');
    if(pos == std::string::npos || pos + 2 > addr.size() || addr[pos+1]!=':')
      return -1;
    ourAddr.assign(addr.c_str() + 1, pos-1);
    port = atoi(addr.substr(pos+2).c_str());
  }
  ret->sin6_scope_id=0;
  ret->sin6_family=AF_INET6;

  if(inet_pton(AF_INET6, ourAddr.c_str(), (void*)&ret->sin6_addr) != 1) {
    struct addrinfo* res;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET6;
    hints.ai_flags = AI_NUMERICHOST;

    int error;
    // getaddrinfo has anomalous return codes, anything nonzero is an error, positive or negative
    if((error=getaddrinfo(ourAddr.c_str(), 0, &hints, &res))) { 
      return -1;
    }

    memcpy(ret, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
  }

  if(port > 65535)
    // negative ports are found with the pdns_stou above
    return -1;

  if(port >= 0)
    ret->sin6_port = htons(port);

  return 0;
}

int makeIPv4sockaddr(const std::string& str, struct sockaddr_in* ret)
{
  if(str.empty()) {
    return -1;
  }
  struct in_addr inp;

  auto pos = str.find(':');
  if(pos == std::string::npos) { // no port specified, not touching the port
    if(inet_aton(str.c_str(), &inp)) {
      ret->sin_addr.s_addr=inp.s_addr;
      return 0;
    }
    return -1;
  }
  if(!*(str.c_str() + pos + 1)) // trailing :
    return -1;

  char *eptr = (char*)str.c_str() + str.size();
  int port = strtol(str.c_str() + pos + 1, &eptr, 10);
  if (port < 0 || port > 65535)
    return -1;

  if(*eptr)
    return -1;

  ret->sin_port = htons(port);
  if(inet_aton(str.substr(0, pos).c_str(), &inp)) {
    ret->sin_addr.s_addr=inp.s_addr;
    return 0;
  }
  return -1;
}
