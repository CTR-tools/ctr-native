OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x8007193c : SUBALIGN(4) { KEEP(*(.SpuSetTransferMode)) }
    .resident_block_1 0x80072c28 : SUBALIGN(4) { KEEP(*(.SpuSetTransferCallback)) . = 0x30; } = 0
    .data.psxSpuTransferModeRaw 0x8008c344 : SUBALIGN(1) { KEEP(*(.data.psxSpuTransferModeRaw)) }
    .data.psxSpuTransferMode 0x8008c3d0 : SUBALIGN(1) { KEEP(*(.data.psxSpuTransferMode)) }
    .data.psxSpuTransferCallback 0x8008c3ec : SUBALIGN(1) { KEEP(*(.data.psxSpuTransferCallback)) }
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
