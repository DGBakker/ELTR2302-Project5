#pragma region ConfigurationAndMasks
// --- 1. Configuration & Masks ---
#include <Arduino.h>

// --- Function Prototypes ---
void moveForward(int speed);
void moveBackward(int speed);
void turnLeft(int leftSpeed, int rightSpeed);
void turnRight(int leftSpeed, int rightSpeed);
void turnLeftReverse(int leftSpeed, int rightSpeed);
void turnRightReverse(int leftSpeed, int rightSpeed);
void pivotLeft(int speed);
void pivotRight(int speed);
void stopMotors();
void mathandoutput();

// Master Masks (For setup only)
// PB3, PB4, PB5
#define RIGHT_MASK 0x38
// PD5, PD6, PD7
#define LEFT_MASK 0xE0

// HC-SR04 Pins
#define TRIG_PIN 9 // PB1 (OC1A)
#define ECHO_PIN 2 // PD2 (INT0)

// Movement Masks
// Right: (Port B): IN1(0x20) is FWD, IN2(0x10) is REV
#define RIGHT_FWD 0x20
#define RIGHT_REV 0x10
// 0x30
#define RIGHT_DIR_CLEAR (RIGHT_FWD | RIGHT_REV)

// Left: (Port D): IN4(0x40) is FWD, IN3(0x80) is REV
#define LEFT_FWD 0x40
#define LEFT_REV 0x80
// 0xC0
#define LEFT_DIR_CLEAR (LEFT_FWD | LEFT_REV)
#pragma endregion

#pragma region GlobalVariables
// --- 2. Global Variables ---
// Pattern: 1 = Boomerang, 2 = Square, 3 = Ultrasonic Obstacle Avoidance
int patternID = 3;
// Scale: 1.0 = Small (Base times), 2.0 = Medium (Double times), etc.
float spaceScale = 1.0;
// Tracks how many sides of the square have been completed (For pattern 2)
int sideCount = 0;

// HC-SR04 Variables (from instructor worksheet)
volatile float distance = 0;
volatile unsigned long int riseTime = 0;
volatile unsigned long int fallTime = 0;
volatile boolean calculation = false;

const int SLOW_SPEED = 120;
const int MED_SPEED = 180;
const int FAST_SPEED = 255;

unsigned long previousT = 0;
unsigned long lastRedraw = 0;
int moveStep = 0;
String currentStatus = "Initializing...";
#pragma endregion

#pragma region Initialization
// --- 3. Initialization (setup) ---
void setup() {
    // Set motor bits to 1 (Output) on DDRB and DDRD
    DDRB |= RIGHT_MASK;
    DDRD |= LEFT_MASK;

    // Set TRIG (D9/PB1) as Output
    DDRB |= (1 << PB1);
    // Set ECHO (D2/PD2) as Input
    DDRD &= ~(1 << PD2);

    // Set default state to LOW (Stopped)
    PORTB &= ~RIGHT_MASK;
    PORTD &= ~LEFT_MASK;

    // Timer 1 Setup for HC-SR04 Trigger (Fast PWM Mode 14)
    // Period = 100ms (10Hz), Pulse Width = 10us
    TCCR1A = 0x82; // Clear OC1A on Compare Match, set OC1A at BOTTOM
    TCCR1B = 0x1B; // Fast PWM mode 14, Prescaler = 64
    ICR1 = 24999;  // Period = (16MHz / (64 * 10Hz)) - 1 = 24999
    OCR1A = 3;     // Pulse Width = ~12us (Close enough to the 10us min)

    // External Interrupt Setup for Echo (INT0 on D2)
    cli();           // Disable interrupts
    EICRA = 0x01;    // Trigger on ANY logical change (Rising or Falling)
    EIMSK |= (1 << INT0); // Enable INT0
    sei();           // Enable interrupts

    // Record the starting time
    previousT = millis();

    Serial.begin(9600);
    // Clear the screen once on startup and HIDE the cursor
    Serial.print("\033[2J\033[?25l"); 
    currentStatus = "UAV Project 5 - Part A Initialized.";
}
#pragma endregion

// --- 4. Main Program Loop ---
void loop() {
    unsigned long currentT = millis();

    // If new echo data is ready, process it
    if (calculation) {
        mathandoutput();
    }

    switch (patternID) {

        #pragma region DoubleBoomerang (Pattern 1)
        case 1: // --- DOUBLE BOOMERANG (Zero Displacement) ---
            switch (moveStep) {
                case 0: //Forward straight (800ms)
                    moveForward(MED_SPEED);
                    if (currentT - previousT >= (unsigned long)(800 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        currentStatus = "State 1: Arc Left Forward";
                    }
                    break;

                case 1: //Arc left forward (1000ms)
                    turnLeft(SLOW_SPEED, FAST_SPEED);
                    if (currentT - previousT >= (unsigned long)(1000 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        currentStatus = "State 2: Arc Left Reverse";
                    }
                    break;

                case 2: //Arc left reverse (1000ms)
                    turnLeftReverse(SLOW_SPEED, FAST_SPEED);
                    if (currentT - previousT >= (unsigned long)(1000 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        currentStatus = "State 3: Arc Right Forward";
                    }
                    break;

                case 3: //Arc right forward (1000ms)
                    turnRight(SLOW_SPEED, FAST_SPEED);
                    if (currentT - previousT >= (unsigned long)(1000 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        currentStatus = "State 4: Arc Right Reverse";
                    }
                    break;

                case 4: //Arc right reverse (1000ms)
                    turnRightReverse(SLOW_SPEED, FAST_SPEED);
                    if (currentT - previousT >= (unsigned long)(1000 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        currentStatus = "State 5: Backward Straight";
                    }
                    break;

                case 5: //Backward straight (800ms)
                    moveBackward(MED_SPEED);
                    if (currentT - previousT >= (unsigned long)(800 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        currentStatus = "State 6: Stop";
                    }
                    break;

                case 6: //Stop, restart after 3 seconds
                    stopMotors();
                    if (currentT - previousT >= 3000) {
                        previousT = currentT;
                        moveStep = 0; //Restart sequence
                        currentStatus = "State 0: Move forward";
                    }
                    break;
            }
            break; //End Pattern 1
            #pragma endregion

        #pragma region SquarePattern (Pattern 2)
        case 2: // --- SQUARE PATTERN ---
            switch (moveStep) {
                case 0: //Forward leg
                    moveForward(MED_SPEED);
                    if (currentT - previousT >= (unsigned long)(1000 * spaceScale)) {
                        previousT = currentT;
                        moveStep++;
                        currentStatus = "Square Step 1: Pivot Left 90";
                    }
                    break;
                
                case 1: //Pivot left 90 degrees
                    pivotLeft(MED_SPEED);
                    if (currentT - previousT >= 500) {
                        previousT = currentT;
                        sideCount++;
                        moveStep++;
                        currentStatus = "Square Step 2: Forward Leg";
                    }
                    break;
                
                case 2: //Check if square is complete, if not, repeat.
                    if (sideCount < 4) {
                        moveStep = 0; //Back to case 0
                        currentStatus = "Starting side: " + String(sideCount + 1); 
                    }
                    else {
                        sideCount = 0; //Reset
                        moveStep++; //Go to case 3 (Stop)
                        currentStatus = "Square Complete.";
                    }
                    break;
                
                case 3: //Stop, restart after 3 seconds
                    stopMotors();
                    if (currentT - previousT >= 3000) {
                        previousT = currentT;
                        moveStep = 0; //Restart sequence
                        currentStatus = "Starting Square Pattern.";
                    }
                    break;
            }
            break; //End Pattern 2
        #pragma endregion

        #pragma region UltrasonicTest (Pattern 3)
        case 3: // --- ULTRASONIC OBSTACLE AVOIDANCE ---
            switch(moveStep) {
                case 0: // Normal Driving & Slow Down (Requirements A & B)
                    // Must be > 0 to prevent triggering immediately on boot before first ping
                    if (distance > 0 && distance <= 25) {
                        // Requirement C: Stop triggered!
                        stopMotors();
                        previousT = currentT;
                        moveStep = 1;
                        currentStatus = "Object < 25cm! Stopping.";
                    } 
                    else if (distance > 0 && distance <= 100) {
                        // Requirement B: Object < 100cm, slow to 70% (~178 PWM)
                        moveForward(178);
                        currentStatus = "Object < 100cm. Slowing down.";
                    } 
                    else {
                        // Requirement A: Path clear, min 85% full duty cycle (~217 PWM)
                        moveForward(217);
                        currentStatus = "Path clear. Forward 85%.";
                    }
                    break;

                case 1: // Requirement C: Brief stop before reversing
                    stopMotors();
                    if (currentT - previousT >= 250) { // Brief 250ms pause to let momentum settle
                        previousT = currentT;
                        moveStep = 2;
                        currentStatus = "Reversing...";
                    }
                    break;

                case 2: // Requirement C: Reverse at moderate speed for 1 second
                    moveBackward(150); // Moderate speed
                    if (currentT - previousT >= 1000) { // 1 second
                        previousT = currentT;
                        moveStep = 3;
                        currentStatus = "Stopping before rotate.";
                    }
                    break;

                case 3: // Requirement C: Stop the vehicle again
                    stopMotors();
                    if (currentT - previousT >= 250) { // Brief 250ms pause
                        previousT = currentT;
                        moveStep = 4;
                        currentStatus = "Rotating Left.";
                    }
                    break;

                case 4: // Requirement D: Rotate left 30 to 40 degrees
                    pivotLeft(150); // Use pivotLeft so wheels spin opposite directions
                    // Time needed to rotate 30-40 degrees (needs physical tuning)
                    if (currentT - previousT >= 400) { 
                        stopMotors();
                        previousT = currentT;
                        moveStep = 5;
                        currentStatus = "Checking clearance...";
                    }
                    break;

                case 5: // Requirement E: Check clearance
                    // Give the sensor a moment to grab a clean reading after stopping
                    if (currentT - previousT >= 200) {
                        // Rubric: repeat step D until no object is identified within 250cm.
                        if (distance < 250) {
                            previousT = currentT;
                            moveStep = 4; // Go back to rotate left
                            currentStatus = "Object < 250cm. Rotating again.";
                        } 
                        else {
                            // Path clear for 250cm, resume forward
                            moveStep = 0; // Go back to state A
                            currentStatus = "Clear path found. Resuming.";
                        }
                    }
                    break;
            }
            break;
        #pragma endregion
    }
}

#pragma region MovementFunctions
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
#pragma endregion

#pragma region UltrasonicFunctions
// --- 6. Ultrasonic Sensor Functions ---

void mathandoutput(void) {
    // Distance = (Time * Speed of Sound) / 2
    distance = (float)(fallTime - riseTime) / 58.0;

    // Filter out bad data (HC-SR04 max range is ~400cm)
    // When objects are too close (<2cm), it often reads as a massive timeout value
    if (distance > 400.0) {
        distance = 400.0; // Clamp it to max range instead of displaying 800+
    }

    // Only update the screen at 5Hz to keep it smooth
    if (millis() - lastRedraw >= 200) {
        lastRedraw = millis();
        
        // ANSI escape codes:
        // \033[H  -> Move cursor to home (top left)
        // \033[K  -> Clear line from cursor to end
        
        Serial.print("\033[H"); 
        Serial.println("--- UAV SENSOR DASHBOARD ---");
        
        Serial.print("\033[KSTATUS: ");
        Serial.println(currentStatus);
        
        Serial.println("\033[K----------------------------");
        
        Serial.print("\033[KTIME: ");
        Serial.print(fallTime - riseTime);
        Serial.print(" us | DISTANCE: ");
        Serial.print(distance);
        Serial.println(" cm");
    }

    calculation = false; // Reset the flag
}

// ========== Interrupt Service Routine ==========
ISR(INT0_vect) {
    
    
    // Check if the pin just went HIGH (Rising Edge)
    if (PIND & (1 << PD2)) {
        riseTime = micros(); // After triggered, get the riseTime
    } 
    // Otherwise, the pin just went LOW (Falling Edge)
    else {
        fallTime = micros(); // Get the fallTime on the echo of signal
        calculation = true;  // Set calculation flag to TRUE
    }
    
    
}
#pragma endregion