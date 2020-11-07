#pragma once

#include <chrono>
using namespace std::chrono;
uint64_t get_now() {
  return duration_cast< milliseconds >(
      system_clock::now().time_since_epoch()
  ).count();
}
