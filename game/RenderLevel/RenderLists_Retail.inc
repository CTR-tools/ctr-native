#define RL_CORNER_TABLE(ENTRY) \
	ENTRY(max_max_max, 0x3c)   \
	ENTRY(min_max_max, 0x4c)   \
	ENTRY(max_min_max, 0x5c)   \
	ENTRY(min_min_max, 0x6c)   \
	ENTRY(max_max_min, 0x78)   \
	ENTRY(min_max_min, 0x88)   \
	ENTRY(max_min_min, 0x98)   \
	ENTRY(min_min_min, 0xa8)

#ifdef CTR_NATIVE

// NOTE(aalhendi): Native scratch retains these observed retail entry words;
// host execution selects the same corners in portable C, never through them.
#define RL_NATIVE_CORNER(name, offset) (0x800702d4u + offset),
static const u32 RenderListsCornerJumpTable[] = {RL_CORNER_TABLE(RL_NATIVE_CORNER)};
#undef RL_NATIVE_CORNER

#else

// NOTE(aalhendi): The public entries use the ordinary argument ABI, but the
// traversal saves registers in scratch and its corner/plane helpers pass
// packed XY in gp, Z in s8, and continuations in t9. These shared instruction
// groups preserve the load/branch delay slots and observable scratch writes.
// clang-format off

#define RL_SAVE \
	"lui $at,0x1f80\n" \
	"sw $s0,48($at)\n" \
	"sw $s1,52($at)\n" \
	"sw $s2,56($at)\n" \
	"sw $s3,60($at)\n" \
	"sw $s4,64($at)\n" \
	"sw $s5,68($at)\n" \
	"sw $s6,72($at)\n" \
	"sw $s7,76($at)\n" \
	"sw $gp,80($at)\n" \
	"sw $sp,84($at)\n" \
	"sw $s8,88($at)\n" \
	"sw $ra,92($at)\n"

#define RL_RESTORE \
	"lw $v0,128($at)\n" \
	"lw $ra,92($at)\n" \
	"lw $s8,88($at)\n" \
	"lw $sp,84($at)\n" \
	"lw $gp,80($at)\n" \
	"lw $s7,76($at)\n" \
	"lw $s6,72($at)\n" \
	"lw $s5,68($at)\n" \
	"lw $s4,64($at)\n" \
	"lw $s3,60($at)\n" \
	"lw $s2,56($at)\n" \
	"lw $s1,52($at)\n" \
	"lw $s0,48($at)\n" \
	"jr $ra\n" \
	"nop\n"

#define RL_CAMERA \
	"lw $t3,40($a2)\n" \
	"lw $t4,44($a2)\n" \
	"lw $t5,48($a2)\n" \
	"lw $t6,52($a2)\n" \
	"lw $t7,56($a2)\n" \
	"lw $s0,60($a2)\n" \
	"lw $s1,64($a2)\n" \
	"lw $s2,68($a2)\n" \
	"ctc2 $t3,$0\n" \
	"ctc2 $t4,$1\n" \
	"ctc2 $t5,$2\n" \
	"ctc2 $t6,$3\n" \
	"ctc2 $t7,$4\n" \
	"ctc2 $s0,$5\n" \
	"ctc2 $s1,$6\n" \
	"ctc2 $s2,$7\n" \
	"lh $v1,32($a2)\n" \
	"lh $t0,34($a2)\n" \
	"lw $t1,24($a2)\n" \
	"sll $v1,$v1,15\n" \
	"sll $t0,$t0,15\n" \
	"ctc2 $v1,$24\n" \
	"ctc2 $t0,$25\n" \
	"ctc2 $t1,$26\n" \
	"addiu $a2,$a2,168\n"

#define RL_PLANES \
	"lh $t3,64($a2)\n" \
	"lh $t4,66($a2)\n" \
	"lh $t5,68($a2)\n" \
	"lh $t6,70($a2)\n" \
	"lh $t7,72($a2)\n" \
	"lh $s0,74($a2)\n" \
	"lw $v1,0($a2)\n" \
	"lw $t0,4($a2)\n" \
	"sw $v1,96($at)\n" \
	"sw $t0,100($at)\n" \
	"lw $v1,8($a2)\n" \
	"lw $t0,12($a2)\n" \
	"sw $v1,104($at)\n" \
	"sw $t0,108($at)\n" \
	"lw $v1,16($a2)\n" \
	"lw $t0,20($a2)\n" \
	"sw $v1,112($at)\n" \
	"sw $t0,116($at)\n" \
	"lw $v1,24($a2)\n" \
	"lw $t0,28($a2)\n" \
	"sw $v1,120($at)\n" \
	"sw $t0,124($at)\n" \
	"addu $s2,$a0,$zero\n"

#define RL_FIRST_THREE_CORNERS \
	"lw $v1,40($a2)\n" \
	"lw $t0,44($a2)\n" \
	"lw $t1,48($a2)\n" \
	"sll $v1,$v1,2\n" \
	"sll $t0,$t0,2\n" \
	"sll $t1,$t1,2\n" \
	"addu $v1,$v1,$at\n" \
	"addu $t0,$t0,$at\n" \
	"addu $t1,$t1,$at\n" \
	"lw $v1,132($v1)\n" \
	"lw $t0,132($t0)\n" \
	"lw $t1,132($t1)\n" \
	"sw $v1,172($at)\n"

#define RL_POP_AND_FRUSTUM(tag) \
	".Lrl_" tag "_pop:\n" \
	"lh $s3,192($v0)\n" \
	"addiu $v0,$v0,-16\n" \
	"andi $v1,$s3,0x4000\n" \
	"andi $s3,$s3,0x3fff\n" \
	"sll $s2,$s3,5\n" \
	"beq $v1,$zero,.Lrl_" tag "_children\n" \
	"addu $s2,$s2,$a0\n" \
	"lh $s1,0($s2)\n" \
	"lw $s7,176($at)\n" \
	"jal RenderLists_Init1P2P+(.Lrl_plane_trampoline-RenderLists_Init1P2P)\n" \
	"addiu $t8,$zero,8\n" \
	"bgtz $t9,.Lrl_" tag "_next\n" \
	"lw $s7,184($at)\n" \
	"jal RenderLists_Init1P2P+(.Lrl_plane_trampoline-RenderLists_Init1P2P)\n" \
	"addiu $t8,$zero,24\n" \
	"bgtz $t9,.Lrl_" tag "_next\n" \
	"lw $s7,172($at)\n" \
	"jal RenderLists_Init1P2P+(.Lrl_plane_trampoline-RenderLists_Init1P2P)\n" \
	"addiu $t8,$zero,0\n" \
	"bgtz $t9,.Lrl_" tag "_next\n" \
	"lw $s7,180($at)\n" \
	"jal RenderLists_Init1P2P+(.Lrl_plane_trampoline-RenderLists_Init1P2P)\n" \
	"addiu $t8,$zero,16\n" \
	"bgtz $t9,.Lrl_" tag "_next\n" \
	"andi $v1,$s1,2\n"

// The two bounding-box stores in branch delay slots also occur on rejection.
#define RL_VISIBLE_CHILD(child, reject) \
	"andi $v1,$v1,0x7fc\n" \
	"addu $v1,$v1,$a1\n" \
	"lw $s4,0($v1)\n" \
	"andi $s3," child ",0x1f\n" \
	"sllv $s4,$s4,$s3\n" \
	"bgez $s4," reject "\n" \
	"andi $s2," child ",0x3fff\n" \
	"sll $s2,$s2,5\n" \
	"addu $s2,$s2,$a0\n" \
	"lw $v1,4($s2)\n" \
	"lw $t0,8($s2)\n" \
	"sra $t1,$v1,16\n" \
	"subu $t1,$t1,$t7\n" \
	"bgtz $t1," reject "\n" \
	"sll $t1,$v1,16\n" \
	"sra $t1,$t1,16\n" \
	"subu $t1,$t1,$t6\n" \
	"bgtz $t1," reject "\n" \
	"sw $v1,212($v0)\n" \
	"lw $v1,12($s2)\n" \
	"sra $t1,$t0,16\n" \
	"subu $t1,$t3,$t1\n" \
	"bgtz $t1," reject "\n" \
	"sll $t1,$t0,16\n" \
	"sra $t1,$t1,16\n" \
	"subu $t1,$t1,$s0\n" \
	"bgtz $t1," reject "\n" \
	"sra $t1,$v1,16\n" \
	"subu $t1,$t5,$t1\n" \
	"bgtz $t1," reject "\n" \
	"sll $t1,$v1,16\n" \
	"sra $t1,$t1,16\n" \
	"subu $t1,$t4,$t1\n" \
	"bgtz $t1," reject "\n" \
	"sw $t0,216($v0)\n" \
	"sw $v1,220($v0)\n" \
	"sh " child ",208($v0)\n" \
	"addiu $v0,$v0,16\n"

#define RL_CHILDREN_AND_RETURN(tag) \
	".Lrl_" tag "_children:\n" \
	"lw $s5,24($s2)\n" \
	"nop\n" \
	"andi $s3,$s5,0x8000\n" \
	"bne $s3,$zero,.Lrl_" tag "_second\n" \
	"sra $s6,$s5,16\n" \
	"srl $v1,$s5,3\n" \
	RL_VISIBLE_CHILD("$s5", ".Lrl_" tag "_second") \
	".Lrl_" tag "_second:\n" \
	"bltz $s6,.Lrl_" tag "_next\n" \
	"srl $v1,$s6,3\n" \
	RL_VISIBLE_CHILD("$s6", ".Lrl_" tag "_next") \
	".Lrl_" tag "_next:\n" \
	"bne $v0,$at,.Lrl_" tag "_pop\n" \
	"nop\n" \
	RL_RESTORE

#define RL_LINK(tag) \
	".Lrl_" tag "_link:\n" \
	"sll $s3,$s3,3\n" \
	"addu $s3,$s3,$t2\n" \
	"lw $s7,0($t8)\n" \
	"sw $s3,0($t8)\n" \
	"lw $v1,128($at)\n" \
	"sw $s7,0($s3)\n" \
	"addiu $v1,$v1,1\n" \
	"bgez $zero,.Lrl_" tag "_next\n" \
	"sw $v1,128($at)\n"

#define RL_CORNER(name, xy, z, delay) \
	".Lrl_corner_" name ":\n" \
	xy \
	"lh $s8," z "($v0)\n" \
	"jr $t9\n" \
	delay

#define RL_CORNER_MIN_MIN(name, z) \
	".Lrl_corner_" name ":\n" \
	"lw $gp,212($v0)\n" \
	"jr $t9\n" \
	"lh $s8," z "($v0)\n"

// Label deltas keep single-entry probes anchored to the actual owner. The
// complete linked block resolves every pointer against its real helper code.
#define RL_RETAIL_CORNER(name, offset) \
	".word RenderLists_PreInit+(.Lrl_corner_" #name "-RenderLists_PreInit)\n"

__asm__(".section .RenderLists_Init1P2P,\"ax\",@progbits\n"
	".align 2\n"
	".ent\tRenderLists_Init1P2P\n"
	".set\tnoreorder\n"
	".set\tnoat\n"
	".globl RenderLists_Init1P2P\n"
	"RenderLists_Init1P2P:\n"
	RL_SAVE
	"sw $zero,128($at)\n"
	"addu $v0,$at,$zero\n"
	"lw $t0,20($sp)\n"
	"lw $t2,16($sp)\n"
	"addiu $v1,$t0,-1\n"
	"beq $v1,$zero,.Lrl_lod_ready\n"
	"lw $t1,24($at)\n"
	"addiu $t1,$zero,0x1540\n"
	".Lrl_lod_ready:\n"
	"nop\n"
	"sw $t1,164($at)\n"
	RL_CAMERA RL_PLANES RL_FIRST_THREE_CORNERS
	"sw $t0,176($at)\n"
	"sw $t1,180($at)\n"
	"lw $v1,52($a2)\n"
	"lw $t0,60($a2)\n"
	"sll $v1,$v1,2\n"
	"sll $t0,$t0,2\n"
	"addu $v1,$v1,$at\n"
	"addu $t0,$t0,$at\n"
	"lw $v1,132($v1)\n"
	"lw $t0,132($t0)\n"
	"sw $v1,184($at)\n"
	"bgez $zero,.Lrl_1p_children\n"
	"sw $t0,192($at)\n"
	RL_POP_AND_FRUSTUM("1p")
	"bne $v1,$zero,.Lrl_1p_link\n"
	"addi $t8,$a3,36\n"
	"andi $v1,$s1,0x20\n"
	"bne $v1,$zero,.Lrl_1p_link\n"
	"addi $t8,$a3,12\n"
	"lw $s7,192($at)\n"
	"jal RenderLists_PreInit+(.Lrl_corner_trampoline-RenderLists_PreInit)\n"
	"nop\n"
	"lw $v1,164($at)\n"
	"mtc2 $gp,$0\n"
	"mtc2 $s8,$1\n"
	"nop\n"
	"nop\n"
	"cop2 0x180001\n"
	"mfc2 $t0,$19\n"
	"nop\n"
	"subu $t1,$t0,$v1\n"
	"bgtz $t1,.Lrl_1p_link\n"
	"addi $t8,$a3,40\n"
	"andi $v1,$s1,0x80\n"
	"bne $v1,$zero,.Lrl_1p_link\n"
	"addi $t8,$a3,4\n"
	"andi $v1,$s1,8\n"
	"bne $v1,$zero,.Lrl_1p_link\n"
	"addi $t8,$a3,28\n"
	"andi $v1,$s1,0x10\n"
	"bne $v1,$zero,.Lrl_1p_link\n"
	"addi $t8,$a3,20\n"
	"addi $t8,$a3,12\n"
	RL_LINK("1p") RL_CHILDREN_AND_RETURN("1p")
	".Lrl_plane_trampoline:\n"
	"lui $t9,%hi(.Lrl_plane_test)\n"
	"jr $s7\n"
	"ori $t9,$t9,%lo(.Lrl_plane_test)\n"
	".Lrl_plane_test:\n"
	"addu $t8,$t8,$at\n"
	"lw $v1,96($t8)\n"
	"lw $t0,100($t8)\n"
	"mtc2 $gp,$0\n"
	"mtc2 $s8,$1\n"
	"ctc2 $v1,$8\n"
	"ctc2 $t0,$9\n"
	"sra $t0,$t0,16\n"
	"sll $t0,$t0,1\n"
	"sub $t0,$zero,$t0\n"
	"ctc2 $t0,$13\n"
	"nop\n"
	"nop\n"
	"cop2 0x4a2012\n"
	"mfc2 $t9,$9\n"
	"jr $ra\n"
	"nop\n"
	".end\tRenderLists_Init1P2P\n"
	".section .RenderLists_PreInit,\"ax\",@progbits\n"
	".align 2\n"
	".ent\tRenderLists_PreInit\n"
	".globl RenderLists_PreInit\n"
	".Lrl_corner_trampoline:\n"
	"jr $s7\n"
	"addu $t9,$ra,$zero\n"
	RL_CORNER("max_max_max", "lwr $gp,218($v0)\n", "222", "lwl $gp,221($v0)\n")
	RL_CORNER("min_max_max", "lhu $gp,212($v0)\n", "222", "lwl $gp,221($v0)\n")
	RL_CORNER("max_min_max", "lw $gp,212($v0)\n", "222", "lwr $gp,218($v0)\n")
	RL_CORNER_MIN_MIN("min_min_max", "222")
	RL_CORNER("max_max_min", "lwr $gp,218($v0)\n", "216", "lwl $gp,221($v0)\n")
	RL_CORNER("min_max_min", "lhu $gp,212($v0)\n", "216", "lwl $gp,221($v0)\n")
	RL_CORNER("max_min_min", "lw $gp,212($v0)\n", "216", "lwr $gp,218($v0)\n")
	RL_CORNER_MIN_MIN("min_min_min", "216")
	// NOTE(aalhendi): The retail symbol bundles the public copy and private
	// helper tail. Preserve their compound extent, not attribution to C.
	".size RenderLists_PreInit,.-RenderLists_PreInit\n"
	".end\tRenderLists_PreInit\n"
	".section .RenderLists_Init3P4P,\"ax\",@progbits\n"
	".align 2\n"
	".ent\tRenderLists_Init3P4P\n"
	".globl RenderLists_Init3P4P\n"
	"RenderLists_Init3P4P:\n"
	RL_SAVE
	"lui $at,0x1f80\n"
	"sw $zero,128($at)\n"
	"addu $v0,$at,$zero\n"
	"lw $t2,16($sp)\n"
	RL_CAMERA RL_PLANES RL_FIRST_THREE_CORNERS
	"lw $v1,52($a2)\n"
	"sw $t0,176($at)\n"
	"sw $t1,180($at)\n"
	"sll $v1,$v1,2\n"
	"addu $v1,$v1,$at\n"
	"lw $v1,132($v1)\n"
	"bgez $zero,.Lrl_3p_children\n"
	"sw $v1,184($at)\n"
	RL_POP_AND_FRUSTUM("3p")
	"bne $v1,$zero,.Lrl_3p_link\n"
	"addi $t8,$a3,20\n"
	"andi $v1,$s1,0x20\n"
	"bne $v1,$zero,.Lrl_3p_link\n"
	"addi $t8,$a3,28\n"
	"andi $v1,$s1,0x80\n"
	"bne $v1,$zero,.Lrl_3p_link\n"
	"addi $t8,$a3,4\n"
	"addi $t8,$a3,12\n"
	RL_LINK("3p") RL_CHILDREN_AND_RETURN("3p")
	".end\tRenderLists_Init3P4P\n"
	".section .rdata,\"a\",@progbits\n"
	".align 2\n"
	".globl RenderListsCornerJumpTable\n"
	".type RenderListsCornerJumpTable,@object\n"
	"RenderListsCornerJumpTable:\n"
	RL_CORNER_TABLE(RL_RETAIL_CORNER)
	".size RenderListsCornerJumpTable,.-RenderListsCornerJumpTable\n"
	".text\n"
	".set\tat\n"
	".set\treorder\n");

#undef RL_SAVE
#undef RL_RESTORE
#undef RL_CAMERA
#undef RL_PLANES
#undef RL_FIRST_THREE_CORNERS
#undef RL_POP_AND_FRUSTUM
#undef RL_VISIBLE_CHILD
#undef RL_CHILDREN_AND_RETURN
#undef RL_LINK
#undef RL_CORNER
#undef RL_CORNER_MIN_MIN
#undef RL_RETAIL_CORNER

// clang-format on

#endif

#undef RL_CORNER_TABLE
