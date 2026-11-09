/* Synthetic compiler controls: every pressure value is actually consumed.
   A provider may overwrite input/log, so entry snapshots must survive it. */
#ifndef HOME
#define HOME(n) __attribute__((spill_home(n)))
#endif
#ifndef OFFSET
#define OFFSET 64
#endif
#ifndef SECOND
#define SECOND
#endif
#ifndef WIDE_CALL
#define WIDE_CALL 0
#endif
extern unsigned int disturb(unsigned int *, unsigned char *);
extern unsigned int disturb6(unsigned int *, unsigned char *, int, int, int, int);
extern unsigned int disturb_short(short *, unsigned int *);

unsigned int real_snapshots(unsigned int *input)
{
  unsigned char log[16];
  unsigned int a0 = input[0];
  unsigned int a1 = input[1];
  unsigned int a2 = input[2], a3 = input[3], a4 = input[4], a5 = input[5];
  unsigned int a6 = input[6], a7 HOME(OFFSET) = input[7];
  unsigned int a8 SECOND = input[8], a9 = input[9];
  unsigned int a10 = input[10], a11 = input[11], a12 = input[12], a13 = input[13];
  unsigned int result;
  log[0] = 7;
#if WIDE_CALL
  result = disturb6(input, log, 1, 2, 3, 4);
#else
  result = disturb(input, log);
#endif
  return result + log[0] + a0 + a1 * 3U + a2 * 5U + a3 * 7U + a4 * 9U
    + a5 * 11U + a6 * 13U + a7 * 15U + a8 * 17U + a9 * 19U
    + a10 * 21U + a11 * 23U + a12 * 25U + a13 * 27U;
}

unsigned int real_short_snapshots(short *input)
{
  short a0 = input[0];
  short a1 = input[1], a2 = input[2], a3 = input[3], a4 = input[4];
  short a5 = input[5], a6 = input[6], a7 HOME(96) = input[7], a8 = input[8];
  short a9 = input[9], a10 = input[10], a11 = input[11];
  short a12 = input[12], a13 = input[13];
  unsigned int checksum, result;
  checksum = a12 + a13;
  result = disturb_short(input, &checksum);
  return result + checksum + a0 + a1 * 3U + a2 * 5U + a3 * 7U + a4 * 9U
    + a5 * 11U + a6 * 13U + a7 * 15U + a8 * 17U + a9 * 19U
    + a10 * 21U + a11 * 23U + a12 * 25U + a13 * 27U;
}

unsigned int register_resident(unsigned int input)
{
  unsigned int value HOME(128) = input + 7U;
  return value ^ 3U;
}
unsigned int eliminated(unsigned int input)
{
  unsigned int dead HOME(128) = 42;
  return input + dead;
}
unsigned int plain_after(unsigned int input) { return input * 5U + 3U; }

#ifdef NATIVE_CHECK
#include <stdio.h>
unsigned int disturb(unsigned int *p, unsigned char *log)
{ p[0] ^= 0xdeadbeefU; log[0] += 3; return 17; }
unsigned int disturb6(unsigned int *p, unsigned char *log, int a, int b, int c, int d)
{ return disturb(p, log) + a + b + c + d - 10; }
unsigned int disturb_short(short *p, unsigned int *sum)
{ p[0] ^= 0xffff; *sum += 11; return 19; }
int main(void)
{
  unsigned int input[14], expected, actual, seed;
  short narrow[14];
  int i;
  for (seed = 0; seed != 1000; seed++)
    {
      for (i = 0; i != 14; i++) input[i] = seed * 1103515245U + i;
      expected = 27;
      for (i = 0; i != 14; i++) expected += input[i] * (2U * i + 1U);
      actual = real_snapshots(input);
      if (actual != expected) return 1;
      for (i = 0; i != 14; i++) narrow[i] = seed * 47U + i;
      expected = 19U + narrow[12] + narrow[13] + 11U;
      for (i = 0; i != 14; i++) expected += narrow[i] * (2U * i + 1U);
      if (real_short_snapshots(narrow) != expected) return 2;
    }
  puts("1000 actual native C snapshot checks passed");
  return 0;
}
#endif
