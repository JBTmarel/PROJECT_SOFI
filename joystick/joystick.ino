n// Numbering from left to right when leftmost cables are ground and 5V (So first pair, i.e. left, is white)
int WHITE1 = 10;
int WHITE2 = 11;
int ORAN1 = 13;
int ORAN2 = 12;
int RED1 = 9;
int RED2 = 8;
int BROWN1 = 7;
int BROWN2 = 6;

string colors[8] = {BROWN2, BROWN1, RED2, RED1, WHITE1, WHITE2, ORAN2, ORAN1};

int lastStates[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  for (int i=6; i<14; i++) {
    int state = digitalRead(i);
    if (state != lastStates[i-6]) {
      Serial.print("Got something on ");
      Serial.println(colors[i-6]);
    }
    lastStates[i-6] = state;
    delay(10);
  }
  
}
