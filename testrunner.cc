#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <algorithm> // std::move() and friends
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h> //unlink(), usleep()
#include <unordered_map>
#include "doctest.h"
#include <chrono>
#include <fmt/chrono.h>
#include <fmt/printf.h>
#include "support.hh"

using namespace std;

TEST_CASE("time") {
  CHECK(getTimeFromLog("26/Apr/2026:13:11:30 +0200") == 1777201890);
  CHECK(getTimeFromLog("26/Apr/2026:11:11:30 +0000") == 1777201890);
  CHECK(getTimeFromLog("26/Apr/2026:09:11:30 -0200") == 1777201890);
  CHECK(getTimeFromLog("26/Apr/2026:08:41:30 -0230") == 1777201890);
}


TEST_CASE("country") {
  CountryDB cdb("dbip-country-lite.csv");
  
  CHECK(cdb.getCountry("86.82.68.237") == "NL");
  CHECK(cdb.getCountry("217.100.190.174") == "NL");
  CHECK(cdb.getCountry("2001:41f0:782d::2") == "NL");

}

TEST_CASE("asn") {
  CountryDB cdb("dbip-asn-lite.csv");
  
  CHECK(cdb.getCountry("86.82.68.237") == R"(1136,"KPN B.V.")");
  CHECK(cdb.getCountry("217.100.190.174") == R"(33915,"Vodafone Libertel B.V.")");
  CHECK(cdb.getCountry("2001:41f0:782d::2") == R"(33915,"Vodafone Libertel B.V.")");
  CHECK(cdb.getCountry("2a02:a440:b085:1:20d:b9ff:fe58:11f0") == R"(1136,"KPN B.V.")");

  CHECK(cdb.getASName("2a02:a440:b085:1:20d:b9ff:fe58:11f0") == "KPN B.V.");
  CHECK(cdb.getAS("2a02:a440:b085:1:20d:b9ff:fe58:11f0") == 1136);


  CHECK(cdb.getAS("240e:838:10::1") == 4134);
  CHECK(cdb.getASName("240e:838:10::1") == "Chinanet");
}

