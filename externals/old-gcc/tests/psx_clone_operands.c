/* Fixed-frame controls with actual ASM operand and constraint vectors. */
extern void visit_a(int *);
extern void visit_b(int *);

/* Empty text with real inputs, a tied output and a memory clobber. */
void operand_constraints(int which, int *p)
{
  int value = p[0], other = p[1];
  __asm__ volatile ("" : "+r" (value) : "r" (which), "r" (p) : "memory");
  if (which)
    {
      visit_a(p);
      p[2] = value; p[3] = other;
      return;
    }
  visit_b(p);
  p[4] = value; p[5] = other;
}

/* Same operands/effects, but nonempty opaque text must still exclude it. */
void opaque_operand_constraints(int which, int *p)
{
  int value = p[0], other = p[1];
  __asm__ volatile ("# opaque operand control"
                    : "+r" (value) : "r" (which), "r" (p) : "memory");
  if (which)
    {
      visit_a(p);
      p[2] = value; p[3] = other;
      return;
    }
  visit_b(p);
  p[4] = value; p[5] = other;
}
