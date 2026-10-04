/* The explicit builtin has ordinary signed-C negation value semantics. */
int sc_ternary_nonpositive(int x)
{
    return x <= 0 ? x : -x;
}

int bn_runtime(int x)
{
    return __builtin_mips_neg_si(x);
}

int bn_conditional(int x, int take)
{
    if (take)
        return __builtin_mips_neg_si(x);
    return x;
}

int bn_argument_once(int *value)
{
    return __builtin_mips_neg_si((*value)++);
}

int bn_assignment_overlap(int x)
{
    x = __builtin_mips_neg_si(x++);
    return x;
}
