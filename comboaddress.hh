#pragma once

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <sstream>
#include <tuple>
#include <string.h>

int makeIPv6sockaddr(const std::string& addr, struct sockaddr_in6* ret);
int makeIPv4sockaddr(const std::string& str, struct sockaddr_in* ret);

constexpr uint32_t chtonl(uint32_t s)
{
    return (
        ((s & 0x000000FF) << 24) | ((s & 0x0000FF00) << 8)
      | ((s & 0xFF000000) >> 24) | ((s & 0x00FF0000) >> 8)
    );
}

constexpr struct sockaddr_in operator "" _ipv4(const char* p, size_t l)
{
  struct sockaddr_in ret={};
  ret.sin_addr.s_addr=0;
  ret.sin_family=AF_INET;
  
  uint8_t octet=0;

  size_t n=0;
  for(; n < l; ++n) {
    if(p[n]==':')
      break;
    if(p[n]=='.') {
      ret.sin_addr.s_addr*=0x100;
      ret.sin_addr.s_addr+=octet;
      octet=0;
    }
    else {
      octet*=10;
      octet+=p[n]-'0';
    }
  }
  ret.sin_addr.s_addr*=0x100;
  ret.sin_addr.s_addr+=octet;

  ret.sin_addr.s_addr = chtonl(ret.sin_addr.s_addr);
  
  if(p[n]==':') {
    for(++n; n < l; ++n) {
      ret.sin_port*=10;
      ret.sin_port += p[n]-'0';
    }
    ret.sin_port = 0x100 * (ret.sin_port&0xff) + (ret.sin_port/0x100);
  }
  return ret;

}

/** The ComboAddress holds an IPv4 or an IPv6 endpoint, including a source port.
    It is ABI-compatible with struct sockaddr, sockaddr_in and sockaddr_in6. 
    This means it can be passed to the kernel via a pointer directly. 

    When doing so, it is imperative to pass the right length parameter, because some 
    operating systems get confused if they see length longer than necessary.

    Canonical use is: connect(sock, (struct sockaddr*)&combo, combo.getSocklen())

    The union has methods for parsing string ('human') IP address representations, and
    these support all kinds of addresses, including scoped IPv6 and port numbers 
    (1.2.3.4:80 and [::1]:80 or even [fe80::1%eth0]:80).
*/

union ComboAddress {
  struct sockaddr_in sin;
  struct sockaddr_in sin4;
  struct sockaddr_in6 sin6;

  //! Tests for equality, including port number. IPv4 and IPv6 are never equal.
  bool operator==(const ComboAddress& rhs) const
  {
    if(std::tie(sin4.sin_family, sin4.sin_port) != std::tie(rhs.sin4.sin_family, rhs.sin4.sin_port))
      return false;
    if(sin4.sin_family == AF_INET)
      return sin4.sin_addr.s_addr == rhs.sin4.sin_addr.s_addr;
    else
      return memcmp(&sin6.sin6_addr.s6_addr, &rhs.sin6.sin6_addr.s6_addr, sizeof(sin6.sin6_addr.s6_addr))==0;
  }

  //! Inequality
  bool operator!=(const ComboAddress& rhs) const
  {
    return(!operator==(rhs));
  }

  //! This ordering is intended to be fast and strict, but not necessarily human friendly.
  bool operator<(const ComboAddress& rhs) const
  {
    if(sin4.sin_family == 0) {
      return false;
    } 
    if(std::tie(sin4.sin_family, sin4.sin_port) < std::tie(rhs.sin4.sin_family, rhs.sin4.sin_port))
      return true;
    if(std::tie(sin4.sin_family, sin4.sin_port) > std::tie(rhs.sin4.sin_family, rhs.sin4.sin_port))
      return false;
    
    if(sin4.sin_family == AF_INET)
      return htonl(sin4.sin_addr.s_addr) < htonl(rhs.sin4.sin_addr.s_addr);
    else
      return memcmp(&sin6.sin6_addr.s6_addr, &rhs.sin6.sin6_addr.s6_addr, sizeof(sin6.sin6_addr.s6_addr)) < 0;
  }

  bool operator>(const ComboAddress& rhs) const
  {
    return rhs.operator<(*this);
  }
  /*
  struct addressOnlyHash
  {
    uint32_t operator()(const ComboAddress& ca) const 
    { 
      const unsigned char* start;
      int len;
      if(ca.sin4.sin_family == AF_INET) {
        start =(const unsigned char*)&ca.sin4.sin_addr.s_addr;
        len=4;
      }
      else {
        start =(const unsigned char*)&ca.sin6.sin6_addr.s6_addr;
        len=16;
      }
      return burtle(start, len, 0);
    }
  };
  */

  //! Convenience comparator that compares regardless of port. 
  struct addressOnlyLessThan
  {
    bool operator()(const ComboAddress& a, const ComboAddress& b) const
    {
      if(a.sin4.sin_family < b.sin4.sin_family)
        return true;
      if(a.sin4.sin_family > b.sin4.sin_family)
        return false;
      if(a.sin4.sin_family == AF_INET)
        return a.sin4.sin_addr.s_addr < b.sin4.sin_addr.s_addr;
      else
        return memcmp(&a.sin6.sin6_addr.s6_addr, &b.sin6.sin6_addr.s6_addr, sizeof(a.sin6.sin6_addr.s6_addr)) < 0;
    }
  };
  //! Convenience comparator that compares regardless of port. 
  struct addressOnlyEqual
  {
    bool operator()(const ComboAddress& a, const ComboAddress& b) const
    {
      if(a.sin4.sin_family != b.sin4.sin_family)
        return false;
      if(a.sin4.sin_family == AF_INET)
        return a.sin4.sin_addr.s_addr == b.sin4.sin_addr.s_addr;
      else
        return !memcmp(&a.sin6.sin6_addr.s6_addr, &b.sin6.sin6_addr.s6_addr, sizeof(a.sin6.sin6_addr.s6_addr));
    }
  };

  //! it is vital to pass the correct socklen to the kernel
  socklen_t getSocklen() const
  {
    if(sin4.sin_family == AF_INET)
      return sizeof(sin4);
    else
      return sizeof(sin6);
  }

  //! Initializes an 'empty', impossible, ComboAddress
  ComboAddress() 
  {
    sin4.sin_family=AF_INET;
    sin4.sin_addr.s_addr=0;
    sin4.sin_port=0;
  }

  //! Make a ComboAddress from a traditional sockaddr
  ComboAddress(const struct sockaddr *sa, socklen_t salen) {
    setSockaddr(sa, salen);
  }

  //! Make a ComboAddress from a traditional sockaddr_in6
  ComboAddress(const struct sockaddr_in6 *sa) {
    setSockaddr((const struct sockaddr*)sa, sizeof(struct sockaddr_in6));
  }

  //! Make a ComboAddress from a traditional sockaddr_in
  ComboAddress(const struct sockaddr_in *sa) {
    setSockaddr((const struct sockaddr*)sa, sizeof(struct sockaddr_in));
  }
  //! Make a ComboAddress from a traditional sockaddr
  ComboAddress(const struct sockaddr_in& sa) {
    setSockaddr((const struct sockaddr*)&sa, sizeof(struct sockaddr_in));
  }

  void setSockaddr(const struct sockaddr *sa, socklen_t salen) {
    if (salen > sizeof(struct sockaddr_in6)) throw std::runtime_error("ComboAddress can't handle other than sockaddr_in or sockaddr_in6");
    memcpy(this, sa, salen);
  }

  /** "Human" representation constructor. 
      The following are all identical: 
      ComboAddress("1.2.3.4:80");
      ComboAddress("1.2.3.4", 80);
      ComboAddress("1.2.3.4:80", 1234)

      As are:
      ComboAddress("fe80::1%eth0", 80);
      ComboAddress("[fe80::1%eth0]:80");
      ComboAddress("[fe::1%eth0]:80", 1234);
  */
  explicit ComboAddress(const std::string& str, uint16_t port=0)
  {
    memset(&sin6, 0, sizeof(sin6));
    sin4.sin_family = AF_INET;
    sin4.sin_port = 0;
    if(makeIPv4sockaddr(str, &sin4)) {
      sin6.sin6_family = AF_INET6;
      if(makeIPv6sockaddr(str, &sin6) < 0)
        throw std::runtime_error("Unable to convert presentation address '"+ str +"'"); 
      
    }
    if(!sin4.sin_port) // 'str' overrides port!
      sin4.sin_port=htons(port);
  }
  //! Sets port, deals with htons for you
  void setPort(uint16_t port)
  {
    sin4.sin_port = htons(port);
  }
  //! Is this an IPv6 address?
  bool isIPv6() const
  {
    return sin4.sin_family == AF_INET6;
  }

  //! Is this an IPv4 address?
  bool isIPv4() const
  {
    return sin4.sin_family == AF_INET;
  }

  //! Is this an ffff:: style IPv6 address?
  bool isMappedIPv4()  const
  {
    if(sin4.sin_family!=AF_INET6)
      return false;
    
    int n=0;
    const unsigned char*ptr = (unsigned char*) &sin6.sin6_addr.s6_addr;
    for(n=0; n < 10; ++n)
      if(ptr[n])
        return false;
    
    for(; n < 12; ++n)
      if(ptr[n]!=0xff)
        return false;
    
    return true;
  }

  //! Extract the IPv4 address from a mapped IPv6 address
  ComboAddress mapToIPv4() const
  {
    if(!isMappedIPv4())
      throw std::runtime_error("ComboAddress can't map non-mapped IPv6 address back to IPv4");
    ComboAddress ret;
    ret.sin4.sin_family=AF_INET;
    ret.sin4.sin_port=sin4.sin_port;
    
    const unsigned char*ptr = (unsigned char*) &sin6.sin6_addr.s6_addr;
    ptr+=(sizeof(sin6.sin6_addr.s6_addr) - sizeof(ret.sin4.sin_addr.s_addr));
    memcpy(&ret.sin4.sin_addr.s_addr, ptr, sizeof(ret.sin4.sin_addr.s_addr));
    return ret;
  }

  //! Returns a string (human) represntation of the address
  std::string toString() const
  {
    char host[1024];
    if(sin4.sin_family && !getnameinfo((struct sockaddr*) this, getSocklen(), host, sizeof(host),0, 0, NI_NUMERICHOST))
      return host;
    else
      return "invalid";
  }

  //! Returns a string (human) represntation of the address, including port
  std::string toStringWithPort() const
  {
    if(sin4.sin_family==AF_INET)
      return toString() + ":" + std::to_string(ntohs(sin4.sin_port));
    else
      return "["+toString() + "]:" + std::to_string(ntohs(sin4.sin_port));
  }

  void truncate(unsigned int bits);
};



/** This class represents a netmask and can be queried to see if a certain
    IP address is matched by this mask */
class Netmask
{
public:
  Netmask()
  {
	d_network.sin4.sin_family=0; // disable this doing anything useful
	d_network.sin4.sin_port = 0; // this guarantees d_network compares identical
	d_mask=0;
	d_bits=0;
  }
  
  explicit Netmask(const ComboAddress& network, uint8_t bits=0xff)
  {
    d_network = network;
    d_network.sin4.sin_port=0;
    if(bits > 128)
      bits = (network.sin4.sin_family == AF_INET) ? 32 : 128;
    
    d_bits = bits;
    if(d_bits<32)
      d_mask=~(0xFFFFFFFF>>d_bits);
    else
      d_mask=0xFFFFFFFF; // not actually used for IPv6
  }

  static std::pair<std::string, std::string> splitField(const std::string& inp, char sepa)
  {
    std::pair<std::string, std::string> ret;
    auto cpos=inp.find(sepa);
    if(cpos==std::string::npos)
      ret.first=inp;
    else {
      ret.first=inp.substr(0, cpos);
      ret.second=inp.substr(cpos+1);
    }
    return ret;
  }

  
  //! Constructor supplies the mask, which cannot be changed 
  Netmask(const std::string &mask) 
  {
    auto split=splitField(mask,'/');
    d_network=ComboAddress(split.first);
    
    if(!split.second.empty()) {
      d_bits = (uint8_t)atoi(split.second.c_str());
      if(d_bits<32)
        d_mask=~(0xFFFFFFFF>>d_bits);
      else
        d_mask=0xFFFFFFFF;
    }
    else if(d_network.sin4.sin_family==AF_INET) {
      d_bits = 32;
      d_mask = 0xFFFFFFFF;
    }
    else {
      d_bits=128;
      d_mask=0;  // silence silly warning - d_mask is unused for IPv6
    }
  }

  bool match(const ComboAddress& ip) const
  {
    return match(&ip);
  }

  //! If this IP address in socket address matches
  bool match(const ComboAddress *ip) const
  {
    if(d_network.sin4.sin_family != ip->sin4.sin_family) {
      return false;
    }
    if(d_network.sin4.sin_family == AF_INET) {
      return match4(htonl((unsigned int)ip->sin4.sin_addr.s_addr));
    }
    if(d_network.sin6.sin6_family == AF_INET6) {
      uint8_t bytes=d_bits/8, n;
      const uint8_t *us=(const uint8_t*) &d_network.sin6.sin6_addr.s6_addr;
      const uint8_t *them=(const uint8_t*) &ip->sin6.sin6_addr.s6_addr;
      
      for(n=0; n < bytes; ++n) {
        if(us[n]!=them[n]) {
          return false;
        }
      }
      // still here, now match remaining bits
      uint8_t bits= d_bits % 8;
      uint8_t mask= (uint8_t) ~(0xFF>>bits);

      return((us[n] & mask) == (them[n] & mask));
    }
    return false;
  }

  //! If this ASCII IP address matches
  bool match(const std::string &ip) const
  {
    ComboAddress address(ip);
    return match(&address);
  }

  //! If this IP address in native format matches
  bool match4(uint32_t ip) const
  {
    return (ip & d_mask) == (ntohl(d_network.sin4.sin_addr.s_addr) & d_mask);
  }

  std::string toString() const
  {
    return d_network.toString()+"/"+std::to_string((unsigned int)d_bits);
  }

  std::string toStringNoMask() const
  {
    return d_network.toString();
  }
  const ComboAddress& getNetwork() const
  {
    return d_network;
  }
  const ComboAddress getMaskedNetwork() const
  {
    ComboAddress result(d_network);
    if(isIpv4()) {
      result.sin4.sin_addr.s_addr = htonl(ntohl(result.sin4.sin_addr.s_addr) & d_mask);
    }
    else if(isIpv6()) {
      size_t idx;
      uint8_t bytes=d_bits/8;
      uint8_t *us=(uint8_t*) &result.sin6.sin6_addr.s6_addr;
      uint8_t bits= d_bits % 8;
      uint8_t mask= (uint8_t) ~(0xFF>>bits);

      if (bytes < sizeof(result.sin6.sin6_addr.s6_addr)) {
        us[bytes] &= mask;
      }

      for(idx = bytes + 1; idx < sizeof(result.sin6.sin6_addr.s6_addr); ++idx) {
        us[idx] = 0;
      }
    }
    return result;
  }
  int getBits() const
  {
    return d_bits;
  }
  bool isIpv6() const 
  {
    return d_network.sin6.sin6_family == AF_INET6;
  }
  bool isIpv4() const
  {
    return d_network.sin4.sin_family == AF_INET;
  }

  bool operator<(const Netmask& rhs) const 
  {
    return std::tie(d_network, d_bits) < std::tie(rhs.d_network, rhs.d_bits);
  }

  bool operator==(const Netmask& rhs) const 
  {
    return std::tie(d_network, d_bits) == std::tie(rhs.d_network, rhs.d_bits);
  }

  bool empty() const 
  {
    return d_network.sin4.sin_family==0;
  }

private:
  ComboAddress d_network;
  uint32_t d_mask;
  uint8_t d_bits;
};


