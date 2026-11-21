// Desk critter: OLED eyes that react to pokes, puffs and messages from Claude.
// Board: any ESP32-S3 with a 128x64 SSD1306 OLED on the default I2C pins.
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "eyes.h"

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Eyes eyes(oled);

const bool online = strlen(WIFI_SSID) > 0;
const int CAPTION_COLS = 21;             // characters per line at text size 1
const unsigned long PAGE_MS = 2500;      // how long each two-line page stays up
const unsigned long MOOD_HOLD_MS = 8000; // drift back to neutral after this

String caption;                          // text still to be shown
unsigned long pageUntil = 0, moodUntil = 0, nextPoll = 0;
String line1, line2;

// ---------- caption: show text two lines at a time ----------

// Take up to one line of whole words off the front of `text`.
String takeLine(String &text) {
  text.trim();
  if ((int)text.length() <= CAPTION_COLS) { String all = text; text = ""; return all; }
  int cut = text.lastIndexOf(' ', CAPTION_COLS);
  if (cut <= 0) cut = CAPTION_COLS;      // one very long word: split it
  String line = text.substring(0, cut);
  text = text.substring(cut);
  return line;
}

void say(const String &text, Mood mood) {
  caption = text;
  line1 = line2 = "";
  pageUntil = 0;
  eyes.setMood(mood);
  moodUntil = millis() + MOOD_HOLD_MS;
}

void updateCaption() {
  if (millis() < pageUntil) return;
  if (caption.length() == 0) { line1 = line2 = ""; return; }
  line1 = takeLine(caption);
  line2 = takeLine(caption);
  pageUntil = millis() + PAGE_MS;
  moodUntil = max(moodUntil, pageUntil);
}

void render() {
  oled.clearDisplay();
  bool talking = line1.length() > 0;
  eyes.draw(talking ? 22 : 32);
  if (talking) {
    oled.setCursor(0, 46); oled.print(line1);
    oled.setCursor(0, 56); oled.print(line2);
  }
  oled.display();
}

// ---------- talking to the relay ----------

String relayUrl(const char *path) {
  return String("http://") + RELAY_HOST + ":" + RELAY_PORT + path;
}

// Read a {"say": "...", "mood": "..."} reply and act on it.
void handleReply(HTTPClient &http, int status) {
  if (status != 200) return;
  JsonDocument doc;
  if (deserializeJson(doc, http.getString())) return;
  say(doc["say"] | "", moodFromName(doc["mood"] | "neutral"));
}

void sendEvent(const char *type) {
  if (!online || WiFi.status() != WL_CONNECTED) return;
  eyes.setMood(THINKING);
  moodUntil = millis() + 30000;
  line1 = line2 = caption = "";
  render();                               // show the thinking face while we wait
  HTTPClient http;
  http.setTimeout(30000);
  http.begin(relayUrl("/event"));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Critter-Token", RELAY_TOKEN);
  int status = http.POST(String("{\"type\":\"") + type + "\"}");
  if (status == 200) handleReply(http, status);
  else say(status < 0 ? "I can't reach my brain." : "My brain said no.", SAD);
  http.end();
}

void pollRelay() {
  if (!online || millis() < nextPoll || WiFi.status() != WL_CONNECTED) return;
  nextPoll = millis() + 1500;
  HTTPClient http;
  http.setTimeout(2000);
  http.begin(relayUrl("/poll"));
  http.addHeader("X-Critter-Token", RELAY_TOKEN);
  handleReply(http, http.GET());
  http.end();
}

// ---------- senses ----------

void onPoke() {
  if (online) { sendEvent("poke"); return; }
  Mood next = (Mood)((eyes.getMood() + 1) % MOOD_COUNT);   // offline: tour the faces
  say(MOOD_NAMES[next], next);
}

void checkButton() {
  static bool wasDown = false;
  static unsigned long lastChange = 0;
  bool down = digitalRead(BUTTON_PIN) == LOW;
  if (down != wasDown && millis() - lastChange > 40) {     // 40 ms debounce
    lastChange = millis();
    wasDown = down;
    if (down) onPoke();
  }
}

void checkPuff() {
  if (PUFF_PIN < 0) return;
  static float baseline = -1;
  static unsigned long quietUntil = 0, nextPrint = 0;
  int v = analogRead(PUFF_PIN);
  if (baseline < 0) baseline = v;
  if (PUFF_DEBUG && millis() > nextPrint) {
    Serial.printf("puff raw=%d baseline=%d\n", v, (int)baseline);
    nextPrint = millis() + 200;
  }
  if (millis() > quietUntil && abs(v - baseline) > PUFF_THRESHOLD) {
    quietUntil = millis() + 3000;
    if (online) sendEvent("puff");
    else say("That tickles!", SURPRISED);
  }
  baseline += (v - baseline) * 0.01;      // follow slow drift, ignore sudden jumps
}

// ---------- setup and main loop ----------

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#ifdef PIN_I2C_POWER                       // some Adafruit boards switch I2C power
  pinMode(PIN_I2C_POWER, OUTPUT);
  digitalWrite(PIN_I2C_POWER, HIGH);
  delay(10);
#endif
  Wire.begin();
  // OLEDs ship at one of two addresses; try both.
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C) && !oled.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    Serial.println("No OLED found. Check the four display wires.");
  }
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextWrap(false);
  randomSeed(esp_random());

  if (!online) { say("Hello! Poke me.", HAPPY); return; }

  eyes.setMood(SLEEPY);
  line1 = "Joining Wi-Fi...";
  render();
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long giveUp = millis() + 20000;
  while (WiFi.status() != WL_CONNECTED && millis() < giveUp) { delay(100); render(); }
  if (WiFi.status() == WL_CONNECTED) sendEvent("boot");
  else say("No Wi-Fi. Check config.h", SAD);
}

void loop() {
  checkButton();
  checkPuff();
  pollRelay();
  updateCaption();
  if (millis() > moodUntil && eyes.getMood() != NEUTRAL) eyes.setMood(NEUTRAL);
  render();
  delay(20);
}
