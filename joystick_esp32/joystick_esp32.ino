#include <Arduino.h>
#include <Encoder.h>
#include <math.h>

// -----------------------------------------------------------------------------
// ESP32 joystick -> left/right BLDC throttle controller
// Uses real DAC outputs on GPIO25 and GPIO26, not PWM.
// Change pins here if needed.
// -----------------------------------------------------------------------------

// ---------------- Debug mode and settings ----------------
const bool debugMode     = true;   // If false, disables all debug printing
const bool enableTimer   = false;  // debugMode must also be true
const bool showThrottle  = false;  // debugMode must also be true
const bool showRatios    = false;  // debugMode must also be true
const bool showXY        = true;   // debugMode must also be true

// ---------------- Pin settings ----------------
// Choose normal ESP32 GPIOs. Avoid GPIO6-11. Avoid boot pins if possible.
// GPIO25 and GPIO26 are the ESP32 DAC pins.

const uint8_t X_ENC_A_PIN = 14;   // old RED1
const uint8_t X_ENC_B_PIN = 17;   // old RED2
const uint8_t Y_ENC_A_PIN = 13;   // old BROWN1
const uint8_t Y_ENC_B_PIN = 16;   // old BROWN2

// Optional button wiring, same idea as old WHITE1/WHITE2:
// one pin is driven LOW, the other is read with INPUT_PULLUP.
const uint8_t BUTTON_GND_PIN = 32; // old WHITE1
const uint8_t BUTTON_IN_PIN  = 33; // old WHITE2

// Optional unused encoder/spinner pins, kept here for easy future use.
// const uint8_t SPINNER_A_PIN = 27; // old ORAN1
// const uint8_t SPINNER_B_PIN = 18; // old ORAN2

const uint8_t LEFT_THROTTLE_DAC_PIN  = 25; // ESP32 DAC1
const uint8_t RIGHT_THROTTLE_DAC_PIN = 26; // ESP32 DAC2

const uint8_t POWER_ON_PIN = 32;

// ---------------- Encoder objects ----------------
Encoder xAxis(X_ENC_A_PIN, X_ENC_B_PIN);
Encoder yAxis(Y_ENC_A_PIN, Y_ENC_B_PIN);
// Encoder spinner(SPINNER_A_PIN, SPINNER_B_PIN);

// -----------------------------------------------------------------------------
// Important values
// -----------------------------------------------------------------------------

// Set these for your BLDC controller throttle input.
const float DAC_SUPPLY_V       = 3.30f; // ESP32 DAC full-scale reference, approximately 3.3 V
const float THROTTLE_OFF_V     = 0.00f; // used briefly before controller enable
const float THROTTLE_IDLE_V    = 1.60f; // no-throttle voltage when joystick is centered
const float THROTTLE_MAX_V     = 3.00f; // max throttle voltage

const float maxSteps           = 68.0f;
const float minToMaxTimeMs     = 500.0f; // ramp time from idle to max throttle
const int resetError           = 3;      // encoder counts near center before auto-zero

// Expo values. 0.0 = linear, higher = softer near center.
const float turningExpo        = 0.30f;
const float speedExpo          = 0.30f;

// -----------------------------------------------------------------------------
// Runtime variables
// -----------------------------------------------------------------------------
unsigned long loopTimerStartMs = 0;
float timerAvg = 0.0f;
unsigned long timerCount = 0;

long positionX = -999999;
long positionY = -999999;

float targetLeftV = THROTTLE_IDLE_V;
float targetRightV = THROTTLE_IDLE_V;
float currentLeftV = THROTTLE_IDLE_V;
float currentRightV = THROTTLE_IDLE_V;

float leftThrottleRatio = 0.0f;
float rightThrottleRatio = 0.0f;

unsigned long lastRampTime = 0;
unsigned long lastMoveTime = 0;
bool lastMoveTrue = false;

int oldButton = HIGH;

// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------
long capMaxSteps(long x, float maxS) {
  long limit = lroundf(maxS);

  if (x > limit)  return limit;
  if (x < -limit) return -limit;
  return x;
}

float applyExpo(float x, float expo) {
  return (1.0f - expo) * x + expo * x * x * x;
}

float ratioToVoltage(float ratio) {
  ratio = constrain(ratio, 0.0f, 1.0f);
  return THROTTLE_IDLE_V + ratio * (THROTTLE_MAX_V - THROTTLE_IDLE_V);
}

int voltageToDacValue(float voltage) {
  voltage = constrain(voltage, 0.0f, DAC_SUPPLY_V);
  return lroundf((voltage / DAC_SUPPLY_V) * 255.0f);
}

void writeThrottleVolts(float leftV, float rightV) {
  dacWrite(LEFT_THROTTLE_DAC_PIN, voltageToDacValue(leftV));
  dacWrite(RIGHT_THROTTLE_DAC_PIN, voltageToDacValue(rightV));
}

float rampToward(float current, float target, float maxStep) {
  if (target > current + maxStep) return current + maxStep;
  if (target < current - maxStep) return current - maxStep;
  return target;
}

void printDebug(long x, long y, float turningRatio, float speedRatio) {
  if (!debugMode) return;

  if (showXY) {
    Serial.println();
    Serial.print("X: ");
    Serial.print(x);
    Serial.print(", Y: ");
    Serial.print(y);
  }

  if (showRatios) {
    Serial.print(", Turn: ");
    Serial.print(turningRatio, 3);
    Serial.print(", Speed: ");
    Serial.print(speedRatio, 3);
    Serial.print(", L ratio: ");
    Serial.print(leftThrottleRatio, 3);
    Serial.print(", R ratio: ");
    Serial.print(rightThrottleRatio, 3);
  }

  if (showThrottle) {
    Serial.print(", Target L V: ");
    Serial.print(targetLeftV, 3);
    Serial.print(", Target R V: ");
    Serial.print(targetRightV, 3);
    Serial.print(", DAC L: ");
    Serial.print(voltageToDacValue(currentLeftV));
    Serial.print(", DAC R: ");
    Serial.print(voltageToDacValue(currentRightV));
  }
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
  if (debugMode || enableTimer) {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32 joystick throttle controller starting...");
  }

  pinMode(BUTTON_GND_PIN, OUTPUT);
  digitalWrite(BUTTON_GND_PIN, LOW);
  pinMode(BUTTON_IN_PIN, INPUT_PULLUP);

  pinMode(POWER_ON_PIN, OUTPUT);
  digitalWrite(POWER_ON_PIN, LOW);

  // Start with outputs safe before enabling the controller.
  writeThrottleVolts(THROTTLE_OFF_V, THROTTLE_OFF_V);
  delay(200);

  // Then set no-throttle/idle voltage and enable controller.
  currentLeftV = THROTTLE_IDLE_V;
  currentRightV = THROTTLE_IDLE_V;
  targetLeftV = THROTTLE_IDLE_V;
  targetRightV = THROTTLE_IDLE_V;
  writeThrottleVolts(currentLeftV, currentRightV);

  digitalWrite(POWER_ON_PIN, HIGH);

  lastRampTime = millis();
  lastMoveTime = millis();
}

// -----------------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------------
void loop() {
  if (enableTimer) {
    loopTimerStartMs = millis();
  }

  long rawX = xAxis.read();
  long rawY = yAxis.read();

  long newX = capMaxSteps(rawX, maxSteps);
  long newY = capMaxSteps(rawY, maxSteps);

  // If the encoder count went outside the allowed range, write the capped value
  // back to the encoder object so it does not keep accumulating error.
  if (newX != rawX) xAxis.write(newX);
  if (newY != rawY) yAxis.write(newY);

  // Optional button reset.
  // Uncomment this if the white button should reset the joystick center.
  /*
  int button = digitalRead(BUTTON_IN_PIN);
  if (button != oldButton && button == HIGH) {
    xAxis.write(0);
    yAxis.write(0);
    newX = 0;
    newY = 0;
    Serial.println("Button reset");
  }
  oldButton = button;
  */

  if (newX != positionX || newY != positionY) {
    positionX = newX;
    positionY = newY;
    lastMoveTime = millis();
    lastMoveTrue = true;
  }

  long calcX = newX;
  long calcY = -newY; // keep original direction from Uno code

  // No reverse in this version. Pulling joystick backward gives zero speed.
  if (calcY < 0) {
    calcY = 0;
  }

  // Y controls speed, X controls left/right mixing.
  float turningRatio = (float)calcX / maxSteps;
  float speedRatio   = (float)calcY / maxSteps;

  turningRatio = applyExpo(turningRatio, turningExpo);
  speedRatio   = applyExpo(speedRatio, speedExpo);

  turningRatio = constrain(turningRatio, -1.0f, 1.0f);
  speedRatio   = constrain(speedRatio, 0.0f, 1.0f);

  // ---------------------------------------------------------------------------
  // Finished throttleRatio section
  // ---------------------------------------------------------------------------
  // This keeps the original intended tank/differential mixer:
  //   left  = speed + turn
  //   right = speed - turn
  // Then it normalizes if either side is above 1.0.
  // Negative values are clamped to 0 because this version has no reverse output.
  leftThrottleRatio  = speedRatio + turningRatio;
  rightThrottleRatio = speedRatio - turningRatio;

  float maxMag = fmaxf(fabsf(leftThrottleRatio), fabsf(rightThrottleRatio));
  if (maxMag > 1.0f) {
    leftThrottleRatio  /= maxMag;
    rightThrottleRatio /= maxMag;
  }

  leftThrottleRatio  = constrain(leftThrottleRatio, 0.0f, 1.0f);
  rightThrottleRatio = constrain(rightThrottleRatio, 0.0f, 1.0f);

  targetLeftV  = ratioToVoltage(leftThrottleRatio);
  targetRightV = ratioToVoltage(rightThrottleRatio);

  printDebug(calcX, calcY, turningRatio, speedRatio);

  // If controller is still for 3 seconds near the center, recalibrate position to 0.
  if (((millis() - lastMoveTime) > 3000) &&
      (-resetError <= newX) && (newX <= resetError) &&
      (-resetError <= newY) && (newY <= resetError) &&
      lastMoveTrue) {

    xAxis.write(0);
    yAxis.write(0);
    positionX = 0;
    positionY = 0;
    lastMoveTrue = false;
    lastMoveTime = millis();

    targetLeftV = THROTTLE_IDLE_V;
    targetRightV = THROTTLE_IDLE_V;

    if (debugMode) {
      Serial.println("\nReset axes to zero");
    }
  }

  // Ramp the actual output voltage toward the target every loop.
  unsigned long now = millis();
  float dtMs = (float)(now - lastRampTime);
  lastRampTime = now;

  float throttleRangeV = THROTTLE_MAX_V - THROTTLE_IDLE_V;
  float maxStepV = (throttleRangeV / minToMaxTimeMs) * dtMs;

  currentLeftV  = rampToward(currentLeftV, targetLeftV, maxStepV);
  currentRightV = rampToward(currentRightV, targetRightV, maxStepV);

  writeThrottleVolts(currentLeftV, currentRightV);

  if (enableTimer) {
    unsigned long sample = millis() - loopTimerStartMs;
    timerCount++;
    timerAvg += ((float)sample - timerAvg) / (float)timerCount;

    if (debugMode) {
      Serial.print("\nTimer AVG ms: ");
      Serial.println(timerAvg, 3);
    }
  }
}
