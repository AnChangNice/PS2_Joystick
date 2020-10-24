#include "ps2.h"

#include <string.h> // for memset()

/*******************************************************************/
static uint8_t di_data[9];
static uint8_t do_data[9];

static PS2_TransferFunc_t ps2transfer;

static uint16_t ps2_button_value_current;
static kPS2_Event_t ps2buttons_state_current[16] = { kPS2_Event_None };
static kPS2_Event_t ps2buttons_event_set[16] = { kPS2_Event_None };
static PS2_ButtonCallback_t ps2buttons_event_callback[16] = { NULL };
static void * ps2buttons_event_param[16] = { NULL };
static uint8_t ps2_small_motor_value    = 0;
static uint32_t ps2_small_motor_on_ms   = 0;
static uint8_t ps2_big_motor_value      = 0;
static uint32_t ps2_big_motor_on_ms     = 0;

static int8_t ps2_left_joy_leftright_value  = 0;
static int8_t ps2_left_joy_updown_value     = 0;
static int8_t ps2_right_joy_leftright_value = 0;
static int8_t ps2_right_joy_updown_value    = 0;

static const char *ps2buttonsName[] = {
    "None",
    "Select",
    "L3",
    "R3",
    "Start",
    "Up",
    "Right",
    "Down",
    "Left",
    "L2",
    "R2",
    "L1",
    "R1",
    "Triangle",
    "Circle",
    "Cross",
    "Square"
};

static const char *ps2buttonsEventName[] = {
    "None",
    "Push",
    "Hold",
    "Release"
};

/************************* 私有函数 ************************************/

static void ps2_deinit(void)
{
    memset(do_data, 0, 5);

    do_data[0] = 0x01;
    do_data[1] = 0x42;
    do_data[2] = 0x00;
    do_data[3] = 0x00;
    do_data[4] = 0x00;

    ps2transfer(do_data, di_data, 5);
}

static void ps2_EnterConfing()
{
    memset(do_data, 0, 9);

    do_data[0] = 0x01;
    do_data[1] = 0x43;
    do_data[2] = 0X00;
    do_data[3] = 0x01;
    do_data[4] = 0x00;
    do_data[5] = 0X00;
    do_data[6] = 0X00;
    do_data[7] = 0X00;
    do_data[8] = 0X00;

    ps2transfer(do_data, di_data, 9);
}

static void ps2_TurnOnAnalogMode()
{
    memset(do_data, 0, 9);

    do_data[0] = 0x01;
    do_data[1] = 0x44;
    do_data[2] = 0X00;
    do_data[3] = 0x01;
    do_data[4] = 0xEE;
    do_data[5] = 0X00;
    do_data[6] = 0X00;
    do_data[7] = 0X00;
    do_data[8] = 0X00;

    ps2transfer(do_data, di_data, 9);
}

static void ps2_VibrationMode()
{
    memset(do_data, 0, 5);

    do_data[0] = 0x01;
    do_data[1] = 0x4D;
    do_data[2] = 0X00;
    do_data[3] = 0x00;
    do_data[4] = 0X01;

    ps2transfer(do_data, di_data, 5);
}

static void ps2_ExitConfing()
{
    memset(do_data, 0, 9);

    do_data[0] = 0x01;
    do_data[1] = 0x43;
    do_data[2] = 0X00;
    do_data[3] = 0x00;
    do_data[4] = 0x5A;
    do_data[5] = 0x5A;
    do_data[6] = 0x5A;
    do_data[7] = 0x5A;
    do_data[8] = 0x5A;
 
    ps2transfer(do_data, di_data, 9);
}

static void ps2_UpdateDataRaw()
{
    memset(do_data, 0, 9);

    do_data[0] = 0x01;
    do_data[1] = 0x42;
    do_data[2] = 0X00;
    do_data[3] = ps2_big_motor_value;
    do_data[4] = ps2_small_motor_value;
    do_data[5] = 0X00;
    do_data[6] = 0X00;
    do_data[7] = 0X00;
    do_data[8] = 0X00;
 
    ps2transfer(do_data, di_data, 9);
}

static int8_t ps2_joy_raw_conv(uint8_t raw)
{
    if ((126 <= raw) && (raw <= 128))
    {
        raw = 127;
    }

    return (int)(100 - (int)raw * 100 / 127);
}

static void ps2_event_update(uint16_t newv)
{
    static uint16_t old = 0;

    // revert the level
    newv = ~newv;

    for (int i = 0; i < 16; i++)
    {
        // None, old_reset, newv_reset
        ps2buttons_state_current[i] = kPS2_Event_None;

        // push, old_reset, newv_set
        if (IS_PS2_BIT_RESET(old, i) && IS_PS2_BIT_SET(newv, i))
        {
            ps2buttons_state_current[i] = kPS2_Event_Push;
        }
        // hold, old_set, newv_set
        if (IS_PS2_BIT_SET(old, i) && IS_PS2_BIT_SET(newv, i))
        {
            ps2buttons_state_current[i] = kPS2_Event_Hold;
        }
        // release, old_set, newv_reset
        if (IS_PS2_BIT_SET(old, i) && IS_PS2_BIT_RESET(newv, i))
        {
            ps2buttons_state_current[i] = kPS2_Event_Release;
        }
    }

    // remember the old state
    old = newv;
}

static void ps2_time_update(uint32_t ms)
{
    int32_t ims;

    // small motor
    ims = (int32_t)ps2_small_motor_on_ms;
    ims -= ms;
    ps2_small_motor_on_ms = (ims > 0) ? (uint32_t)ims : 0;
    ps2_small_motor_value = (ps2_small_motor_on_ms > 0) ? ps2_small_motor_value : 0x00;
    // big motor
    ims = (int32_t)ps2_big_motor_on_ms;
    ims -= ms;
    ps2_big_motor_on_ms = (ims > 0) ? (uint32_t)ims : 0;
    ps2_big_motor_value = (ps2_big_motor_on_ms > 0) ? ps2_big_motor_value : 0x00;
}

static void ps2_event_callback()
{
    kPS2_Buttons_t buttons_push = kPS2_None;
    kPS2_Buttons_t buttons_hold = kPS2_None;
    kPS2_Buttons_t buttons_release = kPS2_None;

    for (int i = 0; i < 16; i++)
    {
        if (ps2buttons_event_set[i] & ps2buttons_state_current[i])
        {
            // common callback
            switch (ps2buttons_state_current[i])
            {
                case kPS2_Event_Push:
                    buttons_push |= (kPS2_Buttons_t)PS2_BIT_SET(i);
                    break;
                case kPS2_Event_Hold:
                    buttons_hold |= (kPS2_Buttons_t)PS2_BIT_SET(i);
                    break;
                case kPS2_Event_Release:
                    buttons_release |= (kPS2_Buttons_t)PS2_BIT_SET(i);
                    break;

                default:
                    break;
            }

            // installed callback
            if (NULL != ps2buttons_event_callback[i])
            {
                ps2buttons_event_callback[i]((kPS2_Buttons_t)PS2_BIT_SET(i), ps2buttons_state_current[i], ps2buttons_event_param[i]);
            }
        }
    }

    // common callback
    if (kPS2_None != buttons_push)
    {
        PS2_PushCallback(buttons_push);
    }
    if (kPS2_None != buttons_hold)
    {
        PS2_HoldCallback(buttons_hold);
    }
    if (kPS2_None != buttons_release)
    {
        PS2_ReleaseCallback(buttons_release);
    }
}

static void ps2_Update()
{
    ps2_UpdateDataRaw();

    // joystick value update
    ps2_right_joy_leftright_value   = ps2_joy_raw_conv(di_data[5]);
    ps2_right_joy_updown_value      = ps2_joy_raw_conv(di_data[6]);
    ps2_left_joy_leftright_value    = ps2_joy_raw_conv(di_data[7]);
    ps2_left_joy_updown_value       = ps2_joy_raw_conv(di_data[8]);

    // button value update
    ps2_button_value_current = 0;
    ps2_button_value_current |= (uint16_t)di_data[3];
    ps2_button_value_current |= (uint16_t)di_data[4] << 8;

    ps2_event_update(ps2_button_value_current);

    // invoke the event callback
    ps2_event_callback();
}

/*****************************************************************/

__attribute__((weak))
void PS2_PushCallback(kPS2_Buttons_t buttons)
{
    // to do
}

__attribute__((weak))
void PS2_HoldCallback(kPS2_Buttons_t buttons)
{
    // to do
}

__attribute__((weak))
void PS2_ReleaseCallback(kPS2_Buttons_t buttons)
{
    // to do
}

void PS2_LoopCallback(uint32_t ms)
{
    // Events & callback
    ps2_Update();
    // Time
    ps2_time_update(ms);
}

int8_t PS2_Init(PS2_TransferFunc_t transfer)
{
    ps2transfer = transfer;

    if (NULL == ps2transfer)
    {
        return -1;
    }

    // reset
    ps2_deinit();
    ps2_deinit();
    ps2_deinit();
    // enter config
    ps2_EnterConfing();

    ps2_TurnOnAnalogMode();
    ps2_VibrationMode();

    // exit config
    ps2_ExitConfing();

    return 0;
}

void PS2_MotorSmall_On(uint32_t ms)
{
    ms = (ms <= 2147483647) ? ms : 2147483647;

    ps2_small_motor_value = 0xff; // need set motor bytes 0xff to turn on
    ps2_small_motor_on_ms = ms;
}
void PS2_MotorSmall_Off()
{
    ps2_small_motor_value = 0; // set motor byte with not 0xff
    ps2_small_motor_on_ms = 0;
}

void PS2_MotorBig_On(uint8_t value, uint32_t ms)
{
    value = (value <= 100) ? value : 100;
    ms = (ms <= 2147483647) ? ms : 2147483647;

    ps2_big_motor_value = (uint8_t)((int)value * 191 / 100 + 64);
    ps2_big_motor_on_ms = ms;
}
void PS2_MotorBig_Off()
{
    ps2_big_motor_value = 0;
    ps2_big_motor_on_ms = 0;
}

int8_t PS2_RightJoy_ReadLeftRight()
{
    return ps2_right_joy_leftright_value;
}

int8_t PS2_RightJoy_ReadUpDown()
{
    return ps2_right_joy_updown_value;
}

int8_t PS2_LeftJoy_ReadLeftRight()
{
    return ps2_left_joy_leftright_value;
}

int8_t PS2_LeftJoy_ReadUpDown()
{
    return ps2_left_joy_updown_value;
}

kPS2_Event_t PS2_ButtonGetState(kPS2_Buttons_t button)
{
    for (int i = 0; i < 16; i++)
    {
        if (PS2_BIT_SET(i) == button)
        {
            return ps2buttons_state_current[i];
        }
    }

    return kPS2_Event_None;
}

const char * PS2_ButtonGetName(kPS2_Buttons_t button)
{
    for (int i = 0; i < 16; i++)
    {
        if (PS2_BIT_SET(i) == button)
        {
            return ps2buttonsName[i+1];
        }
    }

    return ps2buttonsName[0];
}

const char * PS2_ButtonGetEventName(kPS2_Event_t eventType)
{
    for (int i = 0; i < 3; i++)
    {
        if (PS2_BIT_SET(i) == eventType)
        {
            return ps2buttonsEventName[i+1];
        }
    }

    return ps2buttonsEventName[0];
}

void PS2_ButtonEventsSet(kPS2_Buttons_t buttons, kPS2_Event_t eventsType)
{
    for (int i = 0; i < 16; i++)
    {
        if (PS2_BIT_SET(i) & buttons)
        {
            ps2buttons_event_set[i] = eventsType;
        }
    }
}

void PS2_ButtonInstallCallback(kPS2_Buttons_t buttons, PS2_ButtonCallback_t callback, void *param)
{
    for (int i = 0; i < 16; i++)
    {
        if (PS2_BIT_SET(i) & buttons)
        {
            ps2buttons_event_callback[i] = callback;
            ps2buttons_event_param[i]    = param;
        }
    }
}
