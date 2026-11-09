#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "move_command.h"

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
uint32_t remainingSteps = 0;
uint32_t lastStep = 0;

const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="ru"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Управление двигателем</title><style>
body{font:16px system-ui,sans-serif;background:#f2f5f9;color:#182335;margin:0;padding:24px}
main{max-width:420px;margin:10vh auto;background:white;border-radius:16px;padding:24px;box-shadow:0 6px 30px #0001}
h1{font-size:24px}label{display:block;margin:22px 0 8px}.row{display:flex;gap:8px}
input{width:100%;min-width:0;box-sizing:border-box;font:inherit;padding:12px;border:1px solid #bac5d3;border-radius:8px}
button{font:inherit;cursor:pointer;padding:12px 18px;border:0;border-radius:8px;background:#e7edf5;color:#182335}
button:disabled{opacity:.5;cursor:default}.wide{width:100%;margin-top:12px}.start{background:#225ed6;color:white;font-weight:600}
#status{min-height:1.5em;color:#475569}</style></head><body><main>
<h1>Управление двигателем</h1><label for="revs">Количество оборотов (1–1000)</label>
<div class="row"><input id="revs" type="number" min="1" max="1000" step="1" value="5"><button id="minus" type="button" aria-label="Уменьшить">−</button><button id="plus" type="button" aria-label="Увеличить">+</button></div>
<button id="direction" class="wide" type="button">Направление: по часовой</button>
<button id="start" class="wide start" type="button">Вращать</button><p id="status" role="status"></p>
<script>
let clockwise=true, busy=false; const input=document.getElementById('revs'), status=document.getElementById('status');
const start=document.getElementById('start'), direction=document.getElementById('direction');
function adjust(n){let v=Number(input.value); if(!Number.isInteger(v))v=1; input.value=Math.max(1,Math.min(1000,v+n));}
document.getElementById('minus').onclick=()=>adjust(-1);document.getElementById('plus').onclick=()=>adjust(1);
direction.onclick=()=>{clockwise=!clockwise;direction.textContent='Направление: '+(clockwise?'по часовой':'против часовой');};
start.onclick=async()=>{if(busy)return;let v=Number(input.value);if(!Number.isInteger(v)||v<1||v>1000){status.textContent='Введите целое число от 1 до 1000.';return;}
busy=true;start.disabled=true;try{let r=await fetch('/move',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({revolutions:String(v),direction:clockwise?'1':'0'})});if(!r.ok)throw new Error(await r.text());status.textContent='Двигатель вращается…';}catch(e){status.textContent='Не удалось запустить: '+e.message;busy=false;start.disabled=false;}};
async function update(){try{let r=await fetch('/status',{cache:'no-store'});if(!r.ok)throw new Error('Ошибка соединения');let s=await r.json();busy=s.busy;start.disabled=busy;if(!busy && status.textContent==='Двигатель вращается…')status.textContent='Вращение завершено.';}catch(e){start.disabled=true;status.textContent='Нет связи с устройством.';}setTimeout(update,1000);}update();
</script></main></body></html>)HTML";

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
  if (remainingSteps != 0) {
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
  digitalWrite(DIR_PIN, direction == "1" ? HIGH : LOW);
  lastStep = micros();
  remainingSteps = steps;
  server.send(200, "text/plain; charset=utf-8", "Запущено");
}

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  delay(100);
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(WIFI_NAME, WIFI_PASSWORD)) Serial.println("Wi-Fi AP failed");
  Serial.print("Open http://"); Serial.println(WiFi.softAPIP());
  const char* headers[] = {"Origin"};
  server.collectHeaders(headers, 1);
  server.on("/", HTTP_GET, []() { if (authorize()) server.send_P(200, "text/html; charset=utf-8", PAGE); });
  server.on("/status", HTTP_GET, []() { if (authorize()) server.send(200, "application/json", remainingSteps ? "{\"busy\":true}" : "{\"busy\":false}"); });
  server.on("/move", HTTP_POST, handleMove);
  server.begin();
}

void loop() {
  server.handleClient();
  uint32_t now = micros();
  if (remainingSteps && static_cast<uint32_t>(now - lastStep) >= STEP_INTERVAL_US) {
    lastStep = now;
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(STEP_PIN, LOW);
    --remainingSteps;
  }
}
