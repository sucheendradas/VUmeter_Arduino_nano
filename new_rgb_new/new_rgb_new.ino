#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define PIN 6
#define N_PIXELS 144
#define COLOR_ORDER GRB
#define BRIGHTNESS 255
#define MIC_PIN A4
#define DC_OFFSET 280    // Calibrated for your specific microphone
#define NOISE 20         // Adjusted noise floor
#define SAMPLES 60
#define PEAK_FALL 4      // Faster peak decay for 144 LEDs
#define N_PIXELS_HALF (N_PIXELS / 2)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Button setup
const int buttonPin = 3;
int buttonPushCounter = 0;
int buttonState = 0;
int lastButtonState = 0;

// VU Meter variables
int vol[SAMPLES];
int lvl = 10;
int minLvlAvg = 0;
int maxLvlAvg = 512;
byte peak = 0;
byte dotCount = 0;
byte volCount = 0;
float greenOffset = 30;
float blueOffset = 150;

void setup() {
  delay(2000);
  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  buttonPushCounter = EEPROM.read(0);
}

void loop() {
  handleButton();
  
  switch(buttonPushCounter % 4) {
    case 0:{ vu(); Serial.println("vu_0") ;}break;
    case 1: vu2(); break;
    case 2: Vu3(); break;
    case 3: Vu4(); break;
  }
}

void handleButton() {
  buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState && buttonState == HIGH) {
    buttonPushCounter++;
    EEPROM.write(0, buttonPushCounter % 4);
  }
  lastButtonState = buttonState;
}

int processAudio() {
  int n = analogRead(MIC_PIN);
  n = abs(n - DC_OFFSET);          // Apply DC offset
  n = (n <= NOISE) ? 0 : (n - NOISE); // Noise filter
  n /= 2;  // Fixed sensitivity (equivalent to val = -2)
  
  lvl = ((lvl * 7) + n) >> 3;     // Dampened reading
  
  // Store sample for dynamic scaling
  vol[volCount] = n;
  if(++volCount >= SAMPLES) volCount = 0;

  // Calculate dynamic level range
  uint16_t minLvl = vol[0], maxLvl = vol[0];
  for(uint8_t i=1; i<SAMPLES; i++) {
    if(vol[i] < minLvl) minLvl = vol[i];
    else if(vol[i] > maxLvl) maxLvl = vol[i];
  }
  
  if((maxLvl - minLvl) < N_PIXELS) maxLvl = minLvl + N_PIXELS;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6;
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6;

  return map(lvl, 0, 300, 0, N_PIXELS); // Adjusted for your 280-600 range
}

void vu() {
  int height = processAudio();
  height = constrain(height, 0, N_PIXELS);

  // Solid color bar with peak dot
  for(int i=0; i<N_PIXELS; i++) {
    strip.setPixelColor(i, (i < height) ? Wheel(map(i, 0, N_PIXELS-1, 30, 150)) : 0);
  }
  
  updatePeak(height);
  strip.show();
}

void vu2() {
  int height = processAudio();
  height = constrain(height, 0, N_PIXELS_HALF);

  // Mirror effect
  for(int i=0; i<N_PIXELS_HALF; i++) {
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

  // Gradient from green to red
  for(int i=0; i<N_PIXELS; i++) {
    if(i < height) {
      strip.setPixelColor(i, strip.Color(
        map(i, 0, N_PIXELS-1, 0, 255),  // Red
        map(i, 0, N_PIXELS-1, 255, 0),  // Green
        0                               // Blue
      ));
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  
  updatePeak(height);
  strip.show();
}

void Vu4() {
  static float offset = 0;
  int height = processAudio();
  height = constrain(height, 0, N_PIXELS_HALF);
  offset += 0.4;
  if(offset >= 255) offset = 0;

  // Moving rainbow effect
  for(int i=0; i<N_PIXELS_HALF; i++) {
    uint32_t color = (i < height) ? Wheel(map(i, 0, N_PIXELS_HALF-1, offset, offset+150) % 255) : 0;
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