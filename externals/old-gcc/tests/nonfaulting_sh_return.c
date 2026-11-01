typedef unsigned short u16;
typedef unsigned char u8;
typedef unsigned int u32;

extern volatile u16 symbolic;
extern void Provider(void);

void StoreLow(volatile u16 *p, u16 value) { p[1] = value; }
void StoreBare(volatile u16 *p, u16 value) { p[0] = value; }
void StoreZero(volatile u16 *p) { p[2] = 0; }
void StoreTwo(volatile u16 *p, u16 a, u16 b) { p[0] = a; p[1] = b; }
void StoreNegative(volatile u16 *p, u16 value) { p[-2] = value; }
void ByteStore(volatile u8 *p, u8 value) { p[1] = value; }
void WordStore(volatile u32 *p, u32 value) { p[1] = value; }
void PlainStore(u16 *p, u16 value) { p[1] = value; }
void SymbolStore(u16 value) { symbolic = value; }
void OddStore(volatile u8 *p, u16 value) { *(volatile u16 *)(p + 1) = value; }
int StoreThenAlu(volatile u16 *p, u16 value, int input) { *p = value; return input + 1; }
void StoreThenBarrier(volatile u16 *p, u16 value) { *p = value; __asm__ volatile("" : : : "memory"); }
void StoreBeforeCall(volatile u16 *p, u16 value) { *p = value; Provider(); }
void FramedStore(volatile u16 *p, u16 value) { volatile u16 local = value; *p = local; }
void ConditionalStore(volatile u16 *p, u16 value, int condition) { if (condition) *p = value; }
void ReturnAddressBase(volatile u16 *p, u16 value)
{
	register volatile u16 *base __asm__("$31") = p;
	*base = value;
}
void StackBase(u16 value)
{
	register volatile u16 *base __asm__("$29");
	base[0] = value;
}
