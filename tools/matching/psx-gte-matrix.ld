OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x8007170c : SUBALIGN(4)
    {
        KEEP(*(.RotTrans))
    }
    .resident_block_1 0x800718dc : SUBALIGN(4)
    {
        KEEP(*(.SetTransMatrix))
        KEEP(*(.SetRotMatrix))
    }
    .resident_block_2 0x80075560 : SUBALIGN(4)
    {
        KEEP(*(.SetLightMatrix))
    }
    .resident_block_3 0x8007782c : SUBALIGN(4)
    {
        KEEP(*(.SetGeomOffset))
        . = 0x20;
    } = 0
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
