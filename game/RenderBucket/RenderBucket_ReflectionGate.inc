// NOTE(aalhendi): Reflection's two fixed 32-word projection/water gates.
// bounds is also the preceding BLEZ delay slot; done follows JAL's NOP.
// This one temporary instruction macro is undefined after the default entry.
// clang-format off
#define CTR_RB_REFLECTION_GATE(prefix) \
        "lw\t$v1,28($at)\n" \
        "mfc2\t$t2,$14\n" \
        "cfc2\t$a1,$31\n" \
        "sll\t$t0,$t3,0x3\n" \
        "sll\t$a1,$a1,0xd\n" \
        "bltz\t$a1," prefix "done\n" \
        "sll\t$t1,$t3,0x2\n" \
        "bgez\t$t0," prefix "bounds\n" \
        "lh\t$a1,36($at)\n" \
        "cop2\t0x1400006\n" \
        "mfc2\t$t0,$24\n" \
        "xor\t$t1,$a1,$t1\n" \
        "beqz\t$t0," prefix "done\n" \
        "xor\t$t0,$t0,$t1\n" \
        "blez\t$t0," prefix "done\n" \
        prefix "bounds:\n" \
        "mfc2\t$t0,$12\n" \
        "mfc2\t$t1,$13\n" \
        "and\t$a1,$t0,$t2\n" \
        "and\t$a1,$a1,$t1\n" \
        "cop2\t0x158002d\n" \
        "subu\t$t0,$t0,$v1\n" \
        "subu\t$t1,$t1,$v1\n" \
        "subu\t$t2,$t2,$v1\n" \
        "or\t$v1,$t0,$t1\n" \
        "nor\t$v1,$v1,$t2\n" \
        "or\t$v1,$v1,$a1\n" \
        "bltz\t$v1," prefix "done\n" \
        "sll\t$v1,$v1,0x10\n" \
        "bltz\t$v1," prefix "done\n" \
        "swc2\t$24,44($at)\n" \
        "jal\tRenderBucket_WaterSplit_Import\n" \
        "nop\n" \
        prefix "done:\n"
// clang-format on
