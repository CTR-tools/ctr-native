#include <common.h>

#ifdef CTR_NATIVE
s32 Unknown_8006ef98(s32 radicand)
{
	u32 root = 0;
	u32 remainder = 0;
	u32 work;
	s32 bitCount;

	// NOTE(aalhendi): Even the zero path writes LZCS/LZCR. Keep the GTE state
	// change and 32-bit shift/subtract wrapping of the s5-to-s6 retail entry.
	MTC2((u32)radicand, 30);
	if (radicand == 0)
	{
		return 0;
	}

	bitCount = MFC2(31) & 0x1e;
	work = (u32)radicand << bitCount;
	bitCount ^= 0x1e;
	do
	{
		u32 trial;

		remainder |= work >> 30;
		trial = remainder - ((root << 2) + 1);
		root <<= 1;
		work <<= 2;
		if ((s32)trial >= 0)
		{
			root++;
			remainder = trial << 2;
		}
		else
		{
			remainder <<= 2;
		}
		bitCount -= 2;
	} while (bitCount >= 0);

	return (s32)root;
}
#else
// NOTE(aalhendi): The wheel renderers pass s5 and consume s6, preserving t2
// and their live t3-t5 vectors across this entry. It clobbers s1-s3 and s5,
// plus GTE LZCS/LZCR; this is not a normal C-call ABI on the retail target.
__asm__(".section .Unknown_8006ef98,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tUnknown_8006ef98\n"
        ".set\tnoreorder\n"
        ".globl Unknown_8006ef98\n"
        "Unknown_8006ef98:\n"
        "mtc2 $s5,$30\n"
        "beq $s5,$zero,.Ltire_sqrt_zero\n"
        "addiu $s6,$zero,0\n"
        "mfc2 $s3,$31\n"
        "addiu $s1,$zero,0\n"
        "andi $s3,$s3,0x1e\n"
        "sllv $s5,$s5,$s3\n"
        "bgez $zero,.Ltire_sqrt_test\n"
        "xori $s3,$s3,0x1e\n"
        ".Ltire_sqrt_loop:\n"
        "srl $s2,$s5,30\n"
        "or $s1,$s1,$s2\n"
        "sll $s2,$s6,2\n"
        "addiu $s2,$s2,1\n"
        "sll $s6,$s6,1\n"
        "subu $s2,$s1,$s2\n"
        "bltz $s2,.Ltire_sqrt_failed\n"
        "sll $s5,$s5,2\n"
        "addiu $s6,$s6,1\n"
        "sll $s1,$s2,2\n"
        ".Ltire_sqrt_test:\n"
        "bgez $s3,.Ltire_sqrt_loop\n"
        "addiu $s3,$s3,-2\n"
        "jr $ra\n"
        ".Ltire_sqrt_failed:\n"
        "sll $s1,$s1,2\n"
        "bgez $s3,.Ltire_sqrt_loop\n"
        "addiu $s3,$s3,-2\n"
        ".Ltire_sqrt_zero:\n"
        "jr $ra\n"
        "nop\n"
        ".end Unknown_8006ef98\n"
        ".set\treorder\n"
        ".text\n");
#endif
