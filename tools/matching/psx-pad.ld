OUTPUT_ARCH(mips)
SECTIONS
{
    .rodata 0x80011eb0 : SUBALIGN(4) { *(.rodata) *(.rodata.*) }
    .resident_block_0 0x80075590 : SUBALIGN(4) {
        KEEP(*(.PadGetState)) . = 0xd0;
    } = 0
    .resident_block_1 0x8007588c : SUBALIGN(4) {
        KEEP(*(.PadStopCom))
        KEEP(*(.PadSetActAlign))
        KEEP(*(._padSendActAlign))
        KEEP(*(._padRecvActAlign)) . = 0x1b4;
        KEEP(*(.PadSetMainMode))
        KEEP(*(._padSendMainMode))
        KEEP(*(._padRecvMainMode)) . = 0x314;
        KEEP(*(.PadSetAct))
        KEEP(*(.PadInfoAct)) . = 0x434;
    } = 0
    .resident_block_2 0x8007d564 : SUBALIGN(4) {
        KEEP(*(._padSendAtLoadInfo))
        KEEP(*(._padRecvAtLoadInfo))
        KEEP(*(._padGetActSize))
        KEEP(*(._padLoadActInfo))
        KEEP(*(._padSendLoadActInfo))
        KEEP(*(._padRecvLoadActInfo))
        KEEP(*(._padCmdParaMode))
        KEEP(*(._padSendCmd45))
        KEEP(*(._padSendCmd4c))
        KEEP(*(._padSendCmd46))
        KEEP(*(._padSendCmd47))
        KEEP(*(._padSendCmd4b))
    } = 0
    .resident_block_3 0x8007decc : SUBALIGN(4) {
        KEEP(*(._padCheckWait))
        KEEP(*(._padComInit))
        KEEP(*(._padComSendCmd))
        KEEP(*(._padComRecvId))
    } = 0
    .resident_block_4 0x8007f47c : SUBALIGN(4) {
        KEEP(*(._mtapFailAuto))
        KEEP(*(._mtapCanExecCmd)) . = 0x404;
    } = 0
    .resident_block_5 0x80080630 : SUBALIGN(4) {
        KEEP(*(.ReadInitPadFlag))
    } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
