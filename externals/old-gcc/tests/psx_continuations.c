/* Owned compiler contracts; mapped scratch/providers and balance are source duties. */
typedef unsigned int u32;
typedef unsigned long long u64;
typedef void (*Callback2)(u32 *, u32 *);
#ifndef REJECT
#define REJECT 0
#endif

#ifdef TEST_FULL
#define FULL __attribute__((mips_unsafe_manual_at_full_frame))
#if REJECT == 0
#ifdef CTR_NATIVE
#undef FULL
#define FULL
#else
/* These are actual incoming arguments, not internal work colors. */
register u32 *const incomingPayload __asm__("$5");
register Callback2 const incomingCallback __asm__("$6");
register u32 *const incomingContext __asm__("$7");
#endif
void FullScratch(void *, u32 *, Callback2, u32 *) FULL;
void FullScratch(void *root, u32 *payloadArgument, Callback2 callbackArgument, u32 *contextArgument)
{
    u32 *payload;
    u32 *context;
    Callback2 callback;
#ifndef CTR_NATIVE
    register u32 *image __asm__("$1");
    register u32 s0 __asm__("$16");
    register u32 s1 __asm__("$17");
    register u32 s2 __asm__("$18");
    register u32 s3 __asm__("$19");
    register u32 s4 __asm__("$20");
    register u32 s5 __asm__("$21");
    register u32 s6 __asm__("$22");
    register u32 s7 __asm__("$23");
    register u32 gp __asm__("$28");
    register u32 sp __asm__("$29");
    register u32 s8 __asm__("$30");
    register u32 ra __asm__("$31");
    __asm__ volatile("" : "=r"(s0), "=r"(s1), "=r"(s2), "=r"(s3), "=r"(s4), "=r"(s5));
    __asm__ volatile("" : "=r"(s6), "=r"(s7), "=r"(gp), "=r"(s8), "=r"(ra));
    image = (u32 *)0x1f800000u;
    image[12] = s0;
#endif
    if (root == 0)
        return;
#ifndef CTR_NATIVE
    image[13] = s1;
    image[14] = s2;
    image[15] = s3;
    image[16] = s4;
    image[17] = s5;
    image[18] = s6;
    image[19] = s7;
    image[20] = gp;
    image[21] = (u32)__builtin_sp();
    image[22] = s8;
    image[23] = ra;
    payload = incomingPayload;
    context = incomingContext;
    callback = incomingCallback;
#else
    payload = payloadArgument;
    context = contextArgument;
    callback = callbackArgument;
#endif
    payload[0] += context[0];
    callback(payload, context);
    payload[1] ^= context[1];
#ifndef CTR_NATIVE
    image = (u32 *)0x1f800000u;
    ra = image[23];
    s8 = image[22];
    sp = image[21];
    gp = image[20];
    s7 = image[19];
    s6 = image[18];
    s5 = image[17];
    s4 = image[16];
    s3 = image[15];
    s2 = image[14];
    s1 = image[13];
    s0 = image[12];
    __asm__ volatile("" : : "r"(ra), "r"(s8), "r"(sp), "r"(gp), "r"(s7), "r"(s6));
    __asm__ volatile("" : : "r"(s5), "r"(s4), "r"(s3), "r"(s2), "r"(s1), "r"(s0));
#endif
}
#elif REJECT == 2
void Invalid(void *, u32 *, Callback2, u64) FULL;
void Invalid(void *root, u32 *payload, Callback2 callback, u64 value) { payload[0] ^= (u32)value; }
#elif REJECT == 3
typedef void (*Callback5)(u32 *, u32 *, u32, u32, u32);
void Invalid(void *, u32 *, Callback5, u32 *) FULL;
void Invalid(void *root, u32 *payload, Callback5 callback, u32 *context) { callback(payload, context, 1, 2, 3); }
#elif REJECT == 4
typedef u32 (*ValueCallback)(u32 *, u32 *);
void Invalid(void *, u32 *, ValueCallback, u32 *) FULL;
void Invalid(void *root, u32 *payload, ValueCallback callback, u32 *context) { payload[0] += callback(payload, context); }
#else
#if REJECT == 7
#ifdef FULL_FIRST
void Invalid(void *, u32 *, Callback2, u32 *) FULL;
void Invalid(void *, u32 *, Callback2, u32 *) __attribute__((mips_unsafe_manual_at_frame));
#else
void Invalid(void *, u32 *, Callback2, u32 *) __attribute__((mips_unsafe_manual_at_frame));
void Invalid(void *, u32 *, Callback2, u32 *) FULL;
#endif
#else
void Invalid(void *, u32 *, Callback2, u32 *) FULL;
#endif
void Invalid(void *root, u32 *payload, Callback2 callback, u32 *context)
{
#if REJECT == 1
    volatile u32 local[8];
    local[0] = context[0];
    payload[0] += local[0];
#elif REJECT == 5
    payload[0] = (u32)__builtin_sp() + 4u;
#elif REJECT == 6
    __asm__ volatile("nop");
#elif REJECT == 7
    payload[0] ^= context[0];
#else
#error Unsupported full-scratch refusal
#endif
}
#endif
#undef FULL
#else
#if defined(CTR_NATIVE) || defined(ORDINARY_CALL)
#define ADJACENT
#else
#define ADJACENT __attribute__((mips_adjacent_tail("Continue")))
#endif
#if REJECT == 11
register const u32 stackValue __asm__("$29");
#endif
static inline u32 Mix(u32 first, u32 second) { return first ^ second; }
void Continue(u32 *, u32, u32, u32);
void Extra(u32 *, u32, u32, u32);
#if REJECT == 5
void Begin(u32 *, u32, u32, u64) ADJACENT;
void Begin(u32 *out, u32 first, u32 second, u64 salt)
#else
void Begin(u32 *, u32, u32, u32) ADJACENT;
void Begin(u32 *out, u32 first, u32 second, u32 salt)
#endif
{
#ifdef OBSERVE_LO
    register u32 product __asm__("lo");
#else
    u32 product;
#endif
#if REJECT == 3
    u32 counter = first & 3u;
    while (counter != 0) { out[0] ^= counter; --counter; }
#elif REJECT == 4
    volatile u32 local[8];
    local[0] = first;
    out[0] += local[0];
#elif REJECT == 10
    u64 wide = ((u64)first << 32) | second;
    __asm__ volatile("" : : "r"(wide));
#endif
#if REJECT == 1
    Extra(out, first, second, (u32)salt);
#elif REJECT == 2
    if (first == 0) return;
#endif
    out[0] += first;
    if (second & 1u) out[0] ^= (u32)salt;
    product = first * second;
#if REJECT == 6
    __asm__ volatile("" : "=r"(product) : "0"(first));
#elif REJECT == 7
    __asm__ volatile("" : : "m"(out[0]));
#elif REJECT == 8
    __asm__ volatile("" : : "r"(product) : "memory");
#elif REJECT == 9
    __asm__ volatile("nop" : : "r"(product));
#elif REJECT == 11
    __asm__ volatile("" : : "r"(stackValue));
#elif defined(OBSERVE_LO)
    __asm__ volatile("" : : "l"(product));
#elif defined(OBSERVE_GPR)
    __asm__ volatile("" : : "r"(product));
#endif
    out[1] = product;
    Continue(out + 1, first + out[0], Mix(second, product), (u32)salt + 7u);
}
#ifndef OMIT_CONTINUE
void Continue(u32 *out, u32 first, u32 second, u32 salt)
{
    out[0] = first;
    out[1] = second;
    out[2] = salt;
    out[-1] += first ^ second ^ salt;
}
#endif
#undef ADJACENT
#endif
