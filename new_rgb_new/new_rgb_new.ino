#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define PIN 7             // Changed from 6 to 7
#define N_PIXELS 90       // Changed from 144 to 90
#define COLOR_ORDER GRB
#define BRIGHTNESS 255
#define MIC_PIN A4
#define POT_PIN A0        // Added potentiometer pin
#define DC_OFFSET 280
#define NOISE 20
#define SAMPLES 60
#define PEAK_FALL 4
#define N_PIXELS_HALF (N_PIXELS / 2)
#define TOP (N_PIXELS + 2)
#define FIRE_COOLING 55
#define FIRE_SPARKING 120
#define COMET_LENGTH 15

unsigned long previousMillis = 0;
uint8_t gHue = 0;
static float pulsePos = 0;
static int cometPos = 0;

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

// VU meter scaling
float heightScale = 2.0;
int scaledHeight;

#define MODE0_SCALE 4
#define MODE2_SCALE 4

// Helper functions
uint32_t blendColor(uint32_t color1, uint32_t color2, float ratio) {
  uint8_t r = (uint8_t)(red(color1) * (1 - ratio) + red(color2) * ratio);
  uint8_t g = (uint8_t)(green(color1) * (1 - ratio) + green(color2) * ratio);
  uint8_t b = (uint8_t)(blue(color1) * (1 - ratio) + blue(color2) * ratio);
  return strip.Color(r, g, b);
}

uint8_t red(uint32_t color) { return (color >> 16) & 0xFF; }
uint8_t green(uint32_t color) { return (color >> 8) & 0xFF; }
uint8_t blue(uint32_t color) { return color & 0xFF; }

void setup() {
  delay(2000);
  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  buttonPushCounter = EEPROM.read(0);
  pinMode(POT_PIN, INPUT);  // Initialize potentiometer pin
}

void loop() {
  // Read potentiometer and set brightness
  int potValue = analogRead(POT_PIN);
  int brightness = map(potValue, 0, 1023, 5, 255);
  strip.setBrightness(brightness);
  
  handleButton();

  switch(buttonPushCounter % 10) {
    case 0: vu(); break;
    case 1: vu2(); break;
    case 2: Vu3(); break;
    case 3: Vu4(); break;
    case 4: vu5(); break;
    case 5: vu6(); break;
    case 6: vu7(); break;
    case 7: vu8(); break;
    case 8: vu9(); break;
    case 9: vu10(); break;
  }
}

void handleButton() {
  buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState && buttonState == HIGH) {
    buttonPushCounter++;
    EEPROM.write(0, buttonPushCounter % 10);
  }
  lastButtonState = buttonState;
}

int processAudio() {
  int n = analogRead(MIC_PIN);
  n = abs(n - DC_OFFSET);
  n = (n <= NOISE) ? 0 : (n - NOISE);
  n /= 2;
  
  lvl = ((lvl * 7) + n) >> 3;
  
  vol[volCount] = n;
  if(++volCount >= SAMPLES) volCount = 0;

  uint16_t minLvl = vol[0], maxLvl = vol[0];
  for(uint8_t i=1; i<SAMPLES; i++) {
    if(vol[i] < minLvl) minLvl = vol[i];
    else if(vol[i] > maxLvl) maxLvl = vol[i];
  }
  
  if((maxLvl - minLvl) < N_PIXELS) maxLvl = minLvl + N_PIXELS;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6;
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6;

  return map(lvl, 0, 300, 0, N_PIXELS);
}

int processAudioRaw() {
  int n = analogRead(MIC_PIN);
  n = abs(n - DC_OFFSET);
  n = (n <= NOISE) ? 0 : (n - NOISE);
  n /= 2;
  lvl = ((lvl * 7) + n) >> 3;
  return n;
}

void x_updatePeak(int height, int decaySpeed=1) {
  if(height > peak) peak = height;
  if(++dotCount >= (PEAK_FALL / decaySpeed)) {
    if(peak > 0) peak--;
    dotCount = 0;
  }
  
  if(peak > 0 && peak < N_PIXELS) {
    strip.setPixelColor(peak, Wheel2(map(peak, 0, N_PIXELS-1, 30, 150), 255));
    if(peak < N_PIXELS-1) {
      strip.setPixelColor(peak+1, Wheel2(map(peak, 0, N_PIXELS-1, 30, 150), 100));
    }
  }
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

uint32_t Wheel2(byte WheelPos, byte brightness) {
  if(WheelPos < 85) {
    return strip.Color(
      (WheelPos * 3 * brightness) / 255,
      ((255 - WheelPos * 3) * brightness) / 255,
      0
    );
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(
      ((255 - WheelPos * 3) * brightness) / 255,
      0,
      (WheelPos * 3 * brightness) / 255
    );
  } else {
    WheelPos -= 170;
    return strip.Color(
      0,
      (WheelPos * 3 * brightness) / 255,
      ((255 - WheelPos * 3) * brightness) / 255
    );
  }
}

void vu() {
  int height = processAudio();
  height = constrain(height * MODE0_SCALE, 0, N_PIXELS);

  if(height > peak) peak = height + 2;

  for(int i=0; i<N_PIXELS; i++) {
    if(i < height) {
      strip.setPixelColor(i, Wheel2(map(i, 0, N_PIXELS-1, 30, 150), 255));
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  
  x_updatePeak(height, 2);
  strip.show();
}

void Vu3() {
  int height = processAudio();
  height = constrain(pow(height * MODE2_SCALE, 1.2), 0, N_PIXELS);

  for(int i=0; i<N_PIXELS; i++) {
    if(i < height) {
      float position = (float)i / height;
      uint8_t r = position * 255 * 1.2;
      uint8_t g = (1 - position) * 255 * 1.1;
      strip.setPixelColor(i, strip.Color(
        constrain(r, 0, 255),
        constrain(g, 0, 255),
        0
      ));
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  
  for(int i=height; i<height+3; i++) {
    if(i < N_PIXELS) {
      strip.setPixelColor(i, strip.Color(255, 0, 0, 50));
    }
  }
  
  updatePeak(height);
  strip.show();
}

void vu2() {
  uint8_t i;
  uint16_t minLvl, maxLvl;
  int n, rawHeight;

  n = analogRead(MIC_PIN);
  n = abs(n - DC_OFFSET);
  n = (n <= NOISE) ? 0 : (n - NOISE);
  n /= 2;

  lvl = ((lvl * 7) + n) >> 3;

  rawHeight = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);
  int scaledHeight = constrain(rawHeight * heightScale, 0, N_PIXELS_HALF);

  if (scaledHeight > peak) peak = scaledHeight;

  for (i = 0; i < N_PIXELS_HALF; i++) {
    uint32_t color = (i < scaledHeight) ? 
      Wheel(map(i, 0, N_PIXELS_HALF - 1, 30, 150)) : 
      strip.Color(0, 0, 0);
      
    strip.setPixelColor(N_PIXELS_HALF - i - 1, color);
    strip.setPixelColor(N_PIXELS_HALF + i, color);
  }

  if (peak > 0 && peak <= N_PIXELS_HALF - 1) {
    uint32_t peakColor = Wheel(map(peak, 0, N_PIXELS_HALF - 1, 30, 150));
    strip.setPixelColor(N_PIXELS_HALF - peak - 1, peakColor);
    strip.setPixelColor(N_PIXELS_HALF + peak, peakColor);
  }

  strip.show();

  if (++dotCount >= PEAK_FALL) {
    if (peak > 0) peak--;
    dotCount = 0;
  }

  vol[volCount] = n;
  if (++volCount >= SAMPLES) volCount = 0;
  
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl) minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  
  if ((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6;
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6;
}

void Vu4() {
  uint8_t i;
  uint16_t minLvl, maxLvl;
  int n, rawHeight;
  static float colorOffset = 0;

  n = analogRead(MIC_PIN);
  n = abs(n - DC_OFFSET);
  n = (n <= NOISE) ? 0 : (n - NOISE);
  n /= 2;

  lvl = ((lvl * 7) + n) >> 3;

  rawHeight = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);
  int scaledHeight = constrain(rawHeight * heightScale, 0, N_PIXELS_HALF);

  if (scaledHeight > peak) peak = scaledHeight;
  colorOffset += 0.8;
  if (colorOffset >= 255) colorOffset = 0;

  for (i = 0; i < N_PIXELS_HALF; i++) {
    uint32_t color = (i < scaledHeight) ? 
      Wheel((map(i, 0, N_PIXELS_HALF - 1, colorOffset, colorOffset + 150)) % 255) : 
      strip.Color(0, 0, 0);
      
    strip.setPixelColor(N_PIXELS_HALF - i - 1, color);
    strip.setPixelColor(N_PIXELS_HALF + i, color);
  }

  if (peak > 0 && peak <= N_PIXELS_HALF - 1) {
    uint32_t peakColor = Wheel((map(peak, 0, N_PIXELS_HALF - 1, colorOffset, colorOffset + 150)) % 255);
    strip.setPixelColor(N_PIXELS_HALF - peak - 1, peakColor);
    strip.setPixelColor(N_PIXELS_HALF + peak, peakColor);
  }

  strip.show();

  if (++dotCount >= PEAK_FALL) {
    if (peak > 0) peak--;
    dotCount = 0;
  }

  vol[volCount] = n;
  if (++volCount >= SAMPLES) volCount = 0;
  
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl) minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  
  if ((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6;
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6;
}

void vu5() {
  static byte heat[N_PIXELS];
  int n = processAudioRaw();
  
  for(int i=0; i<N_PIXELS; i++) {
    heat[i] = max(heat[i] - random(0, FIRE_COOLING), 0);
  }
  
  int audioHeat = map(lvl, 0, 512, 0, 255);
  for(int i=0; i<map(n,0,512,2,8); i++) {
    int pos = random(N_PIXELS);
    heat[pos] = min(heat[pos] + audioHeat, 255);
  }
  
  for(int i=0; i<N_PIXELS; i++) {
    byte intensity = heat[i];
    strip.setPixelColor(i, strip.Color(
      intensity, 
      intensity * 0.5, 
      intensity * 0.2
    ));
  }
  strip.show();
}

void vu6() { 
  int n = processAudioRaw();
  int center = N_PIXELS/2;
  
  float pulseSpeed = map(lvl, 0, 512, 0.5, 3.0);
  int width = map(lvl, 0, 512, 2, N_PIXELS/2);
  
  pulsePos += pulseSpeed;
  if(pulsePos >= 255) pulsePos = 0;

  uint8_t baseHue = map(lvl, 0, 512, 0, 255);
  
  for(int i=0; i<N_PIXELS; i++) {
    float distance = abs(i - center);
    float pulse = (sin((pulsePos + i)*0.1) + 1) * 0.5;
    float attenuation = 1.0 - (distance/width);
    
    uint8_t brightness = constrain(255 * pulse * attenuation * (lvl/512.0), 0, 255);
    strip.setPixelColor(i, Wheel2(baseHue, brightness));
  }
  strip.show();
}

void vu7() {
  int n = processAudioRaw();
  
  int speed = map(lvl, 0, 512, 2, 20);
  int tailLength = map(lvl, 0, 512, 5, COMET_LENGTH);
  
  float fadeRatio = map(lvl, 0, 512, 0.1, 0.4);
  for(int i=0; i<N_PIXELS; i++) {
    strip.setPixelColor(i, blendColor(strip.getPixelColor(i), 0, fadeRatio));
  }

  cometPos = (cometPos + speed) % (N_PIXELS * 2);
  
  for(int i=0; i<tailLength; i++) {
    int pos = constrain(cometPos - i, 0, N_PIXELS-1);
    uint8_t brightness = 255 - (i*(255/tailLength));
    brightness = constrain(brightness * (lvl/512.0), 0, 255);
    strip.setPixelColor(pos, Wheel2(map(lvl, 0, 512, 0, 255), brightness));
  }
  strip.show();
}

void vu8() {
  int n = processAudioRaw();
  int segWidth = N_PIXELS / 8;
  
  uint8_t hueSpeed = map(lvl, 0, 512, 1, 10);
  uint8_t brightness = map(n, 0, 512, 50, 255);
  
  gHue += hueSpeed;
  
  for(int i=0; i<N_PIXELS; i++) {
    uint8_t hue = (gHue + (i/segWidth)*32 + map(lvl, 0, 512, 0, 32)) % 255;
    strip.setPixelColor(i, Wheel2(hue, brightness));
  }
  strip.show();
}

void vu9() {
  int n = processAudioRaw();
  
  byte fadeRate = map(lvl, 0, 512, 10, 100);
  int sparkProbability = map(lvl, 0, 512, 0, 100);
  
  for(int i=0; i<N_PIXELS; i++) {
    strip.setPixelColor(i, blendColor(strip.getPixelColor(i), 0, fadeRate/255.0));
  }

  if(random(100) < sparkProbability) {
    int pos = random(N_PIXELS);
    uint8_t intensity = map(lvl, 0, 512, 50, 255);
    strip.setPixelColor(pos, Wheel2(random(255), intensity));
  }
  strip.show();
}

void vu10() {
  static uint8_t startHue = 0;
  int n = processAudioRaw();
  startHue += 3;
  
  for(int i=0; i<N_PIXELS; i++) {
    float wave = sin((i*0.2) + (millis()*0.005)) * (n/1023.0);
    uint8_t brightness = constrain(wave * 255, 0, 255);
    strip.setPixelColor(i, Wheel2((startHue + map(i,0,N_PIXELS-1,0,255)) % 255, brightness));
  }
  strip.show();
}