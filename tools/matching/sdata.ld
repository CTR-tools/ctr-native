OUTPUT_ARCH(mips)

SECTIONS
{
    .initialized 0x8008cf6c : { *(.data*) *(.sdata*) }

    /DISCARD/ :
    {
        *(.bss*) *(.sbss*) *(COMMON)
        *(.reginfo) *(.MIPS.abiflags) *(.pdr)
        *(.gnu.attributes) *(.comment)
    }
}

ASSERT(ADDR(.initialized) == 0x8008cf6c, "SDATA section misplaced")
ASSERT(sdata_static == ADDR(.initialized), "SDATA symbol misplaced")
ASSERT(SIZEOF(.initialized) == 0x6fc, "SDATA initialized size changed")
