// Draws the critter's face on a 128x64 OLED.
#pragma once
#include <Adafruit_SSD1306.h>

enum Mood { NEUTRAL, HAPPY, SAD, ANGRY, SURPRISED, SLEEPY, CURIOUS, LOVE, THINKING, MOOD_COUNT };

static const char *const MOOD_NAMES[MOOD_COUNT] = {
  "neutral", "happy", "sad", "angry", "surprised", "sleepy", "curious", "love", "thinking"};

inline Mood moodFromName(const char *name) {
  for (int i = 0; i < MOOD_COUNT; i++)
    if (strcmp(name, MOOD_NAMES[i]) == 0) return (Mood)i;
  return NEUTRAL;
}

class Eyes {
 public:
  explicit Eyes(Adafruit_SSD1306 &d) : oled(d) {}

  void setMood(Mood m) { mood = m; }
  Mood getMood() const { return mood; }

  // Call often. centerY is where the eyes sit (higher when a caption is shown).
  void draw(int centerY) {
    unsigned long now = millis();
    if (now > nextBlink) {            // blink: shut for 120 ms every few seconds
      blinkUntil = now + 120;
      nextBlink = now + random(2000, 6000);
    }
    if (now > nextGlance) {           // glance somewhere new now and then
      lookX = random(-6, 7);
      lookY = random(-3, 4);
      nextGlance = now + random(1500, 4000);
    }
    bool shut = now < blinkUntil && mood != LOVE;
    int dx = lookX, dy = lookY;
    if (mood == THINKING) { dx = 8; dy = -6; }
    if (mood == SLEEPY) { dx = 0; dy = 3; }
    drawEye(40 + dx, centerY + dy, true, shut);
    drawEye(88 + dx, centerY + dy, false, shut);
  }

 private:
  Adafruit_SSD1306 &oled;
  Mood mood = NEUTRAL;
  unsigned long nextBlink = 2000, blinkUntil = 0, nextGlance = 0;
  int lookX = 0, lookY = 0;

  void drawEye(int cx, int cy, bool left, bool shut) {
    int w = 28, h = 30;
    if (mood == SURPRISED) { w = 32; h = 36; }
    if (mood == SLEEPY) h = 8;
    if (mood == THINKING) { w = 22; h = 22; }
    if (mood == CURIOUS) h = left ? 22 : 34;   // one eyebrow up
    if (shut) h = 3;

    int x = cx - w / 2, y = cy - h / 2;
    if (mood == LOVE) {                        // hearts
      oled.fillCircle(cx - 7, cy - 5, 8, SSD1306_WHITE);
      oled.fillCircle(cx + 7, cy - 5, 8, SSD1306_WHITE);
      oled.fillTriangle(cx - 15, cy - 2, cx + 15, cy - 2, cx, cy + 15, SSD1306_WHITE);
      return;
    }
    oled.fillRoundRect(x, y, w, h, min(8, h / 2), SSD1306_WHITE);
    if (shut) return;

    int inner = left ? x + w : x;              // the edge nearest the nose
    int outer = left ? x : x + w;
    if (mood == HAPPY)                         // cut the bottom away into an arch
      oled.fillCircle(cx, y + h + 6, w / 2 + 4, SSD1306_BLACK);
    if (mood == SAD)                           // droop the outer corner
      oled.fillTriangle(outer, y - 1, inner, y - 1, outer, y + h / 2, SSD1306_BLACK);
    if (mood == ANGRY)                         // slant down toward the nose
      oled.fillTriangle(outer, y - 1, inner, y - 1, inner, y + h / 2, SSD1306_BLACK);
  }
};
