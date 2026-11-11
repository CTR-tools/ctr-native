#ifndef CASE
#define CASE 0
#endif
#define GH(r) __attribute__((global_register_hint(r)))
#define LH(r) __attribute__((local_register_hint(r)))
extern void escape(int *);
#if CASE == 1
int global GH("21");
#elif CASE == 2
int formal(int value GH("21")) { return value; }
#elif CASE == 24
register int occupied __asm__("$21");
int reserved(const int *input) { int value GH("21") = input[0]; return value; }
#else
void invalid(const int *input)
{
#if CASE == 3
  int value GH("not-a-register") = input[0];
#elif CASE == 4
  int value GH("29") = input[0];
#elif CASE == 5
  int value GH("memory") = input[0];
#elif CASE == 6
  long long value GH("21") = input[0];
#elif CASE == 7
  register int value __asm__("$21") GH("22") = input[0];
  __asm__ volatile ("" : "+r"(value));
#elif CASE == 8
  int value __attribute__((global_register_hint(3))) = input[0];
#elif CASE == 9
  int value __attribute__((global_register_hint())) = input[0];
#elif CASE == 10
  int value __attribute__((global_register_hint("21", "22"))) = input[0];
#elif CASE == 11
  int value GH("21") GH("22") = input[0];
#elif CASE == 12
  volatile int value GH("21") = input[0];
#elif CASE == 13
  int value GH("") = input[0];
#elif CASE == 14
  int value GH("64") = input[0];
#elif CASE == 15
  static int value GH("21");
#elif CASE == 16
  extern int value GH("21");
#elif CASE == 17
  struct Four { int word; } value GH("21");
#elif CASE == 18
  float value GH("21") = 1.0f;
#elif CASE == 19
  int value GH("21") = input[0];
  escape(&value);
#elif CASE == 20
  int value GH("21") GH("21") = input[0];
#elif CASE == 21
  int value GH("21") LH("22") = input[0];
#elif CASE == 22
  int value LH("22") GH("21") = input[0];
#elif CASE == 23
  int value GH("30") = input[0];
#endif
  (void)input;
}
#endif
