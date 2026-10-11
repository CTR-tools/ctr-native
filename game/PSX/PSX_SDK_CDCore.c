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

#endif
