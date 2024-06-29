#include <FastLED_NeoPixel.h>
#include <Streaming.h>
#include <math.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header
#include <DHT11.h>

// --------------------[  SERIALS  ]-------
#define SERIAL_BAUDRATE 9600
//#define SOFT_SERIAL_BAUDRATE 115200
#define SERIAL_TIMEOUT 5

boolean sStatus = false; // Serial Connected (yes/no)
boolean sReady = false;  // Serial Task Runner Ready (yes/no)
int incomingByte;
int estCount = 0;
int state;

boolean SERIALDEBUG = true;

// --------------------[  COLORS  ]-------
int black[3]  = { 0, 0, 0 };
int white[3]  = { 100, 100, 100 };
int red[3]    = { 100, 0, 0 };
int green[3]  = { 0, 100, 0 };
int blue[3]   = { 0, 0, 100 };
int yellow[3] = { 40, 95, 0 };
int dimWhite[3] = { 30, 30, 30 };


// --------------------[  HARDWARE  ]-------
#define DHT11_PIN 7
DHT11 dht11(DHT11_PIN);
int CurrentTempInFarenheight = 0;

// TIMING & EVENT DELAYS
int screenResetPeriodInMilliSeconds = 10000;
int delayPeriodInMilliSeconds = 1000;
unsigned long time_now = 0;
int loopCount = 0;

// SCREEN CONFIG
hd44780_I2Cexp lcd;
int ScreenSize = 20;
int LabelSize = 4; // Everything inside the brackets [CPU ]
int BarWidth = ScreenSize - LabelSize;

// How many leds in your strip?
#define NUM_LEDS 81
#define DATA_PIN 6
#define BRIGHTNESS 50
#define neoORDER RGB

// Define the array of leds
CRGB leds[NUM_LEDS];
CRGBArray<NUM_LEDS> neoLEDS;

FastLED_NeoPixel<NUM_LEDS, DATA_PIN, NEO_GRB> pixelStrip;      // <- FastLED NeoPixel version

// TODO: Update all this garbage to a decent RGB light setup
int hue_count = 0;
int hue_1_s = 1;
int hue_1_e = 100;
int hue_2_s = 101;
int hue_2_e = 200;
int hue_3_s = 201;
int hue_3_e = 255;
int TOP_INDEX  = int(NUM_LEDS/2);       //-INDEX IT UP
int rgb_pause  = 100;                    //-FX LOOPS DELAY VAR
int rgb_break  = 10;                    //-FX LOOPS DELAY VAR

int thisstep   = 10;                     //-FX LOOPS DELAY VAR
int thishue    = 0;                      //-FX LOOPS DELAY VAR
int thissat    = 255;                    //-FX LOOPS DELAY VAR
int max_bright = 155;                    //-SET MAX BRIGHTNESS TO 1/4
int ir         = 0;                                          
int idex       = 0;                      //-LED INDEX (0 to LED_COUNT-1
int ihue       = 0;                      //-HUE (0-255)
int ibright    = 0;                      //-BRIGHTNESS (0-255)
int isat       = 0;                      //-SATURATION (0-255)
int bouncedirection = 0;                 //-SWITCH FOR COLOR BOUNCE (0-1)
float tcount   = 0.0;                    //-INC VAR FOR SIN LOOPS
int lcount     = 0;                      //-ANOTHER COUNTING VAR

String temperatureHumidityString="";
String temperatureScaleLabel=" °F";
String temperatureLabel="TEMP ";
String stringLEDLabel="rnbw";
String stringSpace=" ";

long randNumber;
float randPercent;

// CUSTOM LETTERS FOR THE SCREEN ☐ & ◼︎ to use as progress bars
byte unused[8] = {
  0b11111,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b11111,
};

byte used[8] = {
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
};

byte degreeSym[8] = {
  0b01110,
  0b01010,
  0b01110,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
};

// --------------------[  SETUP  ]-------
void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  delay(500);
  Serial.println("Setup Started");

  pixelStrip.begin();  // initialize strip (required!)
	pixelStrip.setBrightness(BRIGHTNESS);

  screenSetup();
  // screenDisplayComputerStats();

  Serial << "SETUP DONE" << endl; 
}

void screenSetup() {
  lcd.backlight();
  lcd.begin(20, 4);
  lcd.clear();
  lcd.home();
  lcd.createChar(0, unused);
  lcd.createChar(1, used);
  lcd.createChar(2, degreeSym);

  lcdPrintStaticContent();

  // TODO: update this w/ real data
  randNumber = random(1, 10);
  randPercent = (randNumber * 0.1);
  printProgressBar(randPercent, 0);
  randNumber = random(1, 10);
  randPercent = (randNumber * 0.1);
  printProgressBar(randPercent, 1);
  randNumber = random(1, 10);
  randPercent = (randNumber * 0.1);
  printProgressBar(randPercent, 2);

  lcd.setCursor(4, 3);
  lcd.print("rnbw TEMP 00");
  lcd.write(2);
  lcd.print("F");

}

// --------------------[  HARDWARE FUNCTIONS  ]-------
void printProgressBar(float percentageAmountFull, int startRow) {
  Serial << "Progress Bar " << round(percentageAmountFull * 100) << "% Full" << endl; 

  lcd.setCursor(LabelSize, startRow);

  int fillAmount = floor(BarWidth * percentageAmountFull);
  int counter = 0;
  while(counter < BarWidth){
    if(counter < fillAmount){
      Serial.print("◼︎");
      lcd.write(1);
    } else {
      Serial.print("◻︎");
      lcd.write(0);
    }
    counter++;
  }

  Serial.println(" ");
}

// Clears the screen & Prints all the STATIC content
void lcdPrintStaticContent() {
  lcd.home();
  lcd.setCursor(0, 0);
  lcd.print("CPU");
  lcd.setCursor(0, 1);
  lcd.print("RAM");
  lcd.setCursor(0, 2);
  lcd.print("GPU");
  lcd.setCursor(0, 3);
  lcd.print("LED");
}

void tempReader() {
  // Attempt to read the temperature value from the DHT11 sensor.
    int temperature = dht11.readTemperature();

    // Check the result of the reading.
    // If there's no error, print the temperature value.
    // If there's an error, print the appropriate error message.
    if (temperature != DHT11::ERROR_CHECKSUM && temperature != DHT11::ERROR_TIMEOUT) {
        CurrentTempInFarenheight = round(1.8*temperature+32);
        temperatureHumidityString = temperatureLabel + CurrentTempInFarenheight;
        Serial.println(temperatureHumidityString + temperatureScaleLabel);
    } else {
        // Print error message based on the error code.
        Serial.println(DHT11::getErrorString(temperature));
        temperatureHumidityString = "XX?!";
    }

    Serial.println(" ");
}

void lcdPrintTempAndColorContent(String amountToDisplay) {
  // lcd.home();
  lcd.setCursor(4, 3);
  lcd.print(amountToDisplay);
  lcd.write(2);
  lcd.print("F");
}

/**
 *   RGB COLOR CYCLER
 *   @brief chill rainbow colors
 */
void rainbow_fade(int startat, int endat)
{
  // unsigned long start = millis();
  
  //-m2-FADE ALL LEDS THROUGH HSV RAINBOW
  while(startat < endat)
  {
    ihue++;

    if (ihue > 255) { ihue = 0; }

    for(int idex = 0 ; idex < NUM_LEDS; idex++ )
      pixelStrip.setPixelColor(idex, pixelStrip.gamma32(pixelStrip.ColorHSV(ihue, thissat, 255)));

    pixelStrip.show();
    
    delay(rgb_break);
    startat++;
  }

  return true;
  Serial.println("X | END RGB LOOP | X");
}

/*
* Simple rainbow animation, iterating through all 8-bit hues. LED color changes
* based on position in the pixelStrip. Takes two arguments:
* 
*     1. the amount of time to wait between frames
*     2. the number of rainbows to loop through
*/
void rainbow(unsigned long wait, unsigned int numLoops) {
	for (unsigned int count = 0; count < numLoops; count++) {
		// iterate through all 8-bit hues, using 16-bit values for granularity
		for (unsigned long firstPixelHue = 0; firstPixelHue < 65536; firstPixelHue += 256) {
			for (unsigned int i = 0; i < pixelStrip.numPixels(); i++) {
				unsigned long pixelHue = firstPixelHue + (i * 65536UL / pixelStrip.numPixels()); // vary LED hue based on position
				pixelStrip.setPixelColor(i, pixelStrip.gamma32(pixelStrip.ColorHSV(pixelHue)));  // assign color, using gamma curve for a more natural look
			}
			pixelStrip.show();
			delay(wait);
		}
	}
}

/*
* Blanks the LEDs and waits for a short time.
*/
void blank(unsigned long wait) {
	pixelStrip.clear();
	pixelStrip.show();
	delay(wait);
}

// --------------------[  LOOP IT UP  ]-------
int statRowCounter = 0;
void loop() {
  Serial << "Looping" << endl; 

  // // Control the LED's first
  // switch (hue_count) 
  // {
  //   case 0: 
  //     rainbow_fade(hue_1_s,hue_1_e);
  //     hue_count = 1;
  //     break;
  //   case 1:
  //     rainbow_fade(hue_2_s,hue_2_e);
  //     hue_count = 2;
  //     break;
  //   case 2: 
  //     hue_count = 0;
  //     rainbow_fade(hue_3_s,hue_3_e);
  //     break;
  // }
  rainbow(10, 3);
	
  // Reset Labels w/ a slight pause. Removes any 'artifacts' that may have shown up
  if(millis() >= time_now + screenResetPeriodInMilliSeconds){
      time_now += screenResetPeriodInMilliSeconds;
      Serial.println("Screen Reset"); 
      lcdPrintStaticContent();
      blank(1000);
  }

  // TODO: For each of these read a value from serial
  if(loopCount == 0){
    Serial.println("CPU STAT UPDATE");
    statRowCounter = 0;
  }
  if(loopCount == 1){
    Serial.println("RAM STAT UPDATE");
    statRowCounter = 1;
  }
  if(loopCount == 2){
    Serial.println("GPU STAT UPDATE");
    statRowCounter = 2;
  }

  if(loopCount == 3){
    Serial.println("LED & TEMP");
    tempReader();
    String stringThree = stringLEDLabel + stringSpace + temperatureHumidityString;
    lcdPrintTempAndColorContent(stringThree);
  }
  
  // Update the Progress Bars with a new one on first 3 rows
  if(loopCount != 3){
    randNumber = random(1, 10);
    randPercent = (randNumber * 0.1);
    printProgressBar(randPercent, loopCount);
  }
  
  loopCount = loopCount + 1;
  if(loopCount > 3){
    loopCount = 0;
  }

  delay(delayPeriodInMilliSeconds);
}
