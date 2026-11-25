OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x80076f10 : SUBALIGN(4) {
        KEEP(*(.CdRead))
        KEEP(*(.CdReadSync))
    } = 0
    .resident_block_1 0x80078bf8 : SUBALIGN(4) {
        KEEP(*(.CdDiskReady))
    } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
