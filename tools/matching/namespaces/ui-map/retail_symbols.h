#define UI_MAP_COLORS       uiMapColors
#define UI_MAP_ARROW_POS    uiMapArrowPos
#define UI_MAP_ARROW_COLORS uiMapArrowColors

#include "../../retail_bindings.h"

extern u32 *uiMapColors[NUM_COLORS] asm("data+5072");
extern SVec2 uiMapArrowPos[3] asm("data+23160");
extern s32 uiMapArrowColors[2][3] asm("data+23172");
