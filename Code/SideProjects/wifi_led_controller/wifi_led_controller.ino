//************* HEADER FILES *****************
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include "led_page.h"
#include "wifi_setup.h"
//********************************************


//*************** PIN SETUP ******************
// RGB LED (common-cathode) — all PWM-capable, no conflicts
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
// AP settings for setup mode
const char* AP_SSID = "LED_CONTROLLER_SETUP";
IPAddress ApIp(192, 168, 4, 1);
IPAddress ApGateway(192, 168, 4, 1);
IPAddress ApSubnet(255, 255, 255, 0);
//********************************************


//************ EEPROM CONFIG *****************
#define EEPROM_SIZE 97
#define EEPROM_MAGIC_BYTE 0xAB
#define EEPROM_MAGIC_ADDR 0
#define EEPROM_SSID_ADDR 1
#define EEPROM_SSID_LEN 32
#define EEPROM_PASS_ADDR 33
#define EEPROM_PASS_LEN 64
//********************************************


//************ TIMING PARAMETERS *************
int blinkDelay = 500;
int connectTimeout = 20;
//********************************************


//*********** LED STATE VARIABLES ************
bool LedOn = false;
bool BlinkMode = false;
int Brightness = 255;
uint8_t ColorR = 255;
uint8_t ColorG = 255;
uint8_t ColorB = 255;
//********************************************


//*********** BLINK STATE *******************
unsigned long LastBlinkTime = 0;
bool BlinkState = false;
//********************************************


//*********** DEVICE STATE ******************
enum DeviceMode { SETUP_MODE, NORMAL_MODE };
DeviceMode CurrentMode = SETUP_MODE;
//********************************************


//************* INITIALISING *****************
// Web server instance on port 80
ESP8266WebServer server(80);
// DNS server for captive portal
DNSServer DnsServer;
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


//************** APPLY COLOR FN *************
// Applies the current color at the current brightness level
void ApplyColor() {
  uint8_t r = (uint8_t)((ColorR * Brightness) / 255);
  uint8_t g = (uint8_t)((ColorG * Brightness) / 255);
  uint8_t b = (uint8_t)((ColorB * Brightness) / 255);
  SetLED(r, g, b);
}
//********************************************


//************ INIT EEPROM FN ***************
void InitEeprom() {
  EEPROM.begin(EEPROM_SIZE);
}
//********************************************


//******** HAS STORED CREDENTIALS FN ********
bool HasStoredCredentials() {
  return EEPROM.read(EEPROM_MAGIC_ADDR) == EEPROM_MAGIC_BYTE;
}
//********************************************


//********** READ STORED SSID FN ************
String ReadStoredSsid() {
  String ssid = "";
  for (int i = 0; i < EEPROM_SSID_LEN; i++) {
    char c = (char)EEPROM.read(EEPROM_SSID_ADDR + i);
    if (c == 0) break;
    ssid += c;
  }
  return ssid;
}
//********************************************


//******** READ STORED PASSWORD FN **********
String ReadStoredPassword() {
  String pass = "";
  for (int i = 0; i < EEPROM_PASS_LEN; i++) {
    char c = (char)EEPROM.read(EEPROM_PASS_ADDR + i);
    if (c == 0) break;
    pass += c;
  }
  return pass;
}
//********************************************


//********* SAVE CREDENTIALS FN *************
void SaveCredentials(String ssid, String password) {
  // Write magic byte
  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_BYTE);

  // Write SSID (null-terminated, zero-padded)
  for (int i = 0; i < EEPROM_SSID_LEN; i++) {
    if (i < (int)ssid.length()) {
      EEPROM.write(EEPROM_SSID_ADDR + i, ssid[i]);
    } else {
      EEPROM.write(EEPROM_SSID_ADDR + i, 0);
    }
  }

  // Write password (null-terminated, zero-padded)
  for (int i = 0; i < EEPROM_PASS_LEN; i++) {
    if (i < (int)password.length()) {
      EEPROM.write(EEPROM_PASS_ADDR + i, password[i]);
    } else {
      EEPROM.write(EEPROM_PASS_ADDR + i, 0);
    }
  }

  EEPROM.commit();
  Serial.println("Credentials saved to EEPROM");
}
//********************************************


//********* CLEAR CREDENTIALS FN ************
void ClearCredentials() {
  EEPROM.write(EEPROM_MAGIC_ADDR, 0x00);
  EEPROM.commit();
  Serial.println("Credentials cleared from EEPROM");
}
//********************************************


//************** HANDLE ROOT FN **************
void HandleRoot() {
  server.send_P(200, "text/html", LED_PAGE_HTML);
}
//********************************************


//*********** HANDLE STATUS FN **************
void HandleStatus() {
  String json = "{\"on\":";
  json += LedOn ? "true" : "false";
  json += ",\"blink\":";
  json += BlinkMode ? "true" : "false";
  json += ",\"r\":";
  json += String(ColorR);
  json += ",\"g\":";
  json += String(ColorG);
  json += ",\"b\":";
  json += String(ColorB);
  json += ",\"br\":";
  json += String(Brightness);
  json += "}";
  server.send(200, "application/json", json);
}
//********************************************


//************ HANDLE LED ON FN **************
void HandleLedOn() {
  LedOn = true;
  BlinkMode = false;
  ApplyColor();
  Serial.println("LED turned ON");
  server.send(200, "application/json", "{\"ok\":true}");
}
//********************************************


//************ HANDLE LED OFF FN *************
void HandleLedOff() {
  LedOn = false;
  BlinkMode = false;
  SetLED(0, 0, 0);
  Serial.println("LED turned OFF");
  server.send(200, "application/json", "{\"ok\":true}");
}
//********************************************


//*********** HANDLE LED BLINK FN ************
void HandleLedBlink() {
  BlinkMode = true;
  LedOn = false;
  BlinkState = false;
  LastBlinkTime = millis();
  Serial.println("LED set to BLINK mode");
  server.send(200, "application/json", "{\"ok\":true}");
}
//********************************************


//*********** HANDLE COLOR FN ***************
void HandleColor() {
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
    ColorR = constrain(server.arg("r").toInt(), 0, 255);
    ColorG = constrain(server.arg("g").toInt(), 0, 255);
    ColorB = constrain(server.arg("b").toInt(), 0, 255);
    Serial.print("Color set to R:");
    Serial.print(ColorR);
    Serial.print(" G:");
    Serial.print(ColorG);
    Serial.print(" B:");
    Serial.println(ColorB);

    // Auto-turn on when color is changed
    LedOn = true;
    BlinkMode = false;
    ApplyColor();
  }
  server.send(200, "application/json", "{\"ok\":true}");
}
//********************************************


//********* HANDLE BRIGHTNESS FN *************
void HandleBrightness() {
  if (server.hasArg("value")) {
    Brightness = constrain(server.arg("value").toInt(), 0, 255);
    Serial.print("Brightness set to ");
    Serial.println(Brightness);
    if (LedOn && !BlinkMode) {
      ApplyColor();
    }
  }
  server.send(200, "application/json", "{\"ok\":true}");
}
//********************************************


//********** HANDLE NOT FOUND FN *************
void HandleNotFound() {
  server.send(404, "text/plain", "Page not found");
  Serial.println("404 - Page not found");
}
//********************************************


//******** HANDLE RESET WIFI FN *************
void HandleResetWifi() {
  ClearCredentials();
  server.send(200, "text/html",
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>body{font-family:ui-monospace,monospace;text-align:center;"
    "background:#0d0d0d;color:#E0E0E0;padding:40px;}"
    ".card{max-width:400px;margin:0 auto;border:3px solid #FF3B3B;"
    "background:#1a1a1a;padding:24px;box-shadow:5px 5px 0 #8a1a1a;}"
    "h2{color:#FF3B3B;text-transform:uppercase;letter-spacing:2px;"
    "font-size:18px;margin-bottom:12px;}"
    "p{margin:8px 0;font-size:13px;}"
    "b{color:#39FF14;}</style>"
    "</head><body><div class='card'><h2>WiFi Cleared</h2>"
    "<p>The device is rebooting into setup mode...</p>"
    "<p>Connect to <b>LED_CONTROLLER_SETUP</b> WiFi to reconfigure.</p>"
    "</div></body></html>");
  delay(1000);
  ESP.restart();
}
//********************************************


//******** HANDLE SETUP ROOT FN *************
void HandleSetupRoot() {
  server.send_P(200, "text/html", SETUP_PAGE_HTML);
}
//********************************************


//*********** HANDLE SCAN FN ****************
void HandleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";

  // Deduplicate by SSID (keep strongest signal)
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) continue;

    // Check if this SSID was already added with a stronger signal
    bool duplicate = false;
    for (int j = 0; j < i; j++) {
      if (WiFi.SSID(j) == ssid) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) continue;

    if (json.length() > 1) json += ",";
    json += "{\"ssid\":\"";
    // Escape quotes in SSID
    for (unsigned int c = 0; c < ssid.length(); c++) {
      if (ssid[c] == '"') json += "\\\"";
      else if (ssid[c] == '\\') json += "\\\\";
      else json += ssid[c];
    }
    json += "\",\"rssi\":";
    json += String(WiFi.RSSI(i));
    json += ",\"secure\":";
    json += (WiFi.encryptionType(i) != ENC_TYPE_NONE) ? "true" : "false";
    json += "}";
  }
  json += "]";

  WiFi.scanDelete();
  server.send(200, "application/json", json);
  Serial.print("Scan complete, found ");
  Serial.print(n);
  Serial.println(" networks");
}
//********************************************


//********* HANDLE CONNECT FN ***************
void HandleConnect() {
  if (!server.hasArg("ssid")) {
    server.send(400, "text/plain", "Missing SSID");
    return;
  }

  String ssid = server.arg("ssid");
  String password = server.hasArg("password") ? server.arg("password") : "";

  if (ssid.length() == 0 || ssid.length() > EEPROM_SSID_LEN) {
    server.send(400, "text/plain", "Invalid SSID length");
    return;
  }
  if (password.length() > EEPROM_PASS_LEN) {
    server.send(400, "text/plain", "Password too long");
    return;
  }

  SaveCredentials(ssid, password);

  server.send(200, "text/html",
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>body{font-family:ui-monospace,monospace;text-align:center;"
    "background:#0d0d0d;color:#E0E0E0;padding:40px;}"
    ".card{max-width:400px;margin:0 auto;border:3px solid #39FF14;"
    "background:#1a1a1a;padding:24px;box-shadow:5px 5px 0 #2a8a14;}"
    "h2{color:#39FF14;text-transform:uppercase;letter-spacing:2px;"
    "font-size:18px;margin-bottom:12px;}"
    "p{margin:8px 0;font-size:13px;}</style>"
    "</head><body><div class='card'><h2>Credentials Saved</h2>"
    "<p>Rebooting and connecting to your network...</p>"
    "<p>Check your router or serial monitor for the new IP address.</p>"
    "</div></body></html>");

  Serial.print("Credentials saved for SSID: ");
  Serial.println(ssid);
  Serial.println("Rebooting...");
  delay(1000);
  ESP.restart();
}
//********************************************


//******* HANDLE CAPTIVE PORTAL FN **********
void HandleCaptivePortal() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}
//********************************************


//************* CONNECT WIFI FN **************
bool ConnectWifi(String ssid, String password) {
  Serial.println("######### WiFi Setup ############");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < connectTimeout) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  Serial.println(" ");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("#################################");
    Serial.println(" ");
    Serial.println(" ");
    return true;
  } else {
    Serial.println("Could not connect to WiFi");
    Serial.println("#################################");
    Serial.println(" ");
    Serial.println(" ");
    return false;
  }
}
//********************************************


//************ SETUP ROUTES FN ***************
void SetupRoutes() {
  server.on("/", HandleRoot);
  server.on("/status", HTTP_GET, HandleStatus);
  server.on("/on", HandleLedOn);
  server.on("/off", HandleLedOff);
  server.on("/blink", HandleLedBlink);
  server.on("/color", HandleColor);
  server.on("/brightness", HandleBrightness);
  server.on("/reset-wifi", HandleResetWifi);
  server.onNotFound(HandleNotFound);
}
//********************************************


//********* START SETUP MODE FN *************
void StartSetupMode() {
  Serial.println("####### WiFi Setup Mode #########");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ApIp, ApGateway, ApSubnet);
  WiFi.softAP(AP_SSID);
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Start DNS server for captive portal (redirect all domains to AP IP)
  DnsServer.start(53, "*", ApIp);

  // Setup mode routes
  server.on("/", HandleSetupRoot);
  server.on("/scan", HTTP_GET, HandleScan);
  server.on("/connect", HTTP_POST, HandleConnect);
  server.onNotFound(HandleCaptivePortal);
  server.begin();

  Serial.println("Captive portal started!");
  Serial.println("Connect to the WiFi network above");
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");
}
//********************************************


//******** START NORMAL MODE FN *************
void StartNormalMode() {
  Serial.println("####### Web Server Setup ########");
  SetupRoutes();
  server.begin();
  Serial.println("Web server started!");
  Serial.println("Open your browser and go to:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");
}
//********************************************


//************* PRINT MENU FN ****************
void PrintMenu() {
  Serial.println("############# Menu ##############");
  Serial.println("Press 'S' to show status");
  Serial.println("Press 'O' to turn LED on");
  Serial.println("Press 'F' to turn LED off");
  Serial.println("Press 'B' to toggle blink mode");
  Serial.println("Press 'C' to clear WiFi credentials (no reboot)");
  Serial.println("Press 'R' to clear WiFi and reboot into setup mode");
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");
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
  Serial.println("######### LED Setup #############");
  analogWriteRange(255); // Lock PWM to 0-255 on all ESP8266 core versions
  pinMode(LedR, OUTPUT);
  pinMode(LedG, OUTPUT);
  pinMode(LedB, OUTPUT);
  SetLED(0, 0, 0);
  Serial.println("RGB LED pins initialized!");
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");
  //******************************************


  //************* EEPROM SETUP ***************
  InitEeprom();
  //******************************************


  //*************** WIFI SETUP ***************
  bool connected = false;
  if (HasStoredCredentials()) {
    String storedSsid = ReadStoredSsid();
    String storedPass = ReadStoredPassword();
    connected = ConnectWifi(storedSsid, storedPass);
  } else {
    Serial.println("No stored WiFi credentials");
  }

  if (connected) {
    CurrentMode = NORMAL_MODE;
    StartNormalMode();
  } else {
    CurrentMode = SETUP_MODE;
    StartSetupMode();
  }
  //******************************************

  // Menu
  PrintMenu();
}
//*******************************************


//************** LOOP FUNCTION **************
void loop() {
  // Handle DNS requests in setup mode (captive portal)
  if (CurrentMode == SETUP_MODE) {
    DnsServer.processNextRequest();
  }

  // Handle web requests
  server.handleClient();

  // Non-blocking blink mode (only in normal mode)
  if (CurrentMode == NORMAL_MODE && BlinkMode) {
    if (millis() - LastBlinkTime >= (unsigned long)blinkDelay) {
      LastBlinkTime = millis();
      BlinkState = !BlinkState;
      if (BlinkState) {
        ApplyColor();
      } else {
        SetLED(0, 0, 0);
      }
    }
  }

  // Serial menu commands
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("s")) {
      Serial.println("############ Status #############");
      Serial.print("Mode: ");
      Serial.println(CurrentMode == NORMAL_MODE ? "Normal" : "Setup");
      Serial.print("WiFi: ");
      Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
      Serial.print("IP: ");
      Serial.println(CurrentMode == NORMAL_MODE ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
      Serial.print("LED: ");
      Serial.println(LedOn ? "ON" : "OFF");
      Serial.print("Blink: ");
      Serial.println(BlinkMode ? "ON" : "OFF");
      Serial.print("Color: R=");
      Serial.print(ColorR);
      Serial.print(" G=");
      Serial.print(ColorG);
      Serial.print(" B=");
      Serial.println(ColorB);
      Serial.print("Brightness: ");
      Serial.println(Brightness);
      Serial.println("#################################");
      Serial.println(" ");
      Serial.println(" ");
    }

    if (input.equalsIgnoreCase("o")) {
      LedOn = true;
      BlinkMode = false;
      ApplyColor();
      Serial.println("LED turned ON");
    }

    if (input.equalsIgnoreCase("f")) {
      LedOn = false;
      BlinkMode = false;
      SetLED(0, 0, 0);
      Serial.println("LED turned OFF");
    }

    if (input.equalsIgnoreCase("b")) {
      BlinkMode = !BlinkMode;
      if (BlinkMode) {
        BlinkState = false;
        LastBlinkTime = millis();
      }
      Serial.print("Blink mode ");
      Serial.println(BlinkMode ? "ON" : "OFF");
    }

    if (input.equalsIgnoreCase("c")) {
      ClearCredentials();
      Serial.println("WiFi credentials cleared");
      Serial.println("Use 'R' to reboot into setup mode");
    }

    if (input.equalsIgnoreCase("r")) {
      Serial.println("Clearing WiFi credentials...");
      ClearCredentials();
      Serial.println("Rebooting into setup mode...");
      delay(1000);
      ESP.restart();
    }
  }
}
//*******************************************
