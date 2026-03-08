//************* HEADER FILES *****************
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
//********************************************


//************** WIFI CONFIG *****************
//Change these to match your local WiFi network
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
//********************************************


//*************** PIN SETUP ******************
//LED pin
int ledPin = 14;  //D5 - Yellow
//********************************************


//************ TIMING PARAMETERS *************
int blinkDelay = 500;
int connectTimeout = 20;
//********************************************


//*********** LED STATE VARIABLES ************
bool ledState = false;
bool blinkMode = false;
int brightness = 255;
//********************************************


//************* INITIALISING *****************
//Web server instance on port 80
ESP8266WebServer server(80);
//********************************************


//************* BUILD PAGE FN ****************
String BuildPage() {
  String page = "<!DOCTYPE html><html><head>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "<title>LED Controller</title>";
  page += "<style>";
  page += "body{font-family:Arial;text-align:center;margin:40px;background:#1a1a2e;color:#eee;}";
  page += "h1{color:#e94560;}";
  page += ".btn{display:inline-block;padding:15px 30px;margin:10px;font-size:18px;";
  page += "border:none;border-radius:8px;cursor:pointer;color:#fff;min-width:120px;}";
  page += ".on{background:#0f3460;}";
  page += ".off{background:#e94560;}";
  page += ".blink{background:#533483;}";
  page += ".active{outline:3px solid #16c79a;}";
  page += ".slider-container{margin:20px auto;max-width:300px;}";
  page += "input[type=range]{width:100%;accent-color:#e94560;}";
  page += ".status{font-size:20px;margin:20px;padding:15px;border-radius:8px;background:#16213e;}";
  page += "</style></head><body>";
  page += "<h1>WiFi LED Controller</h1>";

  //Status display
  page += "<div class='status'>";
  if (blinkMode) {
    page += "Mode: BLINKING";
  } else if (ledState) {
    page += "LED: ON";
  } else {
    page += "LED: OFF";
  }
  page += "</div>";

  //Control buttons
  page += "<div>";
  page += "<a href='/on'><button class='btn on";
  if (ledState && !blinkMode) page += " active";
  page += "'>ON</button></a>";
  page += "<a href='/off'><button class='btn off";
  if (!ledState && !blinkMode) page += " active";
  page += "'>OFF</button></a>";
  page += "<a href='/blink'><button class='btn blink";
  if (blinkMode) page += " active";
  page += "'>BLINK</button></a>";
  page += "</div>";

  //Brightness slider
  page += "<div class='slider-container'>";
  page += "<p>Brightness: <span id='val'>" + String(brightness) + "</span></p>";
  page += "<input type='range' min='0' max='255' value='" + String(brightness) + "'";
  page += " onchange=\"location.href='/brightness?value='+this.value\">";
  page += "</div>";

  page += "</body></html>";
  return page;
}
//********************************************


//************** HANDLE ROOT FN **************
void HandleRoot() {
  server.send(200, "text/html", BuildPage());
  Serial.println("Served web page");
}
//********************************************


//************ HANDLE LED ON FN **************
void HandleLedOn() {
  ledState = true;
  blinkMode = false;
  analogWrite(ledPin, brightness);
  Serial.println("LED turned ON");
  server.send(200, "text/html", BuildPage());
}
//********************************************


//************ HANDLE LED OFF FN *************
void HandleLedOff() {
  ledState = false;
  blinkMode = false;
  analogWrite(ledPin, 0);
  Serial.println("LED turned OFF");
  server.send(200, "text/html", BuildPage());
}
//********************************************


//*********** HANDLE LED BLINK FN ************
void HandleLedBlink() {
  blinkMode = true;
  ledState = false;
  Serial.println("LED set to BLINK mode");
  server.send(200, "text/html", BuildPage());
}
//********************************************


//********* HANDLE BRIGHTNESS FN *************
void HandleBrightness() {
  if (server.hasArg("value")) {
    brightness = server.arg("value").toInt();
    brightness = constrain(brightness, 0, 255);
    Serial.print("Brightness set to ");
    Serial.println(brightness);
    if (ledState && !blinkMode) {
      analogWrite(ledPin, brightness);
    }
  }
  server.send(200, "text/html", BuildPage());
}
//********************************************


//********** HANDLE NOT FOUND FN *************
void HandleNotFound() {
  server.send(404, "text/plain", "Page not found");
  Serial.println("404 - Page not found");
}
//********************************************


//************* CONNECT WIFI FN **************
void ConnectWifi() {
  Serial.println("######### WiFi Setup ############");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

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
  } else {
    Serial.println("Error, could not connect to WiFi!");
    Serial.println("Check your SSID and password.");
  }
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");
}
//********************************************


//************ SETUP ROUTES FN ***************
void SetupRoutes() {
  server.on("/", HandleRoot);
  server.on("/on", HandleLedOn);
  server.on("/off", HandleLedOff);
  server.on("/blink", HandleLedBlink);
  server.on("/brightness", HandleBrightness);
  server.onNotFound(HandleNotFound);
}
//********************************************


//************* PRINT MENU FN ****************
void PrintMenu() {
  Serial.println("############# Menu ##############");
  Serial.println("Press 'S' to show status");
  Serial.println("Press 'O' to turn LED on");
  Serial.println("Press 'F' to turn LED off");
  Serial.println("Press 'B' to toggle blink mode");
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");
}
//********************************************


//************** SETUP FUNCTION **************
void setup() {

  //***************SERIAL SETUP **************
  //Starting serial communication
  Serial.begin(9600);
  Serial.println("\n");
  //******************************************


  //*************** LED SETUP ****************
  Serial.println("######### LED Setup #############");
  pinMode(ledPin, OUTPUT);
  analogWrite(ledPin, 0);
  Serial.println("LED pin initialized!");
  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");
  //******************************************


  //*************** WIFI SETUP ***************
  ConnectWifi();
  //******************************************


  //************* SERVER SETUP ***************
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
  //******************************************

  //Menu
  PrintMenu();
}
//*******************************************


//************** LOOP FUNCTION **************
void loop() {
  //Handle web requests
  server.handleClient();

  //Blink mode logic
  if (blinkMode) {
    analogWrite(ledPin, brightness);
    delay(blinkDelay);
    analogWrite(ledPin, 0);
    delay(blinkDelay);
  }

  //Serial menu commands
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("s")) {
      Serial.println("############ Status #############");
      Serial.print("WiFi: ");
      Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("LED: ");
      Serial.println(ledState ? "ON" : "OFF");
      Serial.print("Blink: ");
      Serial.println(blinkMode ? "ON" : "OFF");
      Serial.print("Brightness: ");
      Serial.println(brightness);
      Serial.println("#################################");
      Serial.println(" ");
      Serial.println(" ");
    }

    if (input.equalsIgnoreCase("o")) {
      ledState = true;
      blinkMode = false;
      analogWrite(ledPin, brightness);
      Serial.println("LED turned ON");
    }

    if (input.equalsIgnoreCase("f")) {
      ledState = false;
      blinkMode = false;
      analogWrite(ledPin, 0);
      Serial.println("LED turned OFF");
    }

    if (input.equalsIgnoreCase("b")) {
      blinkMode = !blinkMode;
      Serial.print("Blink mode ");
      Serial.println(blinkMode ? "ON" : "OFF");
    }
  }
}
//*******************************************
