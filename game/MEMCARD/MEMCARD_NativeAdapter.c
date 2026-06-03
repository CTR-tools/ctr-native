#include <common.h>
#include <platform/native_memcard.h>

static u8 s_memcardNativeInfoSeen[2];

static u8 *MEMCARD_NativePrepareIcon(char *iconHeader, int memcardFileSize, u32 saveFlags)
{
	u8 *icon = (u8 *)&data.memcardIcon_PsyqHand[0];

	sdata->memcardIconSize = 0x100;

	if (((saveFlags & MEMCARD_SAVE_FORCE_BACKUP_COPY) == 0) && (((sdata->memcardIconSize + memcardFileSize * 2 + 0x1fff) >> 13) >= 2))
	{
		icon[3] = (sdata->memcardIconSize + memcardFileSize + 0x1fff) >> 13;
		sdata->memcardStatusFlags |= MEMCARD_STATUS_NO_BACKUP_COPY;
	}
	else
	{
		sdata->memcardStatusFlags &= ~MEMCARD_STATUS_NO_BACKUP_COPY;
		icon[3] = (sdata->memcardIconSize + memcardFileSize * 2 + 0x1fff) >> 13;
	}

	for (int i = 0; i < 0x40; i += 2)
	{
		icon[i + 4] = 0x81;
		icon[i + 5] = 0x40;
	}

	if ((iconHeader != NULL) && (iconHeader[0] != '\0'))
	{
		for (int i = 0; (i < 0x40) && (iconHeader[i] != '\0'); i++)
			icon[i + 4] = iconHeader[i];
	}

	return icon;
}

// NOTE(aalhendi): ctr-native adapts host-backed card operations here; the
// retail implementations stay in their numbered MEMCARD files for non-native
// builds.
void MEMCARD_GetFreeBytes(int slotIdx)
{
	(void)slotIdx;
	sdata->memoryCard_SizeRemaining = 0x1e000;
}

u8 MEMCARD_GetInfo(int slotIdx)
{
	// NOTE(aalhendi): Native treats the host save directory as an inserted card;
	// PSX updates free space while handling the async info event that native skips.
	MEMCARD_GetFreeBytes(slotIdx);

	// NOTE(aalhendi): Report the host directory as a new card once; repeated
	// NEWCARD results make RefreshCard reload the profile forever.
	if (s_memcardNativeInfoSeen[slotIdx & 1] == 0)
	{
		s_memcardNativeInfoSeen[slotIdx & 1] = 1;
		return MC_RETURN_NEWCARD;
	}

	return MC_RETURN_IOE;
}

u8 MEMCARD_Format(int slotIdx)
{
	(void)slotIdx;
	return MC_RETURN_IOE;
}

int MEMCARD_IsFile(int slotIdx, char *save_name)
{
	(void)slotIdx;

	return NativeMemcard_FileExists(save_name) ? MC_RETURN_IOE : MC_RETURN_NODATA;
}

char *MEMCARD_FindFirstGhost(int slotIdx, char *srcString)
{
	if (sdata->memcard_stage != MC_STAGE_IDLE)
		return NULL;

	MEMCARD_StringSet(sdata->s_memcardFileCurr, slotIdx, srcString);

	if (NativeMemcard_FindFirstFile(sdata->s_memcardFileCurr, &sdata->s_memcardFindGhostFile[0], sizeof(sdata->s_memcardFindGhostFile)) == 0)
		return NULL;

	sdata->memcard_stage = MC_STAGE_GHOST_FOUND;
	return &sdata->s_memcardFindGhostFile[0];
}

char *MEMCARD_FindNextGhost(void)
{
	if (sdata->memcard_stage != MC_STAGE_GHOST_FOUND)
		return NULL;

	if (NativeMemcard_FindNextFile(&sdata->s_memcardFindGhostFile[0], sizeof(sdata->s_memcardFindGhostFile)) == 0)
	{
		sdata->memcard_stage = MC_STAGE_IDLE;
		return NULL;
	}

	return &sdata->s_memcardFindGhostFile[0];
}

u8 MEMCARD_EraseFile(int slotIdx, char *srcString)
{
	(void)slotIdx;
	return NativeMemcard_RemoveFile(srcString) == NATIVE_MEMCARD_OK ? MC_RETURN_IOE : MC_RETURN_NODATA;
}

int MEMCARD_HandleEvent(void)
{
	// Native MEMCARD operations complete synchronously in this adapter. If the
	// retail polling path reaches here, report a timeout instead of touching PSX
	// card event APIs.
	return MC_RETURN_TIMEOUT;
}

u8 MEMCARD_Load(int slotIdx, char *name, u8 *ptrMemcard, int memcardFileSize, u32 loadFlags)
{
	enum NativeMemcardResult nativeResult;
	int checksumResult;

	(void)slotIdx;
	(void)loadFlags;

	nativeResult = NativeMemcard_ReadSaveData(name, ptrMemcard, memcardFileSize, 0x100);
	if (nativeResult == NATIVE_MEMCARD_NOT_FOUND)
		return MC_RETURN_NODATA;

	if (nativeResult != NATIVE_MEMCARD_OK)
		return MC_RETURN_TIMEOUT;

	sdata->crc16_checkpoint_byteIndex = 0;
	sdata->crc16_checkpoint_status = 0;
	do
	{
		checksumResult = MEMCARD_ChecksumLoad(ptrMemcard, memcardFileSize);
	} while (checksumResult == MC_RETURN_PENDING);

	return checksumResult == MC_RETURN_IOE ? MC_RETURN_IOE : MC_RETURN_TIMEOUT;
}

u8 MEMCARD_Save(int slotIdx, char *name, char *icon, u8 *ptrMemcard, int memcardFileSize, u32 saveFlags)
{
	enum NativeMemcardResult nativeResult;
	u8 *cardIcon;

	(void)slotIdx;

	sdata->crc16_checkpoint_byteIndex = 0;
	sdata->crc16_checkpoint_status = 0;
	MEMCARD_ChecksumSave(ptrMemcard, memcardFileSize);

	cardIcon = MEMCARD_NativePrepareIcon(icon, memcardFileSize, saveFlags);
	nativeResult = NativeMemcard_WriteSaveData(name, cardIcon, sdata->memcardIconSize, ptrMemcard, memcardFileSize);
	if (nativeResult == NATIVE_MEMCARD_OPEN_FAILED)
		return MC_RETURN_FULL;

	if (nativeResult != NATIVE_MEMCARD_OK)
		return MC_RETURN_TIMEOUT;

	return MC_RETURN_IOE;
}
