#ifndef GH
#define GH(r) __attribute__((global_register_hint(r)))
#endif
typedef unsigned int U;
extern U observe(U *, U);
extern U observe_other(U *, U);
U plain_before(U input) { return (input + 7) ^ 3; }
U nested_owners(U *input, U select)
{
  U outer GH("21") = input[0];
  U result;
  {
    U inner GH("22") = input[1];
    if (select & 1) result = observe(input, select);
    else result = observe_other(input, select);
    result += inner;
  }
  return result + outer;
}
U pointer_owner(U **input, U select)
{
  U *cursor GH("23") = input[0];
  if (select & 1) select = observe(cursor, select);
  else select = observe_other(cursor, select);
  return cursor[0] + select;
}
U narrow_owner(U *input, U select)
{
  unsigned short value GH("22") = (unsigned short)input[0];
  __asm__ ("" : "+r"(value));
  if (select & 1) select = observe(input, select);
  else select = observe_other(input, select);
  __asm__ ("" : "+r"(value));
  return value + select;
}
static __inline__ U hinted_inline(U *input, U select)
{
  U kept GH("21") = input[0];
  if (select & 1) select = observe(input, select);
  else select = observe_other(input, select);
  return kept + select;
}
U inline_twice(U *input, U select)
{
  U first = hinted_inline(input, select);
  U second = hinted_inline(input + 1, select + 2);
  return first ^ second;
}
U zero_globals(U input) { U dead GH("21") = 6; return input + dead; }
U local_only(U *input) { U value GH("21") = input[0]; return value + 1; }
U gnu_nested(U *input, U select)
{
  U inner(U value)
  {
    U kept GH("20") = input[0];
    if (value & 1) value = observe(input, value);
    else value = observe_other(input, value);
    return kept + value;
  }
  return inner(select);
}
U plain_after(U input) { return (input + 7) ^ 3; }

U local_conflict(U *input, U select)
{
  register U occupied __asm__("$21") = input[0];
  U candidate GH("21") = input[1];
  U result;
  __asm__ volatile ("" : "+r"(occupied));
  if (select & 1) result = observe(input, select);
  else result = observe_other(input, select);
  __asm__ volatile ("" : "+r"(occupied) : "r"(candidate));
  return result + candidate + occupied;
}
U legal_caller_color(U *input, U select)
{
  U candidate GH("8") = input[0];
  if (select & 1) __asm__ volatile ("# left" : "+r"(candidate));
  else __asm__ volatile ("# right" : "+r"(candidate));
  return candidate + select;
}

unsigned int preferred_roles(unsigned int *input, unsigned int count)
{
  unsigned int snapshot GH("$21") = input[0];
  unsigned int cursor GH("$17");
  unsigned int total = 0;
  for (cursor = 0; cursor < count; ++cursor)
    total += observe(input, cursor) ^ snapshot;
  return total + snapshot;
}


unsigned int unavailable_callcolor(unsigned int *input, unsigned int select)
{
  unsigned int snapshot GH("$2") = input[0];
  unsigned int result;
  if (select & 1)
    result = observe(input, select);
  else
    result = observe_other(input, select);
  return snapshot ^ result;
}

unsigned int live_color_conflict(unsigned int *input, unsigned int select)
{
  unsigned int first GH("$21") = input[0];
  unsigned int second GH("$21") = input[1];
  unsigned int result;
  if (select & 1)
    result = observe(input, select);
  else
    result = observe_other(input, select);
  return (first ^ result) + second;
}
