OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x80072278 : SUBALIGN(4) { KEEP(*(.SpuSetIRQCallback)) . = 0x40; } = 0
    .resident_block_1 0x80072c58 : SUBALIGN(4) { KEEP(*(.SpuSetIRQAddr)) } = 0
    .resident_block_2 0x80072cb8 : SUBALIGN(4) { KEEP(*(.SpuReadDecodedData)) . = 0x70; } = 0
    .resident_block_3 0x800761e0 : SUBALIGN(4) {
        KEEP(*(.SpuSetReverb)) . = 0xd0;
        KEEP(*(.SpuSetTransferStartAddr)) . = 0x130;
        KEEP(*(.SpuWrite)) . = 0x190;
        KEEP(*(.SpuIsTransferCompleted)) . = 0x240;
    } = 0
    .resident_block_4 0x80076840 : SUBALIGN(4) {
        KEEP(*(.SpuSetReverbVoice)) . = 0x30;
        KEEP(*(.SpuInit))
    } = 0
    .resident_block_5 0x8007a594 : SUBALIGN(4) {
        KEEP(*(._SpuInit))
        KEEP(*(.SpuStart)) . = 0x160;
    } = 0
    .resident_block_6 0x8007c840 : SUBALIGN(4) { KEEP(*(._SpuCallback)) . = 0x30; } = 0
    .resident_block_7 0x8007fe00 : SUBALIGN(4) { KEEP(*(.SpuClearReverbWorkArea)) . = 0x1a0; } = 0
    .resident_block_8 0x80080940 : SUBALIGN(4) { KEEP(*(._SpuDataCallback)) . = 0x30; } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
