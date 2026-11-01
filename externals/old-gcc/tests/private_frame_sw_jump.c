/* Generic policy witnesses, not game algorithms or binary-output templates. */
extern void use_pointer(volatile int *);
extern void call_out(void);
extern volatile int *published;

void Zero(int limit)
{
    volatile int counter;
    for (counter = 0; counter < limit; ++counter) { }
}
void Value(int limit, int initial)
{
    volatile int counter;
    for (counter = initial; counter < limit; ++counter) { }
}
void Offset(int limit)
{
    volatile int pad[3];
    volatile int counter;
    pad[0] = 7; pad[1] = 9; pad[2] = 11;
    for (counter = 0; counter < limit; ++counter) pad[1] += 3;
}
void BeforeDevice(volatile int *device, int limit)
{
    volatile int counter;
    *device = 17;
    for (counter = 0; counter < limit; ++counter) { }
}
void Half(int limit)
{
    volatile short counter;
    for (counter = 0; counter < limit; ++counter) { }
}
void Byte(int limit)
{
    volatile unsigned char counter;
    for (counter = 0; counter < limit; ++counter) { }
}
void NonStack(volatile int *counter, int limit)
{
    for (*counter = 0; *counter < limit; ++*counter) { }
}
void Exposed(int limit)
{
    volatile int counter;
    published = &counter;
    for (counter = 0; counter < limit; ++counter) { }
}
void Dynamic(int size, int limit)
{
    volatile int *buffer = __builtin_alloca(size);
    volatile int counter;
    *buffer = size;
    for (counter = 0; counter < limit; ++counter) *buffer += counter;
}
void WithCall(int limit)
{
    volatile int counter;
    for (counter = 0; counter < limit; ++counter) call_out();
}
void Asm(int limit)
{
    volatile int counter;
    __asm__ volatile ("" : : : "memory");
    for (counter = 0; counter < limit; ++counter) { }
}
int Conditional(int test, int value)
{
    volatile int local;
    local = value;
    if (test) return local + 1;
    return local - 1;
}
void ReturnStore(int value)
{
    volatile int local;
    local = value;
}
void Computed(void *target, int value)
{
    volatile int local;
    local = value;
    goto *target;
}
void ConditionalEntry(int test, int limit)
{
    volatile int counter;
    if (!test) return;
    for (counter = 0; counter < limit; ++counter) { }
}
void SourceStack(int limit)
{
    register int value __asm__("$29");
    volatile int counter;
    for (counter = value; counter < limit; ++counter) { }
}
void LargeFrame(int limit)
{
    volatile int local[9000];
    local[8999] = limit;
    for (local[0] = 0; local[0] < limit; ++local[0]) { }
}
/* Deliberately unsupported hard-SP addresses: compile-only, never executed. */
void NegativeAddress(int limit)
{
    register volatile int *base __asm__("$29"); volatile int local;
    local = limit;
    for (base[-1] = 0; base[-1] < limit; ++base[-1]) ++local;
}
void OutsideAddress(int limit)
{
    register volatile int *base __asm__("$29"); volatile int local;
    local = limit;
    for (base[32] = 0; base[32] < limit; ++base[32]) ++local;
}
void OddAddress(int limit)
{
    register volatile char *base __asm__("$29"); volatile int local;
    local = limit;
    for (*(volatile int *)(base + 1) = 0; *(volatile int *)(base + 1) < limit;
         ++*(volatile int *)(base + 1)) ++local;
}

#ifdef MANUAL
void __attribute__((mips_unsafe_manual_at_frame)) Manual(int limit)
{
    volatile int counter;
    for (counter = 0; counter < limit; ++counter) { }
}
#endif
