OUTPUT_ARCH(mips)
SECTIONS
{
    .rodata 0x800123b0 : SUBALIGN(4) { *(.rodata) *(.rodata.*) }
    .resident_block_0 0x80080280 : SUBALIGN(4) {
        KEEP(*(.startIntrDMA))
        KEEP(*(._trapIntrDMA))
        KEEP(*(._setIntrDMA))
        KEEP(*(._clearIntrDMATable))
        . = 0x2a0;
    } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
