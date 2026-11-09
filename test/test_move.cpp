#include <cassert>
#include "move_command.h"

int main() {
  uint32_t steps = 0;
  assert(parseRevolutions("1", steps) && steps == 48);
  assert(parseRevolutions("5", steps) && steps == 240);
  assert(parseRevolutions("1000", steps) && steps == 48000);
  assert(!parseRevolutions("0", steps));
  assert(!parseRevolutions("1001", steps));
  assert(!parseRevolutions("-1", steps));
  assert(!parseRevolutions("1.5", steps));
  assert(!parseRevolutions("5abc", steps));
  assert(!parseRevolutions("", steps));
}
