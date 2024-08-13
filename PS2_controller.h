#include <PS2X_lib.h>

PS2X ps2x; // create PS2 Controller Class object

#define PS2_DAT 12 // MISO  19
#define PS2_CMD 13 // MOSI  23
#define PS2_SEL 15 // SS     5
#define PS2_CLK 14 // SLK   18

#define SERVO 3

#define TOP_SPEED 4090
#define NORM_SPEED 2048
#define TURNING_FACTOR 0.7

int speed = NORM_SPEED;

void setupPS2controller()
{
  int err = -1;
  while (err != 0)
  {
    err = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, true, true);
  }
}

bool PS2control()
{
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
  } else {
    if (ps2x.Button(PSB_L2)) {
      Serial.println("ban bong");
      pwm.setPWM(14, 0, TOP_SPEED);
      delay(500);
      while (ps2x.Button(PSB_L2)) {
        pwm.setPWM(SERVO, 0, 400);
        delay(500);
        pwm.setPWM(SERVO, 0, 210);
        ps2x.read_gamepad(0, 0);
        delay(250);
      }
      pwm.setPWM(14, 0, 0);
      pwm.setPWM(SERVO, 0, 400); 
    }
  }
  return 1;
}

