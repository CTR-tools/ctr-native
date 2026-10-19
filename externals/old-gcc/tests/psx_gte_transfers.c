typedef unsigned int u32;

register u32 psx_sxy2 __asm__("gte_sxy2");
register u32 psx_vxy0 __asm__("gte_vxy0");

u32 psx_read_sxy2(void)
{
    return psx_sxy2;
}

void psx_load_vxy0(const volatile u32 *input)
{
    psx_vxy0 = input[1];
}
