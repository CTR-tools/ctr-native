typedef unsigned short u16;
typedef unsigned int u32;

register const unsigned char *ram_face __asm__("$11");
register u32 ram_vertices __asm__("$12");

void psx_normal_ram_lhu_delay(void)
{
    register u32 count __asm__("$13");
    register u32 first __asm__("$3");
    register u32 pending __asm__("$8");
    register u32 following __asm__("$9");
    __asm__("" : "=r"(count));
    first = *(const u16 *)ram_face;
    pending = *(const volatile u16 *)(ram_face + 2);
    if (count == 0)
        return;
    following = *(const u32 *)(ram_face + 4);
    first += ram_vertices;
    pending += ram_vertices;
    __asm__ volatile("" : : "r"(first), "r"(pending), "r"(following));
}
