OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x80075310 : SUBALIGN(4)
    {
        KEEP(*(.AddPrim))
        . = 0x40;
    } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
