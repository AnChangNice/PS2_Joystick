#ifndef _PS2_H_
#define _PS2_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stddef.h>

#define PS2_BIT_SET(bit)                ((uint16_t)1U<<(bit))
#define IS_PS2_BIT_SET(value, bit)      (0U != ((value) & PS2_BIT_SET(bit)))
#define IS_PS2_BIT_RESET(value, bit)    (0U == ((value) & PS2_BIT_SET(bit)))

typedef enum _buttons_t
{
    kPS2_None      = 0U,
    kPS2_Select    = PS2_BIT_SET(0),
    kPS2_L3        = PS2_BIT_SET(1),
    kPS2_R3        = PS2_BIT_SET(2),
    kPS2_Start     = PS2_BIT_SET(3),
    kPS2_Up        = PS2_BIT_SET(4),
    kPS2_Right     = PS2_BIT_SET(5),
    kPS2_Down      = PS2_BIT_SET(6),
    kPS2_Left      = PS2_BIT_SET(7),
    kPS2_L2        = PS2_BIT_SET(8),
    kPS2_R2        = PS2_BIT_SET(9),
    kPS2_L1        = PS2_BIT_SET(10),
    kPS2_R1        = PS2_BIT_SET(11),
    kPS2_Triangle  = PS2_BIT_SET(12),
    kPS2_Circle    = PS2_BIT_SET(13),
    kPS2_Cross     = PS2_BIT_SET(14),
    kPS2_Square    = PS2_BIT_SET(15)
} kPS2_Buttons_t;

typedef enum _buttons_event_type
{
    kPS2_Event_None        = 0U,

    kPS2_Event_Push        = PS2_BIT_SET(0),
    kPS2_Event_Hold        = PS2_BIT_SET(1),
    kPS2_Event_Release     = PS2_BIT_SET(2),
#if 0 // Not useable now
    kPS2_ButtonEvent_Click       = PS2_BIT_SET(3),
    kPS2_ButtonEvent_DoubleClick = PS2_BIT_SET(4),

    kPS2_ButtonEvent_ShortPress  = PS2_BIT_SET(5),
    kPS2_ButtonEvent_LongPress   = PS2_BIT_SET(6)
#endif
} kPS2_Event_t;

typedef void (*PS2_ButtonCallback_t)(kPS2_Buttons_t button, kPS2_Event_t eventType, void *params);

typedef void (*PS2_TransferFunc_t)(uint8_t *out, uint8_t *in, uint8_t bytes);

int8_t PS2_Init(PS2_TransferFunc_t transfer);

void PS2_LoopCallback(uint32_t ms);

void PS2_MotorSmall_On(uint32_t ms);
void PS2_MotorSmall_Off(void);

// value should between 0-100
void PS2_MotorBig_On(uint8_t value, uint32_t ms);
void PS2_MotorBig_Off(void);

kPS2_Event_t PS2_ButtonGetState(kPS2_Buttons_t button);

const char * PS2_ButtonGetName(kPS2_Buttons_t button);
const char * PS2_ButtonGetEventName(kPS2_Event_t eventType);

int8_t PS2_LeftJoy_ReadLeftRight(void);
int8_t PS2_LeftJoy_ReadUpDown(void);
int8_t PS2_RightJoy_ReadLeftRight(void);
int8_t PS2_RightJoy_ReadUpDown(void);

void PS2_ButtonEventsSet(kPS2_Buttons_t buttons, kPS2_Event_t eventsType);
void PS2_ButtonInstallCallback(kPS2_Buttons_t buttons, PS2_ButtonCallback_t callback, void *param);

/* Weak functions alow user to redefine */
void PS2_PushCallback(kPS2_Buttons_t buttons);
void PS2_HoldCallback(kPS2_Buttons_t buttons);
void PS2_ReleaseCallback(kPS2_Buttons_t buttons);
#if 0 // Not useable now
void PS2_ClickCallback(kPS2_Buttons_t buttons);
void PS2_DoubleClickCallback(kPS2_Buttons_t buttons);
void PS2_ShortPressCallback(kPS2_Buttons_t buttons);
void PS2_LongPressCallback(kPS2_Buttons_t buttons);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PS2_H_ */
