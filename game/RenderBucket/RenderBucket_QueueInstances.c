#include <common.h>

#ifndef CTR_NATIVE
// NOTE(aalhendi): These entries preserve the caller in scratch 0x00..0x2c,
// then use sp as Instance and s8 as Instance + player*0x88. QueueDraw takes
// that register state, keeps a0/a1/at/sp/s3/s8, and advances the live a2 RBI.
// Native retains the ordinary C entries and provider in QueueExecute.
// The two real providers below must resolve to retail, never a C Context body.

// NOTE(aalhendi): C string fragments share identical caller-frame instructions.
// They expand before assembly; neither fragment creates a callable helper.
// clang-format off
#define RB_QUEUE_SAVE_CALLER_ASM \
        "lui\t$at,0x1f80\n" \
        "sw\t$s0,0x00($at)\n" \
        "sw\t$s1,0x04($at)\n" \
        "sw\t$s2,0x08($at)\n" \
        "sw\t$s3,0x0c($at)\n" \
        "sw\t$s4,0x10($at)\n" \
        "sw\t$s5,0x14($at)\n" \
        "sw\t$s6,0x18($at)\n" \
        "sw\t$s7,0x1c($at)\n" \
        "sw\t$gp,0x20($at)\n" \
        "sw\t$sp,0x24($at)\n" \
        "sw\t$s8,0x28($at)\n" \
        "sw\t$ra,0x2c($at)\n"

#define RB_QUEUE_RETURN_TO_CALLER_ASM \
        "addu\t$v0,$a2,$zero\n" \
        "lw\t$t0,0x34($at)\n" \
        "lw\t$ra,0x2c($at)\n" \
        "sw\t$t0,0x0c($a1)\n" \
        "lw\t$s8,0x28($at)\n" \
        "lw\t$sp,0x24($at)\n" \
        "lw\t$gp,0x20($at)\n" \
        "lw\t$s7,0x1c($at)\n" \
        "lw\t$s6,0x18($at)\n" \
        "lw\t$s5,0x14($at)\n" \
        "lw\t$s4,0x10($at)\n" \
        "lw\t$s3,0x0c($at)\n" \
        "lw\t$s2,0x08($at)\n" \
        "lw\t$s1,0x04($at)\n" \
        "lw\t$s0,0x00($at)\n" \
        "jr\t$ra\n" \
        "nop\n"

__asm__(".section .RenderBucket_QueueLevInstances,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tRenderBucket_QueueLevInstances\n"
        ".globl RenderBucket_QueueLevInstances\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        "RenderBucket_QueueLevInstances:\n"
        RB_QUEUE_SAVE_CALLER_ASM
        // Read stack arguments while sp is still the caller's stack pointer.
        "sw\t$a3,0x50($at)\n"
        "lw\t$v1,0x10($sp)\n"
        "lw\t$t2,0x14($sp)\n"
        "lw\t$t0,0x0c($a1)\n"
        "lw\t$t1,0x08($a1)\n"
        "sw\t$v1,0x5c($at)\n"
        "sw\t$t2,0x90($at)\n"
        "sw\t$t0,0x34($at)\n"
        "addiu\t$t1,$t1,-4\n"
        "sw\t$t1,0x38($at)\n"
        // Start with camera count-1; keep the camera owner apart from its list.
        "addiu\t$v1,$v1,-1\n"
        "addiu\t$t1,$zero,0xdc\n"
        "mult\t$t1,$v1\n"
        "mflo\t$t1\n"
        "addu\t$a0,$a0,$t1\n"
        "sw\t$a0,0x60($at)\n"
        "addiu\t$t1,$zero,0x88\n"
        "mult\t$t1,$v1\n"
        "lw\t$a0,0x28($a0)\n"
        "sw\t$zero,0x58($at)\n"
        "mflo\t$t1\n"
        "jal\tRenderBucket_CopyDispatchTables\n"
        "sw\t$t1,0x54($at)\n"
        ".Lrb_queuelev_scan_player:\n"
        // The list read in this delay slot also occurs for a null retail list.
        "beq\t$a0,$zero,.Lrb_queuelev_next_player\n"
        "lw\t$sp,0($a0)\n"
        ".Lrb_queuelev_scan_instance:\n"
        "lw\t$v1,0x54($at)\n"
        "addiu\t$a0,$a0,4\n"
        "beq\t$sp,$zero,.Lrb_queuelev_next_player\n"
        "addu\t$s8,$sp,$v1\n"
        "lw\t$s3,0x18($sp)\n"
        "jal\tRenderBucket_QueueDraw\n"
        "lw\t$gp,0x28($sp)\n"
        "bgez\t$zero,.Lrb_queuelev_scan_instance\n"
        "lw\t$sp,0($a0)\n"
        ".Lrb_queuelev_next_player:\n"
        // Retail fetches the preceding camera list before testing the count.
        "lw\t$a0,0x60($at)\n"
        "lw\t$v1,0x54($at)\n"
        "lw\t$t0,0x5c($at)\n"
        "addiu\t$a0,$a0,-0xdc\n"
        "sw\t$a0,0x60($at)\n"
        "lw\t$a0,0x28($a0)\n"
        "addiu\t$v1,$v1,-0x88\n"
        "addiu\t$t0,$t0,-1\n"
        "sw\t$v1,0x54($at)\n"
        "bgtz\t$t0,.Lrb_queuelev_scan_player\n"
        "sw\t$t0,0x5c($at)\n"
        RB_QUEUE_RETURN_TO_CALLER_ASM
        ".end\tRenderBucket_QueueLevInstances\n"
        ".size RenderBucket_QueueLevInstances,.-RenderBucket_QueueLevInstances\n"
        ".set\tat\n"
        ".set\treorder\n"
        ".text\n");

__asm__(".section .RenderBucket_QueueNonLevInstances,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tRenderBucket_QueueNonLevInstances\n"
        ".globl RenderBucket_QueueNonLevInstances\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        "RenderBucket_QueueNonLevInstances:\n"
        RB_QUEUE_SAVE_CALLER_ASM
        "sw\t$a3,0x50($at)\n"
        "lw\t$v1,0x10($sp)\n"
        "lw\t$t2,0x14($sp)\n"
        "lw\t$t0,0x0c($a1)\n"
        "lw\t$t1,0x08($a1)\n"
        "sw\t$v1,0x5c($at)\n"
        "sw\t$t2,0x90($at)\n"
        "sw\t$t0,0x34($at)\n"
        "addiu\t$t1,$t1,-4\n"
        "sw\t$t1,0x38($at)\n"
        // Keep the original list head in a0; offset/count are persistent scratch.
        "addiu\t$t1,$zero,0x88\n"
        "addiu\t$v1,$v1,-1\n"
        "mult\t$v1,$t1\n"
        "sw\t$t0,0x34($at)\n"
        "sw\t$zero,0x58($at)\n"
        "mflo\t$t1\n"
        "jal\tRenderBucket_CopyDispatchTables\n"
        "sw\t$t1,0x54($at)\n"
        ".Lrb_queuenonlev_restart_player:\n"
        "addu\t$sp,$a0,$zero\n"
        ".Lrb_queuenonlev_scan_instance:\n"
        "lw\t$v1,0x54($at)\n"
        "beq\t$sp,$zero,.Lrb_queuenonlev_next_player\n"
        "addu\t$s8,$sp,$v1\n"
        "lw\t$s3,0x18($sp)\n"
        "jal\tRenderBucket_QueueDraw\n"
        "lw\t$gp,0x28($sp)\n"
        "lw\t$sp,0($sp)\n"
        "bgez\t$zero,.Lrb_queuenonlev_scan_instance\n"
        "nop\n"
        ".Lrb_queuenonlev_next_player:\n"
        "lw\t$v1,0x54($at)\n"
        "lw\t$t0,0x5c($at)\n"
        "addiu\t$v1,$v1,-0x88\n"
        "addiu\t$t0,$t0,-1\n"
        "sw\t$v1,0x54($at)\n"
        "bgtz\t$t0,.Lrb_queuenonlev_restart_player\n"
        "sw\t$t0,0x5c($at)\n"
        RB_QUEUE_RETURN_TO_CALLER_ASM
        ".end\tRenderBucket_QueueNonLevInstances\n"
        ".size RenderBucket_QueueNonLevInstances,.-RenderBucket_QueueNonLevInstances\n"
        ".set\tat\n"
        ".set\treorder\n"
        ".text\n");
#undef RB_QUEUE_RETURN_TO_CALLER_ASM
#undef RB_QUEUE_SAVE_CALLER_ASM
// clang-format on
#endif
