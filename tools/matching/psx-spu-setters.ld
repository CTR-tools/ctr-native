OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x80072c98 : SUBALIGN(4) { KEEP(*(.SpuSetCommonCDVolume)) . = 0x20; } = 0
    .resident_block_1 0x800761b0 : SUBALIGN(4) { KEEP(*(.SpuSetReverbModeDepth)) . = 0x30; } = 0
    .resident_block_2 0x800765d0 : SUBALIGN(4) { KEEP(*(.SpuSetCommonCDMix)) }
    .resident_block_3 0x800767e0 : SUBALIGN(4)
    {
        KEEP(*(.SpuSetCommonMasterVolume))
        . = 0x20;
        KEEP(*(.SpuSetCommonCDReverb))
    } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
