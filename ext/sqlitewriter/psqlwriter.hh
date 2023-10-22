#include <string>
#include <variant>
#include <vector>
#include <thread>
#include <unordered_map>
#include <unistd.h>

/* ok for a remote database, two things are very different:
   1) We need to stream or batch our inserts, otherwise we get killed by latency
   2) We need isolation from database server failures

   The way to do this is to send all the inserts to a worker thread, which takes
   care of the batching, sending, reconnecting etc.

   There can be multiple tables, which means the batching needs to happen per
   table. There could be multiple insert signatures, which makes batching harder.
   It may be possibe to use NULLs to create unified signatures though.

*/


class PSQLWriter
{

public:
  explicit PSQLWriter(std::string_view fname) 
  {
    pipe2(d_pipe, 0); //O_NONBLOCK);
    d_thread = std::thread(&PSQLWriter::commitThread, this);
  }
  typedef std::variant<double, int32_t, uint32_t, int64_t, std::string> var_t;
  void addValue(const std::initializer_list<std::pair<const char*, var_t>>& values, const std::string& table="data")
  {
    addValueGeneric(table, values);
  }
  
  void addValue(const std::vector<std::pair<const char*, var_t>>& values, const std::string& table="data")
  {
    addValueGeneric(table, values);
  }
  
  template<typename T>
  void addValueGeneric(const std::string& table, const T& values);
  ~PSQLWriter()
  {
    //    std::cerr<<"Destructor called"<<std::endl;
    close(d_pipe[1]); // this is the pleasequit signal
    d_thread.join();
  }

private:
  void commitThread();
  bool d_pleasequit{false};
  std::thread d_thread;

  int d_pipe[2]; // [0] = read, [1] = write
  
  std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> d_columns;
  std::unordered_map<std::string, std::vector<std::string>> d_lastsig;
  bool haveColumn(const std::string& table, std::string_view name);

  struct Message
  {
    std::string table;
    std::unordered_map<std::string, var_t> values;
  };
};


template<typename T>
void PSQLWriter::addValueGeneric(const std::string& table, const T& values)
{
  auto msg = new Message({table});
  for(const auto& v : values) {
    msg->values[v.first] = v.second;
  }
  write(d_pipe[1], &msg, sizeof(msg));
}
