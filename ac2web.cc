#include "httplib.h"
#include <iostream>
#include "ac2web.hh"

using namespace std;

std::mutex g_metrics_mutex;  
g_metrics_t g_metrics;
map<map<string, string>,TimedNumUnique> g_tnus;

static void addMetric(ostringstream& ret, const std::string& key, const std::map<string,string>& labels , double value, const string& desc, std::string_view kind)
{
  ret << "# HELP access_" << key << " " <<desc <<endl;
  ret << "# TYPE access_"<< key << " " << kind <<endl;
  ret<<"access2_"<<key;
  if(!labels.empty()) {
    ret<<"{";
    
    for(bool first = true; const auto& [key, val]: labels) {
      if(!first) {
	ret<<",";
      } else first=false;
      ret << key<<"="<<std::quoted(val);
      
    }
    ret << "}";
  }
  
  ret <<" " << std::fixed<< value <<endl;
}


void launchWeb()
{
  // HTTP
  httplib::Server svr;


  svr.Get("/metrics", [](const auto &, auto& res) {
    ostringstream response;
    lock_guard<mutex> mut(g_metrics_mutex);

    for(const auto& [mpair, value] : g_metrics) {
      addMetric(response, mpair.first, mpair.second, value, "Number of HTTP(s) queries", "counter");
    }

    for(auto& [mpair, value] : g_tnus) {
      addMetric(response, "numips" , mpair, value.count(), "Number of distinct IP addresses", "gauge");
    }

    
    res.set_content(response.str(), "text/plain");
  });

  cout<<"Going live!"<<endl;
  svr.listen("0.0.0.0", 9003);
}
