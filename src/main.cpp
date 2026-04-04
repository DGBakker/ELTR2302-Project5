// --- 1. Configuration & Masks ---
#include <Arduino.h>

//Master Masks (For setup only)
//PB3, PB4, PB5
#define RIGHT_MASK 0x38
//PD5, PD6, PD7
#define LEFT_MASK 0xE0

//Movement Masks
//Right: (Port B): IN1(0x20) is FWD, IN2(0x10) is REV
#define RIGHT_FWD 0x20
#define RIGHT_REV 0x10
//0x30
#define RIGHT_DIR_CLEAR (RIGHT_FWD | RIGHT_REV)

//Left: (Port D): IN4(0x40) is FWD, IN3(0x80) is REV
#define LEFT_FWD 0x40
#define LEFT_REV 0x80
//0xC0
#define LEFT_DIR_CLEAR (LEFT_FWD | LEFT_REV)

// --- 2. Global Variables ---
// Pattern: 1 = Boomerang, 2 = Square, 3 = Ultrasonic Obstacle Avoidance
int patternID = 1;
// Scale: 1.0 = Small (Base times), 2.0 = Medium (Double times), etc.
float spaceScale = 1.0;
// Tracks how many sides of the square have been completed (For pattern 2)
int sideCount = 0;

const int SLOW_SPEED = 120;
const int MED_SPEED = 180;
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

    Serial.begin(9600);
    Serial.println("UAV Project 5 - Part A Initialized.");
}

// --- 4. Main Program Loop ---
void loop() {
    unsigned long currentT = millis();

    switch (patternID) {
        case 1: // --- DOUBLE BOOMERANG (Zero Displacement) ---
            switch (moveStep) {
                case 0: //Forward straight (800ms)
                    moveForward(MED_SPEED);
                    if (currentT - previousT >= (unsigned long)(800 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        Serial.println("State 1: Arc Left Forward");
                    }
                    break;

                case 1: //Arc left forward (1000ms)
                    turnLeft(SLOW_SPEED, FAST_SPEED);
                    if (currentT - previousT >= (unsigned long)(1000 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        Serial.println("State 2: Arc Left Reverse");
                    }
                    break;

                case 2: //Arc left reverse (1000ms)
                    turnLeftReverse(SLOW_SPEED, FAST_SPEED);
                    if (currentT - previousT >= (unsigned long)(1000 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        Serial.println("State 3: Arc Right Forward");
                    }
                    break;

                case 3: //Arc right forward (1000ms)
                    turnRight(SLOW_SPEED, FAST_SPEED);
                    if (currentT - previousT >= (unsigned long)(1000 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        Serial.println("State 4: Arc Right Reverse");
                    }
                    break;

                case 4: //Arc right reverse (1000ms)
                    turnRightReverse(SLOW_SPEED, FAST_SPEED);
                    if (currentT - previousT >= (unsigned long)(1000 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        Serial.println("State 5: Backward Straight");
                    }
                    break;

                case 5: //Backward straight (800ms)
                    moveBackward(MED_SPEED);
                    if (currentT - previousT >= (unsigned long)(800 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        Serial.println("State 6: Stop");
                    }
                    break;

                case 6: //Stop, restart after 3 seconds
                    stopMotors();
                    if (currentT - previousT >= 3000) {
                        previousT = currentT;
                        moveStep = 0; //Restart sequence
                        Serial.println("State 0: Move forward");
                    }
                    break;
            }
            break; //End Pattern 1

        case 2: // --- SQUARE PATTERN ---
            switch (moveStep) {
                case 0: //Forward leg
                    moveForward(MED_SPEED);
                    if (currentT - previousT >= (unsigned long)(1000 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        Serial.println("Square Step 1: Pivot Left 90");
                    }
                    break;
                
                case 1: //Pivot left 90 degrees
                    pivotLeft(MED_SPEED);
                    if (currentT - previousT >= 500) {
                        previousT = currentT;
                        sideCount++;
                        moveStep++;
                        Serial.println("Square Step 2: Forward Leg");
                    }
                    break;
                
                case 2: //Check if square is complete, if not, repeat.
                    if (sideCount < 4) {
                        moveStep = 0; //Back to case 0
                        Serial.print("Starting side: ");
                        Serial.println(sideCount + 1); 
                    }
                    else {
                        sideCount = 0; //Reset
                        moveStep++; //Go to case 3 (Stop)
                        Serial.println("Square Complete.");
                    }
                    break;
                
                case 3: //Stop, restart after 3 seconds
                    stopMotors();
                    if (currentT - previousT >= 3000) {
                        previousT = currentT;
                        moveStep = 0; //Restart sequence
                        Serial.println("Starting Square Pattern.");
                    }
                    break;
            }
            break; //End Pattern 2

        case 3: // --- ULTRASONIC TEST ---
            // Placeholder for now
            stopMotors();
            break;
    }
}

// --- 5. Movement Functions ---

void moveForward(int speed) {
    //Wipe PB4, PB5
    PORTB &= ~RIGHT_DIR_CLEAR;
    //Wipe PD6, PD7
    PORTD &= ~LEFT_DIR_CLEAR;
    //Set PB5 HIGH
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
    //Set PB4 HIGH
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

void pivotLeft(int speed) {
    //Left side reverse, right side forward
    PORTB &= ~RIGHT_DIR_CLEAR;
    PORTD &= ~LEFT_DIR_CLEAR;
    PORTB |= RIGHT_FWD;
    PORTD |= LEFT_REV;

    //Apply same speed, opposite directions for pivot
    //ENA (Right wheel)
    analogWrite(11, speed);
    //ENB (Left wheel)
    analogWrite(5, speed);
}

void pivotRight(int speed) {
    //Right side reverse, left side forward
    PORTB &= ~RIGHT_DIR_CLEAR;
    PORTD &= ~LEFT_DIR_CLEAR;
    PORTB |= RIGHT_REV;
    PORTD |= LEFT_FWD;

    //Apply same speed, opposite directions for pivot
    //ENA (Right wheel)
    analogWrite(11, speed);
    //ENB (Left wheel)
    analogWrite(5, speed);
}

void stopMotors() {
    //Wipe all direction bits to LOW (Electronic Brake)
    PORTB &= ~RIGHT_DIR_CLEAR;
    PORTD &= ~LEFT_DIR_CLEAR;

    //Set PWM to 0
    analogWrite(11, 0);
    analogWrite(5, 0);
}