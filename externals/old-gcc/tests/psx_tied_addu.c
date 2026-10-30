/* Fixed local work roles request tied/untied ADDU; they are not incoming ABI. */
typedef unsigned int u32;
typedef char word_is_32[(sizeof(u32) == 4) ? 1 : -1];

#ifndef PROFILE
#define PROFILE 1
#endif
#ifndef SHAPE
#define SHAPE 1
#endif
#ifdef CTR_NATIVE
#define FIXED(name)
#define ATTRIBUTE
#else
#define FIXED(name) __asm__(name)
#if PROFILE == 1
#define ATTRIBUTE __attribute__((mips_unsafe_manual_at_full_frame))
#elif PROFILE == 2
#define ATTRIBUTE __attribute__((mips_unsafe_manual_at_frame))
#elif PROFILE == 0
#define ATTRIBUTE
#else
#error PROFILE must be 0, 1 or 2
#endif
#endif

void AddEmit(u32 *, u32, u32, u32) ATTRIBUTE;
void AddEmit(u32 *words, u32 first, u32 second, u32 salt)
{
    register u32 result FIXED("$10");
    register u32 left FIXED("$8");
    register u32 right FIXED("$9");

    result = words[0] ^ first;
    left = words[1] ^ second;
    right = words[2] ^ salt;
    words[3] = result;
    words[4] = left;
    words[5] = right;
#if SHAPE == 1
    result = left + result;
#elif SHAPE == 2
    result = result + left;
#elif SHAPE == 3
    result = left + right;
#elif SHAPE == 4
    result = left + 7u;
#else
#error SHAPE must be 1, 2, 3 or 4
#endif
    words[6] = result;
}
