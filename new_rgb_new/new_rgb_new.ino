#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define PIN 6
#define N_PIXELS 144
#define COLOR_ORDER GRB
#define BRIGHTNESS 255
#define MIC_PIN A4
#define DC_OFFSET 280     // Set this to your observed silent value
#define NOISE 20          // Adjusted for better noise rejection
#define SAMPLES 60
#define PEAK_FALL 4       // Faster peak decay for 144 LEDs
#define N_PIXELS_HALF (N_PIXELS / 2)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Button setup
const int buttonPin = 3;
int buttonPushCounter = 0;
int buttonState = 0;
int lastButtonState = 0;

// Audio processing variables
int vol[SAMPLES];
int lvl = 10;
int minLvlAvg = 0;
int maxLvlAvg = 512;
byte peak = 0;
byte dotCount = 0;
byte volCount = 0;

void setup() {
  delay(2000);
  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);
  
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  
  buttonPushCounter = EEPROM.read(0);
}

int readSensitivity() {
  return -2; // Fixed sensitivity (originally from pot)
}

void loop() {
  handleButton();
  updateVisualizer();
}

void handleButton() {
  buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState) {
    if (buttonState == HIGH) {
      buttonPushCounter++;
      if (buttonPushCounter >= 4) buttonPushCounter = 1;
      EEPROM.write(0, buttonPushCounter);
    }
    lastButtonState = buttonState;
  }
}

void updateVisualizer() {
  switch(buttonPushCounter) {
    case 1: vu(); break;
    case 2: vu2(); break;
    case 3: Vu3(); break;
    case 4: Vu4(); break;
  }
}

// Common audio processing
int processAudio() {
  static int vol[SAMPLES];
  static int volCount = 0;
  
  int n = analogRead(MIC_PIN);
  n = abs(n - DC_OFFSET);
  n = (n <= NOISE) ? 0 : (n - NOISE);
  
  int val = readSensitivity();
  if(val < 0) n /= abs(val);
  
  lvl = ((lvl * 7) + n) >> 3;
  vol[volCount] = n;
  if(++volCount >= SAMPLES) volCount = 0;

  // Dynamic level adjustment
  uint16_t minLvl = vol[0], maxLvl = vol[0];
  for(uint8_t i=1; i<SAMPLES; i++) {
    if(vol[i] < minLvl) minLvl = vol[i];
    else if(vol[i] > maxLvl) maxLvl = vol[i];
  }
  if((maxLvl - minLvl) < N_PIXELS) maxLvl = minLvl + N_PIXELS;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6;
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6;

  return map(lvl, 0, 600, 0, N_PIXELS); // Map to your observed range
}

// Visualization modes
void vu() {
  int height = processAudio();
  height = constrain(height, 0, N_PIXELS);
  
  for(uint16_t i=0; i<N_PIXELS; i++) {
    strip.setPixelColor(i, (i < height) ? Wheel(map(i, 0, N_PIXELS-1, 30, 150)) : 0);
  }
  updatePeak(height);
  strip.show();
}

void vu2() {
  int height = processAudio();
  height = constrain(height, 0, N_PIXELS_HALF);
  
  for(uint16_t i=0; i<N_PIXELS_HALF; i++) {
    uint32_t color = (i < height) ? Wheel(map(i, 0, N_PIXELS_HALF-1, 30, 150)) : 0;
    strip.setPixelColor(N_PIXELS_HALF - i - 1, color);
    strip.setPixelColor(N_PIXELS_HALF + i, color);
  }
  updatePeak(height);
  strip.show();
}

void Vu3() {
  int height = processAudio();
  height = constrain(height, 0, N_PIXELS);
  
  for(uint16_t i=0; i<N_PIXELS; i++) {
    strip.setPixelColor(i, (i < height) ? strip.Color(0, map(i, 0, N_PIXELS-1, 50, 255), 0) : 0);
  }
  updatePeak(height);
  strip.show();
}

void Vu4() {
  static float greenOffset = 30, blueOffset = 150;
  int height = processAudio();
  height = constrain(height, 0, N_PIXELS_HALF);
  
  greenOffset += 0.2;
  blueOffset += 0.2;
  if(greenOffset >= 255) greenOffset = 0;
  if(blueOffset >= 255) blueOffset = 0;

  for(uint16_t i=0; i<N_PIXELS_HALF; i++) {
    uint32_t color = (i < height) ? Wheel(map(i, 0, N_PIXELS_HALF-1, greenOffset, blueOffset)) : 0;
    strip.setPixelColor(N_PIXELS_HALF - i - 1, color);
    strip.setPixelColor(N_PIXELS_HALF + i, color);
  }
  updatePeak(height);
  strip.show();
}

void updatePeak(int height) {
  if(height > peak) peak = height;
  if(++dotCount >= PEAK_FALL) {
    if(peak > 0) peak--;
    dotCount = 0;
  }
  if(peak > 0 && peak < N_PIXELS) {
    strip.setPixelColor(peak, Wheel(map(peak, 0, N_PIXELS-1, 30, 150)));
  }
}

uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}