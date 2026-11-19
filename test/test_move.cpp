#include <cassert>
#include "move_command.h"

int main() {
  uint32_t steps = 0;
  uint32_t interval = 0;
  assert(parseSpeed("125", interval) && interval == 10000);
  assert(parseSpeed("25", interval) && interval == 50000);
  assert(parseSpeed("250", interval) && interval == 5000);
  assert(!parseSpeed("24", interval));
  assert(!parseSpeed("251", interval));
  assert(!parseSpeed("125.0", interval));
  assert(!parseSpeed("", interval));
  assert(parseRevolutions("1", steps) && steps == 48);
  assert(parseRevolutions("5", steps) && steps == 240);
  assert(parseRevolutions("1000", steps) && steps == 48000);
  assert(!parseRevolutions("0", steps));
  assert(!parseRevolutions("1001", steps));
  assert(!parseRevolutions("-1", steps));
  assert(!parseRevolutions("1.5", steps));
  assert(!parseRevolutions("5abc", steps));
  assert(!parseRevolutions("", steps));

  MotorMove motor;
  assert(motor.start(240));
  motor.step();
  assert(motor.busy());
  motor.stop();
  assert(!motor.busy());
  motor.step();
  assert(!motor.busy());
  assert(motor.start(48));
  for (int i = 0; i < 48; ++i) motor.step();
  assert(!motor.busy());
}
