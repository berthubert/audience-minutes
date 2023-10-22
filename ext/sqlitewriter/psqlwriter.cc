#include <iostream>
#include <stdexcept>
#include <poll.h>
#include "minipsql.hh"
#include <string.h>
#include <unistd.h>
#include <variant>
#include <thread>
#include <mutex>
#include <fcntl.h>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <algorithm>
#include "psqlwriter.hh"
#include <map>
using namespace std;

struct DTime
{
  void start()
  {
    d_start =   std::chrono::steady_clock::now();
  }
  uint32_t lapUsec()
  {
    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()- d_start).count();
    start();
    return usec;
  }

  std::chrono::time_point<std::chrono::steady_clock> d_start;
};

// seconds
static int waitForRWData(int fd, bool waitForRead, double* timeout, bool* error=0, bool* disconnected=0)
{
  int ret;

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fd;

  if(waitForRead)
    pfd.events=POLLIN;
  else
    pfd.events=POLLOUT;

  ret = poll(&pfd, 1, timeout ? (*timeout * 1000) : -1);
  if ( ret == -1 ) {
    throw std::runtime_error("Waiting for data: "+std::string(strerror(errno)));
  }
  if(ret > 0) {
    if (error && (pfd.revents & POLLERR)) {
      *error = true;
    }
    if (disconnected && (pfd.revents & POLLHUP)) {
      *disconnected = true;
    }

  }
  return ret;
}

int waitForData(int fd, double timeout)
{
  return waitForRWData(fd, true, &timeout);
}

void SetNonBlocking(int sock, bool to)
{
  int flags=fcntl(sock,F_GETFL,0);
  if(flags<0)
    std::runtime_error(string("Retrieving socket flags: ")+ strerror(errno));

  // so we could optimize to not do it if nonblocking already set, but that would be.. semantics
  if(to) {
    flags |= O_NONBLOCK;
  }
  else 
    flags &= (~O_NONBLOCK);
      
  if(fcntl(sock, F_SETFL, flags) < 0)
    std::runtime_error(string("Setting socket flags: ")+ strerror(errno));
}


void PSQLWriter::commitThread()
{
  MiniPSQL mp("");
  mp.exec("begin");

  map<string,vector<pair<string,string>>> schemas;

  // so how does this work
  // we get a stream of messages, each aimed at a single table
  // we group the messages by table, and check if the table exists, and if all fields exist
  // if not, we create tables and fields to match
  SetNonBlocking(d_pipe[0], true);
  bool needWait = false;
  time_t prevcommit=time(0);
  for(;;) {
    map<string, vector<Message*>> tabwork; // group by table
    DTime dt;
    dt.start();
    int lim=0;
    int sumparms = 0;
    for(; lim < 10000 && sumparms < 60000; ++lim) {
      Message* msg;
      if(needWait) {
        cout<<"Waiting for data.."<<endl;
        waitForData(d_pipe[0], 1);
      }
      int rc = read(d_pipe[0], &msg, sizeof(msg));

      if(rc == 0 && !tabwork.empty())
        break;
      if(rc == 0 || (rc < 0 && errno != EAGAIN)) {
        DTime dt2;
        dt2.start();
        mp.exec("commit");
//        cout<<"Commit took "<<dt2.lapUsec()/1000.0<<" msec"<<endl;
        return;
      }
      if(rc < 0 && errno==EAGAIN) {
        needWait = true;
        break;
      }
      sumparms += msg->values.size();
      tabwork[msg->table].push_back(msg);
      needWait=false;
    }
    cout<<"Received "<<lim<<" messages "<<"from "<<tabwork.size()<<" tables with work, took "<<dt.lapUsec()/1000.0<<"msec, fields:";
    dt.start();

    for(auto& [table, work] : tabwork) {
      if(!schemas.count(table)) // new table
        schemas[table] = mp.getSchema(table);

      unordered_set<string> fields;
      
      for(const auto& m : work) {
        for(const auto& f : m->values) {
          if(auto iter = fields.find(f.first); iter == fields.end()) {
            fields.insert(f.first);
            pair<string, string> cmp{f.first, std::string()};
            if(!binary_search(schemas[table].begin(), schemas[table].end(), cmp,
                       [](const auto& a, const auto& b)
                       {
                         return a.first < b.first;
                       })) {
              cout<<"shit, we miss "<<f.first<<endl;
              if(std::get_if<double>(&f.second)) {
                mp.addColumn(table, f.first, "REAL");
                schemas[table].push_back({f.first, "REAL"});
              }
              else if(std::get_if<string>(&f.second)) {
                mp.addColumn(table, f.first, "TEXT");
                schemas[table].push_back({f.first, "TEXT"});
              } else  {
                mp.addColumn(table, f.first, "BIGINT");
                schemas[table].push_back({f.first, "BIGINT"});
              }
              
              sort(schemas[table].begin(), schemas[table].end());

            }
          }
        }
      }
      
      string query="insert into "+table+" (";
    
      bool first=true;
      for(const auto& f : fields) {
        if(!first)
          query+=',';
        first=false;
        query+=f;
        cout<<" "<<f;
      }
      query += ") values ";
      int ctr=1;
      
      for(unsigned int n=0; n < work.size(); ++n) {
        if(n)
          query +=',';
        query += '(';
        first=true;              
        for(const auto& f: fields ) {
          if(!first)
            query +=',';
          first=false;
          query += '$';
          query += to_string(ctr);
          ctr++;
        }
        
        query += ')';
      }
      cout<<"building query: "<<dt.lapUsec()/1000.0<<"ms\n";
      
    //    cout<<query<<endl;
      
      vector<std::optional<string>> allstrings;
      allstrings.reserve(work.size()*fields.size());
      
      for(const auto& m : work) {
        for(const auto& f: fields) {
          if(auto iter = m->values.find(f); iter!= m->values.end()) {
            std::visit([&allstrings](auto&&arg) {
              using T = std::decay_t<decltype(arg)>;
              if constexpr (std::is_same_v<T, string>)
                             allstrings.push_back(arg);
              else 
                allstrings.push_back(to_string(arg));
            }, iter->second);
          }
          else {
            allstrings.push_back(std::optional<string>());
          }
        }
      }
      cout<<endl;
//      cout<<"params 1 took "<<dt.lapUsec()/1000.0<<" msec\n";
      // types: integers, values are variable length, the lengths are integers,
      //                                      array of pointers
      
      vector<const char*> allptrs;
      allptrs.reserve(allstrings.size());
      for(const auto& p : allstrings) {
        if(p)
          allptrs.push_back(p->c_str());
        else
          allptrs.push_back(0);
      }
      
//      cout<<"params2 took "<<dt.lapUsec()/1000.0<<" msec\n";
      mp.exec(query, allptrs);
//      cout<<"exec took "<<dt.lapUsec()/1000.0<<" msec\n";
      for(const auto& m : work)
        delete m;
//      cout<<"cleanup took "<<dt.lapUsec()/1000.0<<" msec\n";
      
      if(prevcommit != time(0)) {
        cout<<"commit"<<endl;
        mp.exec("commit");
        mp.exec("begin");
        prevcommit = time(0);
      }
    }
  }
}
  

