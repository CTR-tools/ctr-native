extern volatile unsigned loop_state;
extern unsigned loop_step(unsigned);

void loop_invariant(unsigned *out, unsigned count, unsigned a, unsigned b)
{
    unsigned i;
    for (i = 0; i < count; i++)
        out[i] = a * b + i;
}

void loop_effects(unsigned *out, unsigned count)
{
    unsigned i;
    for (i = 0; i < count; i++)
    {
        loop_state = loop_state + i;
        out[i] = loop_step(i);
    }
}
