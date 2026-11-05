OUTPUT_ARCH(mips)
SECTIONS
{
    .data.psxSpuTransferModeRaw 0x8008c344 : SUBALIGN(1) { KEEP(*(.data.psxSpuTransferModeRaw)) }
    .data.psxSpuTransferMode 0x8008c3d0 : SUBALIGN(1) { KEEP(*(.data.psxSpuTransferMode)) }
    .data.psxSpuTransferCallback 0x8008c3ec : SUBALIGN(1) { KEEP(*(.data.psxSpuTransferCallback)) }
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
