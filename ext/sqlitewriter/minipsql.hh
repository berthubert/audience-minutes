#include <postgresql/libpq-fe.h>
#include <vector>
#include <string>
#include <unordered_map>

class MiniPSQL
{
public:
  MiniPSQL(std::string_view fname);
  ~MiniPSQL();
  std::vector<std::pair<std::string, std::string>> getSchema(const std::string& table);
  void addColumn(const std::string& table, std::string_view name, std::string_view type);
  
  //!< execute a random query, for example a PRAGMA
  std::vector<std::vector<std::string>> exec(const std::string& query);
  //  std::vector<std::vector<std::string>> exec(const std::string& query, const std::vector<std::string>& params);
  std::vector<std::vector<std::string>> exec(const std::string& query, std::vector<const char*> params);

  //!< set the prepared statement for a table, question marks as placeholder
  void prepare(const std::string& table, std::string_view str, unsigned int paramsize);
  // offset from 1!!
  template<typename T>
  void bindPrep(const std::string& table, int idx, const T& value)
  {
    bindPrep(table, idx, std::to_string(value));
  }

  void bindPrep(const std::string& table, int idx, const std::string& value)
  {
    int pos = idx-1;
    if(d_params[table].size() <= pos)
      d_params[table].resize(pos+1);
    d_params[table][pos]=value;
  }
  
  //!< execute the prepared & bound statement
  void execPrep(const std::string& table);
  
  void begin();
  void commit();
  void cycle();
  
  //!< do we have a prepared statement for this table
  bool isPrepared(const std::string& table) const
  {
    return d_stmts.find(table) != d_stmts.end();
  }

private:
  PGconn* d_conn;

  std::unordered_map<std::string, std::string> d_stmts; // keyed on table name
  std::unordered_map<std::string, std::vector<std::string>> d_params; // keyed on table name

  bool d_intransaction{false};
  bool haveTable(const std::string& table);
};
