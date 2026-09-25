#include <QTRSensors.h>
#include <algorithm>
QTRSensors qtr;

const uint8_t SensorCount = 6;
uint16_t sensorValues[SensorCount];

const int switchPin = 3;
bool isMotorOn = false;



int isCalibrationLightOn = false;


const int AIN1 = 13;
const int AIN2 = 12;
const int PWMA = 11;
const int BIN1 = 8;
const int BIN2 = 9;
const int PWMB = 10;

const int speed = 175;
int motorSpeed = 0;

const int callibrationPin = 7;
bool isCalibrated = true;

bool isPrintingSensorData = true;
bool isPrintingPosition = true;

uint16_t lastKnownPosition;

class Motor {

  private:
    int IN1;
    int IN2;
    int PWM;


  public:

    Motor(int in1, int in2, int pwm) {
      IN1 = in1;
      IN2 = in2;
      PWM = pwm;
    }

    void begin() {
      pinMode(IN1, OUTPUT);
      pinMode(IN2, OUTPUT);
      pinMode(PWM, OUTPUT);
    }

    void driveForward(int speed) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      analogWrite(PWM, speed);
    }

    void driveBackward(int speed) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      analogWrite(PWM, speed);
    }

    void stop() {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      analogWrite(PWM, 0);
    }

};


Motor rightMotor(AIN1, AIN2, PWMA);
Motor leftMotor(BIN1, BIN2, PWMB);

void setup()
{
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  leftMotor.begin();
  rightMotor.begin();

  // Zumo reflectance array bruker RC
  qtr.setTypeRC();

  // Sensorene i fysisk rekkefølge
  qtr.setSensorPins(
    (const uint8_t[]){4, A3, 6, A0, A2, 5},
    SensorCount
  );

  // LEDON koblet til D2
  qtr.setEmitterPin(2);



  Serial.println("Sensor test starter...");
  
}

void loop()
{

  bool callibrating = digitalRead(callibrationPin);
  if (callibrating) {
    isCalibrated = false;
  }


  bool switching = digitalRead(switchPin);
  if (switching) {
    isMotorOn = !isMotorOn;
  }
  
  // Leser råverdiene fra alle 6 sensorene
  //qtr.read(sensorValues);

  if (!isCalibrated) {
    for (uint16_t i = 0; i < 400; i++) {
      qtr.calibrate();
      digitalWrite(LED_BUILTIN, isCalibrationLightOn);
      isCalibrationLightOn = !isCalibrationLightOn;
      delay(10);
    }
    isCalibrated = true;
  }

  qtr.readCalibrated(sensorValues);

  for (uint8_t i = 0; i < SensorCount; i++)
  {
    Serial.print(sensorValues[i]);
    Serial.print('\t');
  }

  Serial.println();
  
  uint16_t position = qtr.readLineBlack(sensorValues);
  Serial.println(position);

  bool noLine = false;
  if (!sensorValues.includes(1000)) {
    bool noLine = true;
  }
  else {
    lastKnownPosition = position;
  }

    // position = 1000;

  if (isMotorOn) {
    
    float proportionalGain = 0.08;
    int error = position - 2500;
    int turn = error * proportionalGain;
    int motorBuff = 55;


    int topSpeed = 100;
    int baseRight = topSpeed;
    int baseLeft  = topSpeed;


    int rightSpeed = constrain(baseRight + turn, 0, topSpeed);
    int leftSpeed  = constrain(baseLeft  - turn, 0, topSpeed);
    if (!noLine) {
      rightMotor.driveForward(rightSpeed);
      leftMotor.driveForward(leftSpeed);
    }
    else {

  if (lastKnownPosition < 2500)
  {
    Serial.println("left");

    rightMotor.driveBackward(speed);
    leftMotor.driveForward(speed - 60);

    //turn left;
  }

  else if (lastKnownPosition > 2500)
  {
    Serial.println("right");

    rightMotor.driveForward(speed);
    leftMotor.driveBackward(speed - 60);

    //turn right;
  }


  }
  else 
{
  // Bryteren er AV
  rightMotor.stop();
  leftMotor.stop();

  Serial.println("MOTORS OFF");
}
  
  //delay(250);
}