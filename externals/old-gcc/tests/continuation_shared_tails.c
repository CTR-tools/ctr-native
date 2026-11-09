/* Independent C90 final-crossjump controls, no assembly or target addresses. */
extern unsigned ReadCell(const unsigned *cells, unsigned index);
extern void PublishReport(unsigned *state);
struct Notice { unsigned code, value; };

unsigned OtherJumpContinuation(struct Notice *notice, const unsigned *cells,
                                 unsigned count, int route)
{
    unsigned cursor, sum = 0;
    for (cursor = 0; cursor < count; cursor++) sum += ReadCell(cells, cursor);
    if (route == 1)
    {
        sum += ReadCell(cells, 0);
        notice->code = 17;
        notice->value = sum;
        cursor = 7;
        goto resumed;
    }
    if (route == 2)
    {
        sum ^= ReadCell(cells, 1);
        notice->code = 23;
        notice->value = sum;
        cursor = 7;
        goto resumed;
    }
    cursor = 0;
resumed:
    do {
        sum += ReadCell(cells, cursor);
        cursor++;
    } while (cursor < 9);
    return sum;
}

unsigned OrdinarySharedTail(unsigned *state, const unsigned *cells,
                              unsigned count, int route)
{
    unsigned cursor, sum = 0;
    for (cursor = 0; cursor < count; cursor++) sum += ReadCell(cells, cursor);
    if (route == 1)
    {
        sum += ReadCell(cells, 0);
        state[0] = 17;
        state[1] = sum;
        goto published;
    }
    if (route == 2)
    {
        sum ^= ReadCell(cells, 1);
        state[0] = 23;
        state[1] = sum;
        goto published;
    }
    state[0] = 0;
published:
    PublishReport(state);
    return sum;
}
