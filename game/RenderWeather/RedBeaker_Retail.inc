// NOTE(aalhendi): Retail saves callee registers in scratchpad, then uses sp
// and gp as RNG lanes. Each pass overlaps RTPT with the next seed advance.
// Textual macros retain those hazards and delay slots without an extra call.
// clang-format off

#define RBR_RNG_SHIFT \
	"sll $v1,$s1,24\n" \
	"srl $s1,$s1,8\n"

#define RBR_RNG_BYTE(lane) \
	"addu " lane ",$s0,$s1\n" \
	"srl $s0,$s0,8\n" \
	"or $s0,$s0,$v1\n" \
	"srl $v1,$s0,8\n" \
	"addu " lane "," lane ",$v1\n" \
	"sll " lane "," lane ",24\n" \
	"or $s1,$s1," lane "\n"

#define RBR_RNG_COORDS \
	RBR_RNG_BYTE("$t9") \
	RBR_RNG_SHIFT RBR_RNG_BYTE("$gp") \
	RBR_RNG_SHIFT RBR_RNG_BYTE("$sp") \
	"sra $t9,$t9,24\n" \
	"sra $gp,$gp,7\n" \
	"andi $t9,$t9,0xffff\n"

#define RBR_SEED(tag) \
	"lui $s0,0x3012\n" \
	"ori $s0,$s0,0x5400\n" \
	"lui $s1,0x4935\n" \
	"bgez $zero,.Lrbr_" tag "_rng\n" \
	"ori $s1,$s1,0x83fe\n"

// Both passes restart the same stream. A projected drop advances it once
// while RTPT runs, then again at the shared loop tail; a wrap reject does not.
#define RBR_PASS(tag, color) \
	".Lrbr_" tag "_drop:\n" \
	"or $t0,$t9,$gp\n" \
	"lw $t1,48($at)\n" \
	"addu $v1,$t0,$s3\n" \
	"addu $t0,$t0,$s5\n" \
	"and $v1,$v1,$t6\n" \
	"and $t0,$t0,$t6\n" \
	"subu $v1,$v1,$t1\n" \
	"subu $t0,$t0,$t1\n" \
	"subu $t1,$t0,$v1\n" \
	"bne $t1,$s7,.Lrbr_" tag "_rng\n" \
	"and $v1,$v1,$t5\n" \
	"and $t0,$t0,$t5\n" \
	"mtc2 $v1,$0\n" \
	"mtc2 $t0,$2\n" \
	"mtc2 $t0,$4\n" \
	"sra $t2,$sp,23\n" \
	"addu $t0,$t2,$s4\n" \
	"addu $t1,$t2,$s6\n" \
	"andi $t0,$t0,0xff\n" \
	"andi $t1,$t1,0xff\n" \
	"subu $t2,$t1,$t0\n" \
	"bne $t2,$t8,.Lrbr_" tag "_rng\n" \
	"addiu $t0,$t0,-128\n" \
	"addiu $t1,$t1,-128\n" \
	"mtc2 $t0,$1\n" \
	"mtc2 $t1,$3\n" \
	"mtc2 $t1,$5\n" \
	RBR_RNG_SHIFT \
	"cop2 0x280030\n" \
	RBR_RNG_COORDS \
	"mfc2 $v1,$12\n" \
	"cfc2 $t1,$31\n" \
	"mfc2 $t0,$13\n" \
	"sll $t1,$t1,14\n" \
	"bltz $t1,.Lrbr_" tag "_rng\n" \
	"and $t1,$v1,$t0\n" \
	"subu $v1,$v1,$t4\n" \
	"subu $t0,$t0,$t4\n" \
	"nor $v1,$v1,$t0\n" \
	"or $v1,$v1,$t1\n" \
	"bltz $v1,.Lrbr_" tag "_rng\n" \
	"sll $v1,$v1,16\n" \
	"bltz $v1,.Lrbr_" tag "_rng\n" \
	"lui $v1,0x5200\n" \
	"lw $t0," color "($at)\n" \
	"sw $v1,4($v0)\n" \
	"sw $t0,12($v0)\n" \
	"lw $t2,0($t3)\n" \
	"sll $v1,$v0,8\n" \
	"srl $v1,$v1,8\n" \
	"lui $t1,0x0400\n" \
	"or $t2,$t2,$t1\n" \
	"sw $t2,0($v0)\n" \
	"swc2 $12,8($v0)\n" \
	"swc2 $13,16($v0)\n" \
	"sw $v1,0($t3)\n" \
	"addiu $v0,$v0,20\n" \
	".Lrbr_" tag "_rng:\n" \
	RBR_RNG_SHIFT RBR_RNG_COORDS \
	"bne $t7,$zero,.Lrbr_" tag "_drop\n" \
	"addiu $t7,$t7,-1\n"

#define RBR_DRAW_MODE(mode) \
	"lui $v1,0xe100\n" \
	"ori $v1,$v1," mode "\n" \
	"sw $v1,4($v0)\n" \
	"sw $zero,8($v0)\n" \
	"lw $t2,0($t3)\n" \
	"sll $v1,$v0,8\n" \
	"lui $t1,0x0200\n" \
	"or $t2,$t2,$t1\n" \
	"sw $t2,0($v0)\n" \
	"srl $v1,$v1,8\n" \
	"sw $v1,0($t3)\n" \
	"addiu $v0,$v0,12\n"

__asm__(".section .RedBeaker_RenderRain,\"ax\",@progbits\n"
	".align 2\n"
	".ent\tRedBeaker_RenderRain\n"
	".set\tnoreorder\n"
	".set\tnoat\n"
	".globl RedBeaker_RenderRain\n"
	"RedBeaker_RenderRain:\n"
	"lui $at,0x1f80\n"
	"sw $s0,0($at)\n"
	"sw $s1,4($at)\n"
	"sw $s2,8($at)\n"
	"sw $s3,12($at)\n"
	"sw $s4,16($at)\n"
	"sw $s5,20($at)\n"
	"sw $s6,24($at)\n"
	"sw $s7,28($at)\n"
	"sw $gp,32($at)\n"
	"sw $sp,36($at)\n"
	"sw $s8,40($at)\n"
	"sw $ra,44($at)\n"
	"lw $s8,16($sp)\n"
	"lw $v0,12($a1)\n"
	"lw $a2,12($a2)\n"
	"addiu $a3,$a3,-1\n"
	"addiu $s2,$zero,0\n"
	"beq $a2,$zero,.Lrbr_restore\n"
	"sw $a2,56($at)\n"
	"sw $zero,72($at)\n"
	"sw $zero,76($at)\n"
	"lui $v1,0x0200\n"
	"ori $v1,$v1,0x0080\n"
	"sw $v1,48($at)\n"
	".Lrbr_player:\n"
	"lw $v1,40($a0)\n"
	"lw $t0,44($a0)\n"
	"lw $t1,48($a0)\n"
	"ctc2 $v1,$0\n"
	"ctc2 $t0,$1\n"
	"ctc2 $t1,$2\n"
	"lw $v1,52($a0)\n"
	"lw $t0,56($a0)\n"
	"ctc2 $v1,$3\n"
	"ctc2 $t0,$4\n"
	"lw $a2,56($at)\n"
	"lh $v1,32($a0)\n"
	"lh $t0,34($a0)\n"
	"lw $t1,24($a0)\n"
	"lw $t4,32($a0)\n"
	"lw $t3,244($a0)\n"
	"sll $v1,$v1,15\n"
	"sll $t0,$t0,15\n"
	"ctc2 $v1,$24\n"
	"ctc2 $t0,$25\n"
	"ctc2 $t1,$26\n"
	"sw $t3,52($at)\n"
	".Lrbr_cloud:\n"
	"beq $a2,$zero,.Lrbr_next_player\n"
	"lw $t2,36($a2)\n"
	"lw $t7,8($a2)\n"
	"beq $t2,$zero,.Lrbr_next_cloud\n"
	"lw $s3,12($a2)\n"
	"lh $s4,16($a2)\n"
	"lui $t5,0xfffe\n"
	"ori $t5,$t5,0xffff\n"
	"lui $t6,0x01fe\n"
	"ori $t6,$t6,0x00ff\n"
	"lw $s7,20($a2)\n"
	"lh $t8,24($a2)\n"
	"and $s7,$s7,$t5\n"
	"addu $s5,$s3,$s7\n"
	"addu $s6,$s4,$t8\n"
	"bne $s8,$zero,.Lrbr_scroll_ready\n"
	"and $s5,$s5,$t5\n"
	"sw $s5,12($a2)\n"
	"sw $s6,16($a2)\n"
	".Lrbr_scroll_ready:\n"
	"addu $t2,$t2,$s2\n"
	"lh $v1,140($t2)\n"
	"lh $t1,144($t2)\n"
	"lh $t0,148($t2)\n"
	"ctc2 $v1,$5\n"
	"ctc2 $t1,$6\n"
	"ctc2 $t0,$7\n"
	"sll $t1,$t1,16\n"
	"andi $v1,$v1,0xffff\n"
	"or $v1,$v1,$t1\n"
	"bltz $t0,.Lrbr_next_cloud\n"
	"addiu $t1,$t0,-3072\n"
	"bgez $t1,.Lrbr_next_cloud\n"
	"addiu $t1,$t0,-1024\n"
	"bgez $t1,.Lrbr_fade\n"
	"srl $t1,$t1,3\n"
	"addiu $t1,$zero,0\n"
	".Lrbr_fade:\n"
	"andi $t1,$t1,0xff\n"
	"addiu $v1,$zero,255\n"
	"subu $t1,$v1,$t1\n"
	"sb $t1,76($at)\n"
	"sb $t1,77($at)\n"
	"sb $t1,78($at)\n"
	"sb $t1,74($at)\n"
	"sra $v1,$t1,2\n"
	"subu $t1,$t1,$v1\n"
	"sb $t1,72($at)\n"
	"sb $t1,73($at)\n"
	"lb $t2,80($t2)\n"
	"lw $t3,52($at)\n"
	"addiu $t2,$t2,-6\n"
	"sra $t1,$t0,6\n"
	"sra $t1,$t1,1\n"
	"addu $t1,$t1,$t2\n"
	"bgez $t1,.Lrbr_ot_nonnegative\n"
	"sll $t1,$t1,2\n"
	"addiu $t1,$zero,0\n"
	".Lrbr_ot_nonnegative:\n"
	"addiu $t2,$t1,-4096\n"
	"bltz $t2,.Lrbr_ot_ready\n"
	"nop\n"
	"addiu $t1,$zero,4092\n"
	".Lrbr_ot_ready:\n"
	"addu $t3,$t3,$t1\n"
	RBR_SEED("top") RBR_PASS("top", "72")
	"lw $t7,8($a2)\n"
	RBR_DRAW_MODE("0x0a20")
	RBR_SEED("bottom") RBR_PASS("bottom", "76")
	RBR_DRAW_MODE("0x0a40")
	".Lrbr_next_cloud:\n"
	"lw $a2,0($a2)\n"
	"bgez $zero,.Lrbr_cloud\n"
	"nop\n"
	".Lrbr_next_player:\n"
	"addiu $a0,$a0,272\n"
	"addiu $s2,$s2,136\n"
	"bne $a3,$zero,.Lrbr_player\n"
	"addiu $a3,$a3,-1\n"
	".Lrbr_restore:\n"
	"sw $v0,12($a1)\n"
	"lw $ra,44($at)\n"
	"lw $s8,40($at)\n"
	"lw $sp,36($at)\n"
	"lw $gp,32($at)\n"
	"lw $s7,28($at)\n"
	"lw $s6,24($at)\n"
	"lw $s5,20($at)\n"
	"lw $s4,16($at)\n"
	"lw $s3,12($at)\n"
	"lw $s2,8($at)\n"
	"lw $s1,4($at)\n"
	"lw $s0,0($at)\n"
	"jr $ra\n"
	"nop\n"
	".end RedBeaker_RenderRain\n"
	".set\tat\n"
	".set\treorder\n"
	".text\n");

#undef RBR_RNG_SHIFT
#undef RBR_RNG_BYTE
#undef RBR_RNG_COORDS
#undef RBR_SEED
#undef RBR_PASS
#undef RBR_DRAW_MODE
// clang-format on
