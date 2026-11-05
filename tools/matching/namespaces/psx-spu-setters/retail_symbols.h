#include <macros.h>

// NOTE(aalhendi): SDK setters and CD initialization own distinct stored register pointers.
extern volatile u16 *psxSpuSdkRegs asm("data+47636");
#include "../../retail_bindings.h"
