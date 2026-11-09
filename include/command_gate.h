#pragma once
#include <stdint.h>

// A command is valid only for the controller boot and state it observed.
class CommandGate {
 public:
  explicit CommandGate(uint32_t boot) : boot_(boot) {}
  uint32_t boot() const { return boot_; }
  uint32_t version() const { return version_; }
  bool matches(uint32_t boot, uint32_t version) const {
    return boot == boot_ && version == version_;
  }
  bool accept(uint32_t boot, uint32_t version) {
    if (!matches(boot, version)) return false;
    ++version_;
    return true;
  }
  void stop() { ++version_; }

 private:
  uint32_t boot_;
  uint32_t version_ = 0;
};
