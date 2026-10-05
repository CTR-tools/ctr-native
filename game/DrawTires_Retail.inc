// NOTE(aalhendi): Retail uses at as the scratchpad base and sp as the tire
// state cursor, with live GTE/HI/LO values crossing custom-ABI calls. These
// textual macros share the observed repeated instruction sequences without
// changing their load hazards, delay slots, widths or register assignments.
// NOTE(aalhendi): Keep string-concatenating macro fragments left-aligned;
// clang-format interprets their adjacent fragments as nested C calls.
// clang-format off

#define DT_SAVE \
	"lui $at,0x1f80\n" \
	"sw $s0,0($at)\n" \
	"sw $s1,4($at)\n" \
	"sw $s2,8($at)\n" \
	"sw $s3,12($at)\n" \
	"sw $s4,16($at)\n" \
	"sw $s5,20($at)\n" \
	"sw $s6,24($at)\n" \
	"sw $s7,28($at)\n" \
	"sw $gp,32($at)\n" \
	"sw $sp,36($at)\n" \
	"sw $s8,40($at)\n" \
	"sw $ra,44($at)\n"

#define DT_INIT(tag, table) \
	DT_SAVE \
	"sw $a2,48($at)\n" \
	"addiu $v1,$a2,-2\n" \
	"bgtz $v1,.Ldt_" tag "_lod\n" \
	"addiu $v1,$zero,0\n" \
	"addiu $v1,$zero,2\n" \
	".Ldt_" tag "_lod:\n" \
	"sw $v1,84($at)\n" \
	"addiu $v1,$zero,7\n" \
	"lui $t0,%hi(" table ")\n" \
	"addiu $t0,$t0,%lo(" table ")\n" \
	"addiu $t1,$at,304\n" \
	".Ldt_" tag "_copy_jumps:\n" \
	"lw $t2,0($t0)\n" \
	"addiu $t0,$t0,4\n" \
	"sw $t2,0($t1)\n" \
	"addiu $t1,$t1,4\n" \
	"bne $v1,$zero,.Ldt_" tag "_copy_jumps\n" \
	"addiu $v1,$v1,-1\n" \
	"lw $v0,12($a1)\n" \
	"lw $s0,20($a1)\n" \
	".Ldt_" tag "_thread:\n" \
	"lw $a3,52($a0)\n" \
	"lw $sp,48($a0)\n" \
	"lw $a2,48($at)\n" \
	"lh $v1,86($a3)\n" \
	"move $s8,$a3\n" \
	"sw $a2,52($at)\n" \
	"sh $v1,76($at)\n" \
	".Ldt_" tag "_player:\n" \
	"lh $t0,220($s8)\n" \
	"lh $t1,222($s8)\n" \
	"sll $t0,$t0,2\n" \
	"sll $t1,$t1,2\n" \
	"sh $t0,364($at)\n" \
	"sh $t1,366($at)\n" \
	"lw $t0,184($s8)\n" \
	"lw $t1,216($s8)\n" \
	"sw $t0,352($at)\n" \
	"andi $v1,$t0,0x40\n"

#define DT_TIRE_STATE(tag) \
	"lui $t1,0x2e80\n" \
	"lw $t2,0($sp)\n" \
	"bne $v1,$zero,.Ldt_" tag "_color\n" \
	"ori $t1,$t1,0x8080\n" \
	"lw $t1,8($sp)\n" \
	".Ldt_" tag "_color:\n" \
	"sw $t2,56($at)\n" \
	"sw $t1,60($at)\n"

#define DT_LOCAL_WHEELS \
	"mult $v1,$t0\n" \
	"lh $v1,30($a3)\n" \
	"addiu $t0,$zero,64\n" \
	"mflo $t1\n" \
	"sra $t1,$t1,12\n" \
	"sh $t1,88($at)\n" \
	"mult $v1,$t0\n" \
	"sh $t1,120($at)\n" \
	"sub $t2,$zero,$t1\n" \
	"sh $t2,104($at)\n" \
	"sh $t2,136($at)\n" \
	"addiu $t1,$t1,-4096\n" \
	"sh $t1,96($at)\n" \
	"sh $t1,128($at)\n" \
	"addiu $t2,$t2,4096\n" \
	"sh $t2,112($at)\n" \
	"sh $t2,144($at)\n" \
	"lh $v1,32($a3)\n" \
	"addiu $t0,$zero,199\n" \
	"mflo $t1\n" \
	"sra $t1,$t1,12\n" \
	"sh $t1,90($at)\n" \
	"mult $v1,$t0\n" \
	"sh $zero,98($at)\n" \
	"sh $t1,106($at)\n" \
	"sh $zero,114($at)\n" \
	"sh $t1,122($at)\n" \
	"sh $zero,130($at)\n" \
	"sh $t1,138($at)\n" \
	"sh $zero,146($at)\n" \
	"addiu $t0,$zero,-96\n" \
	"mflo $t1\n" \
	"sra $t1,$t1,12\n" \
	"sw $t1,92($at)\n" \
	"mult $v1,$t0\n" \
	"sw $t1,108($at)\n" \
	"mflo $t1\n" \
	"sra $t1,$t1,12\n" \
	"sw $t1,124($at)\n" \
	"sw $t1,132($at)\n" \
	"sw $t1,140($at)\n" \
	"sw $t1,148($at)\n" \
	"lw $v1,228($s8)\n" \
	"lw $t0,232($s8)\n" \
	"lh $t1,4($sp)\n" \
	"sw $v1,64($at)\n" \
	"sw $t0,68($at)\n" \
	"sw $t1,72($at)\n"

#define DT_HAZARD_PAIR(y, z) \
	"lh $v1," y "($at)\n" \
	"lh $t1," z "($at)\n" \
	"srav $t2,$t2,$t4\n" \
	"srav $t0,$t0,$t4\n" \
	"addu $v1,$v1,$t2\n" \
	"addu $t1,$t1,$t0\n" \
	"sh $v1," y "($at)\n" \
	"sh $t1," z "($at)\n"

#define DT_STEER(tag) \
	"lh $t1,6($sp)\n" \
	"jal TRIG_AngleSinCos_r9r8r10\n" \
	"sll $t1,$t1,2\n" \
	"lh $v1,88($at)\n" \
	"lw $t1,92($at)\n" \
	"subu $v1,$v1,$t2\n" \
	"addu $t1,$t1,$t0\n" \
	"sh $v1,96($at)\n" \
	"sw $t1,100($at)\n" \
	"lh $v1,104($at)\n" \
	"lw $t1,108($at)\n" \
	"addu $v1,$v1,$t2\n" \
	"subu $t1,$t1,$t0\n" \
	"lh $t3,14($sp)\n" \
	"sh $v1,112($at)\n" \
	"sw $t1,116($at)\n" \
	"andi $t0,$t3,1\n" \
	"addiu $t4,$zero,9\n" \
	"beq $t0,$zero,.Ldt_" tag "_hazard\n" \
	"sll $t3,$t3,5\n" \
	"addiu $t4,$zero,6\n" \
	"sll $t3,$t3,1\n" \
	".Ldt_" tag "_hazard:\n" \
	"jal TRIG_AngleSinCos_r9r8r10\n" \
	"move $t1,$t3\n" \
	DT_HAZARD_PAIR("98", "100") \
	"jal TRIG_AngleSinCos_r9r8r10\n" \
	"addiu $t1,$t3,1024\n" \
	DT_HAZARD_PAIR("130", "132") \
	"jal TRIG_AngleSinCos_r9r8r10\n" \
	"addiu $t1,$t3,2048\n" \
	DT_HAZARD_PAIR("114", "116") \
	"jal TRIG_AngleSinCos_r9r8r10\n" \
	"addiu $t1,$t3,3072\n" \
	DT_HAZARD_PAIR("146", "148")

#define DT_GTE_SETUP \
	"lw $v1,48($a3)\n" \
	"lw $t0,52($a3)\n" \
	"lw $t1,56($a3)\n" \
	"ctc2 $v1,$16\n" \
	"ctc2 $t0,$17\n" \
	"ctc2 $t1,$18\n" \
	"lw $v1,60($a3)\n" \
	"lw $t0,64($a3)\n" \
	"ctc2 $v1,$19\n" \
	"ctc2 $t0,$20\n" \
	"lw $v1,152($s8)\n" \
	"lw $t0,156($s8)\n" \
	"lw $t1,160($s8)\n" \
	"ctc2 $v1,$8\n" \
	"ctc2 $t0,$9\n" \
	"ctc2 $t1,$10\n" \
	"lw $v1,164($s8)\n" \
	"lw $t0,168($s8)\n" \
	"ctc2 $v1,$11\n" \
	"ctc2 $t0,$12\n" \
	"lw $v1,68($a3)\n" \
	"lw $t0,72($a3)\n" \
	"lw $t1,76($a3)\n" \
	"lh $t2,76($at)\n" \
	"lh $s7,0($a2)\n" \
	"lh $t8,2($a2)\n" \
	"lh $t9,4($a2)\n" \
	"subu $v1,$v1,$s7\n" \
	"subu $t0,$t0,$t8\n" \
	"subu $t1,$t1,$t9\n" \
	"subu $t2,$t2,$t8\n" \
	"sll $v1,$v1,2\n" \
	"sll $t0,$t0,2\n" \
	"sll $t1,$t1,2\n" \
	"sll $t2,$t2,2\n" \
	"ctc2 $v1,$5\n" \
	"ctc2 $t0,$6\n" \
	"ctc2 $t1,$7\n" \
	"sh $t2,80($at)\n" \
	"addiu $s7,$at,48\n" \
	"addiu $t8,$at,24\n"

#define DT_SQRT_MAC \
	"mfc2 $v1,$25\n" \
	"mfc2 $t0,$26\n" \
	"mfc2 $t1,$27\n" \
	"addu $s5,$v1,$t0\n" \
	"jal Unknown_8006ef98\n" \
	"addu $s5,$s5,$t1\n"

#define DT_SCALE_IR(value, reg, shift) \
	"mult " value "," reg "\n" \
	"mflo " value "\n" \
	"sra " value "," value "," shift "\n"

#define DT_AXES(tag, mirrorLoad, mirrorY, mirrorRim) \
	".Ldt_" tag "_axes:\n" \
	"lwc2 $0,88($s7)\n" \
	"lwc2 $1,92($s7)\n" \
	"lwc2 $2,96($s7)\n" \
	"lwc2 $3,100($s7)\n" \
	"cop2 0x4c0012\n" mirrorLoad "mfc2 $t3,$9\n" \
	"mfc2 $t4,$10\n" \
	"mfc2 $t5,$11\n" mirrorY "cop2 0xa00428\n" \
	"sh $t3,88($s7)\n" \
	"sh $t4,90($s7)\n" DT_SQRT_MAC "lui $t2,1\n" \
	"divu $zero,$t2,$s6\n" \
	"cop2 0x4ce012\n" \
	"mflo $s6\n" \
	"sw $t5,92($s7)\n" \
	"sub $s6,$zero,$s6\n" \
	"mult $t3,$s6\n" \
	"mfc2 $v1,$9\n" \
	"mfc2 $t0,$10\n" \
	"mfc2 $t1,$11\n" mirrorRim "mflo $t3\n" \
	"sh $v1,184($t8)\n" \
	"sra $t3,$t3,4\n" \
	"mult $t4,$s6\n" \
	"sh $t3,152($t8)\n" \
	"mflo $t4\n" \
	"sra $t4,$t4,4\n" \
	"sh $t0,186($t8)\n" \
	"mult $t5,$s6\n" \
	"sw $t1,188($t8)\n" \
	"mflo $t5\n" \
	"sra $t5,$t5,4\n" \
	"andi $t3,$t3,0xffff\n" \
	"andi $t4,$t4,0xffff\n" \
	"ctc2 $t3,$0\n" \
	"ctc2 $t4,$2\n" \
	"ctc2 $t5,$4\n" \
	"sh $t4,154($t8)\n" \
	"sw $t5,156($t8)\n" \
	"cop2 0x178000c\n" \
	"mfc2 $t3,$25\n" \
	"mfc2 $t4,$26\n" \
	"mfc2 $t5,$27\n" \
	"cop2 0xa00428\n" DT_SQRT_MAC "div $zero,$t2,$s6\n" \
	"mflo $s6\n" \
	"nop\n" \
	"nop\n" \
	DT_SCALE_IR("$t3", "$s6", "4") "mtc2 $t3,$9\n" \
	DT_SCALE_IR("$t4", "$s6", "4") "mtc2 $t4,$10\n" \
	DT_SCALE_IR("$t5", "$s6", "4") "mtc2 $t5,$11\n" \
	"lw $v1,72($at)\n" \
	"nop\n" \
	"cop2 0x178000c\n" \
	DT_SCALE_IR("$t3", "$v1", "18") "sh $t3,216($t8)\n" \
	DT_SCALE_IR("$t4", "$v1", "18") "sh $t4,218($t8)\n" \
	DT_SCALE_IR("$t5", "$v1", "18") "sh $t5,220($t8)\n" \
	"mfc2 $t3,$25\n" \
	"mfc2 $t4,$26\n" \
	"mfc2 $t5,$27\n" \
	"cop2 0xa00428\n" DT_SQRT_MAC "div $zero,$t2,$s6\n" \
	"mflo $s6\n" \
	"lw $t2,72($at)\n" \
	"sub $s6,$zero,$s6\n" \
	"mult $t2,$s6\n" \
	"mflo $s6\n" \
	"sra $s6,$s6,12\n" \
	"nop\n" \
	DT_SCALE_IR("$t3", "$s6", "10") "sh $t3,248($t8)\n" \
	DT_SCALE_IR("$t4", "$s6", "10") "sh $t4,250($t8)\n" \
	DT_SCALE_IR("$t5", "$s6", "10") "sh $t5,252($t8)\n" \
	"addiu $s7,$s7,-16\n" \
	"subu $v1,$s7,$at\n" \
	"bgez $v1,.Ldt_" tag "_axes\n" \
	"addiu $t8,$t8,-8\n"

#define DT_PROJECTION_SETUP \
	"lw $v1,40($a2)\n" \
	"lw $t0,44($a2)\n" \
	"lw $t1,48($a2)\n" \
	"ctc2 $v1,$0\n" \
	"ctc2 $t0,$1\n" \
	"ctc2 $t1,$2\n" \
	"lw $v1,52($a2)\n" \
	"lw $t0,56($a2)\n" \
	"ctc2 $v1,$3\n" \
	"ctc2 $t0,$4\n" \
	"ctc2 $zero,$5\n" \
	"ctc2 $zero,$6\n" \
	"ctc2 $zero,$7\n" \
	"lh $v1,32($a2)\n" \
	"lh $t0,34($a2)\n" \
	"lw $t1,24($a2)\n" \
	"sll $v1,$v1,15\n" \
	"sll $t0,$t0,15\n" \
	"ctc2 $v1,$24\n" \
	"ctc2 $t0,$25\n" \
	"ctc2 $t1,$26\n" \
	"addiu $s7,$at,48\n" \
	"addiu $t8,$at,24\n" \
	"addiu $t9,$at,12\n"

#define DT_CORNER_XY(opA, opB, reg, pipe, stores) \
	"lh $v1,88($s7)\n" \
	"lh $t0,216($t8)\n" pipe "lh $t1,248($t8)\n" opA " $s1,$v1,$t0\n" opB " $s1,$s1,$t1\n" \
	"andi $s1,$s1,0xffff\n" \
	"lh $v1,90($s7)\n" \
	"lh $t0,218($t8)\n" \
	"lh $t1,250($t8)\n" opA " $s2,$v1,$t0\n" opB " $s2,$s2,$t1\n" \
	"sll $s2,$s2,16\n" \
	"or $s1,$s1,$s2\n" stores "mtc2 $s1,$" reg "\n"

#define DT_CORNER_Z(opA, opB, reg) \
	"lh $v1,92($s7)\n" \
	"lh $t0,220($t8)\n" \
	"lh $t1,252($t8)\n" opA " $s2,$v1,$t0\n" opB " $s2,$s2,$t1\n" \
	"mtc2 $s2,$" reg "\n"

#define DT_PROJECT_CORNERS(tag, screenStores, finalStore) \
	".Ldt_" tag "_project:\n" \
	DT_CORNER_XY("subu", "subu", "0", "", "") DT_CORNER_Z("subu", "subu", "1") \
	DT_CORNER_XY("addu", "subu", "2", "", "") DT_CORNER_Z("addu", "subu", "3") \
	DT_CORNER_XY("subu", "addu", "4", "", "") DT_CORNER_Z("subu", "addu", "5") \
	DT_CORNER_XY("addu", "addu", "0", "cop2 0x280030\n", screenStores) DT_CORNER_Z("addu", "addu", "1") \
	"lw $t0,60($at)\n" \
	"lw $v1,184($t8)\n" \
	"cop2 0x180001\n" \
	"sw $t0,4($v0)\n" \
	"lw $t0,188($t8)\n" finalStore

#define DT_NORMAL_TO_GTE \
	"lwc2 $0,152($t8)\n" \
	"lwc2 $1,156($t8)\n" \
	"ctc2 $v1,$8\n" \
	"ctc2 $t0,$9\n" \
	"lh $v1,364($at)\n" \
	"lh $t0,366($at)\n"

#define DT_DEPTH_TO_SLOT \
	"addu $v1,$v1,$t6\n" \
	"addu $t0,$t0,$t6\n" \
	"sw $v1,368($at)\n" \
	"sw $t0,372($at)\n" \
	"srl $t7,$t7,17\n" \
	"sll $t7,$t7,2\n" \
	"addu $t7,$t7,$t6\n"

#define DT_SPRITE(tag, cullInputs) \
	"mfc2 $v1,$9\n" \
	"lui $t0,%hi(sDrawTiresSpriteIndexTable)\n" \
	"addiu $t0,$t0,%lo(sDrawTiresSpriteIndexTable)\n" \
	"bgez $v1,.Ldt_" tag "_sprite_abs\n" \
	"sra $v1,$v1,5\n" \
	"sub $v1,$zero,$v1\n" \
	".Ldt_" tag "_sprite_abs:\n" \
	"addiu $t1,$v1,-128\n" \
	"blez $t1,.Ldt_" tag "_sprite_bound\n" \
	"addu $v1,$v1,$t0\n" \
	"addiu $v1,$t0,128\n" \
	".Ldt_" tag "_sprite_bound:\n" \
	"lb $v1,0($v1)\n" \
	"lw $t1,56($at)\n" \
	"sll $v1,$v1,2\n" \
	"addu $v1,$v1,$t1\n" \
	"lw $v1,0($v1)\n" \
	"nop\n" \
	"beq $v1,$zero,.Ldt_" tag "_next_wheel\n" \
	"nop\n" \
	"lw $t0,20($v1)\n" \
	"lw $t1,24($v1)\n" \
	"lw $t2,28($v1)\n" \
	"sw $t0,12($v0)\n" \
	"mfc2 $t0,$9\n" \
	"sw $t1,20($v0)\n" \
	"sw $t2,28($v0)\n" cullInputs "srl $t0,$t0,26\n" \
	"andi $t0,$t0,16\n" \
	"or $t0,$t0,$t9\n" \
	"lw $t0,304($t0)\n" \
	"srl $t2,$t2,16\n" \
	"jr $t0\n" \
	"sw $t2,36($v0)\n"

#define DT_ORDER_LOAD(a, b, c, d) \
	"lw $v1," a "($at)\n" \
	"lw $t0," b "($at)\n" \
	"lw $t1," c "($at)\n" \
	"lw $t2," d "($at)\n"

#define DT_CORNER_ORDER(tag) \
	".Ldt_" tag "_corner0:\n" DT_ORDER_LOAD("292", "284", "288", "280") \
	"lw $s3,296($at)\n" \
	"bgez $zero,.Ldt_" tag "_emit\n" \
	"subu $t7,$t7,$s3\n" \
	".Ldt_" tag "_corner1:\n" DT_ORDER_LOAD("288", "280", "292", "284") \
	"lw $s3,300($at)\n" \
	"bgez $zero,.Ldt_" tag "_emit\n" \
	"subu $t7,$t7,$s3\n" \
	".Ldt_" tag "_corner2:\n" DT_ORDER_LOAD("292", "284", "288", "280") \
	"bgez $zero,.Ldt_" tag "_emit\n" \
	"sw $zero,296($at)\n" \
	".Ldt_" tag "_corner3:\n" DT_ORDER_LOAD("288", "280", "292", "284") \
	"bgez $zero,.Ldt_" tag "_emit\n" \
	"sw $zero,300($at)\n" \
	".Ldt_" tag "_corner4:\n" DT_ORDER_LOAD("284", "292", "280", "288") \
	"lw $s3,296($at)\n" \
	"bgez $zero,.Ldt_" tag "_emit\n" \
	"subu $t7,$t7,$s3\n" \
	".Ldt_" tag "_corner5:\n" DT_ORDER_LOAD("280", "288", "284", "292") \
	"lw $s3,300($at)\n" \
	"bgez $zero,.Ldt_" tag "_emit\n" \
	"subu $t7,$t7,$s3\n" \
	".Ldt_" tag "_corner6:\n" DT_ORDER_LOAD("284", "292", "280", "288") \
	"addiu $s3,$zero,8\n" \
	"subu $t7,$t7,$s3\n" \
	"bgez $zero,.Ldt_" tag "_emit\n" \
	"sw $s3,296($at)\n" \
	".Ldt_" tag "_corner7:\n" DT_ORDER_LOAD("280", "288", "284", "292") \
	"addiu $s3,$zero,8\n" \
	"subu $t7,$t7,$s3\n" \
	"sw $s3,300($at)\n" \
	".Ldt_" tag "_emit:\n"

#define DT_LINK(tag) \
	"sw $t0,16($v0)\n" \
	"sw $t1,24($v0)\n" \
	"sw $t2,32($v0)\n" \
	"lw $v1,368($at)\n" \
	"lw $t0,372($at)\n" \
	"subu $t1,$v1,$t7\n" \
	"blez $t1,.Ldt_" tag "_clamp_end\n" \
	"subu $t1,$t0,$t7\n" \
	"move $t7,$v1\n" \
	".Ldt_" tag "_clamp_end:\n" \
	"bgez $t1,.Ldt_" tag "_link\n" \
	"lui $t2,0x900\n" \
	"move $t7,$t0\n" \
	".Ldt_" tag "_link:\n" \
	"lw $v1,0($t7)\n" \
	"sll $t0,$v0,8\n" \
	"srl $t0,$t0,8\n" \
	"or $v1,$v1,$t2\n" \
	"sw $v1,0($v0)\n" \
	"addiu $v0,$v0,40\n" \
	"sw $t0,0($t7)\n" \
	"addi $s0,$s0,1\n"

#define DT_NEXT(tag) \
	".Ldt_" tag "_next_wheel:\n" \
	"addiu $s7,$s7,-16\n" \
	"addiu $t8,$t8,-8\n" \
	"subu $v1,$s7,$at\n" \
	"bgez $v1,.Ldt_" tag "_project\n" \
	"addiu $t9,$t9,-4\n" \
	".Ldt_" tag "_next_player:\n" \
	"lw $a2,52($at)\n" \
	"addiu $s8,$s8,136\n" \
	"addiu $a2,$a2,-1\n" \
	"bgtz $a2,.Ldt_" tag "_player\n" \
	"sw $a2,52($at)\n" \
	"lw $a0,16($a0)\n" \
	"nop\n" \
	"bne $a0,$zero,.Ldt_" tag "_thread\n" \
	"nop\n" \
	"sw $s0,20($a1)\n" \
	"sw $v0,12($a1)\n" \
	"lw $ra,44($at)\n" \
	"lw $s8,40($at)\n" \
	"lw $sp,36($at)\n" \
	"lw $gp,32($at)\n" \
	"lw $s7,28($at)\n" \
	"lw $s6,24($at)\n" \
	"lw $s5,20($at)\n" \
	"lw $s4,16($at)\n" \
	"lw $s3,12($at)\n" \
	"lw $s2,8($at)\n" \
	"lw $s1,4($at)\n" \
	"lw $s0,0($at)\n" \
	"jr $ra\n" \
	"nop\n"

// NOTE(aalhendi): These data words relocate to the real renderer labels.
// The following shared C sprite table completes [0x8008a344,0x8008a408).
__asm__(".rdata\n"
	".align 2\n"
	"DrawTiresSolidJumpTable:\n"
	".word .Ldt_s_corner0,.Ldt_s_corner1,.Ldt_s_corner2,.Ldt_s_corner3\n"
	".word .Ldt_s_corner4,.Ldt_s_corner5,.Ldt_s_corner6,.Ldt_s_corner7\n"
	"DrawTiresReflectionJumpTable:\n"
	".word .Ldt_r_corner0,.Ldt_r_corner1,.Ldt_r_corner2,.Ldt_r_corner3\n"
	".word .Ldt_r_corner4,.Ldt_r_corner5,.Ldt_r_corner6,.Ldt_r_corner7\n"
	".text\n");

__asm__(".section .DrawTires_Solid,\"ax\",@progbits\n"
	".align 2\n"
	".ent\tDrawTires_Solid\n"
	".set\tnoreorder\n"
	".set\tnoat\n"
	".globl DrawTires_Solid\n"
	"DrawTires_Solid:\n"
	DT_INIT("s", "DrawTiresSolidJumpTable")
	"beq $v1,$zero,.Ldt_s_next_player\n"
	"andi $v1,$t0,0x80\n"
	"lw $t2,84($at)\n"
	"bne $v1,$zero,.Ldt_s_next_player\n"
	"subu $t1,$t1,$t2\n"
	"bgtz $t1,.Ldt_s_next_player\n"
	"andi $v1,$t0,0x100\n"
	DT_TIRE_STATE("s")
	"lw $a2,116($s8)\n"
	"lh $v1,28($a3)\n"
	"beq $a2,$zero,.Ldt_s_next_player\n"
	"addiu $t0,$zero,144\n"
	DT_LOCAL_WHEELS DT_STEER("s") DT_GTE_SETUP
	DT_AXES("s", "", "", "") DT_PROJECTION_SETUP
	DT_PROJECT_CORNERS("s", "swc2 $12,280($at)\n" "swc2 $13,284($at)\n" "swc2 $14,288($at)\n", "")
	"lh $t1,90($s7)\n"
	"lh $t2,80($at)\n"
	"swc2 $14,292($at)\n"
	"cop2 0x168002e\n"
	"subu $t1,$t2,$t1\n"
	"bltz $t1,.Ldt_s_ot\n"
	"lw $t6,64($at)\n"
	"lw $t6,68($at)\n"
	".Ldt_s_ot:\n" DT_NORMAL_TO_GTE
	"cop2 0x4a6012\n"
	"mfc2 $t7,$24\n" DT_DEPTH_TO_SLOT
	DT_SPRITE("s", "lw $t3,68($at)\n" "lw $t4,352($at)\n") DT_CORNER_ORDER("s")
	"bne $t3,$t6,.Ldt_s_write\n"
	"andi $t4,$t4,0x4000\n"
	"bne $t4,$zero,.Ldt_s_next_player\n"
	".Ldt_s_write:\n"
	"sw $v1,8($v0)\n"
	DT_LINK("s") DT_NEXT("s")
	".end DrawTires_Solid\n"
	".set\tat\n"
	".set\treorder\n"
	".text\n");

__asm__(".section .DrawTires_Reflection,\"ax\",@progbits\n"
	".align 2\n"
	".ent\tDrawTires_Reflection\n"
	".set\tnoreorder\n"
	".set\tnoat\n"
	".globl DrawTires_Reflection\n"
	"DrawTires_Reflection:\n"
	DT_INIT("r", "DrawTiresReflectionJumpTable")
	"beq $v1,$zero,.Ldt_r_next_player\n"
	"andi $v1,$t0,0x4000\n"
	"lw $t2,84($at)\n"
	"beq $v1,$zero,.Ldt_r_next_player\n"
	"subu $t1,$t1,$t2\n"
	"bgtz $t1,.Ldt_r_next_player\n"
	"lw $a2,116($s8)\n"
	"lh $v1,28($a3)\n"
	"beq $a2,$zero,.Ldt_r_next_player\n"
	"andi $v1,$t0,0x100\n" DT_TIRE_STATE("r")
	"addiu $t0,$zero,144\n" DT_LOCAL_WHEELS DT_STEER("r") DT_GTE_SETUP
	DT_AXES("r", "lh $t2,80($at)\n", "subu $t4,$t4,$t2\n" "subu $t4,$t2,$t4\n", "sub $t0,$zero,$t0\n")
	DT_PROJECTION_SETUP
	DT_PROJECT_CORNERS("r", "swc2 $12,284($at)\n" "swc2 $13,280($at)\n" "swc2 $14,292($at)\n", "swc2 $14,288($at)\n")
	"cop2 0x168002e\n"
	"lw $t2,352($at)\n"
	"lw $t6,68($at)\n"
	"mfc2 $t7,$24\n" DT_NORMAL_TO_GTE
	"cop2 0x4a6012\n" DT_DEPTH_TO_SLOT
	DT_SPRITE("r", "lh $t3,90($s7)\n" "lh $t4,80($at)\n") DT_CORNER_ORDER("r")
	"subu $t3,$t4,$t3\n"
	"bltz $t3,.Ldt_r_next_wheel\n"
	"sw $v1,8($v0)\n"
	DT_LINK("r") DT_NEXT("r")
	".end DrawTires_Reflection\n"
	".set\tat\n"
	".set\treorder\n"
	".text\n");

#undef DT_SAVE
#undef DT_INIT
#undef DT_TIRE_STATE
#undef DT_LOCAL_WHEELS
#undef DT_HAZARD_PAIR
#undef DT_STEER
#undef DT_GTE_SETUP
#undef DT_SQRT_MAC
#undef DT_SCALE_IR
#undef DT_AXES
#undef DT_PROJECTION_SETUP
#undef DT_CORNER_XY
#undef DT_CORNER_Z
#undef DT_PROJECT_CORNERS
#undef DT_NORMAL_TO_GTE
#undef DT_DEPTH_TO_SLOT
#undef DT_SPRITE
#undef DT_ORDER_LOAD
#undef DT_CORNER_ORDER
#undef DT_LINK
#undef DT_NEXT
// clang-format on
