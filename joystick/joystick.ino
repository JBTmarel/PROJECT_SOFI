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

Encoder xAxis(RED1, RED2);
Encoder yAxis(BROWN1, BROWN2);
Encoder spinner(ORAN1, ORAN2);
Encoder button(WHITE1, WHITE2);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}
long positionX = -999;
long positionY = -999;

void loop() {
  long newX, newY;
  newX = xAxis.read();
  newY = yAxis.read();
  if (newX != positionX || newY != positionY) {
    Serial.print("X: ");
    Serial.print(newX);
    Serial.print(", Right: ");
    Serial.print(newY);
    Serial.println();
    positionX = newX;
    positionY = newY;
  }
  if (Serial.available()) {
    Serial.read();
    Serial.println("Reset both knobs to zero");
    xAxis.write(0);
    yAxis.write(0);
  }
}