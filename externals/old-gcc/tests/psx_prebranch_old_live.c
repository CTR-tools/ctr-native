/* Compile-only ordinary-C liveness control. Pointers designate complete,
   aligned, stable mapped RAM. No volatile, scheduling ASM or global ABI. */
typedef unsigned int u32;
struct RamProbe { u32 word; u32 gap; };

u32 ram_shared_reject_old_live(const struct RamProbe *fall,
                               const struct RamProbe *taken,
                               u32 field, u32 mask)
{
    register u32 gate __asm__("$8");
    register u32 value __asm__("$9");
    register u32 gap __asm__("$8");
    value = field >> 16;
    gate = field & mask;
    if (gate == 0)
        goto rejected;
    if (value != 0)
        goto rejected;
    value = fall->word;
    gap = fall->gap;
    return value ^ gap;
rejected:
    gap = taken->gap;
    return value ^ gap;
}
