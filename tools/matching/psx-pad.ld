OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x80075590 : SUBALIGN(4) {
        KEEP(*(.PadGetState)) . = 0xd0;
    } = 0
    .resident_block_1 0x8007588c : SUBALIGN(4) {
        KEEP(*(.PadStopCom))
    } = 0
    .resident_block_2 0x80075ba0 : SUBALIGN(4) {
        KEEP(*(.PadSetAct))
    } = 0
    .resident_block_3 0x8007d564 : SUBALIGN(4) {
        KEEP(*(._padSendAtLoadInfo))
        KEEP(*(._padRecvAtLoadInfo))
        KEEP(*(._padGetActSize))
    } = 0
    .resident_block_4 0x80080630 : SUBALIGN(4) {
        KEEP(*(.ReadInitPadFlag))
    } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
