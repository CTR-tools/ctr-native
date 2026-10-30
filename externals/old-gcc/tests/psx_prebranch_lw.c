/* Compile-only shape requests, not reached-coverage assertions.
   All ordinary pointers designate complete, aligned, stable mapped RAM.
   No MMIO address, game identity, global register ABI or scheduling ASM. */
typedef unsigned char u8;
typedef unsigned int u32;
typedef int s32;
typedef short s16;
typedef char control_word_is_32[(sizeof(u32) == 4) ? 1 : -1];
typedef char control_half_is_16[(sizeof(s16) == 2) ? 1 : -1];

struct RamProbe
{
    u32 first;
    u32 gap;
    u32 spare;
    u32 slots[4];
};

/* Request BEQ/genuine SRL prefix, then NE-zero9 with first-fallthrough LW9.
   The taken edge does not need the old9 value; both first gaps request LW8. */
u32 ram_forward_alias(const struct RamProbe *fall, const struct RamProbe *taken,
                      u32 field, u32 mask)
{
    register u32 gate __asm__("$8");
    register u32 value __asm__("$9");
    register u32 gap __asm__("$8");
    gate = field & mask;
    if (gate == 0)
        goto rejected;
    value = field >> 16;
    if (value != 0)
        goto rejected;
    value = fall->first;
    gap = fall->gap;
    return value ^ gap;
rejected:
    gap = taken->gap;
    return gap;
}

/* Same requested LW9/LW8 false edge, but old9 is genuinely needed beyond
   an independent taken-edge LW8. The indexed store consumes the old value. */
u32 ram_opposite_old_live(const struct RamProbe *fall, struct RamProbe *taken,
                          u32 field, u32 mask)
{
    register u32 gate __asm__("$8");
    register u32 value __asm__("$9");
    register u32 gap __asm__("$8");
    gate = field & mask;
    if (gate == 0)
        return mask;
    value = field >> 16;
    if (value != 0)
        goto old_value_needed;
    value = fall->first;
    gap = fall->gap;
    return value ^ gap;
old_value_needed:
    gap = taken->spare;
    taken->slots[(gap + value) & 3u] = value;
    return gap;
}

/* Request real SUB10; signed LH9; GT-zero10, with independent LW8 heads.
   The promoted signed-half subtraction fits s32; return arithmetic is u32. */
u32 ram_backward_signed_half(const s16 *halves, const struct RamProbe *gaps,
                             u32 field, s16 bound)
{
    register s32 condition __asm__("$10");
    register s32 pending __asm__("$9");
    register u32 gap __asm__("$8");
    condition = (s32)(s16)(field >> 16) - (s32)bound;
    pending = halves[1];
    if (condition > 0)
        goto positive;
    gap = gaps->gap;
    return (u32)pending ^ gap;
positive:
    gap = gaps->spare;
    return (u32)pending + gap;
}

/* Two explicit type controls of the same fourth function, never a rescue:
   default requests LBU9; CONTROL_VOLATILE_ACCESS requests volatile SI LW9.
   The qualifier is the property under test, not a cue in a positive case. */
#ifdef CONTROL_VOLATILE_ACCESS
u32 ram_refuse_access_kind(const volatile u32 *access, const struct RamProbe *gaps,
                           u32 field, u32 mask)
#else
u32 ram_refuse_access_kind(const u8 *access, const struct RamProbe *gaps,
                           u32 field, u32 mask)
#endif
{
    register u32 condition __asm__("$10");
    register u32 pending __asm__("$9");
    register u32 gap __asm__("$8");
    condition = field & mask;
    if (condition != 0)
        goto rejected;
    pending = access[1];
    gap = gaps->gap;
    return pending ^ gap;
rejected:
    gap = gaps->spare;
    return gap;
}

/* The taken head is an independent EQ/NE branch, except in refusal controls. */
u32 ram_gap_direct_branch(const struct RamProbe *read, const struct RamProbe *other, u32 field, u32 mask)
{
    register u32 gate __asm__("$8");
    register u32 pending __asm__("$9");
    register u32 gap __asm__("$8");
    pending = field >> 16;
    gate = field & mask;
    if (gate == 0) goto rejected;
    if (pending != 0) goto rejected;
    pending = read->first;
    gap = read->gap;
    return pending ^ gap;
rejected:
#ifdef GAP_BRANCH_READ
    if (pending == mask)
#elif defined(GAP_OTHER_BRANCH)
    if ((s32)mask > 0)
#else
    if (read == other)
#endif
    {
#ifdef GAP_OLD_LIVE
        gap = other->gap;
        return pending ^ gap;
#else
        return other->gap;
#endif
    }
    return read->spare;
}

#ifdef GAP_HALF
typedef s16 RamJumpWord;
#else
typedef u32 RamJumpWord;
#endif
#ifdef GAP_VOLATILE
typedef volatile RamJumpWord RamJumpInput;
#else
typedef RamJumpWord RamJumpInput;
#endif
/* Nonzero bounded count and complete mapped inputs; each load is really used. */
u32 ram_jump_load(const RamJumpInput *read, u32 gate, u32 count)
{
    register u32 counter __asm__("$8");
    register u32 pending __asm__("$9");
    counter = count;
    do
    {
        if (gate & counter)
        {
            pending = (u32)read[0];
            goto joined;
        }
        pending = (u32)read[1];
joined:
        counter -= 1u;
        gate ^= pending;
    } while (counter != 0);
    return gate;
}
