OUTPUT_ARCH(mips)
SECTIONS
{
    .data.PSX_videoMode 0x8008c0b4 : SUBALIGN(1) { KEEP(*(.data.PSX_videoMode)) }
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
