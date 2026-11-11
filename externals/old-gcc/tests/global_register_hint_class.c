#ifndef GH
#define GH(r) __attribute__((global_register_hint(r)))
#endif
typedef unsigned int U;
U restricted_class(U *input, U select)
{
  U value GH("21") = input[0];
  __asm__ ("" : "+c"(value));
  if (select & 1) __asm__ volatile ("# left" : "+c"(value));
  else __asm__ volatile ("# right" : "+c"(value));
  return value + select;
}
