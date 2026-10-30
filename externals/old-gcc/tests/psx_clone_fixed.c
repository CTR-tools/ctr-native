/* Ordinary ABI controls for fixed-frame epilogue cloning. */
extern void visit_a(int *);
extern void visit_b(int *);
extern void visit_c(int *);
volatile int observed;

void two_paths(int which, int *p)
{
  int a = p[0], b = p[1], c = p[2];
  if (which)
    {
      visit_a(p);
      p[3] = a; p[4] = b; p[5] = c;
      return;
    }
  visit_b(p);
  p[6] = a; p[7] = b; p[8] = c;
}

/* Void leaf: there is no framed epilogue to clone. */
void no_frame(void)
{
  ++observed;
}

/* Empty constraints have no CPU effect; zero-length RTL vectors are legal. */
void empty_constraints(int which, int *p)
{
  int value = p[0];
  __asm__ volatile ("" : : : "memory");
  if (which)
    { visit_a(p); p[1] = value; return; }
  visit_b(p);
  p[2] = value;
}

/* Value-return interfaces are outside the initial supported shape. */
int returns_value(int which, int *p)
{
  int value = p[0];
  if (which)
    { visit_a(p); return value; }
  visit_b(p);
  return value + 1;
}

/* Two potential shared-exit jumps exceed the one-clone prototype. */
void three_paths(int which, int *p)
{
  int value = p[0];
  if (which == 0)
    { visit_a(p); p[1] = value; return; }
  if (which == 1)
    { visit_b(p); p[2] = value; return; }
  visit_c(p);
  p[3] = value;
}

void dynamic_frame(int which, int *p)
{
  int *buffer = (int *)__builtin_alloca((unsigned int)(*p & 31) + 16);
  buffer[0] = p[0];
  if (which)
    { visit_a(buffer); p[1] = buffer[0]; return; }
  visit_b(buffer);
  p[2] = buffer[0];
}

void large_frame(int which, int *p)
{
  volatile int buffer[9000];
  buffer[0] = p[0];
  buffer[8999] = p[1];
  if (which)
    { visit_a(p); p[2] = buffer[8999]; return; }
  visit_b(p);
  p[3] = buffer[0];
}

/* A nonempty opaque template, but no hand-encoded CPU instruction. */
void opaque_effect(int which, int *p)
{
  int value = p[0];
  __asm__ volatile ("# opaque control" : : : "memory");
  if (which)
    { visit_a(p); p[1] = value; return; }
  visit_b(p);
  p[2] = value;
}

void variable_arguments(int which, int *p, ...)
{
  int value = p[0];
  if (which)
    { visit_a(p); p[1] = value; return; }
  visit_b(p);
  p[2] = value;
}
