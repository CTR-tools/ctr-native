typedef unsigned char u8;
typedef unsigned int u32;
extern int consume(u32);
extern u32 consume6(u32,u32,u32,u32,u32,u32);
#if defined(TEST_REGISTER_VIEWS)
extern u8 sp_view[8] asm("20($29)") __attribute__((section(".data")));
extern u8 gp_view[8] asm("20($28)") __attribute__((section(".data")));
int static_sp(u32 f) { int x=sp_view[f&3]; return consume(f)+x; }
int static_gp(u32 f) { int x=gp_view[f&3]; return consume(f)+x; }
#else
extern u8 bytes[8], small[4], unknown[];
extern u8 alias[8] asm("storage+4") __attribute__((section(".data")));
extern volatile u8 changed[8];
extern u8 *indirect;
extern u32 *published;
int static_plain(u32 f) { int x=bytes[f&3]; return consume(f)+x; }
int static_alias(u32 f) { int x=alias[f&3]; return consume(f)+x; }
int static_local(u32 f) { static u8 local[8]; int x=local[f&3]; return consume(f)+x; }
int static_extent(u32 f) { int x=small[f&7]; return consume(f)+x; }
int static_unknown(u32 f) { int x=unknown[f&3]; return consume(f)+x; }
int static_indirect(u32 f) { int x=indirect[f&3]; return consume(f)+x; }
int static_volatile(u32 f) { int x=changed[f&3]; return consume(f)+x; }
int static_asm(u32 f)
{ int x; asm volatile("" : : : "memory"); x=bytes[f&3]; return consume(f)+x; }
int static_exposed(u32 f)
{ u32 local=f; int x; published=&local; x=bytes[f&3]; return consume(f)+x+local; }
int static_dynamic(u32 f)
{ int x; published=__builtin_alloca(f); x=bytes[f&3]; return consume(f)+x; }
int static_called(u32 f)
{ int y=consume(f); int x=bytes[f&3]; return consume(f)+x+y; }
int static_branch(u32 f)
{ int x; if (f&16) return consume(f); x=bytes[f&3]; return consume(f)+x; }
/* All sixteen ordinary values precede the bounded read and survive calls. */
#define BIT(n) u32 p##n=(f>>(n))&1;
u32 static_reload(u32 f)
{
    BIT(0) BIT(1) BIT(2) BIT(3) BIT(4) BIT(5)
    BIT(6) BIT(7) BIT(8) BIT(9) BIT(10) BIT(11)
    BIT(12) BIT(13) BIT(14) BIT(15)
    u32 x=bytes[(p0+p1+p2+p3+p4+p5+p6+p7+p8+p9+p10+p11+p12+p13+p14+p15)&3];
    u32 result=consume6(f,f,f,f,f,f);
    return result+consume6(p0,p1,p2,p3,p4,p5)+consume6(p6,p7,p8,p9,p10,p11)+consume6(p12,p13,p14,p15,x,f);
}
/* Reuse these numeric homes in the next function, never the reload identities. */
u32 static_source(u32 f)
{
    u32 local[2];
    u32 x;
    local[0]=f; local[1]=f+1;
    x=bytes[f&3];
    return consume6(local[0],local[f&1],f,f,f,x);
}
#endif
