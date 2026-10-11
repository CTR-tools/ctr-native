#if !defined(CTR_NATIVE)
#include <macros.h>

// NOTE(aalhendi): Retail requests interrupt-state changes through syscall 0.
// Native keeps the existing critical-section macros.
__asm__(".section .EnterCriticalSection,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent EnterCriticalSection\n"
        ".set noreorder\n"
        ".globl EnterCriticalSection\n"
        "EnterCriticalSection:\n"
        "li $a0,1\n"
        "syscall\n"
        "jr $ra\n"
        "nop\n"
        ".end EnterCriticalSection\n"
        ".set reorder\n"
        ".text\n");

__asm__(".section .ExitCriticalSection,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent ExitCriticalSection\n"
        ".set noreorder\n"
        ".globl ExitCriticalSection\n"
        "ExitCriticalSection:\n"
        "li $a0,2\n"
        "syscall\n"
        "jr $ra\n"
        "nop\n"
        ".end ExitCriticalSection\n"
        ".set reorder\n"
        ".text\n");

// NOTE(aalhendi): Retail transfers the argument into GTE control register H.
// Native keeps SetGeomScreen and its CTC2 implementation.
__asm__(".section .SetGeomScreen,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent SetGeomScreen\n"
        ".set noreorder\n"
        ".globl SetGeomScreen\n"
        "SetGeomScreen:\n"
        "ctc2 $a0,$26\n"
        "jr $ra\n"
        "nop\n"
        "nop\n"
        ".end SetGeomScreen\n"
        ".set reorder\n"
        ".text\n");

// The private SDK record is word-aligned; its complete layout is unrecovered.
// These are only the command byte, command-data word, and length byte.
void PadSetCmd(u8 *sdkRecord, u32 command, u32 commandData, u32 length)
{
	sdkRecord[0x37] = (u8)command;
	*(u32 *)(sdkRecord + 0x2c) = commandData;
	sdkRecord[0x36] = (u8)length;
}
#endif
