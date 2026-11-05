OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x80077cd8 : SUBALIGN(4)
    {
        KEEP(*(.SetVideoMode))
        KEEP(*(.GetVideoMode))
        . = 0x30;
    } = 0
    .resident_block_1 0x80077d38 : SUBALIGN(4)
    {
        KEEP(*(.SetDefDrawEnv))
        . = 0xc0;
        KEEP(*(.SetDefDispEnv))
        . = 0x100;
    } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
