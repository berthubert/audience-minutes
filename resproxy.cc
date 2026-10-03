#include <pqxx/pqxx>
#include <iostream>
#include <unordered_map>
#include "comboaddress.hh"
#include <set>
#include <fmt/core.h>
#include <fmt/os.h>

using namespace std;

int main(int argc, char** argv)
{
  pqxx::connection cx{""};
  pqxx::work tx{cx};
  struct hit
  {
    unsigned int count=0;
    time_t last=0;
    string lang;
    string country;
    int asn;
  };
  std::unordered_map<string, hit> ips;

  double hours = 0.5;
  if(argc > 1)
    hours = atof(argv[1]);

  double offsetHours = 0;
  if(argc > 2)
    offsetHours = atof(argv[2]);
  
  
  time_t end = time(nullptr) - offsetHours * 3600;
  time_t start =  end - hours*3600;
  const int borderMinutes = 5;
  cout<<"Scanning over "<<hours<<" hours, offset "<<offsetHours<<" hours, cutting off "<<borderMinutes<<" minutes from the ends"<<endl;
  unsigned int count=0;
  for (auto [ip, tstamp, lang, country, asn] : tx.query<string,time_t,string,string,int>("SELECT ip,timestamp,lang,country,asn FROM botfree where timestamp > $1  and timestamp < $2 and url like '/tkconv/%' order by timestamp",
						   pqxx::params(start, end))) {
    auto& entry = ips[ip];
    entry.count++;
    entry.last = tstamp;
    entry.lang = lang;
    entry.country = country;
    entry.asn = asn;
    count++;
  }

  unsigned int singles = 0;

  map<pair<string,string>,unsigned int> conlang;
  set<ComboAddress> bastards;
  set<pair<ComboAddress, int>> nl;
  for(const auto& [ip, hit] : ips) {
    
    if(hit.count == 1) {
      if(hit.last - start < borderMinutes*60 || end - hit.last < borderMinutes*60) {
	//	cout<<"Disregarding single hit "<<ip<<" because too close to the edge"<<endl;
	continue;
      }

      //      cout << ip << endl;
      singles++;
      conlang[{hit.country, hit.lang}]++;
      if(hit.country=="NL") {
	nl.insert(make_pair(ComboAddress(ip,0), hit.asn));
      }
      bastards.insert(ComboAddress(ip,0));
    }
  }
  cout<<singles<<" single query IPs out of "<<ips.size()<<". There were "<<count<<" hits claiming to be browsers in total"<<endl;

  multimap<unsigned int, pair<string, string>> rev;
  for(const auto& p : conlang)
    rev.insert({p.second, p.first});

  int limit = 20;
  for(auto iter = rev.rbegin(); iter != rev.rend() && limit; ++iter, --limit)
    cout << iter->second.first<<" | "<<iter->second.second<<": "<<iter->first<<endl;

  map<int, int> topnlasn;
  cout<<"NL: \n";
  for(const auto& [ip, asn] : nl) {
    cout << ip.toString() << "\t" << asn<<"\n";
    topnlasn[asn]++;
  }

  cout<<"Top NL ASNs: "<<endl;
  for(const auto& [asn, cnt] : topnlasn) {
    cout<<asn<<"\t"<<cnt<<"\n";
  }
  auto out = fmt::output_file("sneaky-crawlers.txt");
  for(const auto& ca : bastards)
    out.print("{}\n", ca.toString()); 
    
}
