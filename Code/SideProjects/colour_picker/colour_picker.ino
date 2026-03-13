//************* HEADER FILES *****************
#include <Adafruit_TCS34725.h>
#include <Wire.h>
#include <math.h> // For pow() and round()
//********************************************

//*************** PIN SETUP ******************
// I2C colour sensor pins
#define SDA_PIN 5 // D1
#define SCL_PIN 4 // D2
//********************************************

//************ TIMING PARAMETERS *************
int sampleDelay = 400;      // Pause between samples
int displayDuration = 5000; // Hold final colour for 5 seconds
//********************************************

//************* GLOBALS *********************
// Colour sensor instance
Adafruit_TCS34725 tcs =
    Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_300MS, TCS34725_GAIN_1X);
//********************************************

//************* READ COLOUR FN ***************
// Reads the sensor and returns accurate RGB via pointers
void ReadColour(uint8_t *outR, uint8_t *outG, uint8_t *outB) {
  uint16_t r, g, b, c;

  // Reading raw data from the sensor
  tcs.getRawData(&r, &g, &b, &c);

  // Check for I2C lockup / power brownout (sensor returns all 1s = 65535)
  if (r == 65535 && g == 65535 && b == 65535) {
    Serial.println("Sensor glitch detected! Re-initializing I2C...");
    tcs.begin();
    *outR = 0;
    *outG = 0;
    *outB = 0;
    return;
  }

  // Get accurate colour values (compensates for IR and adjusts based on clear
  // channel)
  float red, green, blue;
  tcs.getRGB(&red, &green, &blue);

  // Clamp normalized values to 0–255
  *outR = min((uint16_t)red, (uint16_t)255);
  *outG = min((uint16_t)green, (uint16_t)255);
  *outB = min((uint16_t)blue, (uint16_t)255);
}
//********************************************

//********** GAMMA CORRECTION FN *************
// Applies standard 2.2 gamma correction for accurate screen/hex colours
uint8_t ApplyGamma(float linearVal) {
  if (linearVal <= 0.0f)
    return 0;
  if (linearVal >= 255.0f)
    return 255;
  float srgb = 255.0f * pow(linearVal / 255.0f, 1.0f / 2.2f);
  return min((uint8_t)round(srgb), (uint8_t)255);
}
//********************************************

//********* SATURATION BOOST FN **************
// Converts RGB -> HSV, boosts saturation, converts back to RGB
// factor > 1.0 increases saturation (e.g. 1.4 = +40%)
void BoostSaturation(uint8_t &r, uint8_t &g, uint8_t &b, float factor) {
  float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
  float maxC = max(rf, max(gf, bf));
  float minC = min(rf, min(gf, bf));
  float delta = maxC - minC;

  // HSV
  float h = 0, s = 0, v = maxC;
  if (delta > 0.0f) {
    s = delta / maxC;
    if (maxC == rf)
      h = 60.0f * fmod((gf - bf) / delta, 6.0f);
    else if (maxC == gf)
      h = 60.0f * ((bf - rf) / delta + 2.0f);
    else
      h = 60.0f * ((rf - gf) / delta + 4.0f);
    if (h < 0)
      h += 360.0f;
  }

  // Boost S, clamp to 1
  s = min(s * factor, 1.0f);

  // HSV -> RGB
  float c2 = v * s;
  float x = c2 * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
  float m = v - c2;
  float r1, g1, b1;
  int hi = (int)(h / 60.0f) % 6;
  if (hi == 0) {
    r1 = c2;
    g1 = x;
    b1 = 0;
  } else if (hi == 1) {
    r1 = x;
    g1 = c2;
    b1 = 0;
  } else if (hi == 2) {
    r1 = 0;
    g1 = c2;
    b1 = x;
  } else if (hi == 3) {
    r1 = 0;
    g1 = x;
    b1 = c2;
  } else if (hi == 4) {
    r1 = x;
    g1 = 0;
    b1 = c2;
  } else {
    r1 = c2;
    g1 = 0;
    b1 = x;
  }

  r = (uint8_t)round((r1 + m) * 255.0f);
  g = (uint8_t)round((g1 + m) * 255.0f);
  b = (uint8_t)round((b1 + m) * 255.0f);
}
//********************************************

//************* AUTO RANGE FN ****************
// Dynamically adjusts integration gain based on ambient light
void AutoRange() {
  uint16_t r, g, b, c;
  // Start with a safe middle gain and wait for integration
  tcs.setGain(TCS34725_GAIN_16X);
  delay(350);
  tcs.getRawData(&r, &g, &b, &c);

  if (c < 300) {
    tcs.setGain(TCS34725_GAIN_60X);
    Serial.println("Auto-range: Low light (Gain 60X)");
  } else if (c < 2000) {
    tcs.setGain(TCS34725_GAIN_16X);
    Serial.println("Auto-range: Medium light (Gain 16X)");
  } else if (c < 10000) {
    tcs.setGain(TCS34725_GAIN_4X);
    Serial.println("Auto-range: Bright light (Gain 4X)");
  } else {
    tcs.setGain(TCS34725_GAIN_1X);
    Serial.println("Auto-range: Very bright light (Gain 1X)");
  }
  // Wait one integration cycle for sensor to stabilize with new gain
  delay(350);
}
//********************************************

//******** SENSE AND DISPLAY FN **************
// Takes samples, applies statistical filtering and gamma correction
void SenseAndDisplay() {
  Serial.println("\n######### SENSOR ACTIVE  ##############");

  AutoRange();

  delay(sampleDelay);

  // Take 5 samples
  const int numSamples = 5;
  uint8_t r_samples[numSamples];
  uint8_t g_samples[numSamples];
  uint8_t b_samples[numSamples];

  for (int i = 0; i < numSamples; i++) {
    ReadColour(&r_samples[i], &g_samples[i], &b_samples[i]);

    // Print sample data
    Serial.print("Sample ");
    Serial.print(i + 1);
    Serial.print("/5: R=");
    Serial.print(r_samples[i]);
    Serial.print(" G=");
    Serial.print(g_samples[i]);
    Serial.print(" B=");
    Serial.println(b_samples[i]);

    if (i < numSamples - 1) {
      delay(sampleDelay);
    }
  }

  Serial.println("\nComputing final value");
  Serial.println("#####################################");

  // Sort channels independently to drop min and max
  for (int i = 0; i < numSamples - 1; i++) {
    for (int j = i + 1; j < numSamples; j++) {
      if (r_samples[i] > r_samples[j]) {
        uint8_t t = r_samples[i];
        r_samples[i] = r_samples[j];
        r_samples[j] = t;
      }
      if (g_samples[i] > g_samples[j]) {
        uint8_t t = g_samples[i];
        g_samples[i] = g_samples[j];
        g_samples[j] = t;
      }
      if (b_samples[i] > b_samples[j]) {
        uint8_t t = b_samples[i];
        b_samples[i] = b_samples[j];
        b_samples[j] = t;
      }
    }
  }

  // Average the middle 3 samples (trimmed mean)
  uint16_t sumR = 0, sumG = 0, sumB = 0;
  for (int i = 1; i < numSamples - 1; i++) {
    sumR += r_samples[i];
    sumG += g_samples[i];
    sumB += b_samples[i];
  }

  float avgR = (float)sumR / (numSamples - 2);
  float avgG = (float)sumG / (numSamples - 2);
  float avgB = (float)sumB / (numSamples - 2);

  // Apply Gamma Correction
  uint8_t finalR = ApplyGamma(avgR);
  uint8_t finalG = ApplyGamma(avgG);
  uint8_t finalB = ApplyGamma(avgB);

  // Boost saturation to compensate for sensor spectral overlap
  BoostSaturation(finalR, finalG, finalB, 2.0f);

  char hexStr[8];
  sprintf(hexStr, "#%02X%02X%02X", finalR, finalG, finalB);

  Serial.println("\n########### COLOUR VALUE #############");
  Serial.print("RGB: ");
  Serial.print(finalR);
  Serial.print(", ");
  Serial.print(finalG);
  Serial.print(", ");
  Serial.println(finalB);
  Serial.print("HEX: ");
  Serial.println(hexStr);
  Serial.println("#####################################\n");

  // Hold final colour duration before ready again
  delay(displayDuration);

  Serial.println("Press 'S' to scan a colour");
  Serial.println(" ");
}
//*******************************************

//************** SETUP FUNCTION **************
void setup() {

  //*********** SERIAL SETUP ****************
  // Starting serial communication
  Serial.begin(9600);
  Serial.println("\n");
  //******************************************

  //*********** COLOUR SENSOR SETUP **********
  Serial.println("###### Colour Sensor Setup ######");
  Wire.begin(SDA_PIN, SCL_PIN);

  if (tcs.begin()) {
    Serial.println("Colour sensor initialized!");
  } else {
    Serial.println("Error, check your connections!");
  }
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");

  tcs.setInterrupt(false);
  //*****************************************

  Serial.println("Press 'S' to scan a colour");
  Serial.println(" ");
}
//*******************************************

//************** LOOP FUNCTION **************
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.equalsIgnoreCase("s")) {
      SenseAndDisplay();
    }
  }
}
//*******************************************
