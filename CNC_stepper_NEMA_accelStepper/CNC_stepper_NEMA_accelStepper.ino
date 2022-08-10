//////
//Last update: 11/5/2021: Use analog pin input triggered off of IO board; works
//
////

#include <AccelStepper.h>
#include <Arduino.h>

#define EN        8       // stepper motor enable, low level effective
#define X_DIR     2       //X axis, stepper motor direction control 
#define Y_DIR     3       //y axis, stepper motor direction control

#define X_STP     5       //x axis, stepper motor control
#define Y_STP     6       //y axis, stepper motor control

#define INPUT_1   A1      // Analog pin A1
#define LED       13      // on-board LED
#define DIS       12     // D12 pin

int in_1_rdg = 0;
int in_1_val = 0;
bool enabled = false;
int X_STEPS =  -1000; // 1/2 revolution

int Y_STEPS =   200;
bool Y_AT_MAX = false;

int in_1_active = HIGH; // invert switching
int in_1_limit = 19;   // Voltage sensed to trigger rdg

AccelStepper stepper1(AccelStepper::DRIVER, X_STP, X_DIR);
AccelStepper stepper2(AccelStepper::DRIVER, Y_STP, Y_DIR);

void setup()
{  
    //pinMode(INPUT_1, INPUT);
    //digitalWrite(INPUT_1, HIGH); // pullup resistor
    pinMode(EN, OUTPUT);    // Turn on steppers
    digitalWrite(EN, LOW);
    Serial.begin(115200);

    pinMode(DIS, INPUT);  // Enable as input pin
    
    stepper1.setMaxSpeed(4000);
    stepper1.setAcceleration(100000);
    stepper1.moveTo(X_STEPS);

    stepper2.setMaxSpeed(10000);
    stepper2.setAcceleration(999999999);
    stepper2.moveTo(Y_STEPS);
}

void loop()
{
    // Read input analog pin
    in_1_rdg = analogRead(INPUT_1);
    //Serial.println(in_1_rdg); // HUGELY slows the motor speeds

    if (in_1_rdg > in_1_limit){
      in_1_val = HIGH;
    } else {
      in_1_val = LOW;
    }
  
    // enable stepping
    if ((!enabled) && (in_1_val == in_1_active)) {
      // set targets
      stepper1.moveTo(X_STEPS);
      stepper2.moveTo(Y_STEPS);
      
      digitalWrite(EN, LOW);    // turn on steppers
      digitalWrite(LED, HIGH);  // run light on
      enabled = true;
    }

    // stepper 2 invert and keep going
    if ((enabled) && (stepper1.targetPosition() == X_STEPS) && (stepper2.distanceToGo() == 0)){
      if (stepper1.distanceToGo() == 0){
        // re-home
        Y_AT_MAX = false;
        stepper2.moveTo(0);
      } else if (Y_AT_MAX){
        Y_AT_MAX = false;
        stepper2.moveTo(0);
      } else {
        Y_AT_MAX = true;
        stepper2.moveTo(Y_STEPS);
      }      
    }

    // disable stepping, re-set home, 
    if ((enabled) && 
        (in_1_val != in_1_active) && 
        (stepper1.distanceToGo() == 0) && 
        (digitalRead(DIS) == HIGH)){
          delay(500);
          stepper2.stop();
          stepper1.runToNewPosition(0);
          stepper2.runToNewPosition(0);
          digitalWrite(EN, HIGH); // turn off steppers
          digitalWrite(LED, LOW); // run light off
          enabled = false;
    }

    // do a step if we've got a movement to make
    if ((enabled) && ((stepper1.distanceToGo() != 0) || (stepper2.distanceToGo() != 0))) {
      stepper1.run();
      stepper2.run();
    };
}
