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

//Encoder xAxis(RED1, RED2);
//Encoder yAxis(BROWN1, BROWN2);
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
  pinMode(leftThrottlePin, OUTPUT);
  pinMode(rightThrottlePin, OUTPUT);
  pinMode(powerOnPin, OUTPUT);
  pinMode(WHITE1, OUTPUT);
  pinMode(WHITE2, INPUT_PULLUP);
  digitalWrite(powerOnPin, HIGH);
  // analogWrite(leftThrottlePin, 150);
  // analogWrite(rightThrottlePin, 150);
  digitalWrite(WHITE1, LOW);
}

bool button;
int maxSpeed = 180;

void loop() {
  button = digitalRead(WHITE2);
  //Serial.print(button);
  //Serial.println();
  if (!button) {
    analogWrite(leftThrottlePin, maxSpeed);
    analogWrite(rightThrottlePin, maxSpeed);
    // Serial.print("DRIVE MOTHER FUCKER!!!!!!!");
  } else  {
    analogWrite(leftThrottlePin, 0);
    analogWrite(rightThrottlePin, 0);
    // Serial.print("stop");
  }
  
  
  delay(1);
}

