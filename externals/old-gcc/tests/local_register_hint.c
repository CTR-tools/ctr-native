#ifndef HINT
#define HINT(name) __attribute__((local_register_hint(name)))
#endif
#ifndef HINT_CASE
#define HINT_CASE 0
#endif

#if HINT_CASE == 0
typedef struct FourBytes { unsigned char bytes[4]; } FourBytes;

/* Artificial pressure isolates the existing disjoint-block reload exemption. */
void hinted_copy(int select, FourBytes *destination,
                 const FourBytes *source, const int *input)
{
  if (select)
    *destination = *source;
  else
    {
      int a HINT("10") = input[0] + 1;
      int b HINT("11") = input[1] + 1;
      __asm__ volatile ("" : "+r"(a), "+r"(b));
      __asm__ volatile ("" : : "r"(a), "r"(b));
    }
  __asm__ volatile ("" : : : "$2", "$3", "$4", "$5", "$6", "$7",
                     "$8", "$9", "$12", "$13", "$16", "$17", "$18", "$19",
                     "$20", "$21", "$22", "$23", "$24", "$25");
}

void unavailable_hint(const int *input)
{
  register int occupied __asm__("$10") = input[0];
  int candidate HINT("10") = input[1];
  __asm__ volatile ("" : "+r"(occupied));
  __asm__ volatile ("" : "+r"(candidate) : "r"(occupied));
}

void hinted_pointer(int *const *input)
{
  int *value HINT("10") = input[0];
  __asm__ volatile ("" : "+r"(value));
  __asm__ volatile ("" : : "r"(value));
}

void coalesced_conflict(const int *input)
{
  int a HINT("10") = input[0];
  int b HINT("11");
  __asm__ volatile ("" : "+r"(a));
  b = a + 1;
  __asm__ volatile ("" : : "r"(b));
}

static inline int hinted_inline(int input)
{
  int value HINT("11") = input + 2;
  __asm__ ("" : "+r"(value));
  return value;
}

int inline_twice(int input)
{
  int first = hinted_inline(input);
  int second = hinted_inline(input + 4);
  __asm__ volatile ("" : : "r"(first), "r"(second));
  return first ^ second;
}

int plain_after_hints(int input)
{
  int value = input + 7;
  __asm__ ("" : "+r"(value));
  return value;
}

int hard_register_unchanged(int input)
{
  register int value __asm__("$14") = input + 3;
  __asm__ volatile ("" : "+r"(value));
  return value;
}

int stronger_return_preference(const int *input)
{
  int value HINT("10") = input[0] + 1;
  __asm__ volatile ("" : : "r"(value));
  return value;
}

int optimized_away(int input)
{
  int value HINT("10") = 6;
  return value + input;
}
#elif HINT_CASE == 1
int global HINT("10");
#elif HINT_CASE == 2
int parameter(int value HINT("10")) { return value; }
#else
void invalid(const int *input)
{
#if HINT_CASE == 3
  int value HINT("not-a-register") = input[0];
#elif HINT_CASE == 4
  int value HINT("29") = input[0];
#elif HINT_CASE == 5
  int value HINT("memory") = input[0];
#elif HINT_CASE == 6
  long long value HINT("10") = input[0];
#elif HINT_CASE == 7
  register int value __asm__("$10") HINT("11") = input[0];
#elif HINT_CASE == 8
  int value __attribute__((local_register_hint(3))) = input[0];
#elif HINT_CASE == 9
  int value __attribute__((local_register_hint())) = input[0];
#elif HINT_CASE == 10
  int value __attribute__((local_register_hint("10", "11"))) = input[0];
#elif HINT_CASE == 11
  int value HINT("10") HINT("11") = input[0];
#elif HINT_CASE == 12
  volatile int value HINT("10") = input[0];
#elif HINT_CASE == 13
  int value HINT("") = input[0];
#elif HINT_CASE == 14
  int value HINT("64") = input[0];
#elif HINT_CASE == 15
  static int value HINT("10");
#endif
  __asm__ volatile ("" : "+r"(value));
}
#endif
