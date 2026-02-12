#include <GyverMotor2.h>
#include <EnableInterrupt.h>
#include <GyverMotor2.h>

int MOTOR1_HALL1_PIN = 2;
int MOTOR1_HALL2_PIN = 3;
int MOTOR2_HALL1_PIN = 8;
int MOTOR2_HALL2_PIN = 9;

volatile long encoder1Count = 0;
volatile long encoder2Count = 0;

float rot1 = 0;
float rot2 = 0;
float rot1Wanted = 0;
float rot2Wanted = 0;
bool move1;
bool move2;

const int PULSES_PER_REVOLUTION = 1970;

GMotor2<DRIVER2WIRE> motor1(4, 5);
GMotor2<DRIVER2WIRE> motor2(7, 6);

float P1Index = 270;
float MinimumSpeed1 = 100;
float voltage = 0;
void setup() {
  calVol();
  Serial.begin(9600);
  motor1.setMinDuty(70);
  motor2.setMinDuty(65);
  Serial.println("Двойной квадратурный энкодер Холла + направление");

  pinMode(MOTOR1_HALL1_PIN, INPUT_PULLUP);
  pinMode(MOTOR1_HALL2_PIN, INPUT_PULLUP);
  pinMode(MOTOR2_HALL1_PIN, INPUT_PULLUP);
  pinMode(MOTOR2_HALL2_PIN, INPUT_PULLUP);

  enableInterrupt(MOTOR1_HALL1_PIN, updateEncoder1, CHANGE);
  enableInterrupt(MOTOR1_HALL2_PIN, updateEncoder1, CHANGE);

  enableInterrupt(MOTOR2_HALL1_PIN, updateEncoder2, CHANGE);
  enableInterrupt(MOTOR2_HALL2_PIN, updateEncoder2, CHANGE);
}

void loop() {
  static unsigned long lastDisplay = 0;

  noInterrupts();
  long cnt1 = encoder1Count;
  long cnt2 = -encoder2Count;
  rot1 = float(cnt1) / PULSES_PER_REVOLUTION;
  rot2 = float(cnt2) / PULSES_PER_REVOLUTION;
  interrupts();

/*
  Serial.print("М1: ");
  if (cnt1 >= 0) Serial.print("+");
  else Serial.print("-");
  Serial.print(abs(cnt1));
  Serial.print(" имп  →  ");
  Serial.print(float(cnt1) / PULSES_PER_REVOLUTION, 3);
  Serial.print(" об   ");

  Serial.print("   М2: ");
  if (cnt2 >= 0) Serial.print("+");
  else Serial.print("-");
  Serial.print(abs(cnt2));
  Serial.print(" имп  →  ");
  Serial.print(float(cnt2) / PULSES_PER_REVOLUTION, 3);
  Serial.println(" об");
*/
  Serial.println("Voltage: " + String(voltage));
  if (Serial.available()) {
    String message = Serial.readStringUntil('\n');
    float revol = message.toFloat();
    Revolution(revol);
    calVol();
  }

  if (move1 == true) {
    float dif = abs(rot1Wanted - rot1);
    motor1.setSpeed(dif * P1Index + MinimumSpeed1);
    if ((dif * 360) < 10 || rot1 > rot1Wanted) {
      motor1.setSpeed(maxF(-rot1Wanted * 50, -255)); 
      delay(50);     
      motor1.stop();
      move1 = false;
    }
  }
}

void calVol () {
  voltage = analogRead(A0) / 102.4;
  MinimumSpeed1 = map(voltage, 6, 13, 120, 90);
  P1Index = map(voltage, 6, 13, 300, 210);
}
float maxF (float a, float b) {
  if (a >= b) {
    return a;
  }
  if (b > a) {
    return b;
  }
}
void Revolution(float revolutions) {
  encoder1Count = 0;
  encoder2Count = 0;
  if (revolutions < 0) {
    motor1.setSpeed(-(255 * P1Index) - MinimumSpeed1);
  } else if (revolutions > 0) {
    motor1.setSpeed(255 * P1Index + MinimumSpeed1);
  } else if (revolutions == 0) {
    motor1.stop();
  }
  move1 = true;
  rot1Wanted = revolutions;
}

void checkEncoder(int motorEncPin1, int motorEncPin2, volatile long& motorCount) {
  static byte prevState = 0;
  byte A = digitalRead(motorEncPin2);
  byte B = digitalRead(motorEncPin1);

  byte currentState = (A << 1) | B;

  switch (prevState) {
    case 0b00:
      if (currentState == 0b01) motorCount++;
      if (currentState == 0b10) motorCount--;
      break;
    case 0b01:
      if (currentState == 0b11) motorCount++;
      if (currentState == 0b00) motorCount--;
      break;
    case 0b11:
      if (currentState == 0b10) motorCount++;
      if (currentState == 0b01) motorCount--;
      break;
    case 0b10:
      if (currentState == 0b00) motorCount++;
      if (currentState == 0b11) motorCount--;
      break;
  }

  prevState = currentState;
}

void updateEncoder1() {
  checkEncoder(MOTOR1_HALL1_PIN, MOTOR1_HALL2_PIN, encoder1Count);
}

void updateEncoder2() {
  checkEncoder(MOTOR2_HALL1_PIN, MOTOR2_HALL2_PIN, encoder2Count);
}
