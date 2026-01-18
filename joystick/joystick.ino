#include <Encoder.h>

// Numbering from left to right when leftmost cables are ground and 5V (So first
// pair, i.e. left, is white)
int WHITE1 = 5; //B2
int WHITE2 = 6; //A2
int ORAN1 = 4; //B3
int ORAN2 = 7; //A3
int RED1 = 3; //B4
int RED2 = 8; //A4
int BROWN1 = 2; //B5
int BROWN2 = 9; //A5

//int throttlePin = 5;
int leftThrottlePin = 10; 
int rightThrottlePin = 11;

int powerOnPin = 12;

Encoder xAxis(RED1, RED2);
Encoder yAxis(BROWN1, BROWN2);
// Encoder spinner(ORAN1, ORAN2);
// Encoder button(WHITE1, WHITE2);

int minV = 1.6; //Throttle output voltage when joystick is at 0, 0
int maxV = 3; //Throttle output voltage when joystick is at 0, MAX
float maxSteps = 68.0f;
float k_eq = ((255.0f*maxV/5.0f)-255.0f*minV/5.0f)/(maxSteps);
float h_eq = 255.0f*minV/5.0f;

void setup() {
  // put your setup code here, to run once:
  //Serial.begin(9600);
  //pinMode(throttlePin, OUTPUT);
  pinMode(leftThrottlePin, OUTPUT);
  pinMode(rightThrottlePin, OUTPUT);
  pinMode(powerOnPin, OUTPUT);
  digitalWrite(powerOnPin, HIGH);
}

float throttlef = 0.0;
long positionX = -999;
long positionY = -999;

long throttle = 0;
float leftThrottlef = 0;
float rightThrottlef = 0;
long leftThrottle = 0;
long rightThrottle = 0;
float ratio = 0;
float speed = 0;

long start = 0;
bool lastMoveTrue = 0;
int resetError = 3;

long calcY;
long calcX;

void loop() {
  long newX, newY;
  newX = xAxis.read();
  newY = yAxis.read();  
  if (newX != positionX || newY != positionY) {
    //Serial.print("X: ");
    //Serial.print(newX);
    //Serial.print(", Y: ");
    //Serial.print(newY);
    positionX = newX;
    positionY = newY;

    calcY = -newY;
    calcX = newX;
    if (calcY < 0) {
      calcY = 0;
      calcX = 0;
    }

    ratio = abs(1.0f*calcX/maxSteps);
    speed = (15.0f * calcY + 1530.0f) / 17.0f;
    if (calcX < 0) {
      leftThrottlef = speed;
      rightThrottlef = (1.0f-ratio)*speed;
    } else {
      leftThrottlef = (1.0f-ratio)*speed;
      rightThrottlef = speed;
    }
    leftThrottle = lroundf(leftThrottlef);
    rightThrottle = lroundf(rightThrottlef);

    analogWrite(leftThrottlePin, leftThrottle);
    analogWrite(rightThrottlePin, rightThrottle);

    //Serial.print(", ThrottleL: ");
    //Serial.print(leftThrottle);
    //Serial.print(", ThrottleR: ");
    //Serial.print(rightThrottle);
    //Serial.println();
    start = millis();
    lastMoveTrue = 1;
  }
  if (
    ((millis() - start) > 3000) && 
    (-1*resetError <= newX) &&
    (newX <= resetError) && 
    (-1*resetError < newY) &&
    (newY < resetError) &&
    lastMoveTrue) {

    xAxis.write(0);
    yAxis.write(0);
    // spinner.write(0);
    newX = 0;
    newY = 0;
    analogWrite(leftThrottlePin, 0);
    analogWrite(rightThrottlePin, 0);
    //Serial.println("Reset axes to zero");
    //Serial.print("X: ");
    //Serial.print(newX);
    //Serial.print(", Y: ");
    //Serial.print(newY);
    start = millis();
    lastMoveTrue = 0;
  }
}

