typedef unsigned int u32;

#if defined(TEST_VARIABLE_INDEX)
void psx_ctc2_rejected(u32 value, int index)
{
    __builtin_mips_ctc2(value, index);
}
#elif defined(TEST_NEGATIVE_INDEX)
void psx_ctc2_rejected(u32 value)
{
    __builtin_mips_ctc2(value, -1);
}
#elif defined(TEST_LARGE_INDEX)
void psx_ctc2_rejected(u32 value)
{
    __builtin_mips_ctc2(value, 32);
}
#else
void psx_ctc2_write4(u32 value)
{
    __builtin_mips_ctc2(value, 4);
}

void psx_ctc2_write31(u32 value)
{
    __builtin_mips_ctc2(value, 31);
}

void psx_ctc2_ordered(u32 value)
{
    __builtin_mips_ctc2(value, 0);
    __builtin_mips_ctc2(value + 1u, 1);
    __builtin_mips_ctc2(value + 2u, 2);
    __builtin_mips_ctc2(value + 3u, 3);
    __builtin_mips_ctc2(value + 4u, 4);
}

u32 psx_ctc2_postincrement(u32 *value)
{
    __builtin_mips_ctc2((*value)++, 4);
    return *value;
}
#endif
