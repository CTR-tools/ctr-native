#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003d540-0x8003d584.
u32 MEMCARD_CRC16(u32 crc, int nextByte)
{
	int i;
	int bitCheck;

	for (i = 7; i >= 0; i--)
	{
		bitCheck = crc << 1;
		crc = bitCheck | ((nextByte >> i) & 1);

		if ((bitCheck & 0x10000) != 0)
		{
			crc = crc ^ 0x11021;
		}
	}

	return crc;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003d584-0x8003d618.
void MEMCARD_ChecksumSave(u8 *saveBytes, int len)
{
	int i;
	int crc = 0;

	len -= 2;

	for (i = 0; i < len; i++)
	{
		crc = MEMCARD_CRC16(crc, saveBytes[i]);
	}

	sdata->crc16_checkpoint_status = crc;

	// finishing check
	crc = MEMCARD_CRC16(crc, 0);
	crc = MEMCARD_CRC16(crc, 0);

	// write checksum to data (last 2 bytes),
	// swap endians to throw off hackers,
	// which didn't really throw anyone off at all
	saveBytes[i] = (u8)(crc >> 8);
	saveBytes[i + 1] = (u8)crc;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003d618-0x8003d6e8.
int MEMCARD_ChecksumLoad(u8 *saveBytes, int len)
{
	int byteIndex = sdata->crc16_checkpoint_byteIndex;
	int byteIndexEnd;
	b32 boolFinishThisFrame;
	int crc = sdata->crc16_checkpoint_status;

	if ((sdata->memcardStatusFlags & MEMCARD_STATUS_SYNC_CHECKSUM) == 0)
	{
		byteIndexEnd = byteIndex + 0x200;
		boolFinishThisFrame = false;

		if (byteIndexEnd < len - 2)
		{
			goto RunChecksum;
		}
	}

	boolFinishThisFrame = true;
	byteIndexEnd = len - 2;

RunChecksum:
	for (; byteIndex < byteIndexEnd; byteIndex++)
	{
		crc = MEMCARD_CRC16(crc, saveBytes[byteIndex]);
	}

	sdata->crc16_checkpoint_byteIndex = byteIndex;
	sdata->crc16_checkpoint_status = crc;

	if (!boolFinishThisFrame)
	{
		return MC_RETURN_PENDING;
	}

	crc = MEMCARD_CRC16(crc, saveBytes[byteIndex]);
	crc = MEMCARD_CRC16(crc, saveBytes[byteIndex + 1]);

	// Will return one of these:
	// 0: MC_RETURN_IOE
	// 1: MC_RETURN_TIMEOUT
	return (u32)(crc != 0);
}
