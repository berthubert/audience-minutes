#include "minipsql.hh"
#include "sqlwriter.hh"
#include <algorithm>
using namespace std;


MiniPSQL::MiniPSQL(std::string_view fname)
{
  d_conn = PQconnectdb(&fname[0]);
  if (PQstatus(d_conn) != CONNECTION_OK) {
    throw std::runtime_error("Error connecting to postgresql: "+ string(PQerrorMessage(d_conn)));
  }  
}

MiniPSQL::~MiniPSQL()
{
  if(d_intransaction)
    commit();

  PQfinish(d_conn);
}


struct QueryResult
{
  explicit QueryResult(PGconn* conn, const std::string& query)
  {
    d_res = PQexec(conn, query.c_str());
    if(PQresultStatus(d_res) == PGRES_COMMAND_OK) {
      d_ntuples = d_nfields = 0;
    }
    else if (PQresultStatus(d_res) != PGRES_TUPLES_OK) {
      PQclear(d_res);
      throw std::runtime_error(string("query error: ") + PQerrorMessage(conn));
    }
    d_ntuples = PQntuples(d_res);
    d_nfields = PQnfields(d_res);
  }

  explicit QueryResult(PGconn* conn, const std::string& table, const std::string& query, int paramsize)
  {
    d_res = PQprepare(conn, ("procedure_"+table).c_str(), query.c_str(), paramsize, NULL);

    if(PQresultStatus(d_res) != PGRES_COMMAND_OK) {
      PQclear(d_res);
      throw std::runtime_error(string("prepare error: ") + PQerrorMessage(conn));
    }
    PQclear(d_res);
    d_res=0;
  }

  explicit QueryResult(PGconn* conn, const std::string& query, const std::vector<const char*>& params)
  {
    d_res = PQexecParams(conn, query.c_str(), params.size(), NULL, &params[0], NULL, NULL, 0);
    
    if (PQresultStatus(d_res) == PGRES_COMMAND_OK) {
      d_ntuples = d_nfields = 0;
    }
    else if (PQresultStatus(d_res) != PGRES_TUPLES_OK) {
      PQclear(d_res);
      throw std::runtime_error(string("parameter query error: ") + PQerrorMessage(conn));
    }
    d_ntuples = PQntuples(d_res);
    d_nfields = PQnfields(d_res);
  }
  
  explicit QueryResult(PGconn* conn, const std::string& table, const std::vector<string>& params)
  {
    vector<const char*> pms;
    for(const auto& p : params) {
      //      cout<<"Adding param: '"<<p<<"'\n";
      pms.push_back(p.c_str());
    }
    
    d_res = PQexecPrepared(conn, ("procedure_"+table).c_str(), params.size(), &pms[0], NULL, NULL, 0);
    
    if (PQresultStatus(d_res) == PGRES_COMMAND_OK) {
      d_ntuples = d_nfields = 0;
    }
    else if (PQresultStatus(d_res) != PGRES_TUPLES_OK) {
      PQclear(d_res);
      throw std::runtime_error(string("prepared query error: ") + PQerrorMessage(conn));
    }
    d_ntuples = PQntuples(d_res);
    d_nfields = PQnfields(d_res);
  }

  
  vector<string> getRow()
  {
    vector<string> ret;

    if(d_row  < d_ntuples) {
      for (unsigned int j = 0; j < d_nfields; j++)
        ret.push_back(PQgetvalue(d_res, d_row, j));
    }
    d_row++;
    return ret;
  }
  
  ~QueryResult()
  {
    if(d_res)
      PQclear(d_res);
  }
  PGresult* d_res=0;
  unsigned int d_row=0;
  unsigned int d_ntuples;
  unsigned int d_nfields;
};

std::vector<std::vector<std::string>> MiniPSQL::exec(const std::string& query)
{
  std::vector<std::vector<std::string>> ret;

  QueryResult qr(d_conn, query);
  for(;;) {
    auto row = qr.getRow();
    if(row.empty())
      break;;
    ret.push_back(row);
  }

  return ret;
}

std::vector<std::vector<std::string>> MiniPSQL::exec(const std::string& query, vector<const char*> params)
{
  std::vector<std::vector<std::string>> ret;

  QueryResult qr(d_conn, query, params);
  for(;;) {
    auto row = qr.getRow();
    if(row.empty())
      break;;
    ret.push_back(row);
  }

  return ret;
}


void MiniPSQL::execPrep(const std::string& table)
{
  QueryResult qr(d_conn, table, d_params[table]);
  d_params[table].clear();
}

void MiniPSQL::addColumn(const std::string& table, std::string_view name, std::string_view type)
{
  // SECURITY PROBLEM - somehow we can't do prepared statements here
  
  if(!haveTable(table)) {
    exec("create table if not exists "+table+" ( "+(string)name+" "+(string)type+" )");
  } else {
    //    cout<<"Adding column "<<name<<" to table "<<table<<endl;
    exec("ALTER table \""+table+"\" add column \""+string(name)+ "\" "+string(type));
  }

}

//! Get field names and types from a table
vector<pair<string,string> > MiniPSQL::getSchema(const std::string& table)
{
  vector<pair<string,string>> ret;
  
  auto rows = exec("SELECT column_name, udt_name FROM information_schema.columns where table_name='"+table+"'");

  for(const auto& r : rows) {
    ret.push_back({r[0], r[1]});
  }
  sort(ret.begin(), ret.end(), [](const auto& a, const auto& b) {
    return a.first < b.first;
  });

  //  cout<<"returning "<<ret.size()<<" rows for table "<<table<<"\n";
  return ret;
}

void MiniPSQL::prepare(const std::string& table, std::string_view str, unsigned int paramsize)
{
  cout<<"prep"<<endl;
  if(!d_stmts[table].empty()) {
    cout<<"dealloc!"<<endl;
    exec("deallocate procedure_"+table);
  }

  d_stmts[table]=str;
  d_params[table].clear();
  QueryResult qr(d_conn, (string)table, (string)str, paramsize);
}

void MiniPSQL::begin()
{
  d_intransaction=true;
  exec("begin");
}
void MiniPSQL::commit()
{
  d_intransaction=false;
  exec("commit");
}

void MiniPSQL::cycle()
{
  exec("commit;begin");
}

bool MiniPSQL::haveTable(const string& table)
{
  return !getSchema(table).empty();
}


