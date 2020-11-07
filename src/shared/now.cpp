#pragma once

#include <chrono>

namespace swtfb {
using namespace std;
using namespace std::chrono;
uint64_t get_now() {
  return duration_cast< milliseconds >(
      system_clock::now().time_since_epoch()
  ).count();
}

class ClockWatch {
public:
  chrono::high_resolution_clock::time_point t1;

  ClockWatch() { t1 = chrono::high_resolution_clock::now(); }

  auto elapsed() {
    auto t2 = chrono::high_resolution_clock::now();
    chrono::duration<double> time_span =
        chrono::duration_cast<chrono::duration<double>>(t2 - t1);
    return time_span.count();
  }
};
};
