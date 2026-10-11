OUTPUT_ARCH(mips)

SECTIONS
{
    .resident_block_0 0x8007196c : SUBALIGN(4) { KEEP(*(.CdStatus)) }
    .resident_block_1 0x8007197c : SUBALIGN(4) { KEEP(*(.CdMode)) }
    .resident_block_2 0x8007198c : SUBALIGN(4) { KEEP(*(.CdLastCom)) }
    .resident_block_3 0x8007199c : SUBALIGN(4) { KEEP(*(.CdLastPos)) }
    .resident_block_4 0x800719a8 : SUBALIGN(4) { KEEP(*(.CdReset)) }
    .resident_block_5 0x800719c8 : SUBALIGN(4) { KEEP(*(.CdSetDebug)) }
    .resident_block_6 0x800719fc : SUBALIGN(4) { KEEP(*(.CdSyncCallback)) }
    .resident_block_7 0x80071a10 : SUBALIGN(4) { KEEP(*(.CdReadyCallback)) }
    .resident_block_8 0x80071e64 : SUBALIGN(4) { KEEP(*(.CdIntToPos)) }
    .resident_block_9 0x80071f68 : SUBALIGN(4) { KEEP(*(.CdPosToInt)) }
    .resident_block_10 0x8007c044 : SUBALIGN(4) { KEEP(*(.CD_ResetController)) }

    /* NOTE(aalhendi): Byte owners follow words without GCC's section padding. */
    .data.sdk_cd_core 0x8008c41c : SUBALIGN(1)
    {
        KEEP(*(.data.CD_cbsync))
        KEEP(*(.data.CD_cbready))
        KEEP(*(.data.CD_debug))
        KEEP(*(.data.CD_status))
        KEEP(*(.data.CD_status1))
        KEEP(*(.data.CD_nopen))
        KEEP(*(.data.CD_pos))
        KEEP(*(.data.CD_mode))
        KEEP(*(.data.CD_com))
        KEEP(*(.data.CD_unresolved8008c43a))
    }

    .data.psxCdReg0 0x8008c6e0 : SUBALIGN(1) { KEEP(*(.data.psxCdReg0)) }
    .data.psxCdReg2 0x8008c6e8 : SUBALIGN(1) { KEEP(*(.data.psxCdReg2)) }
    .data.psxCdReg3 0x8008c6ec : SUBALIGN(1) { KEEP(*(.data.psxCdReg3)) }
    .data.CD_resetWordReg 0x8008c6f0 : SUBALIGN(1) { KEEP(*(.data.CD_resetWordReg)) }
    .data.CD_controllerState 0x8008c6f8 : SUBALIGN(1) { KEEP(*(.data.CD_controllerState)) }

    /DISCARD/ :
    {
        *(.reginfo) *(.MIPS.abiflags) *(.pdr)
        *(.gnu.attributes) *(.comment)
    }
}
