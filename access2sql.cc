#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include <sstream>
#include <time.h>
#include <deque>
#include <unordered_set>
#include "ac2web.hh"
#include "comboaddress.hh"
#include "IITree.h"
#include <fmt/printf.h>
#include <memory>
#include <random>
#include <algorithm>
#include "support.hh"

// #include "ext/sqlitewriter/sqlwriter.hh"
#include "ext/sqlitewriter/psqlwriter.hh"

/* You can pipe a typical access.log into this, and it will populate a database  for you, in streaming fashion.
   You can safely access that sqlite database while the program runs, see https://berthub.eu/articles/posts/big-data-storage/

   A fun view to create:
create view botfree as select * from data where agent like 'Mozilla/5.0 %' and agent not like '%bot%' and agent not like '%miniflux%' 

CREATE OR REPLACE VIEW public.botfree AS
 SELECT data.siz,
    data.ref,
    data.agent,
    data.params,
    data.url,
    data.ip,
    data.stat,
    data."timestamp"
   FROM data
  WHERE data.agent ~~ 'Mozilla/5.0 %'::text AND data.agent !~~ '%bot%'::text AND data.agent !~~ '%miniflux%'::text AND data.ref <> 'http://www.google.co.uk/url?sa=t&source=web&cd=1'::text
  and data.ip <> '100.25.31.6'::text and data.ip <> '2a04:c47:e00:d3e3:4db:78ff:fe00:57'::text and data.ip <> '194.182.176.137'::text
  


   This filters out the bulk of bots right now. 
*/

using namespace std;

struct Parser {
  explicit Parser(FILE* fp) : d_fp(fp)
  {}
  
  FILE* d_fp;

  struct EofException{};
  
  void skipSpaces()
  {
    int c;
    for(;;) {
      c = getc(d_fp);
      if(c==EOF)
        throw EofException();
      if(c!=' ') {
        ungetc(c, d_fp);
        break;
      }
    }
  }
  void skipToEol()
  {
    int c;
    for(;;) {
      c = getc(d_fp);
      if(c==EOF)
        throw EofException();
      if(c=='\n') {
        break;
      }
    }
  }

  string getWord()
  {
    skipSpaces();
    string ret;
    int c;
    for(;;) {
      c = getc(d_fp);
      if(c==EOF)
        throw EofException();
      if(c==' ')
        break;
      ret.append(1, (char)c);
    }
    return ret;
  }

  string getDelim(char start, char stop)
  {
    skipSpaces();
    string ret;
    int c;
    c = getc(d_fp);
    if(c==EOF)
      throw EofException();

    if(c!=start)
      throw runtime_error("Wrong delimiter, skipping");
    for(;;) {
      c = getc(d_fp);
      if(c==EOF)
        throw EofException();

      if(c==EOF)
        throw EofException();
      if(c==stop)
        break;
      if(c=='\n')
        throw runtime_error("Delimiter not found on line, skipping");
      ret.append(1, (char)c);
    }
    return ret;
  }

  int64_t getNumber()
  {
    string word = getWord();
    return stol(word);
  }
  string getQuotedWord();
};


bool starts_with(const std::string& str, const std::string& prefix)
{
    return str.compare(0, prefix.length(), prefix) == 0;
}

// written by ChatGPT!
vector<string> split_string(const string& input)
{
  istringstream iss(input);
  vector<string> tokens;
  string token;
  while (iss >> token)
    tokens.push_back(token);
  return tokens;
}

/* singles:
   Oops, 157.120.255.255 >= 157.120.255.255
Oops, 159.60.131.186 >= 159.60.131.186
Oops, 159.60.133.131 >= 159.60.133.131
*/

unsigned __int128 to128(const ComboAddress& in)
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

static string p128(unsigned __int128 t)
{
  string reg;
  uint8_t* ptr = (uint8_t*)&t;
  for(int n=15; n>=0; --n) {
    reg += fmt::sprintf("%02x", (unsigned int)ptr[n]);
  }
  return reg;
}

struct CountryDB
{
  explicit CountryDB(const std::string& fname)
  {
    FILE* fp = fopen(fname.c_str(), "r");
    if(!fp)
      throw std::runtime_error("Unable to open "+fname+" for reading IP addresses: "+string(strerror(errno)));
    
    shared_ptr<FILE> rfp(fp, fclose);
    char line[80];
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

  string getCountry(const std::string& ip)
  {
    ComboAddress ca(ip);
    auto s = to128(ca);
    vector<size_t> result;
    d_tree.overlap(s, s+1, result);

    if(result.empty())
      return "??";

    return d_tree.data(result[0]);
  }

  IITree<unsigned __int128, std::string> d_tree;
};

int main(int argc, char** argv)
try
{
  // https://db-ip.com/db/download/ip-to-country-lite
  CountryDB cdb("dbip-country-lite.csv");
  cout<<"Done with import!"<<endl;
  /*
  cout << cdb.getCountry("213.244.168.12") << endl;
  cout << cdb.getCountry("130.161.252.29") << endl;
  cout << cdb.getCountry("159.60.131.186") << endl;

  cout << cdb.getCountry("2001:41f0:782d::2") << endl;  
  cout << cdb.getCountry("2a04:4e42:30::144") << endl;
  cout << cdb.getCountry("2600:3c04:e001:324:0:1991:8:25") << endl;
  cout << cdb.getCountry("2001:503:ba3e::2:30") << endl;
  */
  
  Parser p(stdin);
  // SQLiteWriter sqw(argc > 1 ? argv[1] : "access.sqlite3");
  PSQLWriter sqw(argc > 1 ? argv[1] : "accesslog");

  auto t = std::thread(launchWeb);
  t.detach();
  
  // ::ffff:146.255.56.92 - - [30/Aug/2025:15:12:11 +0200] "GET / HTTP/1.1" 200 6607 "-" "Mastodon/4.4.3-stable+ff1 (http.rb/5.3.1; +https://sloth.es/)" "berthub.eu"

  unsigned int lines=0;
  for(;;) {
    if(!(lines % 16384)) {
      cout <<"\r"<<lines;
      cout.flush();
    }
    lines++;
    try {
      string ip = p.getWord();
      string ign1 = p.getWord();
      string ign2 = p.getWord();
      bool ipv4 = false;
      if(starts_with(ip, "::ffff:")) {
        ip = ip.substr(7);
	ipv4 = true;
      }
      string country = cdb.getCountry(ip);
      string t = p.getDelim('[', ']');
      time_t tim = getTimeFromLog(t);
      string req = p.getDelim('"', '"');
      int64_t stat = p.getNumber();
      int64_t size = p.getNumber();
      string ref = p.getDelim('"', '"');
      string agent = p.getDelim('"', '"');
      string host = p.getDelim('"', '"');
      string range = p.getDelim('"', '"');
      string lang = p.getDelim('"', '"');

      
      auto parts = split_string(req);
      string url;
      string params;
      if(parts.size() >= 2) {
        url = parts[1];
        
        if(auto pos = url.find('?'); pos != string::npos) {
          params = url.substr(pos+1);
          url.resize(pos);
        }
      }

      sqw.addValue({{"timestamp", tim}, {"ip", ip}, {"url", url},
                    {"params", params}, {"agent", agent}, {"ref", ref},
                    {"stat", stat}, {"siz", size}, {"host", host}, {"country", country}, {"lang", lang}});

      lock_guard<mutex> mut(g_metrics_mutex);
      map<string, string> labels;
      labels["host"]=host;
      labels["ipv"] = ipv4 ? "4" : "6";
      labels["bot"] = (!agent.starts_with("Mozilla/5.0 ") || agent.contains("bot") || agent.contains("miniflux")) ? "1" : "0";

      g_tnus[labels].add(ip); // don't want the status in there
      
      labels["status"] = to_string(stat);
      

      
      g_metrics[{"queries",labels}]++;
      g_metrics[{"bytesSent",labels}] += size;
    }
    catch(std::exception& i) {
      cerr<<i.what()<<endl;
    }
    p.skipToEol();
  }
}
catch(Parser::EofException& )
{
  return 0;
}
