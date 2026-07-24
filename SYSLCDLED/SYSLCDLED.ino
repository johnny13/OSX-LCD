#include <Streaming.h>
#include <math.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <DHT11.h>

// --- ACTIVE: AVANTLUMI ANIMATOR ---
// #include <AvantLumi.h>
// #include "LUMIAnimator.h"

// #define NUM_LEDS 81
// #define DATA_PIN 6
// #define BRIGHTNESS 125

// AvantLumi pixelStrip(NUM_LEDS, DATA_PIN);
// LUMIAnimator animator(&pixelStrip);

/* 
// --- SWAP BLOCK: ORIGINAL FASTLED ANIMATOR ---
*/
#include <FastLED_NeoPixel.h>
#include "LEDAnimator.h"

#define NUM_LEDS 81
#define DATA_PIN 6
#define BRIGHTNESS 125

FastLED_NeoPixel<NUM_LEDS, DATA_PIN, NEO_GRB> pixelStrip;
LEDAnimator animator(&pixelStrip);

#define SERIAL_BAUDRATE 9600

int lastCPU = -1, lastRAM = -1, lastGPU = -1;

#define DHT11_PIN 7
DHT11 dht11(DHT11_PIN);
int CurrentTempInFarenheight = 0;

hd44780_I2Cexp lcd;
const int LabelSize = 4;
const int BarWidth = 16;

byte unused[8]    = {0b11111,0b10001,0b10001,0b10001,0b10001,0b10001,0b10001,0b11111};
byte used[8]      = {0b11111,0b11111,0b11111,0b11111,0b11111,0b11111,0b11111,0b11111};
byte degreeSym[8] = {0b01110,0b01010,0b01110,0b00000,0b00000,0b00000,0b00000,0b00000};

String temperatureHumidityString = "";
String stringSpace = " ";

unsigned long lastStatsReceived = 0;
const unsigned long STATS_TIMEOUT_MS = 10000;

void setup() {
  Serial.begin(SERIAL_BAUDRATE);

  pixelStrip.begin();
  pixelStrip.setBrightness(BRIGHTNESS);

  lcd.backlight();
  lcd.begin(20, 4);
  lcd.createChar(0, unused);
  lcd.createChar(1, used);
  lcd.createChar(2, degreeSym);

  lcdPrintStaticContent();
  Serial.println("SETUP DONE");

  lastStatsReceived = millis();
}

void loop() {
  unsigned long now = millis();
  
  // Timeout check for stale data
  bool dataStale = (now - lastStatsReceived >= STATS_TIMEOUT_MS);
  if (dataStale) {
    animator.setStats(0, 0, 0, 0);
  }

  animator.update();

  // Non-blocking full line reading
  if (Serial.available() > 0) {
    String inputLine = Serial.readStringUntil('\n');
    inputLine.trim();

    if (inputLine.length() > 0) {
      int inCPU = 0, inRAM = 0, inGPU = 0, inNight = 0;
      
      // Parse CSV format: CPU,RAM,GPU,Night
      int parsedCount = sscanf(inputLine.c_str(), "%d,%d,%d,%d", &inCPU, &inRAM, &inGPU, &inNight);

      if (parsedCount >= 3) {
        lastStatsReceived = now;

        if (inCPU != lastCPU) { printProgressBar(inCPU / 100.0, 0); lastCPU = inCPU; }
        if (inRAM != lastRAM) { printProgressBar(inRAM / 100.0, 1); lastRAM = inRAM; }
        if (inGPU != lastGPU) { printProgressBar(inGPU / 100.0, 2); lastGPU = inGPU; }

        animator.setStats(inCPU, inRAM, inGPU, inNight);

        Serial.println("K");
      }
    }
  }

  // Periodic Temperature Reading
  static unsigned long lastTempUpdate = 0;
  if (now - lastTempUpdate >= 5000) {
      lastTempUpdate = now;
      tempReader();
      const char* cMode = animator.getCurrentModeCode();
      String tempDisplay = String(cMode) + stringSpace + temperatureHumidityString;
      lcdPrintTempAndColorContent(tempDisplay);
  }

  // Periodic Display Refresh
  static unsigned long lastScreenReset = 0;
  if (now - lastScreenReset >= 60000) {
      lastScreenReset = now;
      lcdPrintStaticContent();
      lastCPU = -1; lastRAM = -1; lastGPU = -1;
  }
}

void printProgressBar(float percentageAmountFull, int startRow) {
  lcd.setCursor(LabelSize, startRow);
  int fillAmount = floor(BarWidth * percentageAmountFull);
  for(int i = 0; i < BarWidth; i++) {
    if(i < fillAmount) lcd.write(1);
    else lcd.write(0);
  }
}

void lcdPrintStaticContent() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("CPU");
  lcd.setCursor(0, 1); lcd.print("RAM");
  lcd.setCursor(0, 2); lcd.print("GPU");
  lcd.setCursor(0, 3); lcd.print("LED");
}

void tempReader() {
  int temperature = dht11.readTemperature();
  
  if (temperature != DHT11::ERROR_CHECKSUM && temperature != DHT11::ERROR_TIMEOUT) {
      int convertedF = round(1.8 * temperature + 32);
      
      // Range validation check to reject impossible sensor glitches
      if (convertedF >= 32 && convertedF <= 120) {
          CurrentTempInFarenheight = convertedF;
          temperatureHumidityString = "TEMP " + String(CurrentTempInFarenheight);
          animator.setTemp(CurrentTempInFarenheight);
      }
  }
}

void lcdPrintTempAndColorContent(String amountToDisplay) {
  lcd.setCursor(4, 3);
  lcd.print(amountToDisplay);
  lcd.write(2);
  lcd.print("F");
}