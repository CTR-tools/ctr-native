/* Standalone compiler-output controls; no hardware execution is claimed. */
extern int Submit(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
struct State { unsigned int bits; unsigned short delay; };

int UpdateOr(unsigned int sample, struct State *p)
{
    unsigned int value = p->bits;
    int result;
    unsigned short delay __attribute__((local_register_hint("$3")));
    if (sample == 0)
        return value;
    __asm__ volatile("" : : "r"(sample));
    delay = 91;
    value |= 0x24;
    p->bits = value;
    p->delay = delay;
    result = Submit(sample, 13, 29, 53, sample);
    return result + p->delay;
}

int UpdateAdd(unsigned int sample, struct State *p)
{
    unsigned int value = p->bits;
    int result;
    unsigned short delay __attribute__((local_register_hint("$3")));
    if (sample == 0)
        return value;
    __asm__ volatile("" : : "r"(sample));
    delay = 47;
    value += 113;
    p->bits = value;
    p->delay = delay;
    result = Submit(sample, 31, 43, 59, sample);
    return result + p->delay;
}

unsigned int NoCall(struct State *p, unsigned int sample)
{
    unsigned int value = p->bits;
    value ^= sample;
    p->bits = value;
    return value + 1;
}

int ArgumentUpdate(unsigned int sample)
{
    sample |= 0x48;
    return Submit(sample, 23, 41, 67, 19);
}

int MemoryInput(struct State *p, unsigned int sample, volatile unsigned int *q)
{
    unsigned int value = p->bits;
    __asm__ volatile("" : : "r"(sample));
    value |= *q;
    p->bits = value;
    return Submit(sample, 37, 61, 71, 11);
}

int AsmUpdate(struct State *p, unsigned int sample)
{
    unsigned int value = p->bits;
    __asm__("or %0,%0,%1" : "+r"(value) : "r"(sample));
    p->bits = value;
    return Submit(sample, 17, 47, 73, 5);
}

int DivideUpdate(struct State *p, unsigned int sample, unsigned int divisor)
{
    unsigned int value = p->bits;
    value /= divisor;
    p->bits = value;
    return Submit(sample, 19, 31, 79, 3);
}

int DependentArgument(struct State *p, unsigned int sample)
{
    unsigned int value = p->bits;
    value += 17;
    p->bits = value;
    return Submit(value, sample, 83, 97, 23);
}
