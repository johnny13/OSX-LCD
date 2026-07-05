#ifndef LED_ANIMATOR_H
#define LED_ANIMATOR_H

#include <FastLED_NeoPixel.h>

class LEDAnimator {
public:
    FastLED_NeoPixel<81,6,NEO_GRB>* strip;

    int cpu = 0;
    int ram = 0;
    int gpu = 0;
    int tempF = 70;
    bool isNight = false; 

    uint32_t lastStatsUpdate = 0;
    const uint32_t STATS_TIMEOUT_MS = 10000;

    uint32_t lastLoadCheck = 0;
    uint16_t loadAverage = 0;

    enum Phase { COLOR_FADE, COLOR_HOLD, COLOR_BREATHE, HIGH_STRESS, TEMP_ALERT };
    Phase phase = COLOR_FADE;
    Phase returnPhase = COLOR_FADE;

    uint32_t highCpuStartTime = 0;
    bool isHighStress = false;
    const uint8_t CPU_HIGH_THRESHOLD = 85;      // %
    const uint8_t RAM_HIGH_THRESHOLD = 90;      // %
    const uint32_t STRESS_HOLD_TIME_MS = 1500;  // 1.5 seconds sustained load trigger

    uint8_t palette[6] = {0, 32, 64, 96, 160, 192}; // Your solid colors
    uint8_t paletteIndex = 0;
    uint32_t phaseStart = 0;

    uint16_t fadeDuration    = 2500; // Slightly slower for a calmer daytime fade
    uint16_t holdDuration    = 2000;
    uint16_t breatheDuration = 4000;

    uint8_t currentHue = 0;
    uint8_t targetHue = 0;
    uint8_t tempFlashCount = 0;
    bool tempFlashOn = false;

    const char* getCurrentModeCode() const {
        if (phase == TEMP_ALERT)   return "temp";
        if (phase == HIGH_STRESS)  return "strs";
        if (isNight)               return "rbow"; // Displays "rbow" for Night Rainbow
        
        switch (phase) {
            case COLOR_FADE:      return "fade";
            case COLOR_HOLD:      return "hold";
            case COLOR_BREATHE:   return "brth";
            default:              return "????";
        }
    }

    LEDAnimator(FastLED_NeoPixel<81,6,NEO_GRB>* s) {
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

        if (isHighStress && c < CPU_HIGH_THRESHOLD) {
            isHighStress = false;
            highCpuStartTime = 0;
        }
    }

    void setTemp(int t) {
        tempF = t;
        lastStatsUpdate = millis();
    }

    void update() {
        uint32_t now = millis();

        bool dataStale = (now - lastStatsUpdate >= STATS_TIMEOUT_MS);
        if (dataStale) {
            cpu = 0; ram = 0; loadAverage = 0; highCpuStartTime = 0; isHighStress = false;
            strip->clear();
            strip->show();
            return;
        }

        updateLoadAverage(now);
        maybeTriggerTempAlert(now);
        checkHighStress(now);

        // Priority 1: Temperature Emergency (Flashing Red/White)
        if (phase == TEMP_ALERT) {
            renderTempAlert(now);
            strip->show();
            return;
        }

        // Priority 2: High System Load (Aggressive Red Strobe)
        if (phase == HIGH_STRESS) {
            uint8_t flash = beatsin8(160, 40, 255); 
            renderSolidHSV(0, 255, flash); 
            strip->show();
            return;
        }

        uint16_t speedFactor = getSpeedMultiplier();

        // Priority 3: NIGHTTIME MODE (Vibrant, Flowing Active Rainbow Wheel)
        if (isNight) {
            // Speed of the rainbow rotation scales up when your Mac works harder
            uint32_t rainbowSpeed = now * speedFactor;
            uint8_t baseHue = (rainbowSpeed / 15) & 0xFF; 
            
            for (int i = 0; i < 81; i++) {
                // Generates a tightly packed, fluidly moving rainbow color wheel across the strip
                uint8_t pixelHue = baseHue + (i * 4); 
                
                // 255 saturation keeps it punchy; 220 brightness is beautifully vivid
                strip->setPixelColor(i, strip->gamma32(strip->ColorHSV(pixelHue * 256, 255, 220)));
            }
            strip->show();
            return;
        }

        // Priority 4: DAYTIME MODE (Smooth Solid-to-Solid Color Fades)
        uint8_t currentBreath = beatsin8(10 * speedFactor, 180, 255);

        switch (phase) {
            case COLOR_FADE:
                renderFade(now);
                if (now - phaseStart > (fadeDuration / speedFactor)) nextPhase(COLOR_HOLD);
                break;

            case COLOR_HOLD:
                renderSolid(targetHue, 255);
                if (now - phaseStart > (holdDuration / speedFactor)) nextPhase(COLOR_BREATHE);
                break;

            case COLOR_BREATHE:
                renderSolid(targetHue, currentBreath);
                if (now - phaseStart > (breatheDuration / speedFactor) && currentBreath >= 254) advancePalette();
                break;
        }
        strip->show();
    }

private:
    void checkHighStress(uint32_t now) {
        if (phase == TEMP_ALERT) return;

        bool cpuHigh = (cpu >= CPU_HIGH_THRESHOLD);

        if (cpuHigh) {
            if (highCpuStartTime == 0) highCpuStartTime = now;
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
        if (now - lastLoadCheck < 1000) return;
        lastLoadCheck = now;
        uint16_t combined = (cpu + ram) / 2;
        loadAverage = (loadAverage * 3 + combined) / 4;
    }

    uint16_t getSpeedMultiplier() {
        if (loadAverage > 80) return 3;
        if (loadAverage > 60) return 2;
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
            if (tempFlashOn) renderSolidHSV(0, 0, 255);
            else renderSolidHSV(0, 0, 0);
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

    // FIXED: Keeps the daytime animations locked in a smooth loop of your solid colors
    void advancePalette() {
        paletteIndex++;
        if (paletteIndex >= 5) { 
            paletteIndex = 0;
            currentHue = palette[5];
            targetHue = palette[0];
            nextPhase(COLOR_FADE);
            return;
        }
        currentHue = targetHue;
        targetHue = palette[paletteIndex + 1];
        nextPhase(COLOR_FADE);
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
        if (t > 1) t = 1;
        uint8_t hue = currentHue + (targetHue - currentHue) * t;
        renderSolid(hue, 255);
    }
};

#endif