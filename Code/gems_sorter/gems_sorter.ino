//************* HEADER FILES *****************
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_TCS34725.h>
//********************************************


//*************** PIN SETUP ******************
//I2C colour sensor pins
#define SDA_PIN 5  //D1 - Blue
#define SCL_PIN 4  //D2 - Green
//Servo pins
int TopServoPin = 14;  //D5 - Yellow
int SlideServoPin = 12;     //D6 - Purple
//********************************************


//************* COLOUR STRUCT ****************
//Structure to store colour names, RGB values & angle
struct Color {
  const char* name;
  uint16_t r, g, b;
  int number;
};
//********************************************


//************* COLOUR VALUES *****************
//Table to compare sensed colour values
Color colors[] = {
  { "Pink", 255, 148, 145, 145 },
  { "Blue", 144, 237, 255, 126 },
  { "Green", 199, 255, 111, 108 },
  { "Yellow", 255, 215, 81, 90 },
  { "Red", 255, 109, 91, 72 },
  { "Purple", 255, 156, 185, 54 },
  { "Orange", 255, 184, 145, 35 },
  { "Blank", 255, 219, 209, -1 }
};
//********************************************


//************ SERVO OFFSETS *****************
//Change the offsets for allignment issues
short int TopServoOffset = 0;
short int SlideServoOffset = 0;
//********************************************


//************* INITIALISING *****************
//Top servo instance
Servo TopServo;
//Slide servo instance
Servo SlideServo;
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
  TopServo.attach(TopServoPin, 544, 2400);
  TopServo.write(160 + TopServoOffset);
  Serial.println("Collecting Gems");
  delay(800);
  TopServo.detach();
}
//********************************************


//********* MOVE GEMS TO SENSOR FN ***********
void MoveToColorSensor() {
  TopServo.attach(TopServoPin, 544, 2400);
  TopServo.write(0 + TopServoOffset);
  Serial.println("Moving it to the colour sensor");
  delay(1000);
  TopServo.detach();
}
//********************************************


//************ Top GEMS FN **************
void TopGems() {
  TopServo.attach(TopServoPin, 544, 2400);
  TopServo.write(50 + TopServoOffset);
  Serial.println("Dispensing");
  delay(800);
  TopServo.detach();
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
    SlideServo.attach(SlideServoPin, 544, 2400);
    Serial.print("Moving slide to ");
    Serial.println(angle - SlideServoOffset);
    SlideServo.write(angle - SlideServoOffset);
    delay(1000);
    SlideServo.detach();
  }
}
//********************************************


//*********** CALIBRATE SERVOS FN ************
void CalibrateServos() {
  Serial.println("######## Calibrate Servos #######");

  Serial.println("Moving top servo to 60");
  TopServo.attach(TopServoPin, 544, 2400);
  TopServo.write(40);
  delay(500);
  TopServo.write(80);
  delay(500);
  TopServo.write(60);
  delay(500);

  Serial.println("Moving slide servo to 90");
  SlideServo.attach(SlideServoPin, 544, 2400);
  SlideServo.write(70);
  delay(500);
  SlideServo.write(110);
  delay(500);
  SlideServo.write(90);

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
  Serial.println("Press 'S' to start Slide");
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
        TopGems();
        delay(300);
        Serial.println("#################################");
        Serial.println(" ");
        Serial.println(" ");
      } while (angle >= 0);

      Serial.println("############ Result #############");
      Serial.print("Number of gems sorted : ");
      Serial.println(count - 1);
      Serial.println("#################################");
      Serial.println(" ");
      Serial.println(" ");
      PrintMenu();
    }

    if (input.equalsIgnoreCase("c")) {
      CalibrateServos();
    }
    if (input.equalsIgnoreCase("r")) {
      SenseColour();
    }
  }
}
//*******************************************