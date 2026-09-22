#include <QTRSensors.h>

QTRSensors qtr;

const uint8_t SensorCount = 6;
uint16_t sensorValues[SensorCount];

const int switchPin = 3;
bool isMotorOn = false;

const int AIN1 = 13;
const int AIN2 = 12;
const int PWMA = 11;
const int BIN1 = 8;
const int BIN2 = 9;
const int PWMB = 10;

const int speed = 175;
int motorSpeed = 0;


bool isCalibrated = false;

bool isPrintingSensorData = true;
bool isPrintingPosition = true;


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

  for (uint16_t i = 0; i < 400; i++) {
    qtr.calibrate();
    delay(10);
  }

  Serial.println("Sensor test starter...");
}

void loop()
{
  bool switching = digitalRead(switchPin);
  if (switching) {
    isMotorOn = !isMotorOn;
  }
  
  // Leser råverdiene fra alle 6 sensorene
  //qtr.read(sensorValues);
  qtr.readCalibrated(sensorValues);

  for (uint8_t i = 0; i < SensorCount; i++)
  {
    Serial.print(sensorValues[i]);
    Serial.print('\t');
  }

  Serial.println();
  
  uint16_t position = qtr.readLineBlack(sensorValues);
  Serial.println(position);

    // position = 1000;

  if (isMotorOn) {

  if (position > 2000 && position < 3000)
  {
    Serial.println("forward");

    rightMotor.driveForward(speed);
    leftMotor.driveForward(speed);
  }

  else if (position <= 2000)
  {
    Serial.println("left");

    leftMotor.driveForward(speed);
    rightMotor.driveBackward(speed);

    //turn left;
  }

  else if (position >= 3000)
  {
    Serial.println("right");

    leftMotor.driveBackward(speed);
    rightMotor.driveForward(speed);

    //turn right;
  }

  else
  {
    Serial.println("ERROR");
  }
  }
  else 
{
  // Bryteren er AV
  rightMotor.stop();
  leftMotor.stop();

  Serial.println("MOTORS OFF");
}
  
  delay(250);
}