//************* HEADER FILES *****************
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_TCS34725.h>
//********************************************


//*************** PIN SETUP ******************
//I2C colour sensor pins
#define SDA_PIN 5  //D1
#define SCL_PIN 4  //D2
//Servo pins
int DispenserServoPin = 14;  //D5
int SorterServoPin = 12;     //D6
//********************************************


//************* COLOUR STRUCT ****************
//Structure to store colour names, RGB values & angles
struct Color {
  const char* name;
  uint16_t r, g, b;
  int number;
};
//********************************************


//************* COLOUR VALUES *****************
//Table to compare sensed colour values
Color colors[] = {
  { "Pink", 236, 200, 255, 144 },
  { "Blue", 80, 190, 255, 128 },
  { "Green", 148, 255, 160, 112 },
  { "Yellow", 238, 255, 140, 92 },
  { "Red", 255, 153, 160, 77 },
  { "Purple", 185, 191, 255, 56 },
  { "Orange", 255, 184, 145, 42 },
  { "Blank", 255, 178, 148, -1 }
};
//********************************************


//************ SERVO OFFSETS *****************
//Change the offsets for allignment issues
short int DispernserServoOffset = 10;
short int SorterServoOffset = 0;
//********************************************


//************* INITIALISING *****************
//Dispense servo instance
Servo dispenserServo;
//Sorting servo instance
Servo sorterServo;
//Colour sensor instance
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_300MS, TCS34725_GAIN_1X);
//********************************************


//******* CALCULATE COLOUR DISTANCE FN ********
float calculateDistance(uint16_t r1, uint16_t g1, uint16_t b1, uint16_t r2, uint16_t g2, uint16_t b2) {
  return sqrt(pow(r2 - r1, 2) + pow(g2 - g1, 2) + pow(b2 - b1, 2));
}
//********************************************


//************ COLLECT GEMS FN ***************
void CollectGems() {
  dispenserServo.attach(DispenserServoPin, 544, 2400);
  dispenserServo.write(160 + DispernserServoOffset);
  Serial.println("Collecting Gems");
  delay(800);
  dispenserServo.detach();
}
//********************************************


//********* MOVE GEMS TO SENSOR FN ***********
void MoveToColorSensor() {
  dispenserServo.attach(DispenserServoPin, 544, 2400);
  dispenserServo.write(0 + DispernserServoOffset);
  Serial.println("Moving it to the colour sensor");
  delay(1000);
  dispenserServo.detach();
}
//********************************************


//************ DISPENSE GEMS FN **************
void DispenseGems() {
  dispenserServo.attach(DispenserServoPin, 544, 2400);
  dispenserServo.write(50 + DispernserServoOffset);
  Serial.println("Dispensing");
  delay(800);
  dispenserServo.detach();
}
//********************************************


//************* SENSE COLOUR FN **************
int SenseColour() {
  Serial.println("Sensing Colour");

  uint16_t r, g, b, c;

  //Reading color data from the sensor
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
  int colorAngle = -1;  // Default value if no match found

  for (Color color : colors) {
    float distance = calculateDistance(normR, normG, normB, color.r, color.g, color.b);
    if (distance < minDistance) {
      minDistance = distance;
      closestColor = color.name;
      colorAngle = color.number;
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
    Serial.println("No gems detected!");
  }


  return colorAngle;  // Return the color number
}
//********************************************


//******** MOVE SLIDE TO ANGLE FN ************
void MoveSlideTo(int angle) {
  if (angle >= 0) {
    sorterServo.attach(SorterServoPin, 544, 2400);
    Serial.print("Moving slide to ");
    Serial.println(angle - SorterServoOffset);
    sorterServo.write(angle - SorterServoOffset);
    delay(1000);
    sorterServo.detach();
  }
}
//********************************************


//*********** CALIBRATE SERVOS FN ************
void CalibrateServos() {
  Serial.println("######## Calibrate Servos #######");

  Serial.println("Moving dispenser servo to 60");
  dispenserServo.attach(DispenserServoPin, 544, 2400);
  dispenserServo.write(60);

  Serial.println("Moving sorter servo to 90");
  sorterServo.attach(SorterServoPin, 544, 2400);
  sorterServo.write(90);

  Serial.println("#################################");
  Serial.println(" ");
  Serial.println(" ");

  delay(5000);
  sorterServo.detach();
  dispenserServo.detach();
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


//************** SETUP FUNCTION **************
void setup() {

  //***************SERIAL SETUP **************
  //Starting serial communication
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

  //Menu
  PrintMenu();
}
//*******************************************


//************** LOOP FUNCTION **************
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("s")) {
      int angle = 0;
      unsigned int count = 0;
      do {
        count++;
        Serial.print("############ LOOP ");
        Serial.print(count);
        Serial.println(" #############");
        CollectGems();
        delay(300);
        MoveToColorSensor();
        delay(300);
        angle = SenseColour();
        delay(300);
        MoveSlideTo(angle);
        delay(300);
        DispenseGems();
        delay(300);
        Serial.println("#################################");
        Serial.println(" ");
        Serial.println(" ");
      } while (angle >= 0);

      Serial.println("############ Result #############");
      Serial.print("Number of gems sorted : ");
      Serial.println(count-1);
      Serial.println("#################################");
      Serial.println(" ");
      Serial.println(" ");
      PrintMenu();
    }

    if (input.equalsIgnoreCase("c")) {
      CalibrateServos();
    }
  }
}
//*******************************************