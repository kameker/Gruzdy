#include <cassert>
#include "command_gate.h"

int main() {
  CommandGate gate(123);
  assert(gate.accept(123, 0));
  assert(!gate.accept(123, 0));  // Повтор не запускает движение.
  assert(!gate.accept(124, 1));  // Команда от другого запуска контроллера.
  gate.stop();
  assert(!gate.accept(123, 1));  // Запоздавший запуск после остановки.
  assert(gate.accept(123, 2));
}
