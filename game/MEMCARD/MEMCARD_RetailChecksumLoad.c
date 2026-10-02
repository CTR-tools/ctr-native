#include <common.h>

// NOTE(aalhendi): Matching binds canonical resident fields; native retains the live sdata pointer.
#ifndef MEMCARD_CRC_LOAD_INDEX
#define MEMCARD_CRC_LOAD_INDEX sdata->crc16_checkpoint_byteIndex
#define MEMCARD_CRC_LOAD_FLAGS sdata->memcardStatusFlags
#define MEMCARD_CRC_LOAD_CRC   sdata->crc16_checkpoint_status
#endif

int MEMCARD_ChecksumLoad(u8 *saveBytes, int len)
{
	// NOTE(aalhendi): Retail keeps payload, index, limit and finish state across CRC calls.
	register u8 *bytes CTR_PSX_REGISTER("$19") = saveBytes;
	register int crc CTR_PSX_REGISTER("$4") = MEMCARD_CRC_LOAD_CRC;
	u32 flags = MEMCARD_CRC_LOAD_FLAGS;
	register int byteIndex CTR_PSX_REGISTER("$16") = MEMCARD_CRC_LOAD_INDEX;
	register int byteIndexEnd CTR_PSX_REGISTER("$17") = len;
	register b32 boolFinishThisFrame CTR_PSX_REGISTER("$18");
	int chunkEnd;
	register u8 *checksumByte CTR_PSX_REGISTER("$3");

	if ((flags & MEMCARD_STATUS_SYNC_CHECKSUM) == 0)
	{
		chunkEnd = byteIndex + 0x200;
		boolFinishThisFrame = false;
		if (chunkEnd < byteIndexEnd - 2)
		{
			byteIndexEnd = chunkEnd;
			goto RunChecksum;
		}
	}
	boolFinishThisFrame = true;
	byteIndexEnd -= 2;
RunChecksum:
	for (; byteIndex < byteIndexEnd; byteIndex++)
		crc = MEMCARD_CRC16(crc, bytes[byteIndex]);
	// NOTE(aalhendi): Commit the prefix checkpoint before checking the two suffix bytes.
	MEMCARD_CRC_LOAD_CRC = crc;
	MEMCARD_CRC_LOAD_INDEX = byteIndex;
	if (!boolFinishThisFrame)
		return MC_RETURN_PENDING;
	crc = MEMCARD_CRC16(crc, bytes[byteIndex++]);
	// NOTE(aalhendi): Retail rebuilds the second byte address after the first CRC call.
	CTR_PSX_ADD_POINTER_OFFSET(checksumByte, bytes, byteIndex);
	crc = MEMCARD_CRC16(crc, checksumByte[0]);
	return (u32)(crc != 0);
}
