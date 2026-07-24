#ifndef LUMI_ANIMATOR_H
#define LUMI_ANIMATOR_H

#include <AvantLumi.h>

// --- Custom Cyberpunk Palette for Night Mode ---
// Blends Neon Cyan, Hot Magenta, Deep Violet, and Midnight Blue
DEFINE_GRADIENT_PALETTE( cyberpunk_gp ) {
    0,     0, 255, 240,  // Bright Cyan
   85,   255,   0, 127,  // Hot Magenta
  170,   128,   0, 255,  // Deep Violet
  255,     0,  15,  60   // Midnight Blue
};

class LUMIAnimator {
public:
  AvantLumi *strip;

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
  Phase lastPhase = COLOR_HOLD; // Track phase transitions

  uint32_t highCpuStartTime = 0;
  bool isHighStress = false;
  const uint8_t CPU_HIGH_THRESHOLD = 75;  // %
  const uint8_t RAM_HIGH_THRESHOLD = 70;  // %
  const uint8_t GPU_HIGH_THRESHOLD = 60;  // %
  const uint32_t STRESS_HOLD_TIME_MS = 1500; // 1.5s sustained load trigger

  uint32_t phaseStart = 0;
  uint32_t fadeDuration = 10000;    // 10s
  uint32_t holdDuration = 10000;    // 10s
  uint32_t breatheDuration = 30000; // 30s
  uint32_t chaseDuration = 15000;   // 15s

  uint8_t tempFlashCount = 0;
  bool tempFlashOn = false;

  CRGBPalette16 cyberpunkPalette = cyberpunk_gp;
  bool lastNightState = false;

  LUMIAnimator(AvantLumi *s) {
    strip = s;
    lastStatsUpdate = millis();
    phaseStart = millis();
  }

  const char *getCurrentModeCode() const {
    if (phase == TEMP_ALERT)
      return "temp";
    if (phase == HIGH_STRESS)
      return "strs";
    if (isNight)
      return "cybr"; // Cyberpunk night mode tag

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

    // 1. Timeout Safety Check (prevents latching onto high stress if PC disconnects)
    bool dataStale = (now - lastStatsUpdate >= STATS_TIMEOUT_MS);
    if (dataStale) {
      cpu = 0;
      ram = 0;
      gpu = 0;
      loadAverage = 0;
      highCpuStartTime = 0;
      isHighStress = false;
      strip->clear();
      return;
    }

    updateLoadAverage(now);
    maybeTriggerTempAlert(now);
    checkHighStress(now);

    uint16_t speedFactor = getSpeedMultiplier();

    // Priority 1: Temperature Emergency (Flashing Alert)
    if (phase == TEMP_ALERT) {
      renderTempAlert(now);
      strip->update();
      return;
    }

    // Priority 2: High System Load (Aggressive Strobe)
    if (phase == HIGH_STRESS) {
      bool flashOn = (now / 150) % 2 == 0;
      strip->setColor(flashOn ? CRGB::Red : CRGB::Black);
      strip->setEffect(AVANTLUMI_SOLID);
      strip->update();
      return;
    }

    // Priority 3: NIGHTTIME MODE (Cyberpunk Palette Wave)
    if (isNight) {
      if (!lastNightState || lastPhase != phase) {
        strip->setPalette(cyberpunkPalette);
        strip->setEffect(AVANTLUMI_PALETTE_SHIFT);
        lastNightState = true;
      }
      strip->setSpeed(15 * speedFactor);
      strip->update();
      lastPhase = phase;
      return;
    }

    // Priority 4: DAYTIME MODE (Dynamic Palette Transitions)
    if (lastNightState) {
      lastNightState = false;
      strip->setPalette(RainbowColors_p);
    }

    switch (phase) {
      case COLOR_CHASE:
        if (lastPhase != COLOR_CHASE) {
          strip->setEffect(AVANTLUMI_THEATER_CHASE);
          strip->setPalette(RainbowColors_p);
        }
        strip->setSpeed(20 * speedFactor);
        if (now - phaseStart > (chaseDuration / speedFactor)) {
          nextPhase(COLOR_FADE);
        }
        break;

      case COLOR_FADE:
        if (lastPhase != COLOR_FADE) {
          strip->setEffect(AVANTLUMI_PALETTE_SHIFT);
          strip->setPalette(RainbowColors_p);
        }
        strip->setSpeed(12 * speedFactor);
        if (now - phaseStart > (fadeDuration / speedFactor)) {
          nextPhase(COLOR_HOLD);
        }
        break;

      case COLOR_HOLD:
        if (lastPhase != COLOR_HOLD) {
          strip->setEffect(AVANTLUMI_SOLID);
          strip->setColor(CRGB::Teal);
        }
        if (now - phaseStart > (holdDuration / speedFactor)) {
          nextPhase(COLOR_BREATHE);
        }
        break;

      case COLOR_BREATHE:
        if (lastPhase != COLOR_BREATHE) {
          strip->setEffect(AVANTLUMI_BREATHE);
          strip->setPalette(PartyColors_p);
        }
        strip->setSpeed(10 * speedFactor);
        if (now - phaseStart > (breatheDuration / speedFactor)) {
          nextPhase(COLOR_CHASE);
        }
        break;
    }

    lastPhase = phase;
    strip->update();
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
      strip->setColor(tempFlashOn ? CRGB::White : CRGB::Black);
      strip->setEffect(AVANTLUMI_SOLID);
      return;
    }

    // Thermal alert breathing state
    strip->setColor(CRGB::Red);
    strip->setEffect(AVANTLUMI_BREATHE);
    strip->setSpeed(30);

    if (elapsed > 4000) {
      phase = returnPhase;
      phaseStart = now;
    }
  }

  void nextPhase(Phase p) {
    phase = p;
    phaseStart = millis();
  }
};

#endif