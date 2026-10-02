OUTPUT_ARCH(mips)

SECTIONS
{
    .initialized 0x800809a0 : { *(.data*) *(.sdata*) }

    /DISCARD/ :
    {
        *(.bss*) *(.sbss*) *(COMMON)
        *(.reginfo) *(.MIPS.abiflags) *(.pdr)
        *(.gnu.attributes) *(.comment)
    }
}

ASSERT(ADDR(.initialized) == 0x800809a0, "DATA section misplaced")
ASSERT(data == ADDR(.initialized), "DATA symbol misplaced")
ASSERT(SIZEOF(.initialized) == 0x9900, "Modeled DATA initialized size changed")
