#include <Arduino.h>

// Выводы ESP32 Dev Module; при необходимости измените под свое подключение.
constexpr uint32_t STEP_PIN = 25;
constexpr uint32_t DIR_PIN = 26;

constexpr uint32_t REVOLUTIONS = 5;          // N оборотов в каждом направлении
constexpr uint32_t FULL_STEPS_PER_REV = 48;   // M49SP-2K: 7,5 градуса на шаг
constexpr uint32_t MICROSTEPS = 1;            // MODE0, MODE1, MODE2 на GND
constexpr uint32_t STEP_INTERVAL_US = 10000; // 100 полных шагов/с
constexpr uint32_t STEP_PULSE_US = 10;
constexpr uint32_t PAUSE_MS = 1000;

static_assert(REVOLUTIONS > 0, "REVOLUTIONS must be positive");
static_assert(MICROSTEPS == 1, "Check mode pins and update this check if changing microstepping");
static_assert(STEP_INTERVAL_US > STEP_PULSE_US + 2, "STEP interval is too short");
static_assert(REVOLUTIONS <= UINT32_MAX / FULL_STEPS_PER_REV / MICROSTEPS,
              "Step count overflows uint32_t");

void moveMotor(bool direction) {
  digitalWrite(DIR_PIN, direction ? HIGH : LOW);
  delayMicroseconds(10); // DIR must be stable before the first STEP edge.

  const uint32_t steps = REVOLUTIONS * FULL_STEPS_PER_REV * MICROSTEPS;
  for (uint32_t i = 0; i < steps; ++i) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_INTERVAL_US - STEP_PULSE_US);
  }
}

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  delay(100); // Allow the externally powered DRV8825 to wake up.
}

void loop() {
  moveMotor(true);
  delay(PAUSE_MS);
  moveMotor(false);
  delay(PAUSE_MS);
}
