//************* HEADER FILES *****************
#include <Adafruit_TCS34725.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <Servo.h>
#include <Wire.h>
//********************************************

//*************** PIN SETUP ******************
// I2C colour sensor pins
#define SDA_PIN 5 // D1 - Blue
#define SCL_PIN 4 // D2 - Green
// Servo pins
int TopServoPin = 14;   // D5 - Yellow
int SlideServoPin = 12; // D6 - Purple
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

//************** WIFI CONFIG *****************
const char *WifiSsid = "CANDY_SORTER";
const char *WifiPassword = "candy1234";
IPAddress LocalIp(192, 168, 4, 1);
IPAddress Gateway(192, 168, 4, 1);
IPAddress Subnet(255, 255, 255, 0);
//********************************************

//************* COLOUR STRUCT ****************
// Structure to store colour names, RGB values & angle
struct Color {
  const char *name;
  uint16_t r, g, b;
  int angle;
  int count;
};
//********************************************

//************* COLOUR VALUES ****************
// Table to compare sensed colour values
Color colors[] = {
    {"Orange", 255, 184, 145, 34, 0}, {"Purple", 255, 156, 185, 52, 0},
    {"Red", 255, 109, 91, 70, 0},     {"Yellow", 255, 215, 81, 90, 0},
    {"Green", 199, 255, 111, 110, 0}, {"Blue", 144, 237, 255, 128, 0},
    {"Pink", 255, 148, 145, 146, 0},  {"Blank", 255, 171, 125, -1, 0}};
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
short int TopServoOffset = 5;
short int SlideServoOffset = -4;
//********************************************

//************ TIMING PARAMETERS *************
int collectDelay = 800;
int moveDelay = 1000;
int dispenseDelay = 800;
int slideDelay = 1000;
int stepDelay = 300;
//********************************************

//************* STATE VARIABLES **************
bool IsSorting = false;
int TopServoAngle = 0;
int SlideServoAngle = 0;
int SortPhase = 0;
unsigned long PhaseStartTime = 0;
unsigned int SortCount = 0;
int DetectedAngle = 0;
// Circular log buffer (last 20 lines)
#define LOG_MAX 20
String LogLines[LOG_MAX];
int LogHead = 0;
int LogCount = 0;
//********************************************

//************* INITIALISING *****************
// Top servo instance
Servo TopServo;
// Slide servo instance
Servo SlideServo;
// Colour sensor instance
Adafruit_TCS34725 tcs =
    Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_300MS, TCS34725_GAIN_1X);
// Web server instance
ESP8266WebServer Server(80);
//********************************************

//*************** LOG FN *********************
// Appends a line to the circular log buffer
void AddLog(String msg) {
  LogLines[LogHead] = msg;
  LogHead = (LogHead + 1) % LOG_MAX;
  if (LogCount < LOG_MAX)
    LogCount++;
  Serial.println(msg);
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

//**************** CLEAR LED *****************
// Turns off the RGB LED
void ClearLed() {
  SetLED(0, 0, 0);
  AddLog("LED cleared");
}
//********************************************

//************** SET LED COLOR ***************
// Accepts the colour name and drives the RGB LED
void SetLedTo(const char *colorName) {
  for (LedColor lc : ledColors) {
    if (strcmp(lc.name, colorName) == 0) {
      SetLED(lc.r, lc.g, lc.b);
      AddLog("LED set to " + String(lc.name));
      return;
    }
  }
  // If no match found, turn off the LED
  ClearLed();
}
//********************************************

//************ ANIMATE EMPTY LED *************
// Flashes LED red to indicate empty tray
void AnimateEmpty() {
  AddLog("Flashing red!");
  for (int i = 0; i < 5; i++) {
    SetLED(255, 0, 0);
    delay(500);
    Server.handleClient();
    SetLED(0, 0, 0);
    delay(500);
    Server.handleClient();
  }
}
//********************************************

//******* CALCULATE COLOUR DISTANCE FN *******
float calculateDistance(uint16_t r1, uint16_t g1, uint16_t b1, uint16_t r2,
                        uint16_t g2, uint16_t b2) {
  return sqrt(sq((float)r2 - r1) + sq((float)g2 - g1) + sq((float)b2 - b1));
}
//********************************************

//************ COLLECT CANDY FN **************
void CollectCandy() {
  TopServo.attach(TopServoPin, 544, 2400);
  TopServoAngle = 160 + TopServoOffset;
  TopServo.write(TopServoAngle);
  AddLog("Collecting Candy");
  delay(collectDelay);
  TopServo.detach();
}
//********************************************

//******** MOVE CANDY TO SENSOR FN ***********
void MoveCandyToSensor() {
  TopServo.attach(TopServoPin, 544, 2400);
  TopServoAngle = 0 + TopServoOffset;
  TopServo.write(TopServoAngle);
  AddLog("Moving to colour sensor");
  delay(moveDelay);
  TopServo.detach();
}
//********************************************

//********** DISPENSE CANDY FN ***************
void DispenseCandy() {
  TopServo.attach(TopServoPin, 544, 2400);
  TopServoAngle = 50 + TopServoOffset;
  TopServo.write(TopServoAngle);
  AddLog("Dispensing");
  delay(dispenseDelay);
  TopServo.detach();
}
//********************************************

//************* SENSE COLOUR FN **************
int SenseColour() {
  AddLog("Sensing Colour");
  // The sensor has a 300ms integration time. Add a delay here
  // to ensure the sensor reads the *current* candy and not the previous one.
  delay(350);

  uint16_t r, g, b, c;

  // Reading color data from the sensor
  tcs.getRawData(&r, &g, &b, &c);

  // Check for I2C lockup / power brownout (sensor returns all 1s = 65535)
  if (r == 65535 && g == 65535 && b == 65535) {
    AddLog("Sensor glitch detected!");
    AddLog("Re-initializing I2C...");
    tcs.begin();
    return -1; // Treat as empty/error to prevent false dispensing
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
  float minDistance = 1e6;
  const char *closestColor = "Unknown";
  int colorAngle = -1;
  int colorIndex = -1;

  for (int i = 0; i < 8; i++) {
    float distance = calculateDistance(normR, normG, normB, colors[i].r,
                                       colors[i].g, colors[i].b);
    if (distance < minDistance) {
      minDistance = distance;
      closestColor = colors[i].name;
      colorAngle = colors[i].angle;
      colorIndex = i;
    }
  }

  AddLog("R:" + String(normR) + " G:" + String(normG) + " B:" + String(normB));

  // Print the closest color
  if (colorAngle >= 0 && colorIndex >= 0) {
    AddLog("Colour is " + String(closestColor));
    // Increment count (cap at 8)
    if (colors[colorIndex].count < 8) {
      colors[colorIndex].count++;
    }
    SetLedTo(closestColor);
  } else {
    AddLog("No candy detected!");
    ClearLed();
  }

  return colorAngle;
}
//********************************************

//******** MOVE SLIDE TO ANGLE FN ************
void MoveSlideTo(int angle) {
  if (angle >= 0) {
    SlideServo.attach(SlideServoPin, 544, 2400);
    SlideServoAngle = angle + SlideServoOffset;
    AddLog("Moving slide to " + String(SlideServoAngle));
    SlideServo.write(SlideServoAngle);
    delay(slideDelay);
    SlideServo.detach();
  }
}
//********************************************

//********** RUN SORT STEP FN ****************
// Non-blocking sort state machine
void RunSortStep() {
  if (!IsSorting)
    return;

  switch (SortPhase) {
  case 1:
    SortCount++;
    AddLog("#### LOOP " + String(SortCount) + " ####");
    CollectCandy();
    delay(stepDelay); // Allow power to stabilize
    Server.handleClient();
    if (!IsSorting)
      return;
    SortPhase = 2;
    break;
  case 2:
    MoveCandyToSensor();
    delay(stepDelay); // Allow power to stabilize before sensing
    Server.handleClient();
    if (!IsSorting)
      return;
    SortPhase = 3;
    break;
  case 3:
    DetectedAngle = SenseColour();
    delay(stepDelay); // Allow power to stabilize
    Server.handleClient();
    if (!IsSorting)
      return;
    if (DetectedAngle < 0) {
      AddLog("Tray empty or sensor error");
      AnimateEmpty();
      AddLog("Stopping...");
      AddLog("Sorted " + String(SortCount > 0 ? SortCount - 1 : 0) +
             " candies");
      IsSorting = false;
      SortPhase = 0;
      return;
    }
    SortPhase = 4;
    break;
  case 4:
    MoveSlideTo(DetectedAngle);
    delay(stepDelay); // Allow power to stabilize
    Server.handleClient();
    if (!IsSorting)
      return;
    SortPhase = 5;
    break;
  case 5:
    DispenseCandy();
    delay(stepDelay); // Allow power to stabilize
    Server.handleClient();
    ClearLed();
    if (!IsSorting)
      return;
    SortPhase = 1;
    break;
  }
}
//********************************************

//************* BUILD LOG JSON ***************
// Builds the log string from the circular buffer
String BuildLogJson() {
  String out = "";
  int start = (LogCount < LOG_MAX) ? 0 : LogHead;
  for (int i = 0; i < LogCount; i++) {
    int idx = (start + i) % LOG_MAX;
    if (i > 0)
      out += "\\n";
    // Escape any quotes in the log line
    String line = LogLines[idx];
    line.replace("\"", "\\\"");
    out += "> " + line;
  }
  return out;
}
//********************************************

//*************** WEB PAGE *******************
#include "index.h"
//********************************************

//************ HANDLE ROOT FN ****************
void HandleRoot() { Server.send_P(200, "text/html", PAGE_HTML); }
//********************************************

//*********** HANDLE STATUS FN ***************
void HandleStatus() {
  String json = "{";
  json += "\"topAngle\":" + String(TopServoAngle) + ",";
  json += "\"slideAngle\":" + String(SlideServoAngle) + ",";
  json += "\"sorting\":" + String(IsSorting ? "true" : "false") + ",";
  json += "\"count\":" + String(SortCount) + ",";
  json += "\"colors\":[";
  // Only include the 7 real colours (skip Blank at index 7)
  for (int i = 0; i < 7; i++) {
    if (i > 0)
      json += ",";
    json += "{\"name\":\"" + String(colors[i].name) +
            "\",\"count\":" + String(colors[i].count) + "}";
  }
  json += "],";
  json += "\"log\":\"" + BuildLogJson() + "\"";
  json += "}";
  Server.send(200, "application/json", json);
}
//********************************************

//*********** HANDLE START FN ****************
void HandleStart() {
  if (!IsSorting) {
    IsSorting = true;
    SortPhase = 1;
    SortCount = 0;
    // Reset colour counts
    for (int i = 0; i < 8; i++) {
      colors[i].count = 0;
    }
    AddLog("Sorting started!");
  }
  Server.send(200, "application/json", "{\"ok\":true}");
}
//********************************************

//************ HANDLE STOP FN ****************
void HandleStop() {
  if (IsSorting) {
    IsSorting = false;
    SortPhase = 0;
    AddLog("Sorting stopped by user");
    AddLog("Sorted " + String(SortCount > 0 ? SortCount - 1 : 0) + " candies");
  }
  Server.send(200, "application/json", "{\"ok\":true}");
}
//********************************************

//************** SETUP FUNCTION **************
void setup() {

  //************* SERIAL SETUP ***************
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

  //************** WIFI AP SETUP *************
  Serial.println("######### WiFi AP Setup #########");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(LocalIp, Gateway, Subnet);
  WiFi.softAP(WifiSsid, WifiPassword);
  Serial.print("SSID: ");
  Serial.println(WifiSsid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");
  //******************************************

  //*********** COLOUR SENSOR SETUP **********
  Serial.println("###### Colour Sensor Setup ######");
  Wire.begin(SDA_PIN, SCL_PIN);

  if (tcs.begin()) {
    Serial.println("Colour sensor initialized!");
    AddLog("Colour sensor initialized!");
  } else {
    Serial.println("Error, check your connections!");
    AddLog("Sensor error!");
  }
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");

  tcs.setInterrupt(false);
  //*****************************************

  //************ WEB SERVER ROUTES ***********
  Server.on("/", HandleRoot);
  Server.on("/status", HTTP_GET, HandleStatus);
  Server.on("/start", HTTP_POST, HandleStart);
  Server.on("/stop", HTTP_POST, HandleStop);
  Server.begin();
  AddLog("Web server started on 192.168.4.1");
  //*****************************************
}
//*******************************************

//************** LOOP FUNCTION **************
void loop() {
  Server.handleClient();

  // Non-blocking sort state machine
  if (IsSorting && SortPhase > 0) {
    RunSortStep();
  }
}
//*******************************************
