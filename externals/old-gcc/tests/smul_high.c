typedef int s32;
typedef unsigned int u32;
typedef short s16;

extern s32 HighProvider(s32 value, u32 *calls);

#define HIGH_VALUE(first, second) __builtin_smul_high_si(first, second)

s32 HighDynamic(s32 first, s32 second)
{
    return HIGH_VALUE(first, second);
}

s32 HighConstant(s32 value)
{
    return HIGH_VALUE(value, 0x66666667);
}

s32 HighZero(s32 value)
{
    return HIGH_VALUE(value, 0);
}

u32 HighAcrossHigh(s32 first, s32 second, s32 third)
{
    s32 high;

    high = HIGH_VALUE(first, second);
    return (u32)high ^ (u32)HIGH_VALUE(high, third);
}

u32 HighAcrossLow(s32 first, s32 second, u32 factor)
{
    s32 high;

    high = HIGH_VALUE(first, second);
    return (u32)high ^ ((u32)high * factor);
}

u32 HighAcrossCall(s32 first, s32 second, u32 *calls)
{
    s32 high;

    high = HIGH_VALUE(first, second);
    return (u32)high ^ (u32)HighProvider(high, calls);
}

s32 HighUpdates(const s32 *input, s32 *after)
{
    s32 first, second, high;

    first = input[0];
    second = input[1];
    high = HIGH_VALUE(first++, second++);
    after[0] = first;
    after[1] = second;
    return high;
}

void HighIgnoredUpdates(const s32 *input, s32 *after)
{
    s32 first, second;

    first = input[0];
    second = input[1];
    HIGH_VALUE(first++, second++);
    after[0] = first;
    after[1] = second;
}

s32 HighCallArguments(s32 first, s32 second, u32 *calls)
{
    return HIGH_VALUE(HighProvider(first, &calls[0]), HighProvider(second, &calls[1]));
}

s32 HighConverted(u32 first, s16 second)
{
    return HIGH_VALUE(first, second);
}

#undef HIGH_VALUE

#ifdef ARITY
s32 WrongArity(s32 value)
{
#if ARITY == 0
    return __builtin_smul_high_si();
#elif ARITY == 1
    return __builtin_smul_high_si(value);
#elif ARITY == 3
    return __builtin_smul_high_si(value, value, value);
#else
#error Select one of the three genuine wrong-arity cases
#endif
}
#endif
