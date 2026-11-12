/*
 * Wash & Cure WiFi bridge - ESP8266 NodeMCU.
 *
 * Unit console: UART0 swapped to D8 (GPIO15, TX -> unit pad "TX"/PE5) and
 *               D7 (GPIO13, RX <- unit pad "RX"/PE7), 115200 8N1.
 * Network:
 *   http://washcure.local/          web UI
 *   GET  /api/status                JSON (from the unit's STAT line, polled every second)
 *   GET  /api/cmd?c=<command>       run one console command, returns its reply text
 *   POST /api/mcu/update            upload a unit firmware .bin -> flashed over UART (FL commands)
 *   /update                         ESP firmware update (HTTP), ArduinoOTA also enabled
 *   TCP port 23                     raw console bridge (one client)
 *
 * Routing: commands from HTTP/internal polling are run synchronously (one at a time) and their
 * reply lines (ending in OK/ERR/STAT/CFG, or 400 ms of silence) go back to the caller. Everything
 * else from the unit (EVT lines, boot banner, replies to TCP-client commands) is forwarded to the
 * TCP client; EVT lines are also kept in a small ring for the web UI.
 */
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include "secrets.h"
#include "web.h"

ADC_MODE(ADC_VCC);                                // A0 unused: measure the ESP's own supply (brownout debugging)

#define HOSTNAME   "washcure"
#define LED_PIN    LED_BUILTIN                     // GPIO2, blue LED on the ESP-12 module, active low

static void led(bool on) { digitalWrite(LED_PIN, on ? LOW : HIGH); }
enum LedMode { LED_SOLID, LED_SLOW_BLINK };
static LedMode ledMode = LED_SOLID;
#define UNIT_BAUD  115200

ESP8266WebServer http(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiServer tcpServer(23);
WiFiClient tcp;

/* ---------------- unit line handling ---------------- */
static String rxLine;
static bool   syncActive = false;
static String syncReply;
static bool   syncDone = false;
static unsigned long syncLastRx = 0;

static String lastStat;              // last STAT line
static unsigned long lastStatMs = 0;
static String events[24];            // EVT ring for the web UI
static uint8_t evHead = 0, evCount = 0;
static uint32_t evSeq = 0;           // sequence number of newest event

static bool isTerminal(const String &l) {
  return l.startsWith("OK") || l.startsWith("ERR") || l.startsWith("STAT") || l.startsWith("CFG");
}

static void handleUnitLine(const String &l) {
  if (l.startsWith("STAT")) { lastStat = l; lastStatMs = millis(); }
  if (l.startsWith("EVT")) {
    events[evHead] = l; evHead = (evHead + 1) % 24; if (evCount < 24) evCount++; evSeq++;
    if (tcp && tcp.connected()) { tcp.print(l); tcp.print("\r\n"); }
    return;
  }
  if (syncActive) {
    syncReply += l; syncReply += "\n"; syncLastRx = millis();
    if (isTerminal(l)) syncDone = true;
    return;
  }
  if (tcp && tcp.connected()) { tcp.print(l); tcp.print("\r\n"); }
}

static void pumpUnit() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') { if (rxLine.length()) handleUnitLine(rxLine); rxLine = ""; }
    else if (rxLine.length() < 600) rxLine += c;
  }
}

/* Run one command synchronously; returns reply text ("" on timeout). */
static String unitCmd(const String &cmd, unsigned long timeoutMs = 1500) {
  pumpUnit();                                     // flush anything pending first
  syncActive = true; syncDone = false; syncReply = ""; syncLastRx = 0;
  Serial.print(cmd); Serial.print("\r\n");
  unsigned long t0 = millis();
  while (millis() - t0 < timeoutMs) {
    pumpUnit();
    if (syncDone) break;
    if (syncLastRx && millis() - syncLastRx > 400) break;   // multi-line replies without OK (HELP)
    delay(1);
  }
  syncActive = false;
  return syncReply;
}

/* ---------------- TCP raw bridge ---------------- */
static void pumpTcp() {
  if (tcpServer.hasClient()) {
    if (tcp && tcp.connected()) tcpServer.accept().stop();   // one client at a time
    else { tcp = tcpServer.accept(); tcp.setNoDelay(true); tcp.print("# washcure bridge connected\r\n"); }
  }
  if (tcp && tcp.connected() && !syncActive) {
    while (tcp.available()) Serial.write(tcp.read());
  }
}

/* ---------------- status JSON ---------------- */
static void jsonEscape(String &out, const String &s) {
  for (char c : s) { if (c == '"' || c == '\\') out += '\\'; if ((uint8_t)c >= 0x20) out += c; }
}
static void handleStatus() {
  String j = "{";
  bool ok = lastStat.length() && millis() - lastStatMs < 3500;
  j += "\"unit_ok\":"; j += ok ? "true" : "false";
  // STAT k=v k=v ...
  int i = lastStat.indexOf(' ');
  while (i >= 0 && i < (int)lastStat.length()) {
    int sp = lastStat.indexOf(' ', i + 1);
    String kv = lastStat.substring(i + 1, sp < 0 ? lastStat.length() : sp);
    int eq = kv.indexOf('=');
    if (eq > 0) { j += ",\""; jsonEscape(j, kv.substring(0, eq)); j += "\":\""; jsonEscape(j, kv.substring(eq + 1)); j += "\""; }
    i = sp;
  }
  j += ",\"rssi\":"; j += WiFi.RSSI();
  j += ",\"esp_uptime\":"; j += millis() / 1000;
  j += ",\"esp_vcc_mv\":"; j += ESP.getVcc();
  j += ",\"esp_reset\":\""; jsonEscape(j, ESP.getResetReason()); j += "\"";
  // events newer than ?since=
  uint32_t since = http.hasArg("since") ? http.arg("since").toInt() : evSeq;
  j += ",\"seq\":"; j += evSeq;
  j += ",\"events\":[";
  uint32_t n = evSeq - since; if (n > evCount) n = evCount;
  for (uint32_t k = 0; k < n; k++) {
    uint8_t idx = (evHead + 24 - n + k) % 24;
    if (k) j += ",";
    j += "\""; jsonEscape(j, events[idx]); j += "\"";
  }
  j += "]}";
  http.send(200, "application/json", j);
}

/* ---------------- unit firmware update over UART ---------------- */
static uint32_t crc32buf(uint32_t crc, const uint8_t *p, size_t n) {
  crc = ~crc;
  while (n--) { crc ^= *p++; for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & -(int32_t)(crc & 1)); }
  return ~crc;
}
static File upFile;
static String flashLog;
static bool expectOk(const String &cmd, unsigned long to = 2000) {
  String r = unitCmd(cmd, to);
  bool ok = r.indexOf("OK") >= 0 && r.indexOf("ERR") < 0;
  if (!ok) { flashLog += "FAIL: " + cmd.substring(0, 40) + " -> " + r + "\n"; }
  return ok;
}
static bool readAt(File &f, uint32_t off, uint8_t *buf, uint32_t n, size_t size) {
  memset(buf, 0xFF, n);
  if (off >= size) return true;
  f.seek(off);
  uint32_t avail = min((uint32_t)(size - off), n);
  return f.read(buf, avail) == avail;
}
static bool flashUnit(const char *path) {
  flashLog = "";
  File f = LittleFS.open(path, "r");
  if (!f) { flashLog = "no file\n"; return false; }
  size_t size = f.size();
  if (size < 0x100 || size > 0x8000) { flashLog = "bad size " + String(size) + "\n"; f.close(); return false; }
  uint32_t padded = (size + 3) & ~3u;
  uint8_t buf[64];
  readAt(f, 0x1C, buf, 16, size);
  uint32_t w[4]; memcpy(w, buf, 16);              // prot, key, len, memcfg
  if (w[0] != 0) { flashLog = "REFUSED: protection word at 0x1C is not 0\n"; f.close(); return false; }
  if (w[1] != 0x0B11FFACu || w[3] != 2 || w[2] < padded || w[2] > 0x8000 || (w[2] & 0xFFF)) {
    flashLog = "REFUSED: bad boot header\n"; f.close(); return false;
  }
  uint32_t crc = 0;                               // streaming CRC of the padded image
  for (uint32_t off = 0; off < padded; off += 64) {
    uint32_t n = min((uint32_t)64, padded - off);
    readAt(f, off, buf, n, size); crc = crc32buf(crc, buf, n);
  }
  bool ok = false;
  do {
    if (!expectOk("VER")) break;
    unsigned long unlockedAt = millis();
    if (!expectOk("FL UNLOCK")) break;
    bool fail = false;
    for (uint32_t off = 0; off < w[2] && !fail; off += 0x1000)
      if (!expectOk("FL ERASE " + String(off, HEX), 3000)) fail = true;
    if (fail) break;
    char hex[64 * 2 + 1];
    auto writeChunk = [&](uint32_t off) -> bool {
      uint32_t n = min((uint32_t)64, padded - off);
      if (!readAt(f, off, buf, n, size)) { flashLog += "file read error\n"; return false; }
      bool blank = true; for (uint32_t k = 0; k < n; k++) if (buf[k] != 0xFF) { blank = false; break; }
      if (blank) return true;
      for (uint32_t k = 0; k < n; k++) sprintf(hex + 2 * k, "%02X", buf[k]);
      if (millis() - unlockedAt > 20000) { if (!expectOk("FL UNLOCK")) return false; unlockedAt = millis(); }
      return expectOk("FL WRITE " + String(off, HEX) + " " + String(hex));
    };
    for (uint32_t off = 64; off < padded && !fail; off += 64) { if (!writeChunk(off)) fail = true; yield(); }
    if (fail || !writeChunk(0)) break;            // header block last
    String r = unitCmd("FL CRC 0 " + String(padded, HEX), 5000);
    char want[9]; sprintf(want, "%08X", crc);
    if (r.indexOf(want) < 0) { flashLog += "CRC mismatch: unit " + r + " want " + want + "\n"; break; }
    flashLog += "written " + String(padded) + " bytes, CRC " + want + " verified, rebooting unit\n";
    unitCmd("REBOOT", 500);
    ok = true;
  } while (0);
  f.close();
  return ok;
}
static void handleMcuUpload() {
  HTTPUpload &u = http.upload();
  if (u.status == UPLOAD_FILE_START) upFile = LittleFS.open("/mcu.bin", "w");
  else if (u.status == UPLOAD_FILE_WRITE && upFile) upFile.write(u.buf, u.currentSize);
  else if (u.status == UPLOAD_FILE_END && upFile) upFile.close();
}
static void handleMcuDone() {
  bool ok = flashUnit("/mcu.bin");
  http.send(ok ? 200 : 500, "text/plain", (ok ? "OK\n" : "FAILED\n") + flashLog);
}

/* ---------------- setup / loop ---------------- */
static unsigned long lastPoll = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  led(false);

  // Boot log on the USB serial (GPIO1/3) until WiFi is up; then UART0 is swapped to the unit.
  Serial.setRxBufferSize(2048);
  Serial.begin(UNIT_BAUD);
  Serial.printf("\r\n\r\n[washcure] boot, reset reason: %s, vcc %u mV\r\n",
                ESP.getResetReason().c_str(), ESP.getVcc());

  // 5 slow blinks, then 1 s dark
  for (int i = 0; i < 5; i++) { led(true); delay(400); led(false); delay(400); }
  delay(1000);

  LittleFS.begin();

  // WiFi: rapid blink while connecting
  Serial.printf("[washcure] connecting to WiFi '%s'\r\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t0 = millis();
  bool on = false;
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 30000) {
    on = !on; led(on); delay(80);
    if ((millis() - t0) % 2000 < 80)
      Serial.printf("[washcure]  status %d, vcc %u mV\r\n", WiFi.status(), ESP.getVcc());
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[washcure] connected, IP %s, RSSI %d\r\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    ledMode = LED_SOLID;                           // connected: LED stays on
  } else {                                         // fallback: own access point
    Serial.println("[washcure] WiFi failed, starting AP 'WashCure-Setup'");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("WashCure-Setup");
    ledMode = LED_SLOW_BLINK;                      // AP fallback: slow blink forever
  }
  led(true);
  Serial.println("[washcure] switching UART to the unit (D8/D7)");
  Serial.flush();
  Serial.swap();                                   // UART0 -> GPIO15 (TX) / GPIO13 (RX)

  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("telnet", "tcp", 23);

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.begin();
  httpUpdater.setup(&http, "/update");

  http.on("/", []() { http.send_P(200, "text/html", INDEX_HTML); });
  http.on("/api/status", handleStatus);
  http.on("/api/cmd", []() {
    if (!http.hasArg("c")) { http.send(400, "text/plain", "missing c"); return; }
    String r = unitCmd(http.arg("c"));
    http.send(200, "text/plain", r.length() ? r : String("TIMEOUT (no reply from unit)\n"));
  });
  http.on("/api/mcu/update", HTTP_POST, handleMcuDone, handleMcuUpload);
  http.onNotFound([]() { http.send(404, "text/plain", "not found"); });
  http.begin();
  tcpServer.begin();
  tcpServer.setNoDelay(true);
}

void loop() {
  pumpUnit();
  pumpTcp();
  http.handleClient();
  MDNS.update();
  ArduinoOTA.handle();
  if (ledMode == LED_SLOW_BLINK) led((millis() / 1000) & 1);
  if (millis() - lastPoll > 1000) { lastPoll = millis(); unitCmd("STATUS", 400); }
}
