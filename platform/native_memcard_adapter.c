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
		{
			icon[i + 4] = iconHeader[i];
		}
	}

	return icon;
}

// NOTE(aalhendi): ctr-native adapts host-backed card operations here; the
// retail implementations stay in the retail MEMCARD domain files for non-native
// builds.
void MEMCARD_GetFreeBytes(int slotIdx)
{
	(void)slotIdx;
	sdata->memoryCard_SizeRemaining = 0x1e000;
}

s32 MEMCARD_GetInfo(int slotIdx)
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

s32 MEMCARD_Format(int slotIdx)
{
	(void)slotIdx;
	return MC_RETURN_IOE;
}

int MEMCARD_IsFile(int slotIdx, char *save_name)
{
	char nativeName[64];

	MEMCARD_StringSet(nativeName, slotIdx, save_name);
	return NativeMemcard_FileExists(nativeName) ? MC_RETURN_IOE : MC_RETURN_NODATA;
}

char *MEMCARD_FindFirstGhost(int slotIdx, char *srcString)
{
	if (sdata->memcard_stage != MC_STAGE_IDLE)
	{
		return NULL;
	}

	MEMCARD_StringSet(sdata->s_memcardFileCurr, slotIdx, srcString);

	if (NativeMemcard_FindFirstFile(sdata->s_memcardFileCurr, &sdata->s_memcardFindGhostFile[0], sizeof(sdata->s_memcardFindGhostFile)) == 0)
	{
		return NULL;
	}

	sdata->memcard_stage = MC_STAGE_GHOST_FOUND;
	return &sdata->s_memcardFindGhostFile[0];
}

char *MEMCARD_FindNextGhost(void)
{
	if (sdata->memcard_stage != MC_STAGE_GHOST_FOUND)
	{
		return NULL;
	}

	if (NativeMemcard_FindNextFile(&sdata->s_memcardFindGhostFile[0], sizeof(sdata->s_memcardFindGhostFile)) == 0)
	{
		sdata->memcard_stage = MC_STAGE_IDLE;
		return NULL;
	}

	return &sdata->s_memcardFindGhostFile[0];
}

s32 MEMCARD_EraseFile(int slotIdx, char *srcString)
{
	char nativeName[64];

	MEMCARD_StringSet(nativeName, slotIdx, srcString);
	return NativeMemcard_RemoveFile(nativeName) == NATIVE_MEMCARD_OK ? MC_RETURN_IOE : MC_RETURN_NODATA;
}

int MEMCARD_HandleEvent(void)
{
	// Native MEMCARD operations complete synchronously in this adapter. If the
	// retail polling path reaches here, report a timeout instead of touching PSX
	// card event APIs.
	return MC_RETURN_TIMEOUT;
}

// NOTE(aalhendi): Native keeps the card setup flow through its SDK shims;
// retail selects the instruction-exact implementation in game_unity.h.
void MEMCARD_InitCard(void)
{
	EnterCriticalSection();
	sdata->SwCARD_EvSpIOE = OpenEvent(SwCARD, EvSpIOE, EvMdNOINTR, NULL);
	sdata->SwCARD_EvSpERROR = OpenEvent(SwCARD, EvSpERROR, EvMdNOINTR, NULL);
	sdata->SwCARD_EvSpTIMOUT = OpenEvent(SwCARD, EvSpTIMOUT, EvMdNOINTR, NULL);
	sdata->SwCARD_EvSpNEW = OpenEvent(SwCARD, EvSpNEW, EvMdNOINTR, NULL);
	sdata->HwCARD_EvSpIOE = OpenEvent(HwCARD, EvSpIOE, EvMdNOINTR, NULL);
	sdata->HwCARD_EvSpERROR = OpenEvent(HwCARD, EvSpERROR, EvMdNOINTR, NULL);
	sdata->HwCARD_EvSpTIMOUT = OpenEvent(HwCARD, EvSpTIMOUT, EvMdNOINTR, NULL);
	sdata->HwCARD_EvSpNEW = OpenEvent(HwCARD, EvSpNEW, EvMdNOINTR, NULL);
	EnableEvent(sdata->SwCARD_EvSpIOE);
	EnableEvent(sdata->SwCARD_EvSpERROR);
	EnableEvent(sdata->SwCARD_EvSpTIMOUT);
	EnableEvent(sdata->SwCARD_EvSpNEW);
	EnableEvent(sdata->HwCARD_EvSpIOE);
	EnableEvent(sdata->HwCARD_EvSpERROR);
	EnableEvent(sdata->HwCARD_EvSpTIMOUT);
	EnableEvent(sdata->HwCARD_EvSpNEW);
	ExitCriticalSection();

	InitCARD(0);
	StartCARD();
	_bu_init();
	// NOTE(aalhendi): Bit 0 makes the first successful card-info event enter
	// the new-card path; with both bits 0 and 1 clear it is unformatted.
	sdata->memcardStatusFlags = 1;
}

s32 MEMCARD_Load(int slotIdx, char *name, u8 *ptrMemcard, int memcardFileSize, u32 loadFlags)
{
	char nativeName[64];
	int checksumResult;

	(void)loadFlags;

	MEMCARD_StringSet(nativeName, slotIdx, name);
	enum NativeMemcardResult nativeResult = NativeMemcard_ReadSaveData(nativeName, ptrMemcard, memcardFileSize, 0x100);
	if (nativeResult == NATIVE_MEMCARD_NOT_FOUND)
	{
		return MC_RETURN_NODATA;
	}

	if (nativeResult != NATIVE_MEMCARD_OK)
	{
		return MC_RETURN_TIMEOUT;
	}

	sdata->crc16_checkpoint_byteIndex = 0;
	sdata->crc16_checkpoint_status = 0;
	do
	{
		checksumResult = MEMCARD_ChecksumLoad(ptrMemcard, memcardFileSize);
	} while (checksumResult == MC_RETURN_PENDING);

	return checksumResult == MC_RETURN_IOE ? MC_RETURN_IOE : MC_RETURN_TIMEOUT;
}

s32 MEMCARD_Save(int slotIdx, char *name, char *icon, u8 *ptrMemcard, int memcardFileSize, u32 saveFlags)
{
	char nativeName[64];

	sdata->crc16_checkpoint_byteIndex = 0;
	sdata->crc16_checkpoint_status = 0;
	MEMCARD_ChecksumSave(ptrMemcard, memcardFileSize);

	u8 *cardIcon = MEMCARD_NativePrepareIcon(icon, memcardFileSize, saveFlags);
	MEMCARD_StringSet(nativeName, slotIdx, name);
	enum NativeMemcardResult nativeResult = NativeMemcard_WriteSaveData(nativeName, cardIcon, sdata->memcardIconSize, ptrMemcard, memcardFileSize);
	if (nativeResult == NATIVE_MEMCARD_OPEN_FAILED)
	{
		return MC_RETURN_FULL;
	}

	if (nativeResult != NATIVE_MEMCARD_OK)
	{
		return MC_RETURN_TIMEOUT;
	}

	return MC_RETURN_IOE;
}
