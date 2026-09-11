class Motor {
public:
    int speedPin;
    int directionPin;

    Motor(int speed, int direction) {
        speedPin = speed;
        directionPin = direction;
    }

    void begin() {
        pinMode(speedPin, OUTPUT);
        pinMode(directionPin, OUTPUT);
    }
};

Motor leftMotor(5, 7);
Motor rightMotor(6, 8);

void setup() {
    leftMotor.begin();
    rightMotor.begin();
}