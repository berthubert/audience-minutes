#pragma once
#include <string>
#include <time.h>
#include <stdint.h>
#include "IITree.h"

time_t getTimeFromLog(const std::string& in);

struct CountryDB
{
  explicit CountryDB(const std::string& fname);

  std::string getCountry(const std::string& ip);
  uint32_t getAS(const std::string& ip);
  std::string getASName(const std::string& ip);

  IITree<unsigned __int128, std::string> d_tree;
};

