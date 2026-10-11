#if !defined(CTR_NATIVE)
// NOTE(aalhendi): These retail entries tail-transfer to BIOS A0/B0/C0.
// Native keeps the host libc and platform API implementations.
#define PSX_BIOS_VECTOR_A "0xa0"
#define PSX_BIOS_VECTOR_B "0xb0"
#define PSX_BIOS_VECTOR_C "0xc0"

// The selector occupies the jump delay slot; arguments and ra pass through.
#define PSX_BIOS_CALL_GATE(name, label, vector, selector)    \
	__asm__(".section ." #name ",\"ax\",@progbits\n"         \
	        ".align 2\n"                                     \
	        ".ent " #name "\n"                               \
	        ".set noreorder\n"                               \
	        ".globl " #name "\n" label "li $t2," vector "\n" \
	        "jr $t2\n"                                       \
	        "li $t1," selector "\n"                          \
	        "nop\n"                                          \
	        ".end " #name "\n"                               \
	        ".set reorder\n"                                 \
	        ".text\n")

// Keep each literal label on its own line for source-owner discovery.
// clang-format off
PSX_BIOS_CALL_GATE(memset,
                  "memset:\n",
                  PSX_BIOS_VECTOR_A, "0x2b");
PSX_BIOS_CALL_GATE(strcmp,
                  "strcmp:\n",
                  PSX_BIOS_VECTOR_A, "0x17");
PSX_BIOS_CALL_GATE(strncpy,
                  "strncpy:\n",
                  PSX_BIOS_VECTOR_A, "0x1a");
PSX_BIOS_CALL_GATE(strcpy,
                  "strcpy:\n",
                  PSX_BIOS_VECTOR_A, "0x19");
PSX_BIOS_CALL_GATE(LoadExec,
                  "LoadExec:\n",
                  PSX_BIOS_VECTOR_A, "0x51");
PSX_BIOS_CALL_GATE(memcpy,
                  "memcpy:\n",
                  PSX_BIOS_VECTOR_A, "0x2a");
PSX_BIOS_CALL_GATE(strlen,
                  "strlen:\n",
                  PSX_BIOS_VECTOR_A, "0x1b");
PSX_BIOS_CALL_GATE(_96_init,
                  "_96_init:\n",
                  PSX_BIOS_VECTOR_A, "0x71");
PSX_BIOS_CALL_GATE(_card_info,
                  "_card_info:\n",
                  PSX_BIOS_VECTOR_A, "0xab");
PSX_BIOS_CALL_GATE(erase,
                  "erase:\n",
                  PSX_BIOS_VECTOR_B, "0x45");
PSX_BIOS_CALL_GATE(lseek,
                  "lseek:\n",
                  PSX_BIOS_VECTOR_B, "0x33");
PSX_BIOS_CALL_GATE(close,
                  "close:\n",
                  PSX_BIOS_VECTOR_B, "0x36");
PSX_BIOS_CALL_GATE(write,
                  "write:\n",
                  PSX_BIOS_VECTOR_B, "0x35");
PSX_BIOS_CALL_GATE(EnableEvent,
                  "EnableEvent:\n",
                  PSX_BIOS_VECTOR_B, "0x0c");
PSX_BIOS_CALL_GATE(_bu_init,
                  "_bu_init:\n",
                  PSX_BIOS_VECTOR_A, "0x70");
PSX_BIOS_CALL_GATE(nextfile,
                  "nextfile:\n",
                  PSX_BIOS_VECTOR_B, "0x43");
PSX_BIOS_CALL_GATE(format,
                  "format:\n",
                  PSX_BIOS_VECTOR_B, "0x41");
PSX_BIOS_CALL_GATE(OpenEvent,
                  "OpenEvent:\n",
                  PSX_BIOS_VECTOR_B, "0x08");
PSX_BIOS_CALL_GATE(read,
                  "read:\n",
                  PSX_BIOS_VECTOR_B, "0x34");
PSX_BIOS_CALL_GATE(TestEvent,
                  "TestEvent:\n",
                  PSX_BIOS_VECTOR_B, "0x0b");
PSX_BIOS_CALL_GATE(open,
                  "open:\n",
                  PSX_BIOS_VECTOR_B, "0x32");
PSX_BIOS_CALL_GATE(CloseEvent,
                  "CloseEvent:\n",
                  PSX_BIOS_VECTOR_B, "0x09");
PSX_BIOS_CALL_GATE(_card_load,
                  "_card_load:\n",
                  PSX_BIOS_VECTOR_A, "0xac");
PSX_BIOS_CALL_GATE(strcat,
                  "strcat:\n",
                  PSX_BIOS_VECTOR_A, "0x15");
PSX_BIOS_CALL_GATE(printf,
                  "printf:\n",
                  PSX_BIOS_VECTOR_A, "0x3f");
PSX_BIOS_CALL_GATE(DeliverEvent,
                  "DeliverEvent:\n",
                  PSX_BIOS_VECTOR_B, "0x07");
PSX_BIOS_CALL_GATE(ChangeClearPAD,
                  "ChangeClearPAD:\n",
                  PSX_BIOS_VECTOR_B, "0x5b");
PSX_BIOS_CALL_GATE(SysDeqIntRP,
                  "SysDeqIntRP:\n",
                  PSX_BIOS_VECTOR_C, "0x03");
PSX_BIOS_CALL_GATE(SysEnqIntRP,
                  "SysEnqIntRP:\n",
                  PSX_BIOS_VECTOR_C, "0x02");
PSX_BIOS_CALL_GATE(setjmp,
                  "setjmp:\n",
                  PSX_BIOS_VECTOR_A, "0x13");
PSX_BIOS_CALL_GATE(HookEntryInt,
                  "HookEntryInt:\n",
                  PSX_BIOS_VECTOR_B, "0x19");
PSX_BIOS_CALL_GATE(ReturnFromException,
                  "ReturnFromException:\n",
                  PSX_BIOS_VECTOR_B, "0x17");
PSX_BIOS_CALL_GATE(ResetEntryInt,
                  "ResetEntryInt:\n",
                  PSX_BIOS_VECTOR_B, "0x18");
PSX_BIOS_CALL_GATE(StartCARD2,
                  "StartCARD2:\n",
                  PSX_BIOS_VECTOR_B, "0x4b");
PSX_BIOS_CALL_GATE(StopCARD2,
                  "StopCARD2:\n",
                  PSX_BIOS_VECTOR_B, "0x4c");
PSX_BIOS_CALL_GATE(_new_card,
                  "_new_card:\n",
                  PSX_BIOS_VECTOR_B, "0x50");
PSX_BIOS_CALL_GATE(_card_write,
                  "_card_write:\n",
                  PSX_BIOS_VECTOR_B, "0x4e");
PSX_BIOS_CALL_GATE(firstfile2,
                  "firstfile2:\n",
                  PSX_BIOS_VECTOR_B, "0x42");
PSX_BIOS_CALL_GATE(memchr,
                  "memchr:\n",
                  PSX_BIOS_VECTOR_A, "0x2e");
PSX_BIOS_CALL_GATE(bzero,
                  "bzero:\n",
                  PSX_BIOS_VECTOR_A, "0x28");
PSX_BIOS_CALL_GATE(WaitEvent,
                  "WaitEvent:\n",
                  PSX_BIOS_VECTOR_B, "0x0a");
PSX_BIOS_CALL_GATE(FlushCache,
                  "FlushCache:\n",
                  PSX_BIOS_VECTOR_A, "0x44");
// clang-format on

#undef PSX_BIOS_CALL_GATE
#undef PSX_BIOS_VECTOR_A
#undef PSX_BIOS_VECTOR_B
#undef PSX_BIOS_VECTOR_C
#endif
