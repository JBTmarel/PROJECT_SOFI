#include <Encoder.h>



//-------------Debug mode and settings-----------------
bool debugMode = true; //If disabled, disables all

bool enableTimer = false; //debugMode must also be on
bool showThrottle = false; //debugMode must also be on
bool showRatios = false; //debugMode must also be on
bool showXY = true;
//-----------------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------------
//Important values
int minV = 1.6; //Throttle output voltage when joystick is at 0, 0
int maxV = 3; //Throttle output voltage when joystick is at 0, MAX
float maxSteps = 68.0f;
float k_eq = ((255.0f*maxV/5.0f)-255.0f*minV/5.0f)/(maxSteps);
float h_eq = 255.0f*minV/5.0f;
float avgLoopTime = 0.15f; //avg time for one loop in milliseconds
float minToMaxTime = 500; //time to ramp up from 0 to max in milliseconds

//-----------------------------------------------------------------------------------------
//Arduino accumulates error which lets the X and Y
//values to go over their maximum step length from the center.
//This caps that
long capMaxSteps(long x, float maxS){ 
  return (min(abs(x), maxS)*((x>0)-(x<0)));
}
//-----------------------------------------------------------------------------------------
//Applies a curve to the turning and speed control
//Makes it more intuitive for humans to use.
//Expo variables makes it adjustable
float turningExpo = 0.3;
float speedExpo = turningExpo;
float applyExpo(float x, float expo) {
  return (1.0f - expo) * x + expo * x * x * x;
}
//-----------------------------------------------------------------------------------------
dk
float rampToward(float minMaxTime, float voltRange, float dt, float target, float current){
  float voltRate = voltRange/minMaxTime;
  float step = voltRate*dt;
  if step > abs(target-current){
    step = target-current;
  }  
  return step;
}
//-----------------------------------------------------------------------------------------

void setup() {
  if (debugMode || enableTimer){
    Serial.begin(9600);
  }
  pinMode(WHITE1, OUTPUT);
  digitalWrite(WHITE1, LOW);
  pinMode(WHITE2, INPUT_PULLUP);
  //pinMode(throttlePin, OUTPUT);
  pinMode(leftThrottlePin, OUTPUT);
  pinMode(rightThrottlePin, OUTPUT);
  pinMode(powerOnPin, OUTPUT);
  digitalWrite(powerOnPin, HIGH);
  lastRampTime = millis();
}
//-----------------------------------------------------------------------------------------

unsigned long timerStart;
float timerAvg = 0.0f;
unsigned long timerCount = 0;


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
double angle;

long start = 0;
bool lastMoveTrue = 0;
int resetError = 3;

long calcY;
long calcX;

int turnNum = 10;

int oldbutton = 1;

//-----------------------------------------------------------------------------------------

void loop() {
  if (enableTimer){
    timerStart = millis();
  }
  long newX = xAxis.read();
  long newY = yAxis.read(); 

  //----------Button---------------------- 
  // int button = digitalRead(WHITE2);
  // // if (button != oldbutton && button == 0){
  // //   Serial.print("Button down");
  // // }
  // if (button != oldbutton && button == 1){
  //   Serial.print("Button up");
  //   newX = 0;
  //   newY = 0;
  // }

  // oldbutton = button;
  //---------------------------------------

  if (newX != positionX || newY != positionY) {
    newX = capMaxSteps(newX, maxSteps);
    newY = capMaxSteps(newY, maxSteps);

    positionX = newX;
    positionY = newY;
    

    calcY = -newY;
    calcX = newX;
    if (calcY < 0) {
      calcY = 0;
      calcX = 0;
    }
    // ---------Y = speed-----------------------------------------------------------
    if (debugMode && showXY) {
    Serial.println();
    Serial.print("X: ");
    Serial.print(calcX);
    Serial.print(", Y: ");
    Serial.print(calcY);
    }


    float turningRatio = 1.0f*(float)calcX/(float)maxSteps;
    turningRatio = applyExpo(turningRatio, turningExpo);
    //ratio = pow(turnNum, -1+calcX/68)*(turnNum/(turnNum-1))-1/(turnNum-1);
    float speedRatio = 1.0f*(float)calcY/(float)maxSteps;
    speedRatio = applyExpo(speedRatio, speedExpo);

    turningRatio = constrain(turningRatio, -1.0f, 1.0f);
    speedRatio = constrain(speedRatio, 0.0f, 1.0f);

    if (debugMode && showRatios) {
      Serial.println();
      Serial.print("Turn: ");
      Serial.print(turningRatio);
      Serial.print(", Speed: ");
      Serial.print(speedRatio);
    }
    
    leftThrottleRatio = speedRatio+turningRatio;
    rightThrottleRatio = speedRatio-turningRatio;
    if (max(abs(leftThrottleRatio), abs(rightThrottleratio)) > 1){
      leftThrottleRatio /= max(abs(leftThrottleRatio), abs(rightThrottleratio));
      rightThrottleRatio /= max(abs(leftThrottleRatio), abs(rightThrottleratio));
    }

    
    
    // -----------------------------------------------------------------------------

    // // -----------R = speed----------
    // float R = sqrt(sq(calcX)+sq(calcY));
    // R = min(R, maxSteps);
    // speed = (15.0f * R + 1530.0f) / 17.0f;
    // angle = atan2(calcY, calcX);
    // ratio = -0.5f+angle/3.1415f
    // sign = (ratio>0)-(ratio<0)
    // if (debugMode){
    //   Serial.println();
    //   Serial.print("R: ");
    //   Serial.print(R);
    //   Serial.print(", 0: ");
    //   Serial.print(angle);
    // }
    // leftThrottlef = speed+
    // rightThrottlef = speed
    // //---------------------------------

    leftThrottle = lroundf(leftThrottlef);
    rightThrottle = lroundf(rightThrottlef);

    analogWrite(leftThrottlePin, leftThrottle);
    analogWrite(rightThrottlePin, rightThrottle);
    
    if (debugMode && showThrottle){
    Serial.print(", ThrottleL: ");
    Serial.print(leftThrottle);
    Serial.print(", ThrottleR: ");
    Serial.print(rightThrottle);
    //Serial.println();
    }
    start = millis();
    lastMoveTrue = 1;
  }

  //If controller is still for 3 seconds near the center
  //it will recalibrate the position to 0
  if (
    ((millis() - start) > 3000) && 
    (-1*resetError <= newX) &&
    (newX <= resetError) && 
    (-1*resetError < newY) &&
    (newY < resetError) &&
    lastMoveTrue)  {

    xAxis.write(0);
    yAxis.write(0);
    // spinner.write(0);
    newX = 0;
    newY = 0;
    analogWrite(leftThrottlePin, 0);
    analogWrite(rightThrottlePin, 0);
    if (debugMode){
    //Serial.println("Reset axes to zero");
    //Serial.print("X: ");
    //Serial.print(newX);
    //Serial.print(", Y: ");
    //Serial.print(newY);
    }
    start = millis();
    lastMoveTrue = 0;
  }
  if (enableTimer){
  timerCount++;
  timerAvg = ((millis()-timerStart) + timerAvg*timerCount)/(timerCount+1);
  Serial.print("Timer AVG: ");
  Serial.println(timerAvg);
  }
}

