#pragma once
#include <stdint.h>

class MotorMove {
 public:
  bool start(uint32_t steps) {
    if (remaining_ || !steps) return false;
    remaining_ = steps;
    return true;
  }
  void stop() { remaining_ = 0; }
  bool busy() const { return remaining_ != 0; }
  void step() { if (remaining_) --remaining_; }

 private:
  uint32_t remaining_ = 0;
};

inline bool parseRevolutions(const char* input, uint32_t& steps) {
  if (!input || !*input) return false;
  uint32_t revolutions = 0;
  for (const char* p = input; *p; ++p) {
    if (*p < '0' || *p > '9') return false;
    revolutions = revolutions * 10 + static_cast<uint32_t>(*p - '0');
    if (revolutions > 1000) return false;
  }
  if (revolutions == 0) return false;
  steps = revolutions * 48;
  return true;
}
