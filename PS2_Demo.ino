#include "ps2.h"
#include <stdio.h>
#include <SPI.h>

unsigned long time;

static char print_buff[128];
#define PRINTF(...) sprintf(print_buff, __VA_ARGS__); Serial.write(print_buff, strlen(print_buff))

// Here I use Arduino_Mega it CS pin is 53
#define SPI_CS_PIN    (53) // -> CS
#define SPI_SCK_PIN   (52) // -> CLK
#define SPI_MOSI_PIN  (51) // -> DO
#define SPI_MISO_PIN  (50) // -> DI

// Here is SPI interface adapter used by PS2 lib
void PS2_TransferFunc(uint8_t *out, uint8_t *in, uint8_t bytes)
{
    digitalWrite(SPI_CS_PIN, LOW);
    SPI.beginTransaction(SPISettings(100000, LSBFIRST, SPI_MODE3));
    for (int i = 0;  i < bytes; i++)
    {
        in[i] = SPI.transfer(out[i]);
        delayMicroseconds(10);
    }
    SPI.endTransaction();
    digitalWrite(SPI_CS_PIN, HIGH);
}

// This is a user defined callback function, each button could have one callback and params.
// Or multiple buttons have same one callback, which is very usefull. Such as, all left side button use one callback.
static void ps2_left_buttons_callback(kPS2_Buttons_t button, kPS2_Event_t eventType, void *params)
{
    PRINTF("%s, %s\r\n", PS2_ButtonGetName(button), PS2_ButtonGetEventName(eventType));
    if (kPS2_Event_Hold != eventType)
    {
        PS2_MotorSmall_On(300); // Here is how to set big motor of left side rotate 300ms.
    }
}
// This is a user defined callback function, each button could have one callback and params.
// Or multiple buttons have same one callback, which is very usefull. Such as, all right side button use one callback.
static void ps2_right_buttons_callback(kPS2_Buttons_t button, kPS2_Event_t eventType, void *params)
{  
    PRINTF("%s, %s\r\n", PS2_ButtonGetName(button), PS2_ButtonGetEventName(eventType));
    if (kPS2_Event_Hold != eventType)
    {
        PS2_MotorBig_On(50, 300); // Here is how to set big motor of right side, rotate 300ms with power 50%.
    }
}

void PS2_PushCallback(kPS2_Buttons_t buttons)
{
    for (int i = 0; i < 16; i++)
    {
        if(IS_PS2_BIT_SET(buttons, i))
        {
            PRINTF("PS2_PushCallback: %s\r\n", PS2_ButtonGetName((kPS2_Buttons_t)PS2_BIT_SET(i)));
        }
    }
}
void PS2_HoldCallback(kPS2_Buttons_t buttons)
{
    for (int i = 0; i < 16; i++)
    {
        if(IS_PS2_BIT_SET(buttons, i))
        {
            PRINTF("PS2_HoldCallback: %s\r\n", PS2_ButtonGetName((kPS2_Buttons_t)PS2_BIT_SET(i)));
        }
    }
}
void PS2_ReleaseCallback(kPS2_Buttons_t buttons)
{
    for (int i = 0; i < 16; i++)
    {
        if(IS_PS2_BIT_SET(buttons, i))
        {
            PRINTF("PS2_ReleaseCallback: %s\r\n", PS2_ButtonGetName((kPS2_Buttons_t)PS2_BIT_SET(i)));
        }
    }
}

void setup() {
  Serial.begin(115200);

  pinMode(53, OUTPUT);

  SPI.begin();

  // Init PS2 with user defined SPI interface
  PS2_Init(PS2_TransferFunc);
  
  kPS2_Buttons_t buttons;
  // select buttons
  buttons = kPS2_Up|kPS2_Right|kPS2_Down|kPS2_Left|kPS2_L1|kPS2_L2|kPS2_L3;
  // set events type, the events is optional
  PS2_ButtonEventsSet(buttons, kPS2_Event_Push | kPS2_Event_Release | kPS2_Event_Hold);
  // install callback
  PS2_ButtonInstallCallback(buttons, ps2_left_buttons_callback, NULL);
  
  // select buttons
  buttons = kPS2_Triangle|kPS2_Circle|kPS2_Cross|kPS2_Square|kPS2_R1|kPS2_R2|kPS2_R3;
  // set events type, the events is optional
  PS2_ButtonEventsSet(buttons, kPS2_Event_Push | kPS2_Event_Release | kPS2_Event_Hold);
  // install callback
  PS2_ButtonInstallCallback(buttons, ps2_right_buttons_callback, NULL);
}

void loop() {
  time = micros();
  // Here is loop scan function, should be called periodically.
  PS2_LoopCallback(20);
  // Here is the two joystick useage.
  if (0 != PS2_LeftJoy_ReadLeftRight() | 0 != PS2_LeftJoy_ReadUpDown() | 0 != PS2_RightJoy_ReadLeftRight() | 0 != PS2_RightJoy_ReadUpDown())
  {
      PRINTF("%d, %d, %d, %d\r\n", PS2_LeftJoy_ReadLeftRight(), PS2_LeftJoy_ReadUpDown(), PS2_RightJoy_ReadLeftRight(), PS2_RightJoy_ReadUpDown());
  }

  // Here is how to read 'START' button's state
  kPS2_Event_t event = PS2_ButtonGetState(kPS2_Start);
  if(kPS2_Event_None != event)
  {
      PRINTF("START is %s now.\r\n", PS2_ButtonGetEventName(event));
  }
  
  while (micros() < time + 20000);
}
