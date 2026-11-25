OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x800771c4 : SUBALIGN(4) {
        KEEP(*(.ResetCallback))
        KEEP(*(.InterruptCallback))
        KEEP(*(.DMACallback))
        KEEP(*(.VSyncCallback))
    } = 0
    .resident_block_1 0x80077a54 : SUBALIGN(4) {
        KEEP(*(.BreakDraw))
        KEEP(*(.IsIdleGPU))
        KEEP(*(.SetRCnt))
        KEEP(*(.GetRCnt))
        KEEP(*(.StartRCnt))
        KEEP(*(.StopRCnt))
        KEEP(*(.ResetRCnt)) . = 0x264;
    } = 0
    .resident_block_2 0x80077e38 : SUBALIGN(4) {
        KEEP(*(.memmove)) . = 0x70;
    } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
