#include "PSX_SDK_StreamState.h"

StStreamCallback St_streamCallback1 CTR_PSX_MATCH_SECTION(".bss.St_streamCallback1");
StStreamCallback St_streamCallback2 CTR_PSX_MATCH_SECTION(".bss.St_streamCallback2");
u32 St_state8009ebf4 CTR_PSX_MATCH_SECTION(".bss.St_state8009ebf4");
u32 St_state8009ebf8 CTR_PSX_MATCH_SECTION(".bss.St_state8009ebf8");
u32 St_state8009ebfc CTR_PSX_MATCH_SECTION(".bss.St_state8009ebfc");
u32 St_state8009ec00 CTR_PSX_MATCH_SECTION(".bss.St_state8009ec00");
u32 St_state8009ec04 CTR_PSX_MATCH_SECTION(".bss.St_state8009ec04");
u32 St_streamMode CTR_PSX_MATCH_SECTION(".bss.St_streamMode");
u32 St_maskEnd CTR_PSX_MATCH_SECTION(".bss.St_maskEnd");
u32 St_mask CTR_PSX_MATCH_SECTION(".bss.St_mask");
u16 St_state8009ec18 CTR_PSX_MATCH_SECTION(".bss.St_state8009ec18");
u32 St_state8009ec1c CTR_PSX_MATCH_SECTION(".bss.St_state8009ec1c");
u32 *St_ringBase CTR_PSX_MATCH_SECTION(".bss.St_ringBase");
u32 St_state8009ec28 CTR_PSX_MATCH_SECTION(".bss.St_state8009ec28");
u32 St_state8009ec2c CTR_PSX_MATCH_SECTION(".bss.St_state8009ec2c");
u32 St_state8009ec30 CTR_PSX_MATCH_SECTION(".bss.St_state8009ec30");
u32 St_ringCount CTR_PSX_MATCH_SECTION(".bss.St_ringCount");
u32 St_maskStart CTR_PSX_MATCH_SECTION(".bss.St_maskStart");
// NOTE(aalhendi): The publisher records a full location and separate frame count.
CdlLOC St_backloc CTR_PSX_MATCH_SECTION(".bss.St_backloc");
u32 St_backframe CTR_PSX_MATCH_SECTION(".bss.St_backframe");
// NOTE(aalhendi): CdRead2 selects this mode gate; the getter and stream IRQ consume it.
u32 St_state8009ec90 CTR_PSX_MATCH_SECTION(".bss.St_state8009ec90");

#ifndef CTR_NATIVE
// NOTE(aalhendi): Describe the genuine C objects for old GCC; these annotations emit no bytes.
__asm__(".type St_streamCallback1,@object\n.size St_streamCallback1,4\n"
        ".type St_streamCallback2,@object\n.size St_streamCallback2,4\n"
        ".type St_state8009ebf4,@object\n.size St_state8009ebf4,4\n"
        ".type St_state8009ebf8,@object\n.size St_state8009ebf8,4\n"
        ".type St_state8009ebfc,@object\n.size St_state8009ebfc,4\n"
        ".type St_state8009ec00,@object\n.size St_state8009ec00,4\n"
        ".type St_state8009ec04,@object\n.size St_state8009ec04,4\n"
        ".type St_streamMode,@object\n.size St_streamMode,4\n"
        ".type St_maskEnd,@object\n.size St_maskEnd,4\n"
        ".type St_mask,@object\n.size St_mask,4\n"
        ".type St_state8009ec18,@object\n.size St_state8009ec18,2\n"
        ".type St_state8009ec1c,@object\n.size St_state8009ec1c,4\n"
        ".type St_ringBase,@object\n.size St_ringBase,4\n"
        ".type St_state8009ec28,@object\n.size St_state8009ec28,4\n"
        ".type St_state8009ec2c,@object\n.size St_state8009ec2c,4\n"
        ".type St_state8009ec30,@object\n.size St_state8009ec30,4\n"
        ".type St_ringCount,@object\n.size St_ringCount,4\n"
        ".type St_maskStart,@object\n.size St_maskStart,4\n"
        ".type St_backloc,@object\n.size St_backloc,4\n"
        ".type St_backframe,@object\n.size St_backframe,4\n"
        ".type St_state8009ec90,@object\n.size St_state8009ec90,4\n");
#endif
