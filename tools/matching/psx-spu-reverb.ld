OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x8007f8a0 : SUBALIGN(4) {
        KEEP(*(._SpuIsInAllocateArea_)) . = 0x90;
        KEEP(*(._spu_setReverbAttr))
    } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
