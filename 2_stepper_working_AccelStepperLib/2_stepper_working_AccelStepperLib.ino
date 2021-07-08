#include <AccelStepper.h>

// Define steppers and the pins they will use
AccelStepper stepper1(AccelStepper::FULL4WIRE, 8, 10, 9, 11);
AccelStepper stepper2(AccelStepper::FULL4WIRE, 2, 4, 3, 5);

// As tested w/ 28BYJ steppers, at 5v 
int maxSpeed = 450;

void setup()
{
  stepper1.setMaxSpeed(maxSpeed);
  stepper1.setAcceleration(1000);
  stepper1.moveTo(200);

  stepper2.setMaxSpeed(maxSpeed);
  stepper2.setAcceleration(1000);
  stepper2.moveTo(200);

}

void loop()
{
  // Change direction at the limits
  if (stepper1.distanceToGo() == 0){
    //stepper1.moveTo(random(-200, 200));
    stepper1.moveTo(-stepper1.currentPosition());
  }
  stepper1.run();
  /*
  if (stepper2.distanceToGo() == 0){
    stepper2.moveTo(-stepper2.currentPosition());
  }
  stepper2.run();
  */
  // Code to continuously spin
  stepper2.setSpeed(maxSpeed); // over-ride acceleration
  if (stepper2.distanceToGo() == 0){
    stepper2.setCurrentPosition(0);
    stepper2.moveTo(10000);
  }
  
  stepper2.runSpeed();
  
}
