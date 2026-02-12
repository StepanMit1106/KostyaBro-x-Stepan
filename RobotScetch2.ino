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
float P2Index = 270;
float MinimumSpeed1 = 100;
float MinimumSpeed2 = 100;
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

  Serial.println("Voltage: " + String(voltage));
  
  if (Serial.available()) {
    String message = Serial.readStringUntil('\n');
    // Пример команд:
    // 5.5     → оба мотора на 5.5 оборота
    // 1:3.2   → мотор 1 → 3.2 об
    // 2:-1.8  → мотор 2 → -1.8 об
    // 0       → оба стоп
    
    message.trim();
    
    if (message.indexOf(':') != -1) {
      int colonPos = message.indexOf(':');
      String motorStr = message.substring(0, colonPos);
      String revStr  = message.substring(colonPos + 1);
      
      int motorNum = motorStr.toInt();
      float revol = revStr.toFloat();
      
      if (motorNum == 1) {
        Revolution1(revol);
      } else if (motorNum == 2) {
        Revolution2(revol);
      }
    } else {
      float revol = message.toFloat();
      Revolution1(revol);
      Revolution2(revol);
    }
    
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

  if (move2 == true) {
    float dif = abs(rot2Wanted - rot2);
    motor2.setSpeed(dif * P2Index + MinimumSpeed2);
    if ((dif * 360) < 10 || rot2 > rot2Wanted) {
      motor2.setSpeed(maxF(-rot2Wanted * 50, -255)); 
      delay(50);     
      motor2.stop();
      move2 = false;
    }
  }
}

void calVol () {
  voltage = analogRead(A0) / 102.4;
  MinimumSpeed1 = map(voltage, 6, 13, 120, 90);
  P1Index = map(voltage, 6, 13, 300, 210);
  MinimumSpeed2 = map(voltage, 6, 13, 115, 85);
  P2Index = map(voltage, 6, 13, 290, 200);
}

float maxF (float a, float b) {
  if (a >= b) {
    return a;
  }
  if (b > a) {
    return b;
  }
}

void Revolution1(float revolutions) {
  noInterrupts();
  encoder1Count = 0;
  interrupts();
  
  rot1Wanted = revolutions;
  
  if (revolutions < 0) {
    motor1.setSpeed(-(255 * P1Index) - MinimumSpeed1);
  } else if (revolutions > 0) {
    motor1.setSpeed(255 * P1Index + MinimumSpeed1);
  } else {
    motor1.stop();
    move1 = false;
    return;
  }
  
  move1 = true;
}

void Revolution2(float revolutions) {
  noInterrupts();
  encoder2Count = 0;
  interrupts();
  
  rot2Wanted = revolutions;
  
  if (revolutions < 0) {
    motor2.setSpeed(-(255 * P2Index) - MinimumSpeed2);
  } else if (revolutions > 0) {
    motor2.setSpeed(255 * P2Index + MinimumSpeed2);
  } else {
    motor2.stop();
    move2 = false;
    return;
  }
  
  move2 = true;
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