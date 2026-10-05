#if defined(TEST_INVALID_ARGS)
void unreachable_invalid(void)
{
    __builtin_unreachable(1);
}
#elif defined(TEST_ORDINARY_ONLY)
unsigned int unreachable_ordinary(unsigned int n)
{
    volatile unsigned int words[8];

    words[n & 7u] = n;
    return words[n & 7u] ^ 0x5a5au;
}
#else
extern volatile unsigned int *unreachable_sink;

void unreachable_terminal(volatile unsigned int *cursor)
{
    *cursor++ = 7u;
    unreachable_sink = cursor;
    __builtin_unreachable();
    *cursor = 9u;
}

unsigned int unreachable_framed(unsigned int n)
{
    volatile unsigned int words[8];

    words[n & 7u] = n;
    if (n == 0)
        __builtin_unreachable();
    return words[n & 7u] ^ 0x5a5au;
}
#endif
