#include <QTRSensors.h>

QTRSensors qtr;

const uint8_t SensorCount = 6;
uint16_t sensorValues[SensorCount];


// ======================================================
// KNAPPER
// ======================================================

const int switchPin = 3;
const int callibrationPin = 7;

bool isMotorOn = false;
bool isCalibrated = true;

bool lastSwitchState = LOW;
bool lastCalibrationState = LOW;


// ======================================================
// SENSOR / STYRING
// ======================================================

int lastError = 0;

bool isCalibrationLightOn = false;

int lastLeftSpeed = 0;
int lastRightSpeed = 0;

int lostDirection = 2;


// ======================================================
// PD-INNSTILLINGER
// ======================================================

const float proportionalGain = 0.020;
const float derivativeGain = 0.12;

// ØKT HASTIGHET
const int baseSpeed = 150;
const int topSpeed = 160;


// ======================================================
// MOTORPINNER
// ======================================================

const int AIN1 = 13;
const int AIN2 = 12;
const int PWMA = 11;

const int BIN1 = 8;
const int BIN2 = 9;
const int PWMB = 10;


// ======================================================
// MOTOR CLASS
// ======================================================

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


    void drive(int speed) {

      if (speed > 0) {

        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);

        analogWrite(PWM, speed);
      }

      else if (speed < 0) {

        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);

        analogWrite(PWM, -speed);
      }

      else {

        stop();
      }
    }


    void stop() {

      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);

      analogWrite(PWM, 0);
    }
};


// ======================================================
// MOTOROBJEKTER
// ======================================================

Motor rightMotor(AIN1, AIN2, PWMA);
Motor leftMotor(BIN1, BIN2, PWMB);


// ======================================================
// SETUP
// ======================================================

void setup()
{

  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  // Knappene bruker eksterne 10k pull-down motstander
  pinMode(switchPin, INPUT);
  pinMode(callibrationPin, INPUT);


  // Motorer
  leftMotor.begin();
  rightMotor.begin();


  // Zumo reflectance sensor array bruker RC
  qtr.setTypeRC();


  // Sensorene i fysisk rekkefølge
  qtr.setSensorPins(
    (const uint8_t[]){4, A3, 6, A0, A2, 5},
    SensorCount
  );


  // LEDON på sensoren koblet til D2
  qtr.setEmitterPin(2);


  Serial.println("Robot klar");
}


// ======================================================
// LOOP
// ======================================================

void loop()
{

  // ====================================================
  // KALIBRERINGSKNAPP
  // ====================================================

  bool callibrating = digitalRead(callibrationPin);


  // Reagerer bare på LOW -> HIGH
  if (callibrating == HIGH &&
      lastCalibrationState == LOW) {

    isCalibrated = false;
  }


  lastCalibrationState = callibrating;



  // ====================================================
  // MOTOR ON/OFF-KNAPP
  // ====================================================

  bool switching = digitalRead(switchPin);


  // Reagerer bare på LOW -> HIGH
  if (switching == HIGH &&
      lastSwitchState == LOW) {

    isMotorOn = !isMotorOn;


    // Nullstill D-leddet når roboten startes
    if (isMotorOn) {

      lastError = 0;
    }
  }


  lastSwitchState = switching;



  // ====================================================
  // KALIBRERING
  // ====================================================

  if (!isCalibrated) {

    // Motorene skal ikke kjøre under kalibrering
    rightMotor.stop();
    leftMotor.stop();


    for (uint16_t i = 0; i < 200; i++) {

      qtr.calibrate();


      digitalWrite(
        LED_BUILTIN,
        isCalibrationLightOn
      );


      isCalibrationLightOn =
        !isCalibrationLightOn;


      delay(10);
    }


    digitalWrite(LED_BUILTIN, LOW);


    isCalibrated = true;


    // Nullstill regulatoren etter kalibrering
    lastError = 0;
  }



  // ====================================================
  // LES SENSOR + FINN LINJEPOSISJON
  // ====================================================

  uint16_t position =
    qtr.readLineBlack(sensorValues);



  // ====================================================
  // MOTORSTYRING
  // ====================================================

  if (isMotorOn) {


    // ==================================================
    // PD-REGULATOR
    // ==================================================

    int error =
      position - 2500;


    int derivative =
      error - lastError;


    lastError =
      error;


    int turn =
      error * proportionalGain +
      derivative * derivativeGain;



    // ==================================================
    // BEREGN MOTORHASTIGHET
    // ==================================================

    int rightSpeed =
      constrain(
        baseSpeed + turn,
        -topSpeed,
        topSpeed
      );


    int leftSpeed =
      constrain(
        baseSpeed - turn,
        -topSpeed,
        topSpeed
      );



    // ==================================================
    // SJEKK OM LINJEN ER SYNLIG
    // ==================================================

    bool noLineDetected = true;


    for (uint8_t i = 0;
         i < SensorCount;
         i++) {

      if (sensorValues[i] >= 700) {

        noLineDetected = false;
      }
    }



    // ==================================================
    // HUSK HVILKEN SIDE LINJEN FORSVANT PÅ
    // ==================================================

    if (sensorValues[0] >= 700) {

      lostDirection = 0;
    }


    if (sensorValues[5] >= 700) {

      lostDirection = 1;
    }



    // ==================================================
    // LINJEN ER BORTE
    // ==================================================

    if (noLineDetected) {


      // Linjen forsvant på venstre side
      if (lostDirection == 0) {

        rightMotor.drive(-topSpeed);
        leftMotor.drive(topSpeed);
      }


      // Linjen forsvant på høyre side
      else if (lostDirection == 1) {

        rightMotor.drive(topSpeed);
        leftMotor.drive(-topSpeed);
      }


      // Vi vet ikke hvor linjen er
      else {

        rightMotor.stop();
        leftMotor.stop();
      }
    }



    // ==================================================
    // NORMAL LINJEFØLGING
    // ==================================================

    else {

      lastLeftSpeed = leftSpeed;
      lastRightSpeed = rightSpeed;


      rightMotor.drive(rightSpeed);
      leftMotor.drive(leftSpeed);
    }
  }



  // ====================================================
  // MOTORER AV
  // ====================================================

  else {

    rightMotor.stop();
    leftMotor.stop();
  }
}