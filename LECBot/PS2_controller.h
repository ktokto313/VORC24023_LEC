#include <PS2X_lib.h>

PS2X ps2x; // create PS2 Controller Class object

#define PS2_DAT 12 // MISO  19
#define PS2_CMD 13 // MOSI  23
#define PS2_SEL 15 // SS     5
#define PS2_CLK 14 // SLK   18

#define SENSOR 3
#define SERVO_GOC_BAN 4
#define SERVO_BAN_BONG 5
#define SERVO_LOC_BONG 6
#define SERVO_DO_BONG 7

#define TOP_SPEED 4090
#define NORM_SPEED 2048
#define TURNING_FACTOR 0.7

int speed;
boolean banBongMode = false;
boolean thuBongMode = true;

// Task handle
TaskHandle_t banBongTask;

// Shared variable between threads with mutex lock to avoid racing condition
bool hasSpooled = false;
bool isFiring = false;
SemaphoreHandle_t mutex;

void waitBanBong(void * parameter);

void setupPS2controller()
{
  int err = -1;
  while (err != 0)
  {
    err = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, true, true);
  }

  mutex = xSemaphoreCreateMutex();
  assert(mutex);
}

bool PS2control()
{
  if (ps2x.NewButtonState()) {
    // Thu bong mode
    if (PSB_PAD_DOWN) {
      banBongMode = false;
      thuBongMode = true;
    } else if (PSB_PINK) { // Ban bong mode
      banBongMode = true;
      thuBongMode = false;
    } else if (PSB_R1) { // Do bong mode
      banBongMode = false;
      thuBongMode = false;
    }
  }
  speed = TOP_SPEED;
  if (banBongMode) {
    speed = NORM_SPEED;
    //TODO: test neu bam l2 2 lan thi co crash ko
    // Running the shooter's wheels
    if (ps2x.Button(PSB_L2)) {
      // Create dedicated thread for firing
      xTaskCreatePinnedToCore(
                      waitBanBong,   /* Task function. */
                      "waitBanBong",     /* name of task. */
                      8192,       /* Stack size of task */
                      NULL,        /* parameter of the task */
                      1,           /* priority of the task */
                      &banBongTask,      /* Task handle to keep track of created task */
                      1);          /* pin task to core 1 */
      xSemaphoreTake(mutex, 1);
      if (ps2x.Button(PSB_L1)) {
        bool isFiring = true;
      } else {
        bool isFiring = false;
      }
      xSemaphoreGive(mutex);
    } else {vTaskDelete(banBongTask);}

    if(ps2x.ButtonPressed(PSB_GREEN)){
      turnServo90(SERVO_GOC_BAN);
    } else if (ps2x.ButtonPressed(PSB_RED)) {
      turnServoM90(SERVO_GOC_BAN);
    }
  } else if (thuBongMode) {
    // The first two if is for manual mode
    if (ps2x.ButtonPressed(PSB_PAD_RIGHT)) {
      // Black ball
      // Position servo1 at -90 degrees
      turnServoM90(SERVO_LOC_BONG);
    } else if (ps2x.ButtonPressed(PSB_PAD_LEFT)) {
      // White ball
      // Position servo1 at 90 degrees
      turnServo90(SERVO_LOC_BONG);
    } else if (ps2x.ButtonPressed(PSB_PAD_UP)){ // Auto mode
      int irvalue = digitalRead(SENSOR);
      if (irvalue == 0) {
        // Black ball
        // Position servo1 at -90 degrees
        turnServoM90(SERVO_LOC_BONG);
      } else if (irvalue== 1){ 
        // White ball
        // Position servo1 at 90 degrees
        turnServo90(SERVO_LOC_BONG);
      }
    }
  } else {
    if (ps2x.Button(PSB_R2)) {
      turnServo90(SERVO_DO_BONG);
      delay(500);
      turnServo0(SERVO_DO_BONG);
    }
  }

  int nJoyX = 128 - ps2x.Analog(PSS_RY); // read x-joystick
  int nJoyY = 128 - ps2x.Analog(PSS_LX); // read y-joystick
  int nMotMixL;                          // Motor (left) mixed output
  int nMotMixR;                          // Motor (right) mixed output


  bool temp = (nJoyY * nJoyX > 0);
  if (nJoyY > 1 or nJoyY < -1) // Turning
  {
    nMotMixL = -nJoyY * !temp;
    nMotMixR = -nJoyY * !temp;
    nMotMixL *= TURNING_FACTOR;
    nMotMixR *= TURNING_FACTOR;
  }
  else // Forward or Reverse
  {
    nMotMixL = -nJoyX;
    nMotMixR = nJoyX;
  }

  int c1 = 0, c2 = 0, c3 = 0, c4 = 0;

  if (nMotMixR > 0)
  {
    c3 = nMotMixR;
    c3 = map(c3, 0, 128, 0, speed);
  }

  else if (nMotMixR < 0)
  {
    c4 = abs(nMotMixR);
    c4 = map(c4, 0, 128, 0, speed);
  }

  if (nMotMixL > 0)
  {
    c1 = nMotMixL;
    c1 = map(c1, 0, 128, 0, speed);
  }
  else if (nMotMixL < 0)
  {
    c2 = abs(nMotMixL);
    c2 = map(c2, 0, 128, 0, speed);
  }
  setPWMMotors(c1, c2, c3, c4);
  return 1;
}

// The function to run on the shooting thread
void waitBanBong(void * parameter) {
  xSemaphoreTake(mutex, 1);
  bool prv_isFiring = isFiring;
  xSemaphoreGive(mutex);
  if (!hasSpooled) {
    // bat motor chay da
    delay(500);
    xSemaphoreTake(mutex, 1);
    hasSpooled = true;
    xSemaphoreGive(mutex);
  } else {
    if (prv_isFiring) {
      pwm.setPWM(SERVO_BAN_BONG, 0, 284);
      delay(500);
      pwm.setPWM(SERVO_BAN_BONG, 0, 205);
      delay(500);
    }
  }
}