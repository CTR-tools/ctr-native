// NOTE(aalhendi): Game-local instruction text for the four renderer decoders.
// Each leaf includes this only in its default branch and undefines all five
// macros afterward. prefix is a unique local-label string for each real FUNC.
// No include guard: separate leaves deliberately reopen this temporary scope.
// clang-format off
#define CTR_RB_DECODER_SINGLE_FRAME(prefix) \
        "srl\t$a1,$t3,0xd\n" \
        "andi\t$a1,$a1,0x7f8\n" \
        "sll\t$v1,$t3,0x5\n" \
        "bgez\t$v1," prefix "uncached\n" \
        "addu\t$a1,$a1,$at\n" \
        "lw\t$t0,320($a1)\n" \
        "bgez\t$zero," prefix "select_color\n" \
        "lw\t$v1,324($a1)\n" \
        prefix "uncached:\n" \
        "bnez\t$s8," prefix "delta\n" \
        "andi\t$t0,$s7,0x2\n" \
        "addiu\t$s7,$s7,3\n" \
        "beqz\t$t0," prefix "pack_coordinates\n" \
        "lwr\t$v1,-3($s7)\n" \
        "bgez\t$zero," prefix "pack_coordinates\n" \
        "lwl\t$v1,0($s7)\n" \
        prefix "delta:\n" \
        "lw\t$s4,0($s8)\n" \
        "addiu\t$s8,$s8,4\n" \
        "srl\t$v1,$s4,0x6\n" \
        "andi\t$v1,$v1,0x7\n" \
        "xori\t$t0,$v1,0x1f\n" \
        "addiu\t$v1,$v1,1\n" \
        "addiu\t$t2,$v1,-8\n" \
        "addu\t$s3,$s3,$v1\n" \
        "blez\t$s3," prefix "x_field_decoded\n" \
        "srav\t$t1,$s1,$t0\n" \
        "addu\t$t1,$s1,$zero\n" \
        "lw\t$s1,0($s7)\n" \
        "addiu\t$s7,$s7,4\n" \
        "subu\t$v1,$v1,$s3\n" \
        "srlv\t$v1,$s1,$v1\n" \
        "or\t$t1,$t1,$v1\n" \
        "srav\t$t1,$t1,$t0\n" \
        "addu\t$v1,$s3,$zero\n" \
        "addiu\t$s3,$s3,-32\n" \
        prefix "x_field_decoded:\n" \
        "beqz\t$t2," prefix "x_accum_ready\n" \
        "sllv\t$s1,$s1,$v1\n" \
        "sra\t$t0,$s4,0x19\n" \
        "sll\t$t0,$t0,0x1\n" \
        "lb\t$v1,88($at)\n" \
        "addu\t$t1,$t1,$t0\n" \
        "addu\t$t1,$t1,$v1\n" \
        prefix "x_accum_ready:\n" \
        "sb\t$t1,88($at)\n" \
        "srl\t$v1,$s4,0x3\n" \
        "andi\t$v1,$v1,0x7\n" \
        "xori\t$t0,$v1,0x1f\n" \
        "addiu\t$v1,$v1,1\n" \
        "addiu\t$t2,$v1,-8\n" \
        "addu\t$s3,$s3,$v1\n" \
        "blez\t$s3," prefix "z_field_decoded\n" \
        "srav\t$t1,$s1,$t0\n" \
        "addu\t$t1,$s1,$zero\n" \
        "lw\t$s1,0($s7)\n" \
        "addiu\t$s7,$s7,4\n" \
        "subu\t$v1,$v1,$s3\n" \
        "srlv\t$v1,$s1,$v1\n" \
        "or\t$t1,$t1,$v1\n" \
        "srav\t$t1,$t1,$t0\n" \
        "addu\t$v1,$s3,$zero\n" \
        "addiu\t$s3,$s3,-32\n" \
        prefix "z_field_decoded:\n" \
        "beqz\t$t2," prefix "z_accum_ready\n" \
        "sllv\t$s1,$s1,$v1\n" \
        "sll\t$t0,$s4,0x7\n" \
        "sra\t$t0,$t0,0x18\n" \
        "lb\t$v1,90($at)\n" \
        "addu\t$t1,$t1,$t0\n" \
        "addu\t$t1,$t1,$v1\n" \
        prefix "z_accum_ready:\n" \
        "sb\t$t1,90($at)\n" \
        "andi\t$v1,$s4,0x7\n" \
        "xori\t$t0,$v1,0x1f\n" \
        "addiu\t$v1,$v1,1\n" \
        "addiu\t$t2,$v1,-8\n" \
        "addu\t$s3,$s3,$v1\n" \
        "blez\t$s3," prefix "y_field_decoded\n" \
        "srav\t$t1,$s1,$t0\n" \
        "addu\t$t1,$s1,$zero\n" \
        "lw\t$s1,0($s7)\n" \
        "addiu\t$s7,$s7,4\n" \
        "subu\t$v1,$v1,$s3\n" \
        "srlv\t$v1,$s1,$v1\n" \
        "or\t$t1,$t1,$v1\n" \
        "srav\t$t1,$t1,$t0\n" \
        "addu\t$v1,$s3,$zero\n" \
        "addiu\t$s3,$s3,-32\n" \
        prefix "y_field_decoded:\n" \
        "beqz\t$t2," prefix "y_accum_ready\n" \
        "sllv\t$s1,$s1,$v1\n" \
        "sll\t$t0,$s4,0xf\n" \
        "sra\t$t0,$t0,0x18\n" \
        "lb\t$v1,89($at)\n" \
        "addu\t$t1,$t1,$t0\n" \
        "addu\t$t1,$t1,$v1\n" \
        prefix "y_accum_ready:\n" \
        "sb\t$t1,89($at)\n" \
        "lw\t$v1,88($at)\n" \
        prefix "pack_coordinates:\n" \
        "lw\t$t2,48($at)\n" \
        "and\t$t0,$v1,$t4\n" \
        "addu\t$t0,$t0,$t2\n" \
        "sll\t$t0,$t0,0x2\n" \
        "and\t$t0,$t0,$t5\n" \
        "lw\t$t2,52($at)\n" \
        "srl\t$v1,$v1,0x8\n" \
        "andi\t$v1,$v1,0xff\n" \
        "addu\t$v1,$v1,$t2\n" \
        "sll\t$v1,$v1,0x2\n"

#define CTR_RB_DECODER_PAIRED_FRAMES(prefix) \
        "srl\t$a1,$t3,0xd\n" \
        "andi\t$a1,$a1,0x7f8\n" \
        "sll\t$v1,$t3,0x5\n" \
        "bgez\t$v1," prefix "uncached\n" \
        "addu\t$a1,$a1,$at\n" \
        "lw\t$t0,320($a1)\n" \
        "bgez\t$zero," prefix "select_color\n" \
        "lw\t$v1,324($a1)\n" \
        prefix "uncached:\n" \
        "bnez\t$s8," prefix "delta\n" \
        "andi\t$t0,$s7,0x2\n" \
        "addiu\t$s7,$s7,3\n" \
        "addiu\t$t8,$t8,3\n" \
        "lwr\t$t1,-3($s7)\n" \
        "beqz\t$t0," prefix "pack_coordinates\n" \
        "lwr\t$t2,-3($t8)\n" \
        "lwl\t$t1,0($s7)\n" \
        "bgez\t$zero," prefix "pack_coordinates\n" \
        "lwl\t$t2,0($t8)\n" \
        prefix "delta:\n" \
        "sw\t$a1,32($at)\n" \
        "lw\t$s4,0($s8)\n" \
        "addiu\t$s8,$s8,4\n" \
        "srl\t$v1,$s4,0x6\n" \
        "andi\t$v1,$v1,0x7\n" \
        "xori\t$t0,$v1,0x1f\n" \
        "addiu\t$v1,$v1,1\n" \
        "addiu\t$a1,$v1,-8\n" \
        "addu\t$s3,$s3,$v1\n" \
        "srav\t$t1,$s1,$t0\n" \
        "blez\t$s3," prefix "x_field_decoded\n" \
        "srav\t$t2,$s2,$t0\n" \
        "addu\t$t1,$s1,$zero\n" \
        "addu\t$t2,$s2,$zero\n" \
        "lw\t$s1,0($s7)\n" \
        "lw\t$s2,0($t8)\n" \
        "addiu\t$s7,$s7,4\n" \
        "addiu\t$t8,$t8,4\n" \
        "subu\t$v1,$v1,$s3\n" \
        "srlv\t$a2,$s1,$v1\n" \
        "srlv\t$v1,$s2,$v1\n" \
        "or\t$t1,$t1,$a2\n" \
        "or\t$t2,$t2,$v1\n" \
        "srav\t$t1,$t1,$t0\n" \
        "srav\t$t2,$t2,$t0\n" \
        "addu\t$v1,$s3,$zero\n" \
        "addiu\t$s3,$s3,-32\n" \
        prefix "x_field_decoded:\n" \
        "sllv\t$s1,$s1,$v1\n" \
        "beqz\t$a1," prefix "x_accum_ready\n" \
        "sllv\t$s2,$s2,$v1\n" \
        "sra\t$t0,$s4,0x19\n" \
        "sll\t$t0,$t0,0x1\n" \
        "lb\t$v1,88($at)\n" \
        "addu\t$t1,$t1,$t0\n" \
        "addu\t$t1,$t1,$v1\n" \
        "lb\t$v1,92($at)\n" \
        "addu\t$t2,$t2,$t0\n" \
        "addu\t$t2,$t2,$v1\n" \
        prefix "x_accum_ready:\n" \
        "sb\t$t1,88($at)\n" \
        "sb\t$t2,92($at)\n" \
        "srl\t$v1,$s4,0x3\n" \
        "andi\t$v1,$v1,0x7\n" \
        "xori\t$t0,$v1,0x1f\n" \
        "addiu\t$v1,$v1,1\n" \
        "addiu\t$a1,$v1,-8\n" \
        "addu\t$s3,$s3,$v1\n" \
        "srav\t$t1,$s1,$t0\n" \
        "blez\t$s3," prefix "z_field_decoded\n" \
        "srav\t$t2,$s2,$t0\n" \
        "addu\t$t1,$s1,$zero\n" \
        "addu\t$t2,$s2,$zero\n" \
        "lw\t$s1,0($s7)\n" \
        "lw\t$s2,0($t8)\n" \
        "addiu\t$s7,$s7,4\n" \
        "addiu\t$t8,$t8,4\n" \
        "subu\t$v1,$v1,$s3\n" \
        "srlv\t$a2,$s1,$v1\n" \
        "srlv\t$v1,$s2,$v1\n" \
        "or\t$t1,$t1,$a2\n" \
        "or\t$t2,$t2,$v1\n" \
        "srav\t$t1,$t1,$t0\n" \
        "srav\t$t2,$t2,$t0\n" \
        "addu\t$v1,$s3,$zero\n" \
        "addiu\t$s3,$s3,-32\n" \
        prefix "z_field_decoded:\n" \
        "sllv\t$s1,$s1,$v1\n" \
        "beqz\t$a1," prefix "z_accum_ready\n" \
        "sllv\t$s2,$s2,$v1\n" \
        "sll\t$t0,$s4,0x7\n" \
        "sra\t$t0,$t0,0x18\n" \
        "lb\t$v1,90($at)\n" \
        "addu\t$t1,$t1,$t0\n" \
        "addu\t$t1,$t1,$v1\n" \
        "lb\t$v1,94($at)\n" \
        "addu\t$t2,$t2,$t0\n" \
        "addu\t$t2,$t2,$v1\n" \
        prefix "z_accum_ready:\n" \
        "sb\t$t1,90($at)\n" \
        "sb\t$t2,94($at)\n" \
        "andi\t$v1,$s4,0x7\n" \
        "xori\t$t0,$v1,0x1f\n" \
        "addiu\t$v1,$v1,1\n" \
        "addiu\t$a1,$v1,-8\n" \
        "addu\t$s3,$s3,$v1\n" \
        "srav\t$t1,$s1,$t0\n" \
        "blez\t$s3," prefix "y_field_decoded\n" \
        "srav\t$t2,$s2,$t0\n" \
        "addu\t$t1,$s1,$zero\n" \
        "addu\t$t2,$s2,$zero\n" \
        "lw\t$s1,0($s7)\n" \
        "lw\t$s2,0($t8)\n" \
        "addiu\t$s7,$s7,4\n" \
        "addiu\t$t8,$t8,4\n" \
        "subu\t$v1,$v1,$s3\n" \
        "srlv\t$a2,$s1,$v1\n" \
        "srlv\t$v1,$s2,$v1\n" \
        "or\t$t1,$t1,$a2\n" \
        "or\t$t2,$t2,$v1\n" \
        "srav\t$t1,$t1,$t0\n" \
        "srav\t$t2,$t2,$t0\n" \
        "addu\t$v1,$s3,$zero\n" \
        "addiu\t$s3,$s3,-32\n" \
        prefix "y_field_decoded:\n" \
        "sllv\t$s1,$s1,$v1\n" \
        "beqz\t$a1," prefix "y_accum_ready\n" \
        "sllv\t$s2,$s2,$v1\n" \
        "sll\t$t0,$s4,0xf\n" \
        "sra\t$t0,$t0,0x18\n" \
        "lb\t$v1,89($at)\n" \
        "addu\t$t1,$t1,$t0\n" \
        "addu\t$t1,$t1,$v1\n" \
        "lb\t$v1,93($at)\n" \
        "addu\t$t2,$t2,$t0\n" \
        "addu\t$t2,$t2,$v1\n" \
        prefix "y_accum_ready:\n" \
        "sb\t$t1,89($at)\n" \
        "sb\t$t2,93($at)\n" \
        "lw\t$t1,88($at)\n" \
        "lw\t$t2,92($at)\n" \
        "lw\t$a1,32($at)\n" \
        prefix "pack_coordinates:\n" \
        "lw\t$a2,48($at)\n" \
        "and\t$t0,$t1,$t4\n" \
        "and\t$v1,$t2,$t4\n" \
        "addu\t$t0,$t0,$v1\n" \
        "addu\t$t0,$t0,$a2\n" \
        "sll\t$t0,$t0,0x1\n" \
        "and\t$t0,$t0,$t5\n" \
        "lw\t$a2,52($at)\n" \
        "andi\t$t1,$t1,0xff00\n" \
        "andi\t$t2,$t2,0xff00\n" \
        "addu\t$v1,$t1,$t2\n" \
        "srl\t$v1,$v1,0x7\n" \
        "addu\t$v1,$v1,$a2\n"

#define CTR_RB_DECODER_CACHE_PACKED_VERTEX \
        "sw\t$t0,320($a1)\n" \
        "sw\t$v1,324($a1)\n"

#define CTR_RB_DECODER_TRANSFORM_PACKED_VERTEX \
        "mtc2\t$t0,$0\n" \
        "mtc2\t$v1,$1\n" \
        "nop\n" \
        "nop\n" \
        "cop2\t0x4a6012\n" \
        "mfc2\t$v1,$11\n" \
        "mfc2\t$t0,$10\n" \
        "mfc2\t$t1,$9\n" \
        "sll\t$t0,$t0,0x10\n" \
        "andi\t$t1,$t1,0xffff\n" \
        "or\t$t0,$t0,$t1\n" \
        "sw\t$t0,320($a1)\n" \
        "sw\t$v1,324($a1)\n"

#define CTR_RB_DECODER_RETURN_COMMAND_COLOR(prefix) \
        prefix "select_color:\n" \
        "srl\t$t1,$t3,0x7\n" \
        "sll\t$t2,$t3,0x4\n" \
        "bltz\t$t2," prefix "scratch_color\n" \
        "andi\t$t1,$t1,0x1fc\n" \
        "addu\t$t1,$t1,$gp\n" \
        "jr\t$ra\n" \
        "lw\t$t1,0($t1)\n" \
        prefix "scratch_color:\n" \
        "addu\t$t1,$t1,$at\n" \
        "jr\t$ra\n" \
        "lw\t$t1,320($t1)\n"
// clang-format on
