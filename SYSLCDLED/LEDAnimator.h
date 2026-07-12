#ifndef LED_ANIMATOR_H
#define LED_ANIMATOR_H

#include <FastLED_NeoPixel.h>

class LEDAnimator {
public:
  FastLED_NeoPixel<81, 6, NEO_GRB> *strip;

  int cpu = 0;
  int ram = 0;
  int gpu = 0;
  int tempF = 70;
  bool isNight = false;

  uint32_t lastStatsUpdate = 0;
  const uint32_t STATS_TIMEOUT_MS = 10000;

  uint32_t lastLoadCheck = 0;
  uint16_t loadAverage = 0;

  enum Phase {
    COLOR_CHASE,
    COLOR_FADE,
    COLOR_HOLD,
    COLOR_BREATHE,
    HIGH_STRESS,
    TEMP_ALERT
  };
  Phase phase = COLOR_FADE;
  Phase returnPhase = COLOR_FADE;

  uint32_t highCpuStartTime = 0;
  bool isHighStress = false;
  const uint8_t CPU_HIGH_THRESHOLD = 75;  // %
  const uint8_t RAM_HIGH_THRESHOLD = 70;  // %
  const uint8_t GPU_HIGH_THRESHOLD = 60;
  const uint32_t STRESS_HOLD_TIME_MS = 1500;  // 1.5 seconds sustained load trigger

  // MODIFIED: Removed '0' (Red) and shifted palette to start at 32 (Orange/Yellow) up to 192 (Purple)
  uint8_t palette[6] = { 32, 64, 96, 128, 160, 192 };  
  uint8_t paletteIndex = 0;
  uint32_t phaseStart = 0;

  uint32_t fadeDuration = 10000;     // 10 seconds
  uint32_t holdDuration = 10000;     // 10 seconds
  uint32_t breatheDuration = 45000;  // 45 seconds
  uint32_t chaseDuration = 20000;    // 20 seconds

  uint8_t currentHue = 0;
  uint8_t targetHue = 0;
  uint8_t tempFlashCount = 0;
  bool tempFlashOn = false;

  unsigned long lastUpdate = 0;

  const char *getCurrentModeCode() const {
    if (phase == TEMP_ALERT)
      return "temp";
    if (phase == HIGH_STRESS)
      return "strs";
    if (isNight)
      return "rbow";

    switch (phase) {
      case COLOR_CHASE:
        return "chse";
      case COLOR_FADE:
        return "fade";
      case COLOR_HOLD:
        return "hold";
      case COLOR_BREATHE:
        return "brth";
      default:
        return "▪1∃▪";
    }
  }

  LEDAnimator(FastLED_NeoPixel<81, 6, NEO_GRB> *s) {
    strip = s;
    currentHue = palette[0];
    targetHue = palette[1];
    lastStatsUpdate = millis();
  }

  void setStats(int c, int r, int g, int nightFlag) {
    cpu = c;
    ram = r;
    gpu = g;
    isNight = (nightFlag == 1);
    lastStatsUpdate = millis();
  }

  void setTemp(int t) {
    tempF = t;
    lastStatsUpdate = millis();
  }

  void update() {
    uint32_t now = millis();

    bool dataStale = (now - lastStatsUpdate >= STATS_TIMEOUT_MS);
    if (dataStale) {
      cpu = 0;
      ram = 0;
      loadAverage = 0;
      highCpuStartTime = 0;
      isHighStress = false;
      strip->clear();
      strip->show();
      return;
    }

    updateLoadAverage(now);
    maybeTriggerTempAlert(now);
    checkHighStress(now);

    // Priority 1: Temperature Emergency (Flashing Red/White Alert)
    if (phase == TEMP_ALERT) {
      renderTempAlert(now);
      strip->show();
      return;
    }

    // Priority 2: High System Load (Aggressive Red Strobe)
    if (phase == HIGH_STRESS) {
      bool flashOn = (now / 150) % 2 == 0;
      renderSolidHSV(0, 255, flashOn ? 255 : 0); // Pure Red (Hue 0) preserved here!
      strip->show();
      return;
    }

    uint16_t speedFactor = getSpeedMultiplier();

    // Priority 3: NIGHTTIME MODE (Vibrant, Flowing Active Rainbow Wheel - Red Filtered)
    if (isNight) {
      uint32_t rainbowSpeed = now * speedFactor;
      uint8_t baseHue = (rainbowSpeed / 15) & 0xFF;

      for (int i = 0; i < 81; i++) {
        uint8_t pixelHue = baseHue + (i * 4);
        // MODIFIED: Maps the hue spectrum away from the red zone (0-31 and 221-255)
        uint8_t safeHue = map(pixelHue, 0, 255, 32, 220); 
        strip->setPixelColor(i, strip->gamma32(strip->ColorHSV(safeHue * 256, 255, 220)));
      }
      strip->show();
      return;
    }

    // Priority 4: DAYTIME MODE (Smooth Solid-to-Solid Color Fades)
    uint8_t currentBreath = beatsin8(10 * speedFactor, 180, 255);

    switch (phase) {
      case COLOR_CHASE:
        theaterChaseRainbow();
        if (now - phaseStart > (chaseDuration / speedFactor))
          nextPhase(COLOR_FADE);
        break;

      case COLOR_FADE:
        renderFade(now);
        if (now - phaseStart > (fadeDuration / speedFactor))
          nextPhase(COLOR_HOLD);
        break;

      case COLOR_HOLD:
        renderSolid(targetHue, 255);
        if (now - phaseStart > (holdDuration / speedFactor))
          nextPhase(COLOR_BREATHE);
        break;

      case COLOR_BREATHE:
        renderSolid(targetHue, currentBreath);
        if (now - phaseStart > (breatheDuration / speedFactor) && currentBreath >= 254)
          advancePalette();
        break;
    }
    strip->show();
  }

private:
  void checkHighStress(uint32_t now) {
    if (phase == TEMP_ALERT)
      return;

    bool resourcesHigh = (cpu >= CPU_HIGH_THRESHOLD) || (ram >= RAM_HIGH_THRESHOLD) || (gpu >= GPU_HIGH_THRESHOLD);

    if (resourcesHigh) {
      if (highCpuStartTime == 0)
        highCpuStartTime = now;
      if (now - highCpuStartTime >= STRESS_HOLD_TIME_MS) {
        if (!isHighStress) {
          returnPhase = phase;
          phase = HIGH_STRESS;
          phaseStart = now;
          isHighStress = true;
        }
      }
    } else {
      highCpuStartTime = 0;
      if (isHighStress) {
        phase = returnPhase;
        phaseStart = now;
        isHighStress = false;
      }
    }
  }

  void updateLoadAverage(uint32_t now) {
    if (now - lastLoadCheck < 1000)
      return;
    lastLoadCheck = now;
    uint16_t combined = (cpu + ram) / 2;
    loadAverage = (loadAverage * 3 + combined) / 4;
  }

  uint16_t getSpeedMultiplier() {
    if (loadAverage > 80)
      return 3;
    if (loadAverage > 60)
      return 2;
    return 1;
  }

  // NOTE: Wheel() is no longer utilized by theaterChaseRainbow to prevent red creeping in.
  uint32_t Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
      return strip->Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170) {
      WheelPos -= 85;
      return strip->Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return strip->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }

  void maybeTriggerTempAlert(uint32_t now) {
    static uint32_t lastAlertTime = 0;
    if (tempF > 85 && (now - lastAlertTime > 30000)) {
      lastAlertTime = now;
      returnPhase = phase;
      phase = TEMP_ALERT;
      phaseStart = now;
      tempFlashCount = 0;
      tempFlashOn = true;
    }
  }

  void renderTempAlert(uint32_t now) {
    uint32_t elapsed = now - phaseStart;
    if (tempFlashCount < 6) {
      if (elapsed > 150) {
        phaseStart = now;
        tempFlashOn = !tempFlashOn;
        tempFlashCount++;
      }
      if (tempFlashOn)
        renderSolidHSV(0, 0, 255);  // Flash White
      else
        renderSolidHSV(0, 0, 0);  // Flash Off
      return;
    }

    uint8_t hue = map(tempF, 60, 100, 160, 0);
    hue = constrain(hue, 0, 160);
    uint8_t breath = beatsin8(12, 120, 255);
    renderSolidHSV(hue, 255, breath);

    if (elapsed > 4000) {
      phase = returnPhase;
      phaseStart = now;
    }
  }

  void nextPhase(Phase p) {
    phase = p;
    phaseStart = millis();
  }

  void advancePalette() {
    paletteIndex++;
    if (paletteIndex >= 5) {
      paletteIndex = 0;
      currentHue = palette[5];
      targetHue = palette[0];
      nextPhase(COLOR_CHASE);
      return;
    }
    currentHue = targetHue;
    targetHue = palette[paletteIndex + 1];
    nextPhase(COLOR_CHASE);
  }

  void renderSolid(uint8_t hue, uint8_t bright) {
    renderSolidHSV(hue, 255, bright);
  }

  void renderSolidHSV(uint8_t h, uint8_t s, uint8_t v) {
    for (int i = 0; i < 81; i++)
      strip->setPixelColor(i, strip->gamma32(strip->ColorHSV(h * 256, s, v)));
  }

  void renderFade(uint32_t now) {
    float t = (float)(now - phaseStart) / fadeDuration;
    if (t > 1)
      t = 1;
    uint8_t hue = currentHue + (targetHue - currentHue) * t;
    renderSolid(hue, 255);
  }

  void theaterChaseRainbow() {
    uint32_t now = millis();
    if (now - lastUpdate < 100) return;
    lastUpdate = now;

    static int j = 0, q = 0;

    for (int i = 0; i < strip->numPixels(); i++) {
      strip->setPixelColor(i, 0);
    }

    for (int i = 0; i < strip->numPixels(); i = i + 3) {
      if (i + q < strip->numPixels()) {
        // MODIFIED: Shifted away from RGB Wheel() to use a Red-Filtered HSV map
        uint8_t rawHue = (i + j) % 255;
        uint8_t safeHue = map(rawHue, 0, 255, 32, 220); 
        strip->setPixelColor(i + q, strip->gamma32(strip->ColorHSV(safeHue * 256, 255, 255)));
      }
    }

    q++;
    if (q >= 3) {
      q = 0;
      j += 2;
      if (j >= 256)
        j = 0;
    }
  }
};

#endif