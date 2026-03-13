//************* HEADER FILES *****************
#include <Adafruit_TCS34725.h>
#include <Wire.h>
#include <math.h>
//********************************************

//*************** PIN SETUP ******************
// I2C colour sensor (same as colour_picker)
#define SDA_PIN 5 // D1
#define SCL_PIN 4 // D2

// PWM output pins for common-cathode RGB LED
#define LedR 13 // D7
#define LedG 15 // D8
#define LedB 0  // D3
//********************************************

//************* TUNING PARAMETERS ************
// Timing
const int SCAN_INTERVAL_MS = 500;  // How often to sample the surface
const int SAMPLE_DELAY_MS = 80;    // Pause between individual samples
const int TRANSITION_STEP_MS = 15; // PWM update tick during fade
const int NUM_SAMPLES = 5;         // Readings per scan (trimmed mean)

// AutoRange frequency
// AutoRange blocks ~350 ms when gain changes. Only run every N scans.
const int AUTO_RANGE_EVERY = 10; // ~every 5 s

// Colour change detection
// Compare squared distance to avoid sqrt() on every scan.
// CHANGE_THRESHOLD = 12  →  threshold squared = 144
const int CHANGE_THRESHOLD = 12;
const uint32_t CHANGE_THRESHOLD_SQ =
    (uint32_t)CHANGE_THRESHOLD * CHANGE_THRESHOLD;

// Colour processing
const float SATURATION_BOOST = 3.0f; // >1 = more vivid colours
const float GAMMA = 2.2f;            // sRGB display gamma
// Min chroma after black-level subtraction — below this the surface is
// grey/white/black and correction would only amplify noise.
const uint8_t GREY_THRESHOLD = 15;

// Sensor channel correction (calibrate by pointing at white paper)
// Lowers green to remove the cool-white sensor LED spectral bias.
float SENSOR_SCALE_R = 1.00f;
float SENSOR_SCALE_G = 0.70f;
float SENSOR_SCALE_B = 0.90f;

// LED output channel balance (tune to match your specific LED hardware)
// Red and Green LEDs naturally have lower forward voltages than Blue,
// making them much brighter at 3.3V. This scales them down to match.
const float LED_SCALE_R = 1.00f;
const float LED_SCALE_G = 0.55f;
const float LED_SCALE_B = 0.85f;

// Transition
// Fraction of remaining distance moved each tick.
// 0.05 ≈ 2–3 s fade, 0.10 ≈ 1 s, 0.02 ≈ 5 s
const float LERP_SPEED = 0.05f;
//********************************************

//************* GLOBALS **********************
// Sensor instance
// Start at GAIN_1X — RGB LED adds extra ambient light, higher gain saturates.
Adafruit_TCS34725 tcs =
    Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_300MS, TCS34725_GAIN_1X);

// Current (displayed) LED colour — stored as floats for smooth lerp
float currentR = 0.0f;
float currentG = 0.0f;
float currentB = 0.0f;

// Scan timing
unsigned long lastScanMs = 0;
int scanCount = 0; // Counts scans; used to throttle AutoRange
//********************************************

//****************** SET LED FN **************
// Writes a colour to the RGB LED via PWM.
// analogWriteRange(255) is called in setup(), so r/g/b map directly.
// LED_SCALE constants are applied here to balance hardware brightness.
void SetLED(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t rScaled = (uint8_t)min(r * LED_SCALE_R, 255.0f);
  uint8_t gScaled = (uint8_t)min(g * LED_SCALE_G, 255.0f);
  uint8_t bScaled = (uint8_t)min(b * LED_SCALE_B, 255.0f);

  analogWrite(LedR, rScaled);
  analogWrite(LedG, gScaled);
  analogWrite(LedB, bScaled);
}
//********************************************

//************* READ COLOUR FN ***************
// Reads sensor and returns 8-bit RGB. Guards against I2C glitch.
void ReadColour(uint8_t *outR, uint8_t *outG, uint8_t *outB) {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  // Sensor I2C lockup returns all 65535 — re-init and bail
  if (r == 65535 && g == 65535 && b == 65535) {
    Serial.println("[WARN] Sensor glitch — re-initializing I2C");
    tcs.begin();
    *outR = 0;
    *outG = 0;
    *outB = 0;
    return;
  }

  float red, green, blue;
  tcs.getRGB(&red, &green, &blue);

  // Apply sensor-side white balance calibration
  red *= SENSOR_SCALE_R;
  green *= SENSOR_SCALE_G;
  blue *= SENSOR_SCALE_B;

  *outR = (uint8_t)min((uint16_t)red, (uint16_t)255);
  *outG = (uint8_t)min((uint16_t)green, (uint16_t)255);
  *outB = (uint8_t)min((uint16_t)blue, (uint16_t)255);
}
//********************************************

//******* BLACK LEVEL CORRECTION FN **********
// Removes the grey floor caused by sensor spectral overlap.
// Subtracts the minimum channel, then rescales so max = 255.
// Skipped when chroma is below GREY_THRESHOLD to avoid noise amplification
// on neutral (white/grey/black) surfaces.
void BlackLevelCorrection(uint8_t &r, uint8_t &g, uint8_t &b) {
  uint8_t minVal = min(r, min(g, b));
  r -= minVal;
  g -= minVal;
  b -= minVal;

  uint8_t maxVal = max(r, max(g, b));
  if (maxVal < GREY_THRESHOLD) {
    // Neutral surface — restore and return unchanged
    r += minVal;
    g += minVal;
    b += minVal;
    return;
  }

  // Rescale so the dominant channel reaches full brightness
  r = (uint8_t)((uint16_t)r * 255 / maxVal);
  g = (uint8_t)((uint16_t)g * 255 / maxVal);
  b = (uint8_t)((uint16_t)b * 255 / maxVal);
}
//********************************************

//********** GAMMA CORRECTION FN *************
// Applies sRGB gamma (exponent 1/2.2) for perceptually correct LED output.
uint8_t ApplyGamma(float linearVal) {
  if (linearVal <= 0.0f)
    return 0;
  if (linearVal >= 255.0f)
    return 255;
  float srgb = 255.0f * pow(linearVal / 255.0f, 1.0f / GAMMA);
  return (uint8_t)min((float)round(srgb), 255.0f);
}
//********************************************

//********* SATURATION BOOST FN **************
// Pushes washed-out sensor readings toward vivid colours.
// factor > 1.0 increases saturation.
void BoostSaturation(uint8_t &r, uint8_t &g, uint8_t &b, float factor) {
  float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
  float maxC = max(rf, max(gf, bf));
  float delta = maxC - min(rf, min(gf, bf));

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

  s = min(s * factor, 1.0f);

  float c2 = v * s;
  float x = c2 * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
  float m = v - c2;
  float r1, g1, b1;
  int hi = (int)(h / 60.0f) % 6;

  switch (hi) {
  case 0:
    r1 = c2;
    g1 = x;
    b1 = 0;
    break;
  case 1:
    r1 = x;
    g1 = c2;
    b1 = 0;
    break;
  case 2:
    r1 = 0;
    g1 = c2;
    b1 = x;
    break;
  case 3:
    r1 = 0;
    g1 = x;
    b1 = c2;
    break;
  case 4:
    r1 = x;
    g1 = 0;
    b1 = c2;
    break;
  default:
    r1 = c2;
    g1 = 0;
    b1 = x;
    break;
  }

  r = (uint8_t)round((r1 + m) * 255.0f);
  g = (uint8_t)round((g1 + m) * 255.0f);
  b = (uint8_t)round((b1 + m) * 255.0f);
}
//********************************************

//************** AUTO RANGE FN ***************
// Adjusts TCS34725 gain based on clear-channel brightness.
// Capped at GAIN_4X — the RGB LED adds ambient light so higher gains
// saturate the sensor and produce false white readings.
void AutoRange() {
  uint16_t r, g, b, c;
  tcs.setGain(TCS34725_GAIN_4X); // Safe probe gain with LED on
  delay(350);                    // One full integration cycle
  tcs.getRawData(&r, &g, &b, &c);

  if (c >= 2000) {
    // Bright/white surface — step down to avoid clipping
    tcs.setGain(TCS34725_GAIN_1X);
    delay(350);
    Serial.println("[AUTO] Gain 1X (bright surface)");
  } else {
    // Gain already correct — no second settle needed
    Serial.println("[AUTO] Gain 4X (dark/medium surface)");
  }
}
//********************************************

//****** SAMPLE AND PROCESS COLOUR FN ********
// LED stays ON throughout. AutoRange runs every AUTO_RANGE_EVERY scans to
// avoid its settle delay blocking the loop too often.
// Pipeline: trimmed mean -> black-level correction -> saturation boost ->
// gamma.
void SampleAndProcessColour(uint8_t &outR, uint8_t &outG, uint8_t &outB) {
  // Dynamic gain (throttled — AutoRange may block ~350-700 ms)
  scanCount++;
  if (scanCount >= AUTO_RANGE_EVERY) {
    AutoRange();
    scanCount = 0;
  }

  // Collect NUM_SAMPLES readings (LED stays on)
  uint8_t rS[NUM_SAMPLES], gS[NUM_SAMPLES], bS[NUM_SAMPLES];

  for (int i = 0; i < NUM_SAMPLES; i++) {
    ReadColour(&rS[i], &gS[i], &bS[i]);
    if (i < NUM_SAMPLES - 1)
      delay(SAMPLE_DELAY_MS);
  }

  // Sort each channel to prepare for trimmed mean (bubble sort)
  for (int i = 0; i < NUM_SAMPLES - 1; i++) {
    for (int j = i + 1; j < NUM_SAMPLES; j++) {
      if (rS[i] > rS[j]) {
        uint8_t t = rS[i];
        rS[i] = rS[j];
        rS[j] = t;
      }
      if (gS[i] > gS[j]) {
        uint8_t t = gS[i];
        gS[i] = gS[j];
        gS[j] = t;
      }
      if (bS[i] > bS[j]) {
        uint8_t t = bS[i];
        bS[i] = bS[j];
        bS[j] = t;
      }
    }
  }

  // Trimmed mean: drop min and max, average the middle 3
  float avgR = 0, avgG = 0, avgB = 0;
  for (int i = 1; i < NUM_SAMPLES - 1; i++) {
    avgR += rS[i];
    avgG += gS[i];
    avgB += bS[i];
  }
  const float divisor = (float)(NUM_SAMPLES - 2);
  avgR /= divisor;
  avgG /= divisor;
  avgB /= divisor;

  uint8_t fR = (uint8_t)avgR;
  uint8_t fG = (uint8_t)avgG;
  uint8_t fB = (uint8_t)avgB;

  // Stage 1: black-level correction — strips grey floor from sensor overlap
  BlackLevelCorrection(fR, fG, fB);

  // Stage 2: saturation boost — applied in linear space before gamma
  BoostSaturation(fR, fG, fB, SATURATION_BOOST);

  // Stage 3: gamma correction — applied last as final LED-space conversion
  fR = ApplyGamma(fR);
  fG = ApplyGamma(fG);
  fB = ApplyGamma(fB);

  outR = fR;
  outG = fG;
  outB = fB;
}
//********************************************

//****** COLOUR DISTANCE SQUARED FN **********
// Returns squared Euclidean distance in RGB space.
// Avoids sqrt() — compare directly against CHANGE_THRESHOLD_SQ in loop().
uint32_t ColourDistanceSq(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2,
                          uint8_t g2, uint8_t b2) {
  int32_t dr = (int32_t)r2 - r1;
  int32_t dg = (int32_t)g2 - g1;
  int32_t db = (int32_t)b2 - b1;
  return (uint32_t)(dr * dr + dg * dg + db * db);
}
//********************************************

//****** SMOOTH TRANSITION FN ****************
// Lerps the current LED colour toward the target.
// Runs until within 1 unit per channel, then snaps.
void TransitionToColour(uint8_t targetR, uint8_t targetG, uint8_t targetB) {
  char hex[8];
  sprintf(hex, "%02X%02X%02X", targetR, targetG, targetB);
  Serial.print("[FADE] → #");
  Serial.println(hex);

  float tR = (float)targetR;
  float tG = (float)targetG;
  float tB = (float)targetB;

  while (true) {
    currentR += (tR - currentR) * LERP_SPEED;
    currentG += (tG - currentG) * LERP_SPEED;
    currentB += (tB - currentB) * LERP_SPEED;

    // Snap once close enough — avoids infinite asymptotic approach
    if (fabs(tR - currentR) < 1.0f && fabs(tG - currentG) < 1.0f &&
        fabs(tB - currentB) < 1.0f) {
      currentR = tR;
      currentG = tG;
      currentB = tB;
      SetLED(targetR, targetG, targetB);
      break;
    }

    SetLED((uint8_t)currentR, (uint8_t)currentG, (uint8_t)currentB);
    delay(TRANSITION_STEP_MS);
  }
}
//********************************************

//************** SETUP FUNCTION **************
void setup() {
  //*********** SERIAL **************************
  Serial.begin(9600);
  Serial.println("\n");
  //*********************************************

  //*********** LED PINS ************************
  analogWriteRange(255); // Lock PWM to 0–255 on all ESP8266 core versions
  pinMode(LedR, OUTPUT);
  pinMode(LedG, OUTPUT);
  pinMode(LedB, OUTPUT);
  SetLED(0, 0, 0);
  //*********************************************

  //*********** COLOUR SENSOR *******************
  Serial.println("####### Chameleon Lamp v1.2 #######");
  Wire.begin(SDA_PIN, SCL_PIN);

  if (tcs.begin()) {
    Serial.println("Colour sensor OK");
  } else {
    Serial.println("ERROR: Colour sensor not found! Check wiring.");
    while (true) {
      SetLED(255, 0, 0);
      delay(300);
      SetLED(0, 0, 0);
      delay(300);
    }
  }

  tcs.setInterrupt(false); // Keep sensor LED on for illumination
  //*********************************************

  //*********** LED SELF-TEST *******************
  // Cycles R → G → B → White before scanning.
  // If any colour is wrong or missing, check that channel's wiring.
  Serial.println("LED self-test: R → G → B → White");
  SetLED(255, 0, 0);
  delay(800); // Should be RED
  SetLED(0, 255, 0);
  delay(800); // Should be GREEN
  SetLED(0, 0, 255);
  delay(800); // Should be BLUE
  SetLED(255, 255, 255);
  delay(800); // Should be WHITE
  SetLED(0, 0, 0);
  delay(200);
  Serial.println("Self-test done. Lamp scanning...");
  Serial.println("###################################\n");
  //*********************************************

  // Start from black so the first scan always triggers a full transition
  currentR = 0;
  currentG = 0;
  currentB = 0;

  lastScanMs = millis();
}
//*******************************************

//************** LOOP FUNCTION **************
void loop() {
  unsigned long now = millis();

  // Only scan on interval to avoid hammering the sensor
  if (now - lastScanMs < (unsigned long)SCAN_INTERVAL_MS)
    return;
  lastScanMs = now;

  // Sample surface colour
  uint8_t targetR, targetG, targetB;
  SampleAndProcessColour(targetR, targetG, targetB);

  // Squared distance — no sqrt() needed for threshold comparison
  uint32_t distSq =
      ColourDistanceSq((uint8_t)currentR, (uint8_t)currentG, (uint8_t)currentB,
                       targetR, targetG, targetB);

  // Single-line scan output
  char buf[56];
  sprintf(buf, "[SCAN] R=%d G=%d B=%d  #%02X%02X%02X  dist=%.1f  -> %s",
          targetR, targetG, targetB, targetR, targetG, targetB,
          sqrtf((float)distSq),
          distSq <= CHANGE_THRESHOLD_SQ ? "STABLE" : "TRANSITIONING");
  Serial.println(buf);

  // Check for serial 'W' calibration command
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'w' || c == 'W') {
      Serial.println("\n[CAL] =========================================");
      Serial.println("[CAL] White Balance Calibration Triggered");

      // Temporarily disable scaling to read raw uncorrected values
      float old_r = SENSOR_SCALE_R;
      float old_g = SENSOR_SCALE_G;
      float old_b = SENSOR_SCALE_B;
      SENSOR_SCALE_R = 1.0f;
      SENSOR_SCALE_G = 1.0f;
      SENSOR_SCALE_B = 1.0f;

      uint8_t rawR, rawG, rawB;
      ReadColour(&rawR, &rawG, &rawB);

      SENSOR_SCALE_R = old_r;
      SENSOR_SCALE_G = old_g;
      SENSOR_SCALE_B = old_b;

      Serial.print("[CAL] Raw: R=");
      Serial.print(rawR);
      Serial.print(" G=");
      Serial.print(rawG);
      Serial.print(" B=");
      Serial.println(rawB);

      if (rawR > 0 && rawG > 0 && rawB > 0) {
        float minChan = min(rawR, min(rawG, rawB));
        Serial.print("[CAL] Suggested SENSOR_SCALE_R = ");
        Serial.println(minChan / rawR, 2);
        Serial.print("[CAL] Suggested SENSOR_SCALE_G = ");
        Serial.println(minChan / rawG, 2);
        Serial.print("[CAL] Suggested SENSOR_SCALE_B = ");
        Serial.println(minChan / rawB, 2);
        Serial.println("[CAL] Update TUNING PARAMETERS with these values.");
      }
      Serial.println("[CAL] =========================================");
    }
  }

  // Dead-zone: ignore noise on stable surfaces
  if (distSq <= CHANGE_THRESHOLD_SQ)
    return;

  // Smooth transition
  TransitionToColour(targetR, targetG, targetB);
}
//*******************************************
