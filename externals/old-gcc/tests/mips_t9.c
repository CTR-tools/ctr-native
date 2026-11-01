/* Ordinary pseudo operands, no permanent T9 binding; S0 is a usage-flag witness. */
typedef unsigned u32;
extern u32 sink (u32);
#ifdef ENABLE_T9
#define CUE(value) __asm__ ("" : "+c" (value))
#else
#define CUE(value) ((void)(value))
#endif
u32 leaf (u32 x) { CUE(x); return x + 1; }
u32 across (u32 x) { CUE(x); sink(7); return x + 3; }
u32 across_twice (u32 x) { CUE(x); sink(7); CUE(x); sink(8); return x + 3; }
u32 pointer (u32 *p) { CUE(p); sink(7); return *p; }
u32 pressure (u32 a,u32 b,u32 c,u32 d,u32 e,u32 f,u32 g,u32 h)
{ u32 x=a^b^c^d; CUE(x); return sink(a+b+c+d+e+f+g+h)+x+a+b+c+d+e+f+g+h; }
extern volatile u32 t9_state;
u32 effects (u32 x) { CUE(x); t9_state=x; sink(x); return t9_state+x; }
u32 requested_s0 (u32 x)
{ register u32 value __asm__("$16")=x; __asm__("" : "+r"(value)); return value+3; }
#if NEGATIVE == 1
unsigned long long wide (unsigned long long x) { CUE(x); return x; }
#elif NEGATIVE == 2
u32 conflicting (u32 x) { __asm__("" : "+c"(x) : : "$25"); return x; }
#elif NEGATIVE == 3
u32 two (u32 x,u32 y) { __asm__("" : "+c"(x), "+c"(y)); return x+y; }
#endif
