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

