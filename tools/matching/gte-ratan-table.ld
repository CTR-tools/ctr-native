OUTPUT_ARCH(mips)

SECTIONS
{
    .initialized 0x8008a480 : { *(.data) }

    /DISCARD/ :
    {
        *(.bss*) *(.sbss*) *(COMMON)
        *(.reginfo) *(.MIPS.abiflags) *(.pdr)
        *(.gnu.attributes) *(.comment)
    }
}

ASSERT(ADDR(.initialized) == 0x8008a480, "Arctangent table section misplaced")
ASSERT(ratan_tbl == ADDR(.initialized), "Arctangent table symbol misplaced")
ASSERT(SIZEOF(.initialized) == 0x802, "Arctangent table size changed")
