#include <AccelStepper.h>

// Define steppers and the pins they will use
AccelStepper stepper1(AccelStepper::FULL4WIRE, 8, 10, 9, 11);
AccelStepper stepper2(AccelStepper::FULL4WIRE, 2, 4, 3, 5);

// As tested w/ 28BYJ steppers, at 5v 
int maxSpeed = 450;

int IN_A = A0;        // Input pin
int IN_A_VAL = 0;     
int threshold = 520;  // input threshold for triggering
bool running = false;
int s1_target = 50;
int s2_target = 1200;

void setup()
{
  Serial.begin(9600);
  // S1 is diameter
  stepper1.setMaxSpeed(maxSpeed);
  stepper1.setAcceleration(10000);
  stepper1.moveTo(s1_target);
  
  // S2 is location
  stepper2.setMaxSpeed(maxSpeed);
  stepper2.setAcceleration(10000);
  stepper2.moveTo(s2_target);

  pinMode(LED_BUILTIN, OUTPUT);

}

void loop()
{
  IN_A_VAL = analogRead(IN_A);
  Serial.print("val:");
  Serial.println(IN_A_VAL);

  
  
  if ((!running) && (IN_A_VAL < threshold)){
    return;
  } else {
    running = true;
    digitalWrite(LED_BUILTIN, running);
  }
  
  // Change direction at the limits
  if (stepper1.distanceToGo() == 0){
    //stepper1.moveTo(random(-s1_target, s1_target));
    stepper1.moveTo(-stepper1.currentPosition());
  }
  stepper1.run();
  
  if (stepper2.distanceToGo() == 0){
    // re-home
    stepper1.moveTo(0);
    stepper1.runToPosition();
    stepper2.moveTo(0);
    stepper2.runToPosition();

    // re-set targets
    stepper1.moveTo(s1_target);
    stepper2.moveTo(s2_target);

    running = false;
    digitalWrite(LED_BUILTIN, running);
  }
  stepper2.run();
  /*
  // Code to continuously spin
  stepper2.setSpeed(maxSpeed); // over-ride acceleration
  if (stepper2.distanceToGo() == 0){
    stepper2.setCurrentPosition(0);
    stepper2.moveTo(10000);
    
  }
  stepper2.runSpeed();
  */
}
