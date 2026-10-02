#ifndef CTR_MEMCARD_CHECKSUM_LOAD_RETAIL_SYMBOLS_H
#define CTR_MEMCARD_CHECKSUM_LOAD_RETAIL_SYMBOLS_H
#include "../../retail_bindings.h"
// NOTE(aalhendi): Typed aliases name canonical resident storage and define no data.
#define MEMCARD_CRC_LOAD_INDEX ctr_checksumLoadIndex
extern __typeof__(sdata->crc16_checkpoint_byteIndex) ctr_checksumLoadIndex asm("2372($28)");
#define MEMCARD_CRC_LOAD_FLAGS ctr_checksumLoadFlags
extern __typeof__(sdata->memcardStatusFlags) ctr_checksumLoadFlags asm("2400($28)");
#define MEMCARD_CRC_LOAD_CRC ctr_checksumLoadCrc
extern __typeof__(sdata->crc16_checkpoint_status) ctr_checksumLoadCrc asm("2404($28)");
#endif
