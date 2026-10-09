#pragma once
#define NEO_GRB 0
#define NEO_KHZ800 0
class Adafruit_NeoPixel {
public:
  unsigned long envios = 0;
  Adafruit_NeoPixel(int, int, int) {}
  void begin() {}
  void setBrightness(int) {}
  void clear() {}
  void show() { ++envios; }
  uint32_t Color(int r, int g, int b) { return ((uint32_t)r << 16) | (g << 8) | b; }
  uint32_t ColorHSV(uint16_t v) { return v; }
  void setPixelColor(int, uint32_t) {}
};
