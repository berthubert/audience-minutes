#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include <sstream>
#include <time.h>
#include "ext/sqlitewriter/sqlwriter.hh"

/* You can pipe a typical access.log into this, and it will populate sqlite3 database ('access.sqlite3') for you, in streaming fashion.
   You can safely access that sqlite database while the program runs, see https://berthub.eu/articles/posts/big-data-storage/

   A fun view to create:
 create view botfree as select * from data where agent like 'Mozilla/5.0 %' and agent not like '%bot%' and agent not like '%miniflux%' 

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
  SQLiteWriter sqw(argc > 1 ? argv[1] : "access.sqlite3");
  
  // ::ffff:194.117.254.60 - - [19/Mar/2023:00:00:10 +0100] "GET /articles/posts/nerdfluisteraar/ HTTP/1.1" 200 16627 "-" "Friendica 'Giant Rhubarb' 2023.01-1502; https://friendica.se1.eu"

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
                    {"stat", stat}, {"siz", size}});
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
