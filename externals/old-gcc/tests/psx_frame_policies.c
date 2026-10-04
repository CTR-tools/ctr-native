struct FrameInput
{
    struct FrameInput *next;
    int value;
};

extern void frame_consume(int);
extern unsigned int frame_call(unsigned int);
extern void frame_escape(void *);

#if defined(TEST_ALIGNED_SLOT)
void frame_aligned_slot(void)
{
    /* DFmode reaches the allocator with a genuine eight-byte alignment. */
    double local;
    frame_escape(&local);
}
#elif defined(TEST_DYNAMIC_FRAME)
void frame_dynamic(unsigned int count)
{
    frame_escape(__builtin_alloca(count));
}
#elif defined(TEST_ORDER_EXCLUDED)
struct FrameAggregate { unsigned int first, second, third; };

unsigned int frame_order_large(unsigned int seed)
{
    unsigned char buffer[40000];
    buffer[0] = (unsigned char)seed;
    buffer[39999] = (unsigned char)(seed + 1u);
    frame_escape(buffer);
    return buffer[0] + buffer[39999];
}
unsigned int frame_order_aggregate(struct FrameAggregate value, unsigned int seed)
{
    unsigned int first;
    frame_escape(&value);
    first = frame_call(value.first);
    return first + value.second + value.third + seed;
}
void frame_order_variadic(unsigned int first, ...)
{
    frame_consume(first);
}
#elif defined(TEST_ORDER_FPU)
extern double frame_fpu_call(double);
double frame_order_fpu(double input)
{
    double first;
    double second;
    first = frame_fpu_call(input);
    second = frame_fpu_call(input);
    return first + second;
}
#elif defined(TEST_ORDER_PIC)
extern volatile unsigned int frame_external;
unsigned int frame_order_pic(unsigned int input)
{
    unsigned int value;
    value = frame_call(input);
    return value + frame_external;
}
#else
/* Incoming pointers retain their real o32 argument-register origins. */
void frame_a0(struct FrameInput *p) { frame_consume(p->value); }
void frame_a1(int a, struct FrameInput *p) { frame_consume(p->value); }
void frame_a2(int a, int b, struct FrameInput *p) { frame_consume(p->value); }
void frame_a3(int a, int b, int c, struct FrameInput *p) { frame_consume(p->value); }

/* Only the first pointer read has incoming-argument provenance. */
void frame_indirect(struct FrameInput *p) { frame_consume(p->next->value); }
void frame_integer(unsigned long bits)
{
    frame_consume(((struct FrameInput *)bits)->value);
}
void frame_changed(struct FrameInput *p, int offset)
{
    p += offset;
    frame_consume(p->value);
}
void frame_volatile(volatile struct FrameInput *p) { frame_consume(p->value); }

void frame_basic_asm(struct FrameInput *p)
{
    __asm__("");
    frame_consume(p->value);
}
void frame_memory_asm(struct FrameInput *p)
{
    __asm__ volatile("" : : : "memory");
    frame_consume(p->value);
}

unsigned int frame_leaf(unsigned int value) { return (value + 3u) ^ 0x12u; }

/* Real calls, several saved GPRs and two scalar-return paths. */
unsigned int frame_branch(unsigned int a, unsigned int b,
                          unsigned int c, unsigned int selector)
{
    unsigned int first;
    unsigned int second;
    first = frame_call(a);
    second = frame_call(b);
    if (selector & 1u)
        return first + c;
    return second ^ c;
}
#endif
