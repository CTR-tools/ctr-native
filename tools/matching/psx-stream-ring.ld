OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x80071e20 : SUBALIGN(4) { KEEP(*(.CdDataCallback)) }
    .resident_block_1 0x80071e64 : SUBALIGN(4)
    {
        KEEP(*(.CdIntToPos))
        . = 0x104;
        KEEP(*(.CdPosToInt))
        . = 0x184;
    } = 0
    .resident_block_2 0x80079824 : SUBALIGN(4) { KEEP(*(.StClearRing)) }
    .resident_block_3 0x80079d54 : SUBALIGN(4)
    {
        KEEP(*(.CdRead2))
        . = 0x84;
        KEEP(*(.St_cdready_callback))
        . = 0xb0;
    } = 0
    .resident_block_4 0x8007a184 : SUBALIGN(4)
    {
        KEEP(*(.StSetStream))
        . = 0x90;
        KEEP(*(.StSetMask))
        . = 0xb0;
        KEEP(*(.StSetRing))
        . = 0xe0;
        KEEP(*(.StGetNext))
        . = 0x1a0;
        KEEP(*(.data_ready_callback))
        . = 0x22c;
        KEEP(*(.StGetBackloc))
        . = 0x290;
    } = 0
    .resident_block_5 0x8007a414 : SUBALIGN(4)
    {
        KEEP(*(.StFreeRing))
        . = 0xb0;
        KEEP(*(.StRingStatus))
        . = 0x180;
    } = 0
    .resident_block_6 0x80080900 : SUBALIGN(4)
    {
        KEEP(*(.init_ring_status))
        . = 0x40;
    } = 0
    .bss.St_streamCallback1 0x8009ebec (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_streamCallback1)) }
    .bss.St_streamCallback2 0x8009ebf0 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_streamCallback2)) }
    .bss.St_state8009ebf4 0x8009ebf4 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_state8009ebf4)) }
    .bss.St_state8009ebf8 0x8009ebf8 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_state8009ebf8)) }
    .bss.St_state8009ebfc 0x8009ebfc (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_state8009ebfc)) }
    .bss.St_state8009ec00 0x8009ec00 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_state8009ec00)) }
    .bss.St_state8009ec04 0x8009ec04 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_state8009ec04)) }
    .bss.St_streamMode 0x8009ec08 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_streamMode)) }
    .bss.St_maskEnd 0x8009ec0c (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_maskEnd)) }
    .bss.St_mask 0x8009ec10 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_mask)) }
    .bss.St_state8009ec18 0x8009ec18 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_state8009ec18)) }
    .bss.St_state8009ec1c 0x8009ec1c (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_state8009ec1c)) }
    .bss.St_ringBase 0x8009ec24 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_ringBase)) }
    .bss.St_state8009ec28 0x8009ec28 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_state8009ec28)) }
    .bss.St_state8009ec2c 0x8009ec2c (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_state8009ec2c)) }
    .bss.St_state8009ec30 0x8009ec30 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_state8009ec30)) }
    .bss.St_ringCount 0x8009ec34 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_ringCount)) }
    .bss.St_maskStart 0x8009ec38 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_maskStart)) }
    .bss.St_backloc 0x8009ec3c (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_backloc)) }
    .bss.St_backframe 0x8009ec40 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_backframe)) }
    .bss.St_state8009ec90 0x8009ec90 (NOLOAD) : SUBALIGN(1) { KEEP(*(.bss.St_state8009ec90)) }
    /* NOTE(aalhendi): Only the declared stream and CD-provider ranges are owned here. */
    /DISCARD/ : { *(*) }
}
