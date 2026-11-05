#include <macros.h>

#ifdef CTR_NATIVE
// NOTE(aalhendi): Native starts with unknown mode until selected; retail initializes it to NTSC.
s32 PSX_videoMode = -1;
#else
s32 PSX_videoMode CTR_PSX_MATCH_SECTION(".data.PSX_videoMode") = 0;
__asm__(".type PSX_videoMode,@object\n.size PSX_videoMode,4\n");
#endif
