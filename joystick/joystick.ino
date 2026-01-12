#include <Encoder.h>

// Numbering from left to right when leftmost cables are ground and 5V (So first pair, i.e. left, is white)
int WHITE1 = 9;
int WHITE2 = 10;
int ORAN1 = 12;
int ORAN2 = 11;
int RED1 = 3; //opposite order
int RED2 = 8; //opposite order
int BROWN1 = 2; //opposite order
int BROWN2 = 7; //opposite order

int throttlePin = 5;

Encoder xAxis(RED1, RED2);
Encoder yAxis(BROWN1, BROWN2);
Encoder spinner(ORAN1, ORAN2);
Encoder button(WHITE1, WHITE2);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(throttlePin, OUTPUT);
}

float throttlef = 0.0;
long positionX = -999;
long positionY = -999;
long posSpinner = -999;
long throttle = 0;

void loop() {
  long newX, newY, newSpinner;
  newX = xAxis.read();
  newY = yAxis.read();
  newSpinner = spinner.read();
  if (newX != positionX || newY != positionY || newSpinner != posSpinner) {
    Serial.print("X: ");
    Serial.print(newX);
    Serial.print(", Right: ");
    Serial.print(newY);
    // Serial.print(", Spinner: ");
    // Serial.print(newSpinner);
    //Serial.println();
    positionX = newX;
    positionY = newY;
    posSpinner = newSpinner;
    throttlef = (newX / 68.0f) * 255.0f;
    //throttlef = (96.9f*newX+3814.8f)/68.0f;
    throttle = lroundf(throttlef);
    if (throttle < 0){
      throttle = 0;
      Serial.print(" THROTTLE_ZERO ");
    }
    // else if (throttle > 255){
    //   throttle = 255;
    // }
    
    analogWrite(throttlePin, throttle);
    Serial.print(", Throttle:");
    Serial.print(throttle);
    Serial.println();
  }
  if (Serial.available()) {
    Serial.read();
    Serial.println("Reset all to zero");
    xAxis.write(0);
    yAxis.write(0);
    spinner.write(0);
  }
}