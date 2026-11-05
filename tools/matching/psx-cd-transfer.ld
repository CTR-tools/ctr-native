OUTPUT_ARCH(mips)
SECTIONS
{
    .resident_block_0 0x800719dc : SUBALIGN(4) { KEEP(*(.CdReady)) }
    .resident_block_1 0x80071de0 : SUBALIGN(4) { KEEP(*(.CdGetSector)) KEEP(*(.CdGetSector2)) KEEP(*(.CdDataCallback)) KEEP(*(.CdDataSync)) }
    .resident_block_2 0x800771b0 : SUBALIGN(4) { KEEP(*(.CdReadCallback)) }
    .data.CD_cbread 0x8008ad10 : SUBALIGN(1) { KEEP(*(.data.CD_cbread)) }
    /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) }
}
