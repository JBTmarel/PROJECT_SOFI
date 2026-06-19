#include <Arduino.h>
#include <Encoder.h>
#include <math.h>

// ESP32 translation of the original Arduino Uno joystick sketch (translated by ChatGPT).
// Target shown by your board info: classic ESP32 / ESP32-D0WD-V3.
//
// Pin notes:
// - Avoid GPIO 6-11: used for ESP32 flash on most dev boards.
// - Avoid boot strapping pins unless you know your wiring will not affect boot.
// - These encoder pins are normal GPIOs with interrupt support and internal pullups.
// - Throttle outputs use the ESP32 DAC pins, GPIO25 and GPIO26, because the
//   original sketch's comments describe voltage outputs rather than pure PWM.

constexpr bool debugMode = true;

// Original cable color names, remapped from Uno pins to ESP32 GPIOs.
// All of these are interrupt-capable ESP32 GPIOs.
constexpr int WHITE1 = 18;  // optional encoder pair
constexpr int WHITE2 = 19;
constexpr int ORAN1  = 21;  // optional encoder pair
constexpr int ORAN2  = 22;
constexpr int RED1   = 32;  // xAxis encoder A/B
constexpr int RED2   = 33;
constexpr int BROWN1 = 27;  // yAxis encoder A/B
constexpr int BROWN2 = 14;

// ESP32 DAC outputs. On classic ESP32, DAC1 = GPIO25, DAC2 = GPIO26.
constexpr int leftThrottlePin  = 25;
constexpr int rightThrottlePin = 26;

// Safe general-purpose output pin. Change this if your board already uses GPIO23.
constexpr int powerOnPin = 23;

Encoder xAxis(RED1, RED2);
Encoder yAxis(BROWN1, BROWN2);

// Optional encoders from the original sketch. These pins are also interrupt-capable.
// Uncomment only if these are actually wired, otherwise they are not needed.
// Encoder spinner(ORAN1, ORAN2);
// Encoder button(WHITE1, WHITE2);

constexpr float maxSteps = 68.0f;
constexpr float originalUnoPwmVoltage = 5.0f;
constexpr float esp32DacVoltage = 3.3f;
constexpr int maxOriginalThrottleDuty = 150;  // original formula gives about 150 at maxSteps
constexpr int resetError = 3;

long positionX = -999;
long positionY = -999;

float leftThrottlef = 0.0f;
float rightThrottlef = 0.0f;
long leftThrottle = 0;
long rightThrottle = 0;
float ratio = 0.0f;
float speed = 0.0f;

unsigned long start = 0;
bool lastMoveTrue = false;

long calcY = 0;
long calcX = 0;

static int clampInt(int value, int low, int high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

static float clampFloat(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

// The Uno sketch used 8-bit PWM at 5 V. This converts the old 0-255, 5 V PWM
// duty value into an ESP32 DAC value that produces approximately the same voltage.
// Example: old duty 150 at 5 V ~= 2.94 V, so ESP32 DAC writes about 227/255.
static void writeThrottleEquivalentVoltage(int pin, long originalUnoDuty) {
  originalUnoDuty = clampInt((int)originalUnoDuty, 0, 255);

  int dacValue = lroundf((float)originalUnoDuty * originalUnoPwmVoltage / esp32DacVoltage);
  dacValue = clampInt(dacValue, 0, 255);

  dacWrite(pin, (uint8_t)dacValue);
}

static void writeBothThrottles(long leftDuty, long rightDuty) {
  writeThrottleEquivalentVoltage(leftThrottlePin, leftDuty);
  writeThrottleEquivalentVoltage(rightThrottlePin, rightDuty);
}

void setup() {
  if (debugMode) {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32 joystick controller starting...");
  }

  // Explicit pullups are useful for mechanical/open-collector encoder outputs.
  // Do not feed 5 V encoder signals directly into ESP32 GPIOs.
  pinMode(WHITE1, INPUT_PULLUP);
  pinMode(WHITE2, INPUT_PULLUP);
  pinMode(ORAN1, INPUT_PULLUP);
  pinMode(ORAN2, INPUT_PULLUP);
  pinMode(RED1, INPUT_PULLUP);
  pinMode(RED2, INPUT_PULLUP);
  pinMode(BROWN1, INPUT_PULLUP);
  pinMode(BROWN2, INPUT_PULLUP);

  pinMode(leftThrottlePin, OUTPUT);
  pinMode(rightThrottlePin, OUTPUT);
  writeBothThrottles(0, 0);

  pinMode(powerOnPin, OUTPUT);
  digitalWrite(powerOnPin, HIGH);
}

void loop() {
  long newX = xAxis.read();
  long newY = yAxis.read();

  if (newX != positionX || newY != positionY) {
    if (debugMode) {
      Serial.print("X: ");
      Serial.print(newX);
      Serial.print(", Y: ");
      Serial.print(newY);
    }

    positionX = newX;
    positionY = newY;

    calcY = -newY;
    calcX = newX;

    if (calcY < 0) {
      calcY = 0;
      calcX = 0;
    }

    ratio = fabsf((float)calcX / maxSteps);
    ratio = clampFloat(ratio, 0.0f, 1.0f);

    // Preserves the original sketch's throttle curve:
    // y = 0 -> 90, y = 68 -> 150, before steering reduction.
    speed = (15.0f * (float)calcY + 1530.0f) / 17.0f;
    speed = clampFloat(speed, 0.0f, (float)maxOriginalThrottleDuty);

    if (calcX < 0) {
      leftThrottlef = speed;
      rightThrottlef = (1.0f - ratio) * speed;
    } else {
      leftThrottlef = (1.0f - ratio) * speed;
      rightThrottlef = speed;
    }

    leftThrottle = clampInt(lroundf(leftThrottlef), 0, maxOriginalThrottleDuty);
    rightThrottle = clampInt(lroundf(rightThrottlef), 0, maxOriginalThrottleDuty);

    writeBothThrottles(leftThrottle, rightThrottle);

    if (debugMode) {
      Serial.print(", ThrottleL old-duty: ");
      Serial.print(leftThrottle);
      Serial.print(", ThrottleR old-duty: ");
      Serial.println(rightThrottle);
    }

    start = millis();
    lastMoveTrue = true;
  }

  // If controller is still for 3 seconds near the center, recalibrate position to 0.
  if (((millis() - start) > 3000UL) &&
      (-resetError <= newX) && (newX <= resetError) &&
      (-resetError < newY) && (newY < resetError) &&
      lastMoveTrue) {

    xAxis.write(0);
    yAxis.write(0);
    // spinner.write(0);

    newX = 0;
    newY = 0;
    writeBothThrottles(0, 0);

    if (debugMode) {
      Serial.println("Reset axes to zero");
    }

    start = millis();
    lastMoveTrue = false;
  }
}
