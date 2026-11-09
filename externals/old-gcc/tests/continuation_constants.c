/* Independent C90 controls. Valid arrays/providers; Publish count <=65535,
   Restart count >=9. These are compiler-output controls, not MIPS execution. */
struct QueueState { short ready, page, owner; };
extern unsigned ReadCell(const unsigned *cells, unsigned index);

unsigned PublishAndScan(struct QueueState *state, const short *owners,
                        const unsigned *cells, unsigned count,
                        unsigned selected, int publish)
{
    unsigned short cursor;
    unsigned sum = 0;
    for (cursor = 0; cursor < count; cursor++)
        sum += ReadCell(cells, cursor);
    if (publish)
    {
        short owner = owners[selected];
        state->ready = 1;
        state->page = 0;
        state->owner = owner;
        cursor = 0;
        goto read_fixed;
    }
    cursor = 0;
read_fixed:
    do {
        sum += ReadCell(cells, cursor);
        cursor++;
    } while (cursor < 3);
    return sum;
}

unsigned ReportAndRestart(unsigned *report, const unsigned *cells,
                          unsigned count, int restart)
{
    unsigned cursor;
    unsigned checksum = 0;
    for (cursor = 0; cursor < count; cursor++)
        checksum ^= ReadCell(cells, cursor);
    if (restart)
    {
        unsigned measured = cells[count - 1];
        report[0] = count;
        report[1] = checksum;
        report[2] = measured;
        cursor = 7;
        goto suffix;
    }
    cursor = 7;
suffix:
    do {
        checksum += ReadCell(cells, cursor);
        cursor++;
    } while (cursor < 9);
    return checksum;
}

/* C90 source-level negative controls; actual RTL scope is audited separately. */
extern unsigned ReadCell(const unsigned *cells, unsigned index);
extern void PublishReport(unsigned *state);

unsigned TargetWriteFirst(unsigned *state, const unsigned *cells,
                           unsigned selected, unsigned value, int branch)
{
    if (branch) { value = 7; state[0] = value; goto joined; }
    value = 11;
joined:
    value = cells[selected];
    return value + state[0];
}

unsigned SourceOverwriteFirst(unsigned *state, const unsigned *cells,
                               unsigned selected, unsigned value, int branch)
{
    if (branch)
    {
        value = 7;
        state[0] = value;
        value = cells[selected];
        state[1] = value;
        goto joined;
    }
    value = 11;
joined:
    return value + state[0];
}

unsigned TargetCall(unsigned *state, const unsigned *cells,
                     unsigned selected, unsigned value, int branch)
{
    if (branch) { state[0] = cells[selected]; value = 7; goto joined; }
    value = 11;
joined:
    PublishReport(state);
    return value + state[0];
}

unsigned TargetControlSplit(unsigned *state, const unsigned *cells,
                             unsigned selected, unsigned value, int branch)
{
    if (branch) { state[0] = cells[selected]; value = 7; goto joined; }
    value = 11;
joined:
    if (cells[0]) state[2] = selected;
    return value + state[0];
}

unsigned ConditionalEnd(unsigned *state, const unsigned *cells,
                         unsigned selected, unsigned value, int branch)
{
    state[0] = cells[selected];
    if (branch) value = 7;
    return value + state[0];
}

unsigned LoopControl(const unsigned *cells, unsigned count)
{
    unsigned cursor, total = 0;
    for (cursor = 0; cursor < count; cursor++) total += ReadCell(cells, cursor);
    return total;
}

unsigned ExpensiveMaterialization(unsigned *state, const unsigned *cells,
                                    unsigned selected, unsigned value, int branch)
{
    if (branch) { state[0] = cells[selected]; value = 0x12345678u; goto joined; }
    value = 11;
joined:
    return value + state[0];
}
