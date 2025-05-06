#include <Servo.h>
#include <Wire.h>
#include <Adafruit_TCS34725.h>

// Define custom I2C pins for ESP32
#define SDA_PIN 5  //D1
#define SCL_PIN 4  //D2

// Pin declarations
int DispenserServoPin = 14;  //D5
int SorterServoPin = 12;     //D6

//Offset
short int offset = 0;

//Initialise servo
Servo myServo;

// Create an instance of the TCS34725 sensor
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_300MS, TCS34725_GAIN_1X);

// Predefined RGB values for basic colors
struct Color {
  const char* name;
  uint16_t r, g, b;
  int number;
};

Color colors[] = {
  { "Pink", 236, 200, 255, 6 },
  { "Blue", 80, 190, 255, 5 },
  { "Green", 148, 255, 160, 4 },
  { "Yellow", 238, 255, 140, 3 },
  { "Red", 255, 153, 160, 2 },
  { "Purple", 185, 191, 255, 1 },
  { "Orange", 255, 170, 136, 0 },
  { "Blank", 225, 222, 220, -1 }
};

int angles[] = { 42, 56, 77, 92, 112, 128, 144 };
// Function to calculate the Euclidean distance between two colors
float calculateDistance(uint16_t r1, uint16_t g1, uint16_t b1, uint16_t r2, uint16_t g2, uint16_t b2) {
  return sqrt(pow(r2 - r1, 2) + pow(g2 - g1, 2) + pow(b2 - b1, 2));
}


void CollectGems() {
  myServo.attach(DispenserServoPin, 544, 2400);
  myServo.write(180);
  Serial.println("Collecting Gems");
  delay(800);
  myServo.detach();
}

void MoveToColorSensor() {
  myServo.attach(DispenserServoPin, 544, 2400);
  myServo.write(0);
  Serial.println("Moving the Gems to the colour sensor");
  delay(1000);
  myServo.detach();
}

void DispenseGems() {
  myServo.attach(DispenserServoPin, 544, 2400);
  myServo.write(80);
  Serial.println("Dispensing Gems");
  delay(800);
  myServo.detach();
}

int SenseColour() {
  Serial.println("Sensing Colour");

  uint16_t r, g, b, c;

  // Read color data
  tcs.getRawData(&r, &g, &b, &c);

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
  float minDistance = 1e6;  // Large number for initialization
  const char* closestColor = "Unknown";
  int colorNumber = -1;  // Default value if no match found

  for (Color color : colors) {
    float distance = calculateDistance(normR, normG, normB, color.r, color.g, color.b);
    if (distance < minDistance) {
      minDistance = distance;
      closestColor = color.name;
      colorNumber = color.number;
    }
  }

  // Print the raw and normalized values
  Serial.println("-----------");
  Serial.print("Raw Red: ");
  Serial.print(r);
  Serial.print(" Green: ");
  Serial.print(g);
  Serial.print(" Blue: ");
  Serial.println(b);

  Serial.print("Normalized Red: ");
  Serial.print(normR);
  Serial.print(" Green: ");
  Serial.print(normG);
  Serial.print(" Blue: ");
  Serial.println(normB);

  // Print the closest color
  Serial.print("Closest Color: ");
  Serial.println(closestColor);

  return colorNumber;  // Return the color number
}

void MoveDispenserTo(int a) {
  if (a >= 0) {
    myServo.attach(SorterServoPin, 544, 2400);
    myServo.write(angles[a] - offset);
    delay(1000);
    myServo.detach();
  }
}

void ResetDispenser() {
  myServo.attach(SorterServoPin, 544, 2400);
  myServo.write(42);
  delay(1500);
  myServo.detach();
}

void setup() {
  Serial.begin(115200);  // Start Serial Monitor
  Serial.println(" ");

  Wire.begin(SDA_PIN, SCL_PIN);

  if (tcs.begin()) {
    Serial.println("TCS34725 found and initialized!");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1)
      ;  // Halt the program
  }

  // Enable interrupts to avoid saturation
  tcs.setInterrupt(false);  // Turn on LED (if connected)
}

void loop() {
  if (Serial.available()) {                       // Check if data is received
    String input = Serial.readStringUntil('\n');  // Read input
    input.trim();                                 // Remove extra spaces or newlines

    if (input.equalsIgnoreCase("a")) {
      int num = 0;
      do {

        CollectGems();
        delay(300);
        MoveToColorSensor();
        delay(300);
        num = SenseColour();
        delay(300);
        MoveDispenserTo(num);
        delay(300);
        DispenseGems();
        delay(300);
      } while (num >= 0);
    }

    if (input.equalsIgnoreCase("s")) {
      Serial.println("Moving dispenser servo to 90");
      myServo.attach(DispenserServoPin, 544, 2400);
      myServo.write(0);
      delay(1000);
      myServo.detach();
      Serial.println("Moving sorter servo to 90");
      myServo.attach(SorterServoPin, 544, 2400);
      myServo.write(90);
      delay(1000);
      myServo.detach();      
    }

    if (input.equalsIgnoreCase("c")) {
      MoveToColorSensor();
      delay(2000);
      SenseColour();
    }
    if (input.equalsIgnoreCase("d")) {
      DispenseGems();
    }
  }
}