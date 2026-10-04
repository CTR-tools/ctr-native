typedef unsigned int u32;

unsigned char source_bytes[12];
unsigned char result_bytes[8];
unsigned state_words[2];

unsigned ordinary_alu(unsigned a, unsigned b)
{
    return ((a << 3) + (b ^ 0x55aa55aau)) ^ (a >> 7);
}

unsigned possible_alias(unsigned *destination, const unsigned *source,
                        unsigned value)
{
    *destination = value;
    return *source ^ (value << 3);
}

unsigned real_provider(unsigned value)
{
    state_words[1] = state_words[0] ^ 0x55aa55aau;
    return value + 1u;
}

unsigned independent_bytes(unsigned seed)
{
    unsigned first;
    unsigned second;

    first = source_bytes[0] + 48u;
    second = source_bytes[4] + 48u;
    result_bytes[0] = (unsigned char)first;
    first = source_bytes[8] + 48u;
    result_bytes[2] = (unsigned char)second;
    result_bytes[3] = (unsigned char)first;
    return (seed << 3) ^ (first + second);
}

unsigned call_boundary(unsigned value)
{
    unsigned result;

    state_words[0] = value;
    result = real_provider(value + 2u);
    return state_words[1] ^ result;
}

unsigned volatile_boundary(volatile unsigned *destination,
                           const volatile unsigned *source, unsigned value)
{
    *destination = value;
    return *source ^ (value >> 3);
}

struct four_words
{
    unsigned word[4];
};

void copy_words(struct four_words *destination, const struct four_words *source,
                unsigned value)
{
    result_bytes[5] = (unsigned char)value;
    *destination = *source;
}

#ifndef SMALL_LITERAL
#define SMALL_LITERAL 9
#endif

extern u32 ConsumeFour(u32 first, u32 second, u32 third, u32 selector);

u32 LiteralJoin(u32 first, u32 second, u32 third, u32 selector)
{
    if (selector != 0)
    {
        first = (u32)SMALL_LITERAL;
        second = (u32)SMALL_LITERAL;
        third = (u32)SMALL_LITERAL;
    }
    else
    {
        first += 1u;
        second ^= 2u;
        third += 3u;
    }
    return ConsumeFour(first, second, third, selector);
}

u32 OrdinaryReuse(u32 *destination, const u32 *source, u32 value)
{
    u32 cached;

    cached = *source;
    destination[0] = cached;
    destination[1] = *source;
    return (value + 9u) ^ cached;
}
