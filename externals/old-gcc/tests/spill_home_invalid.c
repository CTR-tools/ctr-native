#ifndef CASE
#define CASE 0
#endif
#define HOME(n) __attribute__((spill_home(n)))
#if CASE == 1
int global HOME(64);
#elif CASE == 2
int parameter(int a HOME(64)) { return a; }
#elif CASE == 3
typedef int Bad HOME(64);
#elif CASE == 4
static inline int inlined(int input) { int a HOME(64) = input + 3; return a; }
int caller(int input) { return inlined(input); }
#else
extern void touch(int *);
int invalid(int input)
{
#if CASE == 5
  int a HOME(input) = input;
#elif CASE == 6
  int a HOME(-1) = input;
#elif CASE == 7
  int a HOME(32760) = input;
#elif CASE == 8
  int a HOME(64) HOME(72) = input;
#elif CASE == 9
  volatile int a HOME(64) = input;
#elif CASE == 10
  static int a HOME(64);
#elif CASE == 11
  long long a HOME(64) = input;
#elif CASE == 12
  register int a __asm__("$16") HOME(64) = input;
#elif CASE == 13
  int a HOME(64) = input; touch(&a);
#elif CASE == 14
  struct { int a; int b; } a HOME(64);
  a.a = input; return a.a;
#elif CASE == 15
  int a __attribute__((spill_home())) = input;
#elif CASE == 16
  int a __attribute__((spill_home(64, 72))) = input;
#else
  int a HOME(64) = input + 1;
#endif
#if CASE != 14
  return a;
#endif
}
#endif
