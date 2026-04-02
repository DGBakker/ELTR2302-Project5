//Tell the compiler we are using the Arduino framework.
#include <Arduino.h>

//Define the right motor enable pin (ENA) as digital pin 11.
const int ENA = 11;

//Define the right motor direction pin 1 (IN1) as digital pin 12.
const int IN1 = 12;

//Define the right motor direction pin 2 (IN2) as digital pin 13.
const int IN2 = 13;

//Define the left motor enable pin (ENB) as digital pin 5.
const int ENB = 5;

//Define the left motor direction pin 3 (IN3) as digital pin 7.
const int IN3 = 7;

//Define the left motor direction pin 4 (IN4) as digital pin 6.
const int IN4 = 6;

//Timing variable
unsigned long previousT = 0;

//Create the setup() function, set motor pins as outputs.
void setup() {

//Right motor outputs.
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

//Left motor outputs.
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

//Set the initial state (stop).
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);

//Get the starting time.
    previousT = millis();
}

//Create the loop() functions and define the movement pattern.
void loop() {

//Calculate how much time has passed since previousT.
    unsigned long currentT = millis();

//Step 1: Forward (2 seconds).
    if (currentT - previousT <= 2000) {

    //Set speed to 150.
        moveForward(150);
    }