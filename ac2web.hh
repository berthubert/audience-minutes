#pragma once
#include <mutex>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>

extern std::mutex g_metrics_mutex;  
typedef std::map<
  std::pair<std::string, std::map<std::string, std::string>>
  , double> g_metrics_t;

extern g_metrics_t g_metrics;

struct TimedNumUnique
{
  explicit TimedNumUnique(unsigned int seconds = 60) : d_seconds(seconds)
  {}
  
  void add(const std::string& str)
  {
    maintenance();
    // ttd
    d_store.push_back({time(nullptr) + d_seconds, str});
  }

  size_t count() 
  {
    std::unordered_set<std::string> s;
    maintenance(); // this makes us non-const
    for(const auto& [_, str]: d_store)
      s.insert(str);
    return s.size();
  }
  
  void maintenance()
  {
    time_t now=time(nullptr);
    while(!d_store.empty() && d_store.front().first < now)
      d_store.pop_front();
    
  }
  std::deque<std::pair<time_t,std::string>> d_store;
  const unsigned int d_seconds;
};


extern std::map<std::map<std::string, std::string>,TimedNumUnique> g_tnus;

void launchWeb();
