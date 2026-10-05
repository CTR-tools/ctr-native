#ifdef TEST_BORROWER
/* Static relocation/owned-instruction witness, not executed hardware proof. */
__asm__(".section .Borrower,\"ax\",@progbits\n"
        ".globl Borrower\n.set noreorder\nBorrower:\n"
        "beq $a0,$zero,OwnedProbe_Return\n"
        "nop\njr $ra\nnop\n.set reorder\n");
#else
#include "psx_at44.h"
#ifndef CASE
#define CASE 2
#endif
#ifndef ENTRY
#define ENTRY OwnedProbe
#endif
#ifndef RETURN_NAME
#define RETURN_NAME "OwnedProbe_Return"
#endif
#define NAMED __attribute__((mips_unsafe_manual_at44(RETURN_NAME)))
#if CASE == 1 || defined(UNNAMED)
#define OWNER __attribute__((mips_unsafe_manual_at44))
#elif CASE == 6
#define OWNER
#elif CASE == 8
#define OWNER __attribute__((mips_unsafe_manual_at44))
#elif CASE == 10
#define OWNER __attribute__((mips_unsafe_manual_at44(RETURN_NAME, "Extra")))
#elif CASE == 11
#define OWNER __attribute__((mips_unsafe_manual_at_frame(RETURN_NAME)))
#elif CASE == 15
#define OWNER NAMED __attribute__((mips_unsafe_manual_at44("Other_Return")))
#else
#define OWNER NAMED
#endif
#if CASE == 3 || CASE == 6 || CASE == 8
void NAMED ENTRY(u32, u32, u32, u32);
#elif CASE == 4
void __attribute__((mips_unsafe_manual_at44("Other_Return"))) ENTRY(u32, u32, u32, u32);
#elif CASE == 5
void ENTRY(u32, u32, u32, u32);
#elif CASE == 7
void __attribute__((mips_unsafe_manual_at44)) ENTRY(u32, u32, u32, u32);
#elif CASE == 9
void ENTRY(u32, u32, u32, u32) asm("RenamedProbe");
#elif CASE == 14
void NAMED NAMED ENTRY(u32, u32, u32, u32);
#endif
void OWNER ENTRY(u32 a, u32 b, u32 c, u32 d)
{
    FRAME_DECLARE();
    (void)b; (void)c; (void)d;
    FRAME_SAVE();
#if CASE == 12
    if (a) { FRAME_RESTORE(); return; }
#else
    (void)a;
#endif
    FRAME_RESTORE();
}
#if CASE == 13
void __attribute__((mips_unsafe_manual_at44("Second_Return"))) SecondProbe(u32 a, u32 b, u32 c, u32 d)
{
    FRAME_DECLARE();
    (void)a; (void)b; (void)c; (void)d;
    FRAME_SAVE();
    FRAME_RESTORE();
}
#elif CASE == 16
__asm__(".text\n.globl OwnedProbe_Return\nOwnedProbe_Return:\n");
#endif
#endif
