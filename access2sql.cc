#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include <sstream>
#include <time.h>
// #include "ext/sqlitewriter/sqlwriter.hh"
#include "ext/sqlitewriter/psqlwriter.hh"

/* You can pipe a typical access.log into this, and it will populate sqlite3 database ('access.sqlite3') for you, in streaming fashion.
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

// 19/Mar/2023:00:00:10 +0100
time_t getTime(const string& in)
{
  struct tm tm{};
  strptime(in.c_str(), "%d/%b/%Y:%H:%M:%S %z", &tm);

  // this gets the timezone wrong! XXX
  return mktime(&tm);
}

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

int main(int argc, char** argv)
try
{
  Parser p(stdin);
  // SQLiteWriter sqw(argc > 1 ? argv[1] : "access.sqlite3");
  PSQLWriter sqw(argc > 1 ? argv[1] : "accesslog");
  
  // ::ffff:146.255.56.92 - - [30/Aug/2025:15:12:11 +0200] "GET / HTTP/1.1" 200 6607 "-" "Mastodon/4.4.3-stable+ff1 (http.rb/5.3.1; +https://sloth.es/)" "berthub.eu"

  for(;;) {
    try {
      string ip = p.getWord();
      string ign1 = p.getWord();
      string ign2 = p.getWord();
      if(starts_with(ip, "::ffff:"))
        ip = ip.substr(7);
      string t = p.getDelim('[', ']');
      time_t tim = getTime(t);
      string req = p.getDelim('"', '"');
      int64_t stat = p.getNumber();
      int64_t size = p.getNumber();
      string ref = p.getDelim('"', '"');
      string agent = p.getDelim('"', '"');
      string host = p.getDelim('"', '"');
      cout << "host: "<<host<<endl;
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
                    {"stat", stat}, {"siz", size}, {"host", host}});
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
