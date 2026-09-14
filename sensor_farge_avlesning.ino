#include <QTRSensors.h>

QTRSensors qtr;

const uint8_t SensorCount = 6;
uint16_t sensorValues[SensorCount];

// PIN VARIABLES
// the motor will be controlled by the motor A pins on the motor driver
const int AIN1 = 13; // control pin 1 on the motor driver for the right motor
const int AIN2 = 12; // control pin 2 on the motor driver for the right motor
const int PWMA = 11; // speed control pin on the motor driver for the right motor
const int BIN1 = 8;
const int BIN2 = 9;
const int PWMB = 10;

// VARIABLES
int motorSpeed = 0; // starting speed for the motor

void setup()
{
  // configure the sensors
  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){A0, A1, A2, A3, A4, A5}, SensorCount);
  qtr.setEmitterPin(2);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);

  delay(500);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // turn on Arduino's LED to indicate we are in calibration mode

  // analogRead() takes about 0.1 ms on an AVR.
  // 0.1 ms per sensor * 4 samples per sensor read (default) * 6 sensors
  // * 10 reads per calibrate() call = ~24 ms per calibrate() call.
  // Call calibrate() 400 times to make calibration take about 10 seconds.
  for (uint16_t i = 0; i < 400; i++)
  {
    qtr.calibrate();
  }
  digitalWrite(LED_BUILTIN, LOW); // turn off Arduino's LED to indicate we are through with calibration

  // print the calibration minimum values measured when emitters were on
  Serial.begin(9600);
  for (uint8_t i = 0; i < SensorCount; i++)
  {
    Serial.print(qtr.calibrationOn.minimum[i]);
    Serial.print(' ');
  }
  Serial.println();

  // print the calibration maximum values measured when emitters were on
  for (uint8_t i = 0; i < SensorCount; i++)
  {
    Serial.print(qtr.calibrationOn.maximum[i]);
    Serial.print(' ');
  }
  Serial.println();
  Serial.println();
  delay(1000);
}

void loop()
{
  // read calibrated sensor values and obtain a measure of the line position
  // from 0 to 5000 (for a white line, use readLineWhite() instead)
  uint16_t position = qtr.readLineBlack(sensorValues);

  // print the sensor values as numbers from 0 to 1000, where 0 means maximum
  // reflectance and 1000 means minimum reflectance, followed by the line
  // position
  for (uint8_t i = 0; i < SensorCount; i++)
  {
    Serial.print(sensorValues[i]);
    Serial.print('\t');
  }
  Serial.println(position);
  /*
  delay(250);

  digitalWrite(AIN1, HIGH); // set pin 1 to high
  analogWrite(PWMA, 255);
  digitalWrite(BIN1, LOW); // set pin 1 to high
  digitalWrite(BIN2, HIGH);  // set pin 2 to low
  digitalWrite(AIN2, LOW);  // set pin 2 to low
  analogWrite(PWMB, 255);
  delay(250);

  digitalWrite(AIN1, LOW);  // set pin 1 to low
  digitalWrite(AIN2, HIGH); // set pin 2 to high
  analogWrite(PWMA, 255);
  digitalWrite(BIN1, HIGH);  // set pin 1 to low
  digitalWrite(BIN2, LOW); // set pin 2 to high
  analogWrite(PWMB, 255);

  delay(250);
  digitalWrite(AIN1, LOW); // set pin 1 to low
  digitalWrite(AIN2, LOW); // set pin 2 to low
  analogWrite(PWMA, 0);
  digitalWrite(BIN1, LOW); // set pin 1 to low
  digitalWrite(BIN2, LOW); // set pin 2 to low
  analogWrite(PWMB, 0);*/
}

//https://learn.sparkfun.com/tutorials/activity-guide-for-sparkfun-tinker-kit/circuit-10-motor-basics