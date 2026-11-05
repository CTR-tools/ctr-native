OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x80076560 : SUBALIGN(4) { KEEP(*(.SpuSetVoicePitch)) }
    .resident_block_1 0x80076890 : SUBALIGN(4) { KEEP(*(.SpuSetVoiceVolume)) . = 0x80; } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
