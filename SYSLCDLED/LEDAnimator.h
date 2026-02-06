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

    uint32_t lastLoadCheck = 0;
    uint16_t loadAverage = 0;

    enum Phase { COLOR_FADE, COLOR_HOLD, COLOR_BREATHE, RAINBOW_DANCE, TEMP_ALERT };
    Phase phase = COLOR_FADE;
    Phase returnPhase = COLOR_FADE;

    uint8_t palette[6] = {0, 32, 64, 96, 160, 192};
    uint8_t paletteIndex = 0;
    uint32_t phaseStart = 0;

    uint16_t fadeDuration = 2000;
    uint16_t holdDuration = 1500;
    uint16_t breatheDuration = 4000;
    uint16_t rainbowDuration = 7000;

    uint8_t currentHue = 0;
    uint8_t targetHue = 0;
    uint8_t tempFlashCount = 0;
    bool tempFlashOn = false;

    LEDAnimator(FastLED_NeoPixel<81,6,NEO_GRB>* s) {
        strip = s;
        currentHue = palette[0];
        targetHue = palette[1];
    }

    void setStats(int c, int r, int g) { cpu = c; ram = r; gpu = g; }
    void setTemp(int t) { tempF = t; }

    void update() {
        uint32_t now = millis();
        updateLoadAverage(now);
        maybeTriggerTempAlert(now);

        if(phase == TEMP_ALERT) {
            renderTempAlert(now);
            strip->show();
            return;
        }

        uint16_t speedFactor = getSpeedMultiplier();
        // Global breath used to "gate" the transition so it only happens at full brightness
        uint8_t currentBreath = beatsin8(10 * speedFactor, 180, 255);

        switch(phase) {
            case COLOR_FADE:
                renderFade(now);
                if(now - phaseStart > (fadeDuration / speedFactor))
                    nextPhase(COLOR_HOLD);
                break;

            case COLOR_HOLD:
                renderSolid(targetHue, 255);
                if(now - phaseStart > (holdDuration / speedFactor))
                    nextPhase(COLOR_BREATHE);
                break;

            case COLOR_BREATHE:
                renderSolid(targetHue, currentBreath);
                // Wait until timer is up AND brightness is at peak
                if(now - phaseStart > (breatheDuration / speedFactor) && currentBreath >= 254)
                    advancePalette();
                break;

            case RAINBOW_DANCE:
                renderRainbowDance(now, speedFactor);
                if(now - phaseStart > (rainbowDuration / speedFactor)) {
                    paletteIndex = 0;
                    currentHue = targetHue; 
                    targetHue = palette[0];
                    nextPhase(COLOR_FADE);
                }
                break;
        }
        strip->show();
    }

private:
    void updateLoadAverage(uint32_t now) {
        if(now - lastLoadCheck < 1000) return;
        lastLoadCheck = now;
        uint16_t combined = (cpu + ram) / 2;
        loadAverage = (loadAverage * 3 + combined) / 4;
    }

    uint16_t getSpeedMultiplier() {
        if(loadAverage > 80) return 3;
        if(loadAverage > 60) return 2;
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
        if(tempFlashCount < 6) {
            if(elapsed > 150) {
                phaseStart = now;
                tempFlashOn = !tempFlashOn;
                tempFlashCount++;
            }
            if(tempFlashOn) renderSolidHSV(0,0,255);
            else renderSolidHSV(0,0,0);
            return;
        }
        uint8_t hue = map(tempF,60,100,160,0);
        hue = constrain(hue,0,160);
        uint8_t breath = beatsin8(12,120,255);
        renderSolidHSV(hue,255,breath);
        if(elapsed > 4000) {
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
        if(paletteIndex >= 5) {
            nextPhase(RAINBOW_DANCE);
            return;
        }
        currentHue = targetHue;
        targetHue = palette[paletteIndex+1];
        nextPhase(COLOR_FADE);
    }

    void renderSolid(uint8_t hue, uint8_t bright) { renderSolidHSV(hue, 255, bright); }

    void renderSolidHSV(uint8_t h, uint8_t s, uint8_t v) {
        for(int i=0; i<81; i++)
            strip->setPixelColor(i, strip->gamma32(strip->ColorHSV(h*256, s, v)));
    }

    void renderFade(uint32_t now) {
        float t = (float)(now - phaseStart) / fadeDuration;
        if(t > 1) t = 1;
        uint8_t hue = currentHue + (targetHue - currentHue) * t;
        renderSolid(hue, 255);
    }

    void renderRainbowDance(uint32_t now, uint16_t speed) {
        uint16_t shift = (now * speed) / 12;
        for(int i=0; i<81; i++) {
            uint16_t hue = shift + i * (6 * speed);
            // FIXED: Changed speedFactor to speed (the local variable)
            uint8_t wave = beatsin8(7 * speed, 160, 255, 0, i * 2); 
            strip->setPixelColor(i, strip->gamma32(strip->ColorHSV(hue*256, 255, wave)));
        }
    }
};

#endif