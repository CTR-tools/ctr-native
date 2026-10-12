#if !defined(CTR_NATIVE)
#include <psx/libcd.h>
#include <psx/hwregs.h>

// NOTE(aalhendi): Retail SDK state has independent owners outside game Data.
// Native keeps platform/native_cd.c and its existing Cd* exports.
CdlCB CD_cbsync CTR_PSX_MATCH_SECTION(".data.CD_cbsync") = 0;
CdlCB CD_cbready CTR_PSX_MATCH_SECTION(".data.CD_cbready") = 0;
s32 CD_debug CTR_PSX_MATCH_SECTION(".data.CD_debug") = 0;

// NOTE(aalhendi): The SDK stores unsigned response bytes as complete words.
// CdStatus deliberately reads only the low byte of the first word.
u32 CD_status CTR_PSX_MATCH_SECTION(".data.CD_status") = 0;
u32 CD_status1 CTR_PSX_MATCH_SECTION(".data.CD_status1") = 0;
u32 CD_nopen CTR_PSX_MATCH_SECTION(".data.CD_nopen") = 0;
CdlLOC CD_pos CTR_PSX_MATCH_SECTION(".data.CD_pos") = {2, 0, 0, 0};
u8 CD_mode CTR_PSX_MATCH_SECTION(".data.CD_mode") = 0;
u8 CD_com CTR_PSX_MATCH_SECTION(".data.CD_com") = 0;

// NOTE(aalhendi): These two initialized bytes have no recovered purpose or mutator.
u8 CD_unresolved8008c43a[2] CTR_PSX_MATCH_SECTION(".data.CD_unresolved8008c43a") = {0, 0};

// NOTE(aalhendi): The old PSX compiler emits object labels without ELF type/size.
// Annotate the real C definitions; these directives allocate no storage.
__asm__(".type CD_cbsync,@object\n"
        ".size CD_cbsync,4\n"
        ".type CD_cbready,@object\n"
        ".size CD_cbready,4\n"
        ".type CD_debug,@object\n"
        ".size CD_debug,4\n"
        ".type CD_status,@object\n"
        ".size CD_status,4\n"
        ".type CD_status1,@object\n"
        ".size CD_status1,4\n"
        ".type CD_nopen,@object\n"
        ".size CD_nopen,4\n"
        ".type CD_pos,@object\n"
        ".size CD_pos,4\n"
        ".type CD_mode,@object\n"
        ".size CD_mode,1\n"
        ".type CD_com,@object\n"
        ".size CD_com,1\n"
        ".type CD_unresolved8008c43a,@object\n"
        ".size CD_unresolved8008c43a,2\n");

int CdStatus(void)
{
	return ((u8 *)&CD_status)[0];
}

int CdMode(void)
{
	return CD_mode;
}

int CdLastCom(void)
{
	return CD_com;
}

CdlLOC *CdLastPos(void)
{
	return &CD_pos;
}

// NOTE(aalhendi): Retail uses three BCD fields. Native keeps packed virtual
// file offsets in platform/native_cd.c instead.
CdlLOC *CdIntToPos(int i, CdlLOC *p)
{
	s32 minute;

	// NOTE(aalhendi): Wrap the lead-in before signed division; additive itob
	// preserves the SDK's negative-input bytes.
	i = (s32)((u32)i + 150u);
	p->sector = itob(i % 75);
	i /= 75;
	// NOTE(aalhendi): Keep the minute quotient before BCD encoding so the
	// compiler retains the retail division sequence.
	minute = i / 60;
	p->second = itob(i % 60);
	p->minute = itob(minute);
	return p;
}

int CdPosToInt(CdlLOC *p)
{
	return (DECODE_BCD(p->minute) * 60 + DECODE_BCD(p->second)) * 75 + DECODE_BCD(p->sector) - 150;
}

// NOTE(aalhendi): These are the four pointer words consumed by the controller
// reset. Reg1, SPU and the intervening bytes remain outside these owners.
volatile u8 *psxCdReg0 CTR_PSX_MATCH_SECTION(".data.psxCdReg0") = (volatile u8 *)0x1f801800;
volatile u8 *psxCdReg2 CTR_PSX_MATCH_SECTION(".data.psxCdReg2") = (volatile u8 *)0x1f801802;
volatile u8 *psxCdReg3 CTR_PSX_MATCH_SECTION(".data.psxCdReg3") = (volatile u8 *)0x1f801803;
volatile u32 *CD_resetWordReg CTR_PSX_MATCH_SECTION(".data.CD_resetWordReg") = (volatile u32 *)0x1f801020;

// NOTE(aalhendi): Only these three initialized controller-state bytes are
// consumed here; the following byte has no recovered ownership in this slice.
volatile u8 CD_controllerState[3] CTR_PSX_MATCH_SECTION(".data.CD_controllerState") = {0, 0, 0};

// NOTE(aalhendi): Annotate the real C owners for the old PSX compiler.
__asm__(".type psxCdReg0,@object\n"
        ".size psxCdReg0,4\n"
        ".type psxCdReg2,@object\n"
        ".size psxCdReg2,4\n"
        ".type psxCdReg3,@object\n"
        ".size psxCdReg3,4\n"
        ".type CD_resetWordReg,@object\n"
        ".size CD_resetWordReg,4\n"
        ".type CD_controllerState,@object\n"
        ".size CD_controllerState,3\n");

// NOTE(aalhendi): The reset provider ignores mode and preserves the SDK's
// pointer reloads, acknowledgement and delay slots. State and MMIO pointer
// storage remain the genuine C owners above.
// Tabbed .set directives control maspsx; spaced forms reach GNU as.
__asm__(".section .CD_ResetController,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tCD_ResetController\n"
        ".frame $sp,0,$ra\n"
        ".mask 0x00000000,0\n"
        ".fmask 0x00000000,0\n"
        ".set\tnoreorder\n"
        ".set noreorder\n"
        ".set noat\n"
        ".globl CD_ResetController\n"
        "CD_ResetController:\n"
        // Begin register index1; the incoming mode is intentionally ignored.
        "lui $v1,%hi(psxCdReg0)\n"
        "lw $v1,%lo(psxCdReg0)($v1)\n"
        "addiu $v0,$zero,1\n"
        "sb $v0,0($v1)\n"
        // Read low interrupt bits and preserve the branch delay instruction.
        "lui $v0,%hi(psxCdReg3)\n"
        "lw $v0,%lo(psxCdReg3)($v0)\n"
        "nop\n"
        "lbu $v0,0($v0)\n"
        "nop\n"
        "andi $v0,$v0,7\n"
        "beq $v0,$zero,.LCD_ResetController_clear\n"
        "addiu $a0,$zero,1\n"
        "addiu $v1,$zero,7\n"
        ".LCD_ResetController_ack:\n"
        // Acknowledge in reg0/reg3/reg2 order, reloading every pointer word.
        "lui $v0,%hi(psxCdReg0)\n"
        "lw $v0,%lo(psxCdReg0)($v0)\n"
        "nop\n"
        "sb $a0,0($v0)\n"
        "lui $v0,%hi(psxCdReg3)\n"
        "lw $v0,%lo(psxCdReg3)($v0)\n"
        "nop\n"
        "sb $v1,0($v0)\n"
        "lui $v0,%hi(psxCdReg2)\n"
        "lw $v0,%lo(psxCdReg2)($v0)\n"
        "nop\n"
        "sb $v1,0($v0)\n"
        "lui $v0,%hi(psxCdReg3)\n"
        "lw $v0,%lo(psxCdReg3)($v0)\n"
        "nop\n"
        "lbu $v0,0($v0)\n"
        "nop\n"
        "andi $v0,$v0,7\n"
        "bne $v0,$zero,.LCD_ResetController_ack\n"
        "nop\n"
        // Clear state2, reread it, and copy the observed byte to state1.
        ".LCD_ResetController_clear:\n"
        "lui $v1,%hi(CD_controllerState)\n"
        "addiu $v1,$v1,%lo(CD_controllerState)\n"
        "sb $zero,2($v1)\n"
        "lbu $v0,2($v1)\n"
        "nop\n"
        "sb $v0,1($v1)\n"
        // Capture reg0 before writing state0, then clear reg0 and reg3.
        "lui $a0,%hi(psxCdReg0)\n"
        "lw $a0,%lo(psxCdReg0)($a0)\n"
        "addiu $v0,$zero,2\n"
        "sb $v0,0($v1)\n"
        "sb $zero,0($a0)\n"
        "lui $v0,%hi(psxCdReg3)\n"
        "lw $v0,%lo(psxCdReg3)($v0)\n"
        "nop\n"
        "sb $zero,0($v0)\n"
        // Write the full final word in the return delay slot and return it.
        "lui $v1,%hi(CD_resetWordReg)\n"
        "lw $v1,%lo(CD_resetWordReg)($v1)\n"
        "addiu $v0,$zero,0x1325\n"
        "jr $ra\n"
        "sw $v0,0($v1)\n"
        ".end\tCD_ResetController\n"
        ".set at\n"
        ".set\treorder\n"
        ".set reorder\n"
        ".text\n");

#endif
