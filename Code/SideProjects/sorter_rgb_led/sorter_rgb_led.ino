//************* HEADER FILES *****************
#include <Adafruit_TCS34725.h>
#include <Servo.h>
#include <Wire.h>
//********************************************

//*************** PIN SETUP ******************
// I2C colour sensor pins
#define SDA_PIN 5 // D1 - Blue
#define SCL_PIN 4 // D2 - Green
// Servo pins
#define TopServoPin 14   // D5 - Yellow
#define SlideServoPin 12 // D6 - Purple
// Led Pins
#define LedR 13 // D7
#define LedG 15 // D8
#define LedB 0  // D3
//********************************************

//************* TUNING PARAMETERS ************
// LED output channel balance (tune to match your specific LED hardware)
// Red and Green LEDs naturally have lower forward voltages than Blue,
// making them much brighter at 3.3V. This scales them down to match.
const float LED_SCALE_R = 1.00f;
const float LED_SCALE_G = 0.55f;
const float LED_SCALE_B = 0.85f;
//********************************************

//************* COLOUR STRUCT ****************
// Structure to store colour names, RGB values & slide angle
struct Color {
  const char *name;
  uint16_t r, g, b;
  int angle;
};
//********************************************

//************* COLOUR VALUES ****************
// Table to compare sensed colour values
Color colors[] = {{"Pink", 255, 148, 145, 146},  {"Blue", 144, 237, 255, 128},
                  {"Green", 199, 255, 111, 110}, {"Yellow", 255, 215, 81, 90},
                  {"Red", 255, 109, 91, 70},     {"Purple", 255, 156, 185, 52},
                  {"Orange", 255, 184, 145, 34}, {"Blank", 255, 171, 125, -1}};
//********************************************

//************* LED COLOUR STRUCT ************
// Structure to store LED display colour values
struct LedColor {
  const char *name;
  uint8_t r, g, b;
};
//********************************************

//*********** LED COLOUR VALUES **************
// Deep saturated colours for RGB LED display
LedColor ledColors[] = {{"Pink", 255, 0, 40},  {"Blue", 0, 0, 255},
                        {"Green", 0, 255, 0},  {"Yellow", 255, 150, 0},
                        {"Red", 255, 0, 0},    {"Purple", 180, 0, 255},
                        {"Orange", 255, 40, 0}};
//********************************************

//************ SERVO OFFSETS *****************
// Adjust these offsets to fine-tune servo alignment
short int TopServoOffset = 0;
short int SlideServoOffset = -2;
//********************************************

//************ TIMING PARAMETERS *************
int collectDelay = 800;
int moveDelay = 1000;
int dispenseDelay = 800;
int slideDelay = 1000;
int stepDelay = 300;
//********************************************

//************* INITIALISING *****************
// Top servo instance
Servo TopServo;
// Slide servo instance
Servo SlideServo;
// Colour sensor instance
Adafruit_TCS34725 tcs =
    Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_300MS, TCS34725_GAIN_1X);
//********************************************

//****** CALCULATE COLOUR DISTANCE FN ********
float calculateDistance(uint16_t r1, uint16_t g1, uint16_t b1, uint16_t r2,
                        uint16_t g2, uint16_t b2) {
  return sqrt(sq((float)r2 - r1) + sq((float)g2 - g1) + sq((float)b2 - b1));
}
//********************************************

//************ COLLECT GEMS FN ***************
void CollectGems() {
  TopServo.attach(TopServoPin, 544, 2400);
  TopServo.write(160 + TopServoOffset);
  Serial.println("Collecting Gems");
  delay(collectDelay);
  TopServo.detach();
}
//********************************************

//********* MOVE GEMS TO SENSOR FN ***********
void MoveToColorSensor() {
  TopServo.attach(TopServoPin, 544, 2400);
  TopServo.write(TopServoOffset);
  Serial.println("Moving it to the colour sensor");
  delay(moveDelay);
  TopServo.detach();
}
//********************************************

//*********** DISPENSE GEMS FN ***************
void DispenseGems() {
  TopServo.attach(TopServoPin, 544, 2400);
  TopServo.write(50 + TopServoOffset);
  Serial.println("Dispensing");
  delay(dispenseDelay);
  TopServo.detach();
}
//********************************************

//************* SENSE COLOUR FN **************
// Senses the colour and returns the colour name
const char *SenseColour() {
  Serial.println("Sensing Colour");

  uint16_t r, g, b, c;

  // Reading color data from the sensor
  tcs.getRawData(&r, &g, &b, &c);

  // Check for I2C lockup / power brownout (sensor returns all 1s = 65535)
  if (r == 65535 && g == 65535 && b == 65535) {
    Serial.println("Sensor glitch detected! Re-initializing I2C...");
    tcs.begin();
    return "Blank"; // Return Blank on error
  }

  // Normalize RGB values (scale to 255 based on the highest value)
  uint16_t maxVal = max(r, max(g, b));
  uint16_t normR = (maxVal > 0) ? (r * 255 / maxVal) : 0;
  uint16_t normG = (maxVal > 0) ? (g * 255 / maxVal) : 0;
  uint16_t normB = (maxVal > 0) ? (b * 255 / maxVal) : 0;

  // Clamp normalized values to 0–255
  normR = min(normR, (uint16_t)255);
  normG = min(normG, (uint16_t)255);
  normB = min(normB, (uint16_t)255);

  // Determine the closest color
  float minDistance = 1e6; // Large number for initialization
  const char *closestColor = "Unknown";
  int colorAngle = -1; // Default value if no match found

  for (const Color &color : colors) {
    float distance =
        calculateDistance(normR, normG, normB, color.r, color.g, color.b);
    if (distance < minDistance) {
      minDistance = distance;
      closestColor = color.name;
      colorAngle = color.angle;
    }
  }
  Serial.println("--------------------------------");
  Serial.print("Red: ");
  Serial.print(normR);
  Serial.print(" Green: ");
  Serial.print(normG);
  Serial.print(" Blue: ");
  Serial.println(normB);

  // Print the closest color
  if (colorAngle >= 0) {
    Serial.print("Colour is ");
    Serial.println(closestColor);
    Serial.println("--------------------------------");
  } else {
    Serial.println("--------------------------------");
    Serial.println("No candy detected!");
  }

  return closestColor; // Return the colour name
}
//********************************************

//******* GET COLOUR ANGLE FN ****************
// Returns the slide angle for a given colour name
int GetColorAngle(const char *colorName) {
  for (Color color : colors) {
    if (strcmp(color.name, colorName) == 0) {
      return color.angle;
    }
  }
  return -1;
}
//********************************************

//******** MOVE SLIDE TO ANGLE FN ************
void MoveSlideTo(int angle) {
  if (angle >= 0) {
    SlideServo.attach(SlideServoPin, 544, 2400);
    Serial.print("Moving slide to ");
    Serial.println(angle + SlideServoOffset);
    SlideServo.write(angle + SlideServoOffset);
    delay(slideDelay);
    SlideServo.detach();
  }
}
//********************************************

//*********** CALIBRATE SERVOS FN ************
void CalibrateServos() {
  Serial.println("######## Calibrate Servos #######");

  Serial.print("Moving top servo to 60° + ");
  Serial.print(TopServoOffset);
  Serial.println("° offset");
  TopServo.attach(TopServoPin, 544, 2400);
  TopServo.write(40 + TopServoOffset);
  delay(500);
  TopServo.write(80 + TopServoOffset);
  delay(500);
  TopServo.write(60 + TopServoOffset);
  delay(500);

  Serial.print("Moving slide servo to 90° + ");
  Serial.print(SlideServoOffset);
  Serial.println("° offset");
  SlideServo.attach(SlideServoPin, 544, 2400);
  SlideServo.write(70 + SlideServoOffset);
  delay(500);
  SlideServo.write(110 + SlideServoOffset);
  delay(500);
  SlideServo.write(90 + SlideServoOffset);

  delay(3000);
  SlideServo.detach();
  TopServo.detach();

  Serial.println("Servo calibration complete!");
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");
}
//********************************************

//************** PRINT MENU FN ***************
void PrintMenu() {
  Serial.println("############# Menu ##############");
  Serial.println("Press 'S' to start sorting");
  Serial.println("Press 'C' to calibrate the servos");
  Serial.println("Press 'R' to read the colour");
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");
}
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

//************** SET LED COLOR ***************
// Accepts the colour name and drives the RGB LED
void SetLedTo(const char *colorName) {
  for (LedColor lc : ledColors) {
    if (strcmp(lc.name, colorName) == 0) {
      SetLED(lc.r, lc.g, lc.b);
      Serial.print("LED set to ");
      Serial.println(lc.name);
      return;
    }
  }
  // If no match found, turn off the LED
  ClearLed();
}
//********************************************

//**************** CLEAR LED *****************
// Turns off the RGB LED
void ClearLed() {
  SetLED(0, 0, 0);
  Serial.println("LED cleared");
}
//********************************************

//************ ANIMATE EMPTY LED *************
// Flashes LED red to indicate empty tray
void AnimateEmpty() {
  Serial.println("Flashing red!");
  for (int i = 0; i < 5; i++) {
    SetLED(255, 0, 0);
    delay(500);
    SetLED(0, 0, 0);
    delay(500);
  }
}
//********************************************

//************** SETUP FUNCTION **************
void setup() {

  //***************SERIAL SETUP **************
  // Starting serial communication
  Serial.begin(9600);
  Serial.println("\n");
  //******************************************

  //************** LED PIN SETUP **************
  // Setting LED pins as outputs
  analogWriteRange(255); // Lock PWM to 0–255 on all ESP8266 core versions
  pinMode(LedR, OUTPUT);
  pinMode(LedG, OUTPUT);
  pinMode(LedB, OUTPUT);
  SetLED(0, 0, 0);
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

  // Menu
  PrintMenu();
}
//*******************************************

//************** LOOP FUNCTION **************
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("s")) {
      const char *colorName = "";
      int angle = 0;
      unsigned int count = 0;
      do {
        count++;
        Serial.print("############ LOOP ");
        Serial.print(count);
        Serial.println(" #############");
        CollectGems();
        delay(stepDelay);
        MoveToColorSensor();
        delay(stepDelay);
        colorName = SenseColour();
        angle = GetColorAngle(colorName);
        delay(stepDelay);
        SetLedTo(colorName);
        delay(stepDelay);
        MoveSlideTo(angle);
        delay(stepDelay);
        DispenseGems();
        delay(stepDelay);
        ClearLed();
        Serial.println("#################################");
        Serial.println(" ");
        Serial.println(" ");
      } while (angle >= 0);

      AnimateEmpty();
      ClearLed();

      Serial.println("############ Result #############");
      Serial.print("Number of gems sorted : ");
      Serial.println(count - 1);
      Serial.println("#################################");
      Serial.println(" ");
      Serial.println(" ");
      PrintMenu();
    } else if (input.equalsIgnoreCase("c")) {
      CalibrateServos();
      PrintMenu();
    } else if (input.equalsIgnoreCase("r")) {
      const char *colorName = SenseColour();
      SetLedTo(colorName);
      delay(3000);
      ClearLed();
    }
  }
}
//*******************************************