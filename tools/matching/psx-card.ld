OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x80077ee8 : SUBALIGN(4) {
        KEEP(*(.InitCARD))
        KEEP(*(.StartCARD))
        KEEP(*(.StopCARD)) . = 0xd0;
    } = 0
    .resident_block_1 0x80077ff8 : SUBALIGN(4) {
        KEEP(*(._card_clear)) . = 0x40;
    } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
