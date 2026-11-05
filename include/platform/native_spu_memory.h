#ifndef CTR_NATIVE_SPU_MEMORY_H
#define CTR_NATIVE_SPU_MEMORY_H

/* HOWL stores runtime addresses in 16-byte units so its u16 fields span 1 MiB.
   Sample sizes in KART.HWL remain in 8-byte units. Based on ctr-native-ap's
   native_spu_memory.h. The final 8 KiB remain reserved as in retail. */
enum
{
	CTR_SPU_BYTES = 1024 * 1024,
	CTR_SPU_CEILING = CTR_SPU_BYTES - 0x2000,
	CTR_SPU_ADDR_SHIFT = 4,
	CTR_SPU_SAMPLE_SIZE_SHIFT = 3,
};

#define CTR_SPU_UNITS(bytes) ((bytes) >> CTR_SPU_ADDR_SHIFT)
#define CTR_SPU_BYTES_AT(units) ((units) << CTR_SPU_ADDR_SHIFT)
#define CTR_SPU_SIZE_UNITS(size) ((size) >> (CTR_SPU_ADDR_SHIFT - CTR_SPU_SAMPLE_SIZE_SHIFT))

#endif
