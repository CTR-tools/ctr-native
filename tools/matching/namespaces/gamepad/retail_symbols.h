#define GAMEPAD_ACT_ALIGN  gamepadActAlign
#define GAMEPAD_BUTTON_MAP gamepadButtonMap
#define GAMEPAD_WHEEL_DATA gamepadWheelData

#include "../../retail_bindings.h"

extern u8 gamepadActAlign[8] asm("sdata_static+204");
extern struct GamepadButtonMap gamepadButtonMap[20] asm("data+6920");
extern struct RacingWheelData gamepadWheelData[4] asm("data+14428");
