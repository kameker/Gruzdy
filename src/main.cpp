#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_system.h>
#include "move_command.h"
#include "command_gate.h"

// Измените оба пароля перед использованием устройства.
constexpr char WIFI_NAME[] = "Gruzdy-ESP32";
constexpr char WIFI_PASSWORD[] = "Gruzdy2026";
constexpr char WEB_USER[] = "admin";
constexpr char WEB_PASSWORD[] = "motor2026";
constexpr uint32_t STEP_PIN = 25;
constexpr uint32_t DIR_PIN = 26;
constexpr uint32_t STEP_INTERVAL_US = 10000;
constexpr uint32_t STEP_PULSE_US = 10;
WebServer server(80);
MotorMove motor;
portMUX_TYPE motorMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t* stepTimer = nullptr;
uint32_t moveStartedAt = 0;
CommandGate gate(0);

extern const char PAGE[] asm("_binary_src_index_html_start");

void IRAM_ATTR onStepTimer() {
  portENTER_CRITICAL_ISR(&motorMux);
  if (motor.busy() && static_cast<uint32_t>(micros() - moveStartedAt) >= STEP_INTERVAL_US) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(STEP_PIN, LOW);
    motor.step();
  }
  portEXIT_CRITICAL_ISR(&motorMux);
}

bool motorBusy() {
  portENTER_CRITICAL(&motorMux);
  bool busy = motor.busy();
  portEXIT_CRITICAL(&motorMux);
  return busy;
}

String commandToken() {
  return String(gate.boot()) + "-" + String(gate.version());
}

bool authorize() {
  if (server.authenticate(WEB_USER, WEB_PASSWORD)) return true;
  server.requestAuthentication();
  return false;
}

void handleMove() {
  if (!authorize()) return;
  if (server.header("Origin") != "http://192.168.4.1") {
    server.send(403, "text/plain; charset=utf-8", "Недопустимый источник запроса");
    return;
  }
  if (!stepTimer) {
    server.send(503, "text/plain; charset=utf-8", "Таймер двигателя недоступен");
    return;
  }
  if (motorBusy()) {
    server.send(409, "text/plain; charset=utf-8", "Двигатель уже вращается");
    return;
  }
  uint32_t steps = 0;
  const String revolutions = server.arg("revolutions");
  const String direction = server.arg("direction");
  if (!parseRevolutions(revolutions.c_str(), steps) || (direction != "0" && direction != "1")) {
    server.send(400, "text/plain; charset=utf-8", "Некорректные параметры");
    return;
  }
  if (server.arg("token") != commandToken()) {
    server.send(409, "text/plain; charset=utf-8", "Команда устарела");
    return;
  }
  gate.accept(gate.boot(), gate.version());
  portENTER_CRITICAL(&motorMux);
  digitalWrite(DIR_PIN, direction == "1" ? HIGH : LOW);
  moveStartedAt = micros();
  motor.start(steps);
  portEXIT_CRITICAL(&motorMux);
  server.send(200, "text/plain; charset=utf-8", "Запущено");
}

void handleStop() {
  if (!authorize()) return;
  if (server.header("Origin") != "http://192.168.4.1") {
    server.send(403, "text/plain; charset=utf-8", "Недопустимый источник запроса");
    return;
  }
  gate.stop();
  portENTER_CRITICAL(&motorMux);
  motor.stop();
  digitalWrite(STEP_PIN, LOW);
  portEXIT_CRITICAL(&motorMux);
  server.send(200, "text/plain; charset=utf-8", "Остановлено");
}

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  delay(100);
  Serial.begin(115200);
  stepTimer = timerBegin(1000000);
  if (stepTimer) {
    timerAttachInterrupt(stepTimer, &onStepTimer);
    timerAlarm(stepTimer, STEP_INTERVAL_US, true, 0);
  } else {
    Serial.println("Motor timer initialization failed");
  }
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(WIFI_NAME, WIFI_PASSWORD)) Serial.println("Wi-Fi AP failed");
  gate = CommandGate(esp_random());
  Serial.print("Open http://"); Serial.println(WiFi.softAPIP());
  const char* headers[] = {"Origin"};
  server.collectHeaders(headers, 1);
  server.on("/", HTTP_GET, []() { if (authorize()) server.send_P(200, "text/html; charset=utf-8", PAGE); });
  server.on("/status", HTTP_GET, []() {
    if (!authorize()) return;
    String state = motorBusy() ? "{\"busy\":true" : "{\"busy\":false";
    state += ",\"ready\":";
    state += stepTimer ? "true" : "false";
    state += ",\"token\":\"" + commandToken() + "\"}";
    server.send(200, "application/json", state);
  });
  server.on("/move", HTTP_POST, handleMove);
  server.on("/stop", HTTP_POST, handleStop);
  server.begin();
}

void loop() {
  server.handleClient();
}
