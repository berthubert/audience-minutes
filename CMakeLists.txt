cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -g -O3")

project(audience VERSION 1.0
                  DESCRIPTION "Hello deep learning"
                  LANGUAGES CXX)


#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)
                
set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(access2sql access2sql.cc ext/sqlitewriter/psqlwriter.cc ext/sqlitewriter/minipsql.cc)
target_link_libraries(access2sql sqlite3 pq Threads::Threads)
