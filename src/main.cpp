// --- 1. Configuration & Masks ---
#include <Arduino.h>

//Master Masks (For setup only)
//PB3, PB4, PB5
#define RIGHT_MASK 0x38
//PD5, PD6, PD7
#define LEFT_MASK 0xE0

//Movement Masks
//Right: (Port B): IN1(0x10) is FWD, IN2(0x20) is REV
#define RIGHT_FWD 0x10
#define RIGHT_REV 0x20
//0x30
#define RIGHT_DIR_CLEAR (RIGHT_FWD | RIGHT_REV)

//Left: (Port D): IN4(0x40) is FWD, IN3(0x80) is REV
#define LEFT_FWD 0x40
#define LEFT_REV 0x80
//0xC0
#define LEFT_DIR_CLEAR (LEFT_FWD | LEFT_REV)

// --- 2. Global Variables ---
const int SLOW_SPEED = 150;
const int MED_SPEED = 200;
const int FAST_SPEED = 255;

unsigned long previousT = 0;
int moveStep = 0;

// --- 3. Initialization (setup) ---
void setup() {
    //Set motor bits to 1 (Output) on DDRB and DDRD
    DDRB |= RIGHT_MASK;
    DDRD |= LEFT_MASK;

    //Set default state to LOW (Stopped)
    PORTB &= ~RIGHT_MASK;
    PORTD &= ~LEFT_MASK;

    //Record the starting time
    previousT = millis();
}

// --- 4. Main Program Loop ---
void loop() {
    unsigned long currentT = millis();

    switch (moveStep) {
        case 0: //Forward cruise at MED_SPEED for 2 seconds
            moveForward(MED_SPEED);
            if (currentT - previousT >= 2000) {
                previousT = currentT;
                moveStep++;
            }
            break;

            case 1: //Arc left 1.5 seconds
            turnLeft(SLOW_SPEED, MED_SPEED);
            if (currentT - previousT >= 1500) {
                previousT = currentT;
                moveStep++;
            }
            break;

            case 2: //Reverse at SLOW_SPEED for 2 seconds
            moveBackward(SLOW_SPEED);
            if (currentT - previousT >= 2000) {
                previousT = currentT;
                moveStep++;
            }
            break;

            case 3: //Arc right in reverse for 1.5 seconds
            turnRightReverse(SLOW_SPEED, MED_SPEED);
            if (currentT - previousT >= 1500) {
                previousT = currentT;
                moveStep++;
            }
            break;

            case 4: //Stop, restart after 3 seconds
            stopMotors();
            if (currentT - previousT >= 3000) {
                previousT = currentT;
                moveStep = 0; //Restart sequence
            }
            break;    
    }
}

// --- 5. Movement Functions ---

void moveForward(int speed) {
    //Wipe PB4, PB5
    PORTB &= ~RIGHT_DIR_CLEAR;
    //Wipe PD6, PD7
    PORTD &= ~LEFT_DIR_CLEAR;
    //Set PB4 HIGH
    PORTB |= RIGHT_FWD;
    //Set PD6 HIGH
    PORTD |= LEFT_FWD;
    //ENA
    analogWrite(11, speed);
    //ENB
    analogWrite(5, speed);
}

void moveBackward(int speed) {
    //Wipe PB4, PB5
    PORTB &= ~RIGHT_DIR_CLEAR;
    //Wipe PD6, PD7
    PORTD &= ~LEFT_DIR_CLEAR;
    //Set PB5 HIGH
    PORTB |= RIGHT_REV;
    //Set PD7 HIGH
    PORTD |= LEFT_REV;
    //ENA
    analogWrite(11, speed);
    //ENB
    analogWrite(5, speed);
}
 
void turnLeft(int leftSpeed, int rightSpeed) {
    //Both sides forward for smooth arc
    PORTB &= ~RIGHT_DIR_CLEAR;
    PORTD &= ~LEFT_DIR_CLEAR;
    PORTB |= RIGHT_FWD;
    PORTD |= LEFT_FWD;

    //Apply different speeds to each motor
    //ENA (Outer wheel)
    analogWrite(11, rightSpeed);
    //ENB (Inner wheel)
    analogWrite(5, leftSpeed);
}

void turnRight(int leftSpeed, int rightSpeed) {
    //Both sides forward for smooth arc
    PORTB &= ~RIGHT_DIR_CLEAR;
    PORTD &= ~LEFT_DIR_CLEAR;
    PORTB |= RIGHT_FWD;
    PORTD |= LEFT_FWD;

    //Apply different speeds to each motor
    //ENA (Inner wheel)
    analogWrite(11, leftSpeed);
    //ENB (Outer wheel)
    analogWrite(5, rightSpeed);
}

void turnRightReverse(int leftSpeed, int rightSpeed) {
    //Both sides reverse for smooth arc
    PORTB &= ~RIGHT_DIR_CLEAR;
    PORTD &= ~LEFT_DIR_CLEAR;
    PORTB |= RIGHT_REV;
    PORTD |= LEFT_REV;

    //Apply different speeds to each motor
    //ENA (Inner wheel)
    analogWrite(11, leftSpeed);
    //ENB (Outer wheel)
    analogWrite(5, rightSpeed);
}

void turnLeftReverse(int leftSpeed, int rightSpeed) {
    //Both sides reverse for smooth arc
    PORTB &= ~RIGHT_DIR_CLEAR;
    PORTD &= ~LEFT_DIR_CLEAR;
    PORTB |= RIGHT_REV;
    PORTD |= LEFT_REV;

    //Apply different speeds to each motor
    //ENA (Outer wheel)
    analogWrite(11, rightSpeed);
    //ENB (Inner wheel)
    analogWrite(5, leftSpeed);
}

void stopMotors() {
    //Wipe all direction bits to LOW (Electronic Brake)
    PORTB &= ~RIGHT_DIR_CLEAR;
    PORTD &= ~LEFT_DIR_CLEAR;

    //Set PWM to 0
    analogWrite(11, 0);
    analogWrite(5, 0);
}