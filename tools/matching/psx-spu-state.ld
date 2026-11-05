OUTPUT_ARCH(mips)
SECTIONS
{
    .initialized.depth 0x8008c35c : SUBALIGN(1) { KEEP(*(.data.psxSpuReverbDepth)) }
    .initialized.regs 0x8008c3b4 : SUBALIGN(1) { KEEP(*(.data.psxSpuSdkRegs)) }
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
