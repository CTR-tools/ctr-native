#include <common.h>

#ifndef MEMCARD_TRANSFER_STAGE
#define MEMCARD_TRANSFER_STAGE sdata->memcard_stage
#endif
#ifndef MEMCARD_TRANSFER_FD
#define MEMCARD_TRANSFER_FD sdata->memcard_fd
#endif
#ifndef MEMCARD_TRANSFER_STATUS
#define MEMCARD_TRANSFER_STATUS sdata->memcardStatusFlags
#endif
#ifndef MEMCARD_TRANSFER_ICON_SIZE
#define MEMCARD_TRANSFER_ICON_SIZE sdata->memcardIconSize
#endif
#ifndef MEMCARD_TRANSFER_PATH
#define MEMCARD_TRANSFER_PATH sdata->s_memcardFileCurr
#endif
#ifndef MEMCARD_TRANSFER_ICON_HAND
#define MEMCARD_TRANSFER_ICON_HAND data.memcardIcon_PsyqHand
#endif

// NOTE(aalhendi): PSX open bits from the existing SDK sys/fcntl.h.
enum MEMCARD_TRANSFER_SDK_FLAGS
{
	MEMCARD_TRANSFER_SDK_READ = 0x1,
	MEMCARD_TRANSFER_SDK_WRITE = 0x2,
	MEMCARD_TRANSFER_SDK_CREATE = 0x200,
	MEMCARD_TRANSFER_SDK_ASYNC = 0x8000,
};

s32 MEMCARD_Load(int slotIdx, char *name, u8 *ptrMemcard, int memcardFileSize, u32 loadFlags)
{
	if (MEMCARD_TRANSFER_STAGE != MC_STAGE_IDLE || MEMCARD_NewTask(slotIdx, name, ptrMemcard, memcardFileSize) != 0)
		return MC_RETURN_TIMEOUT;

	MEMCARD_TRANSFER_FD = open(MEMCARD_TRANSFER_PATH, MEMCARD_TRANSFER_SDK_ASYNC | MEMCARD_TRANSFER_SDK_READ);
	if (MEMCARD_TRANSFER_FD == -1)
	{
		MEMCARD_CloseFile();
		return MC_RETURN_NODATA;
	}

	if (loadFlags & MEMCARD_LOAD_SYNC_CHECKSUM)
		sdata->memcardStatusFlags |= MEMCARD_STATUS_SYNC_CHECKSUM;
	else
		sdata->memcardStatusFlags &= ~MEMCARD_STATUS_SYNC_CHECKSUM;
	MEMCARD_TRANSFER_STAGE = MC_STAGE_LOAD_PART0_START;
	return MEMCARD_ReadFile(0, 128);
}

s32 MEMCARD_Save(int slotIdx, char *name, char *title, u8 *ptrMemcard, int memcardFileSize, u32 saveFlags)
{
	u8 *icon;
	// NOTE(aalhendi): Retail reuses s1 for the path after the file-size checksum call.
	register int fileSize CTR_PSX_REGISTER("$17") = memcardFileSize;
	register char *cardPath CTR_PSX_REGISTER("$17");
	int invalidFd;
	char *text = title;
	u8 *payload = ptrMemcard;
	char *cursor;
	// NOTE(aalhendi): Retail carries the byte offset in a0 and the first icon address in v0.
	register int i CTR_PSX_REGISTER("$4");
	register u8 *evenByte CTR_PSX_REGISTER("$2");
	u8 *oddByte;
	u8 writeResult;
	int result;
	u32 pathPage;

	if (MEMCARD_TRANSFER_STAGE != MC_STAGE_IDLE)
		return MC_RETURN_TIMEOUT;

	MEMCARD_TRANSFER_ICON_SIZE = 0x100;
	if (MEMCARD_NewTask(slotIdx, name, payload, fileSize) != 0)
		return MC_RETURN_TIMEOUT;

	icon = (u8 *)MEMCARD_TRANSFER_ICON_HAND;
	if ((saveFlags & MEMCARD_SAVE_FORCE_BACKUP_COPY) == 0 && ((MEMCARD_TRANSFER_ICON_SIZE + (fileSize << 1) + 0x1fff) >> 13) >= 2)
	{
		icon[3] = (MEMCARD_TRANSFER_ICON_SIZE + fileSize + 0x1fff) >> 13;
		MEMCARD_TRANSFER_STATUS |= MEMCARD_STATUS_NO_BACKUP_COPY;
	}
	else
	{
		MEMCARD_TRANSFER_STATUS &= ~MEMCARD_STATUS_NO_BACKUP_COPY;
		icon[3] = (MEMCARD_TRANSFER_ICON_SIZE + (fileSize << 1) + 0x1fff) >> 13;
	}

	for (i = 0; i < 64; i += 2)
	{
		evenByte = icon + i;
		// NOTE(aalhendi): Keep the second address as offset + icon, matching retail operand order.
		CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(oddByte, icon, i);
		evenByte[4] = 0x81;
		oddByte[5] = 0x40;
	}

	// Retail reads the next title byte before testing the 64-byte copy limit again.
	i = 0;
	if (*text != 0)
	{
		cursor = text;
		for (; *cursor != 0; i++)
		{
			if (i >= 64)
				break;
			oddByte = icon + i;
			oddByte[4] = *cursor++;
		}
	}

	MEMCARD_ChecksumSave(payload, fileSize);
	// NOTE(aalhendi): Form the canonical path through v0 before binding it to s1.
	CTR_PSX_LOAD_SYMBOL_PAGE(pathPage, "sdata_static+49464");
	CTR_PSX_ADD_SYMBOL_LOW(cardPath, pathPage, "sdata_static+49464", MEMCARD_TRANSFER_PATH);
	MEMCARD_TRANSFER_FD = open(cardPath, ((u32)icon[3] << 16) | MEMCARD_TRANSFER_SDK_CREATE);
	invalidFd = -1;
	if (MEMCARD_TRANSFER_FD != invalidFd)
	{
		close(MEMCARD_TRANSFER_FD);
		MEMCARD_TRANSFER_FD = invalidFd;
	}
	MEMCARD_TRANSFER_FD = open(cardPath, (MEMCARD_TRANSFER_SDK_ASYNC | MEMCARD_TRANSFER_SDK_WRITE));
	if (MEMCARD_TRANSFER_FD == invalidFd)
	{
		MEMCARD_CloseFile();
		return MC_RETURN_FULL;
	}
	MEMCARD_TRANSFER_STAGE = MC_STAGE_SAVE_PART0_START;
	writeResult = MEMCARD_WriteFile(0, (u8 *)MEMCARD_TRANSFER_ICON_HAND, MEMCARD_TRANSFER_ICON_SIZE);
	// NOTE(aalhendi): Retail forwards the complete return register from the u8 helper.
	CTR_PSX_CAPTURE_REGISTER(result, writeResult);
	return result;
}
