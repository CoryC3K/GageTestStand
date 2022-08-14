//////
// 11/5/2021: Use analog pin input triggered off of IO board; works
// 8/10/2022: Use digital pin triggered from Opto-isolator board, pins 11/12, works great!
//            Also re-named a bunch of vars to make more sense, using correct types.
//
////

#include <stdio.h>
#include <AccelStepper.h>

// Serial slows steppers a LOT, especially on 5v non-driver boards.
#define USE_SERIAL

// Define steppers and the pins they will use
#define NANO_DRIVER

#ifdef CNC_DRIVER
// TMC2208 With NMEA steppers and CNC shield
#define ENABLE_ALL_PIN 8  // stepper motor enable, low level effective
#define STEP_1_DIR_PIN 2  // X axis, stepper motor direction control
#define STEP_2_DIR_PIN 3  // y axis, stepper motor direction control
#define STEP_1_STEP_PIN 5 // x axis, stepper motor control
#define STEP_2_STEP_PIN 6 // y axis, stepper motor control
AccelStepper stepper1(AccelStepper::DRIVER, STEP_1_STEP_PIN, STEP_1_DIR_PIN);
AccelStepper stepper2(AccelStepper::DRIVER, STEP_2_STEP_PIN, STEP_2_DIR_PIN);

long STEP_1_TARGET = -1000;    // 1/2 revolution
float STEP_1_MXSPEED = 2000;   // max speed, steps per second
float STEP_1_MXACCEL = 100000; // max acceleration
bool STEP_1_AT_MAX = false;

long STEP_2_TARGET = 200;      // End target posiiton from 0
float STEP_2_MXSPEED = 4000;   // max speed, steps per second
float STEP_2_MXACCEL = 100000; // max acceleration
bool STEP_2_AT_MAX = false;
#else
// As tested w/ 28BYJ steppers, at USB port power.
#define STEP_1_OUTPUT_A 2 // x axis, stepper motor control
#define STEP_1_OUTPUT_B 4 // x axis, stepper motor control
#define STEP_1_OUTPUT_C 3 // x axis, stepper motor control
#define STEP_1_OUTPUT_D 5 // x axis, stepper motor control

#define STEP_2_OUTPUT_A 6 // y axis, stepper motor control
#define STEP_2_OUTPUT_B 8 // y axis, stepper motor control
#define STEP_2_OUTPUT_C 7 // y axis, stepper motor control
#define STEP_2_OUTPUT_D 9 // y axis, stepper motor control

AccelStepper stepper1(AccelStepper::FULL4WIRE,
                      STEP_1_OUTPUT_A,
                      STEP_1_OUTPUT_B,
                      STEP_1_OUTPUT_C,
                      STEP_1_OUTPUT_D);

AccelStepper stepper2(AccelStepper::FULL4WIRE,
                      STEP_2_OUTPUT_A,
                      STEP_2_OUTPUT_B,
                      STEP_2_OUTPUT_C,
                      STEP_2_OUTPUT_D);

long STEP_1_TARGET = 1200;    // 1/2 revolution
float STEP_1_MXSPEED = 300;   // max speed, steps per second
float STEP_1_MXACCEL = 90000; // max acceleration
bool STEP_1_AT_MAX = false;

long STEP_2_TARGET = 100;     // End target posiiton from 0
float STEP_2_MXSPEED = 400;   // max speed, steps per second
float STEP_2_MXACCEL = 20000; // max acceleration
bool STEP_2_AT_MAX = false;
#endif

#define CYCLE_START_PIN 11 // D11 pin (Z- on Keyes shield)
#define CYCLE_STOP_PIN 12  // D12 pin

bool running = false;    // true if running
bool cycle_start = false;    // should we run a cycle
bool cycle_complete = false; // is the machine finished
bool await_reset = false;    // are we waiting to go back home
char status_str[128];        // buffer for output to serial monitor (warn: slows cycle speeds)

int last_step = 0; // last step we took

// Profile for the data stepper. Uses length of array and the
// 0-100% position of the other stepper to determine which target
// val to use, and then interpolates between the two.
int prof_arr[10] = {100, 60, 20, 10, 5, 5, 10, 20, 60, 100}; // profile array
int prof_steps = sizeof(prof_arr) / sizeof(prof_arr[0]); // number of steps in profile

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

#ifdef CNC_DRIVER
    pinMode(ENABLE_ALL_PIN, OUTPUT); // Turn on steppers
    digitalWrite(ENABLE_ALL_PIN, LOW);
#endif

#ifdef USE_SERIAL
    Serial.begin(115200);
#endif

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

    STEP_2_TARGET = prof_arr[0]; // set initial target
    stepper2.setMaxSpeed(STEP_2_MXSPEED);
    stepper2.setAcceleration(STEP_2_MXACCEL);
    stepper2.moveTo(STEP_2_TARGET);
}

void loop()
{
    cycle_start = digitalRead(CYCLE_START_PIN);
    cycle_complete = digitalRead(CYCLE_STOP_PIN);

    // enable stepping
    if ((false == running) && (true == cycle_start))
    {
        // set targets
        stepper1.moveTo(STEP_1_TARGET);
        stepper2.moveTo(prof_arr[0]);
#ifdef CNC_DRIVER
        digitalWrite(ENABLE_ALL_PIN, LOW); // turn on steppers
#endif
        digitalWrite(LED_BUILTIN, HIGH); // run light on
        running = true;
    }

    // Check for stepper 2 target change
    float cycle_percent = (float)stepper1.currentPosition()/
                (float)stepper1.targetPosition();
    int index = (int)(cycle_percent * prof_steps);
    if (stepper2.targetPosition() != prof_arr[index])
    {
#ifdef USE_SERIAL
        snprintf(status_str, sizeof(status_str),
                 "S2T: %d, S2TG: %d",
                 stepper2.targetPosition(),
                 stepper2.distanceToGo());
        Serial.println(status_str);
        if (stepper2.distanceToGo() != 0)
        {
            Serial.println("Warn: Stepper 2 change target before arriving");
        }
#endif
        stepper2.moveTo(prof_arr[index]);

        // TODO: set the speed so they both arrive at the same time
    }

    // disable stepping, wait for drop of complete
    if ((true == running) &&
        (stepper1.distanceToGo() == 0) &&
        (false == cycle_start) &&
        (true == cycle_complete))
    {
        await_reset = true;
        running = false;
        // might be a delay if EN held on, so turn 'em off to cut down on heat
#ifdef CNC_DRIVER
        digitalWrite(ENABLE_ALL_PIN, HIGH); // turn off steppers
#endif
    }

    // cycle complete has dropped, go home
    if ((true == await_reset) && (false == cycle_complete))
    {
        await_reset = false;
#ifdef CNC_DRIVER
        digitalWrite(ENABLE_ALL_PIN, LOW); // turn on steppers
#endif
        stepper2.stop();
        stepper1.runToNewPosition(0);
        stepper2.runToNewPosition(0);
#ifdef CNC_DRIVER
        digitalWrite(ENABLE_ALL_PIN, HIGH); // turn off steppers
#endif
        digitalWrite(LED_BUILTIN, LOW); // run light off
    }

    // do a step if we've got a movement to make
    if ((true == running) &&
        ((stepper1.distanceToGo() != 0) ||
         (stepper2.distanceToGo() != 0)))
    {
        stepper1.run();
        stepper2.run();
    };
}
