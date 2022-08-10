//////
// 11/5/2021: Use analog pin input triggered off of IO board; works
// 8/10/2022: Use digital pin triggered from Opto-isolator board, pins 11/12, works great!
//            Also re-named a bunch of vars to make more sense, using correct types.
//
////

#include <AccelStepper.h>
#include <Arduino.h>

#define ENABLE_ALL_PIN     8       // stepper motor enable, low level effective
#define STEP_1_DIR_PIN     2       //X axis, stepper motor direction control 
#define STEP_2_DIR_PIN     3       //y axis, stepper motor direction control

#define STEP_1_STEP_PIN    5       //x axis, stepper motor control
#define STEP_2_STEP_PIN    6       //y axis, stepper motor control

#define CYCLE_START_PIN    11      // D11 pin (Z- on Keyes shield)
#define CYCLE_STOP_PIN     12      // D12 pin
#define LED                13      // on-board LED

bool cycle_start = false;       // should we run a cycle
bool cycle_complete = false;    // is the machine finished
bool enabled = false;           // are we currently running
bool await_reset = false;       // are we waiting to go back home
//char status_str[128];         // buffer for output to serial monitor (warn: slows cycle speeds)

// Stepper motion settings
long  STEP_1_TARGET  =  -1000;   // 1/2 revolution
float STEP_1_MXSPEED =   2000;   // max speed, steps per second
float STEP_1_MXACCEL = 100000;   // max acceleration
bool  STEP_1_AT_MAX  =  false;

long  STEP_2_TARGET  =     200; // End target posiiton from 0
float STEP_2_MXSPEED =    4000; // max speed, steps per second
float STEP_2_MXACCEL =  100000; // max acceleration
bool  STEP_2_AT_MAX  =   false;

AccelStepper stepper1(AccelStepper::DRIVER, STEP_1_STEP_PIN, STEP_1_DIR_PIN);
AccelStepper stepper2(AccelStepper::DRIVER, STEP_2_STEP_PIN, STEP_2_DIR_PIN);

void setup()
{  
    pinMode(ENABLE_ALL_PIN, OUTPUT);    // Turn on steppers
    digitalWrite(ENABLE_ALL_PIN, LOW);
    //Serial.begin(115200);

    // Enable as input pin
    pinMode(CYCLE_START_PIN, INPUT);
    pinMode(CYCLE_STOP_PIN, INPUT);
    // set pullup resistors
    digitalWrite(CYCLE_START_PIN, LOW);
    digitalWrite(CYCLE_STOP_PIN, LOW);

    // setup steppers and move to home
    stepper1.setMaxSpeed(STEP_1_MXSPEED);
    stepper1.setAcceleration(STEP_1_MXACCEL);
    stepper1.moveTo(STEP_1_TARGET);

    stepper2.setMaxSpeed(STEP_2_MXSPEED);
    stepper2.setAcceleration(STEP_2_MXACCEL);
    stepper2.moveTo(STEP_2_TARGET);
}

void loop()
{
    cycle_start = digitalRead(CYCLE_START_PIN);
    cycle_complete = digitalRead(CYCLE_STOP_PIN);

    // HUGELY slows the motor speeds to write to console.
    // But at MaxSpeed < ~10,000 it's fine.
    //snprintf(status_str, sizeof(status_str), "START:%d STOP:%d", cycle_start, cycle_complete);
    //Serial.println(status_str);  
    
    
    // enable stepping
    if ((false == enabled) && (true == cycle_start)) {
      // set targets
      stepper1.moveTo(STEP_1_TARGET);
      stepper2.moveTo(STEP_2_TARGET);
      
      digitalWrite(ENABLE_ALL_PIN, LOW);    // turn on steppers
      digitalWrite(LED, HIGH);  // run light on
      enabled = true;
    }

    // stepper 2 invert and keep going
    if ((true == enabled) && (stepper1.targetPosition() == STEP_1_TARGET) && (stepper2.distanceToGo() == 0)){
      if (stepper1.distanceToGo() == 0){
        // re-home
        STEP_2_AT_MAX = false;
        stepper2.moveTo(0);
      } else if (STEP_2_AT_MAX){
        STEP_2_AT_MAX = false;
        stepper2.moveTo(0);
      } else {
        STEP_2_AT_MAX = true;
        stepper2.moveTo(STEP_2_TARGET);
      }      
    }

    // disable stepping, wait for drop of complete
    if ((true == enabled) && 
        (stepper1.distanceToGo() == 0) && 
        (false == cycle_start) && 
        (true == cycle_complete)){
          await_reset = true;
          enabled = false;
          // might be a delay if EN held on, so turn 'em off to cut down on heat
          digitalWrite(ENABLE_ALL_PIN, HIGH); // turn off steppers
    }

    // cycle complete has dropped, go home
    if ((true == await_reset) && (false == cycle_complete)){
          await_reset = false;
          digitalWrite(ENABLE_ALL_PIN, LOW); // turn on steppers
          stepper2.stop();
          stepper1.runToNewPosition(0);
          stepper2.runToNewPosition(0);
          digitalWrite(ENABLE_ALL_PIN, HIGH); // turn off steppers
          digitalWrite(LED, LOW); // run light off
    }

    // do a step if we've got a movement to make
    if ((true == enabled) &&
        ((stepper1.distanceToGo() != 0) || 
         (stepper2.distanceToGo() != 0))) {
      stepper1.run();
      stepper2.run();
    };
}
