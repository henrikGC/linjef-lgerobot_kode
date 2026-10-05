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

unsigned long lastSeenMs = 0;


// ======================================================
// PD-INNSTILLINGER
// ======================================================

const float proportionalGain = 0.020;
const float derivativeGain = 0.12;

// Maksfart
const int baseSpeed = 240;
const int topSpeed = 240;

// Maksimal vanlig PD-korreksjon
const int maxTurn = 180;


// ======================================================
// DYNAMISK FART I VANLIGE SVINGER
// ======================================================

const int slowdownDivisor = 15;

// Maks nedbremsing:
// 240 - 90 = 150 PWM
const int maxSlowdown = 90;


// ======================================================
// HARD-CORNER-MODUS
// ======================================================

// Når error blir større enn dette,
// regner vi svingen som svært skarp.
//
// Center = 2500
//
// error > 1500 betyr:
// position > 4000 eller position < 1000
const int hardCornerThreshold = 1500;


// I en hard 90-graders sving:
// ytterhjulet kjører raskt fremover
// innerhjulet går svakt i revers.
//
// Dette gjør at roboten roterer mye skarpere
// enn vanlig PD-styring.
const int hardCornerFastSpeed = 240;
const int hardCornerReverseSpeed = 40;


// ======================================================
// LOST-LINE / BUMP-HÅNDTERING
// ======================================================

// Beholder 100 ms for humpene på banen.
//
// Hvis sensoren kortvarig mister linjen,
// fortsetter roboten med siste kjente styring.
const unsigned long lostGraceMs = 100;


// Begrens store hopp i derivative-leddet
const int maxDerivative = 300;


// ======================================================
// SEARCH-HASTIGHETER
// ======================================================

// Hvis linjen faktisk er borte etter 100 ms,
// søker roboten videre mot siden linjen forsvant.
const int searchFastSpeed = 165;
const int searchSlowSpeed = 85;


// Grense for svart linje
const uint16_t lineThreshold = 700;


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

  if (callibrating == HIGH &&
      lastCalibrationState == LOW) {

    isCalibrated = false;
  }

  lastCalibrationState = callibrating;


  // ====================================================
  // MOTOR ON/OFF-KNAPP
  // ====================================================

  bool switching = digitalRead(switchPin);

  if (switching == HIGH &&
      lastSwitchState == LOW) {

    isMotorOn = !isMotorOn;


    if (isMotorOn) {

      lastError = 0;
      lastSeenMs = millis();

      lastLeftSpeed = 0;
      lastRightSpeed = 0;
    }
  }

  lastSwitchState = switching;


  // ====================================================
  // KALIBRERING
  // ====================================================

  if (!isCalibrated) {

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
    // SJEKK OM LINJEN ER SYNLIG
    // ==================================================

    bool noLineDetected = true;


    for (uint8_t i = 0;
         i < SensorCount;
         i++) {

      if (sensorValues[i] >= lineThreshold) {

        noLineDetected = false;
      }
    }


    // ==================================================
    // HUSK HVILKEN SIDE LINJEN FORSVANT PÅ
    // ==================================================

    if (sensorValues[0] >= lineThreshold) {

      lostDirection = 0;
    }


    if (sensorValues[5] >= lineThreshold) {

      lostDirection = 1;
    }


    // ==================================================
    // LINJEN ER SYNLIG
    // ==================================================

    if (!noLineDetected) {

      lastSeenMs = millis();


      // ----------------------------------------------
      // ERROR
      // ----------------------------------------------

      int error =
        (int)position - 2500;


      // ----------------------------------------------
      // DERIVATIVE
      // ----------------------------------------------

      int derivative = constrain(
        error - lastError,
        -maxDerivative,
        maxDerivative
      );


      lastError = error;


      // =================================================
      // HARD-CORNER-MODUS
      // =================================================
      //
      // Hvis linjen ligger helt ute mot kanten av
      // sensorarrayet, trenger vi en mye kraftigere
      // rotasjon enn vanlig PD gir.
      // =================================================

      if (abs(error) > hardCornerThreshold) {


        // ---------------------------------------------
        // HARD CORNER - SIDE 1
        // ---------------------------------------------

        if (error > 0) {

          int rightSpeed =
            hardCornerFastSpeed;

          int leftSpeed =
            -hardCornerReverseSpeed;


          lastRightSpeed = rightSpeed;
          lastLeftSpeed = leftSpeed;


          rightMotor.drive(rightSpeed);
          leftMotor.drive(leftSpeed);
        }


        // ---------------------------------------------
        // HARD CORNER - SIDE 0
        // ---------------------------------------------

        else {

          int rightSpeed =
            -hardCornerReverseSpeed;

          int leftSpeed =
            hardCornerFastSpeed;


          lastRightSpeed = rightSpeed;
          lastLeftSpeed = leftSpeed;


          rightMotor.drive(rightSpeed);
          leftMotor.drive(leftSpeed);
        }
      }


      // =================================================
      // VANLIG PD-STYRING
      // =================================================

      else {

        // ----------------------------------------------
        // PD
        // ----------------------------------------------

        int turn =
          error * proportionalGain +
          derivative * derivativeGain;


        turn = constrain(
          turn,
          -maxTurn,
          maxTurn
        );


        // ----------------------------------------------
        // DYNAMISK FART
        // ----------------------------------------------

        int slowdown =
          abs(error) / slowdownDivisor;


        slowdown = constrain(
          slowdown,
          0,
          maxSlowdown
        );


        int currentBaseSpeed =
          baseSpeed - slowdown;


        // ----------------------------------------------
        // MOTORHASTIGHETER
        // ----------------------------------------------

        int rightSpeed = constrain(
          currentBaseSpeed + turn,
          -topSpeed,
          topSpeed
        );


        int leftSpeed = constrain(
          currentBaseSpeed - turn,
          -topSpeed,
          topSpeed
        );


        lastRightSpeed = rightSpeed;
        lastLeftSpeed = leftSpeed;


        rightMotor.drive(rightSpeed);
        leftMotor.drive(leftSpeed);
      }
    }


    // ==================================================
    // LINJEN MIDLERTIDIG BORTE
    //
    // Sannsynligvis hump.
    // Fortsett med siste kjente styring i opptil 100 ms.
    // ==================================================

    else if (
      millis() - lastSeenMs < lostGraceMs
    ) {

      rightMotor.drive(lastRightSpeed);
      leftMotor.drive(lastLeftSpeed);
    }


    // ==================================================
    // LINJEN FORTSATT BORTE
    // SEARCH SIDE 0
    // ==================================================

    else if (lostDirection == 0) {

      rightMotor.drive(searchSlowSpeed);
      leftMotor.drive(searchFastSpeed);
    }


    // ==================================================
    // LINJEN FORTSATT BORTE
    // SEARCH SIDE 1
    // ==================================================

    else if (lostDirection == 1) {

      rightMotor.drive(searchFastSpeed);
      leftMotor.drive(searchSlowSpeed);
    }


    // ==================================================
    // VI VET IKKE HVOR LINJEN ER
    // ==================================================

    else {

      rightMotor.stop();
      leftMotor.stop();
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