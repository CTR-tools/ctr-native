#include <common.h>

#ifndef CTR_NATIVE
#include <psx/hwregs.h>
#endif

void LOAD_StringToUpper(char *path)
{
	u8 *letter;
	u8 character;

	letter = (u8 *)path;
	character = *letter;
	while (character != 0)
	{
		// if lowercase letter
		if ((u32)(character - 0x61) < 0x1a)
		{
			// uppercase
			*letter = character + 0xe0;
		}
		letter++;
		character = *letter;
	}
}

#ifdef CTR_NATIVE
#include <platform/native_cd.h>
#endif

int LOAD_InitCDvol(void)
{
#ifndef CTR_NATIVE
	if ((SPU_CURRENT_VOL_L == 0) && (SPU_CURRENT_VOL_R == 0))
	{
		SPU_MASTER_VOL_L = 0x3fff;
		SPU_MASTER_VOL_R = 0x3fff;
	}

	SPU_CD_VOL_L = 0x3fff;
	SPU_CD_VOL_R = 0x3fff;
	SPU_CTRL = 0xc001;

	CD_REG(0) = 2;
	CD_REG(2) = 0x80;
	CD_REG(3) = 0;
	CD_REG(0) = 3;
	CD_REG(1) = 0x80;
	CD_REG(2) = 0;
	CD_REG(3) = 0x20;
#else
	// NOTE(aalhendi): Retail CdInit calls this hook to reset SPU volume
	// state before howl_InitGlobals re-applies game defaults. Native has
	// no CdInit path (CDSYS_Init(0) skips it), so mirror the SPU CD
	// volume write through the native SPU API to keep parity with the
	// retail init ordering. Master volume defaults are already handled
	// by NativeAudio_SpuInit (0x3fff), matching retail's "only if zero"
	// guard.
	SpuSetCommonCDVolume(0x3fff, 0x3fff);
#endif

	return 0;
}

void LOAD_InitCD()
{
#ifdef CTR_NATIVE
	NativeCD_Init();
	CDSYS_Init(0);
	// NOTE(aalhendi): Retail chains LOAD_InitCD -> CDSYS_Init(1) -> CdInit
	// -> LOAD_InitCDvol. Native skips CdInit (no disc), so call the volume
	// hook explicitly to preserve the same init ordering.
	LOAD_InitCDvol();
	return;
#endif

	CDSYS_Init(1);
}

void *LOAD_ReadDirectory(char *filename)
{
	u8 buf[8];
	CdlFILE cdlFile;
	CdlFILE *filePtr;
	struct BigHeader *bh;
	struct BigHeader *result;

	filePtr = &cdlFile;
	CDSYS_SetMode_StreamData();

	if (CdSearchFile(filePtr, filename) == NULL)
	{
		return NULL;
	}

	bh = MEMPACK_AllocMem(LOAD_BIGFILE_HEADER_ALLOC_BYTES, filename);

	// Search for file on disc
	// Set Cd laser to file position
	// Read the bigfile header
	// Wait for read to end
	CdControl(CdlSetloc, (u8 *)filePtr, buf);
	if (CdRead(LOAD_BIGFILE_HEADER_SECTORS, (u32 *)bh, CdlModeSpeed) == 0)
	{
		return NULL;
	}

	if (CdReadSync(0, 0) != 0)
	{
		return NULL;
	}

	// Save position
	bh->cdpos = CdPosToInt(&filePtr->pos);

	// undo header allocation, only use "needed" size
	MEMPACK_ReallocMem(sizeof(struct BigHeader) + sizeof(struct BigEntry) * bh->numEntry);

	// NOTE(aalhendi): Retail copies the result before publishing the pointer;
	// keep the store out of the epilogue jump's delay slot.
	CTR_PSX_COPY_VALUE(result, bh);
	sdata->ptrBigfileCdPos_2 = result;
	CTR_PSX_MEMORY_BARRIER();
	return result;
}

void LOAD_DramFileCallback(struct LoadQueueSlot *lqs)
{
	char *fileBuf = lqs->ptrDestination;
	void (*callback)(struct LoadQueueSlot *);

	if (fileBuf != NULL)
	{
		int ptrMapOffset = *(int *)&fileBuf[0];
		char *realFileBuf = &fileBuf[4];

		if (ptrMapOffset >= 0)
		{
			struct DramPointerMap *dpm = (struct DramPointerMap *)&realFileBuf[ptrMapOffset];

			LOAD_RunPtrMap(realFileBuf, (int *)DRAM_GETOFFSETS(dpm), dpm->numBytes >> 2);

#if defined(CTR_NATIVE)
			if ((lqs->flags & LT_MEMPACK) != 0)
#else
			if ((lqs->flags & LT_SETADDR) != 0)
#endif
			{
				MEMPACK_ReallocMem(ptrMapOffset + 4);
			}
		}
		else
		{
			lqs->flags |= LT_GETADDR;
		}

		lqs->ptrDestination = (char *)lqs->ptrDestination + 4;
	}

	callback = lqs->callbackFuncPtr;

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): CTR_NATIVE keeps host callback pointers and queue sentinels.
	if ((callback != NULL) && (callback != LOAD_DramFileCallback) && (callback != (void (*)(struct LoadQueueSlot *))-1) &&
	    (callback != LOAD_QUEUE_CALLBACK_SET_POINTER))
#else
	if ((callback != NULL) && (((u32)callback & 0xff000000) == 0x80000000))
#endif
	{
		callback(lqs);
	}

	GAME_LOAD_QUEUE_READY = 1;
}

void *LOAD_DramFile(void *bigfilePtr, int subfileIndex, void *ptrDestination, u32 *sizePtr, int callbackOrFlags)
{
	struct LoadQueueSlot lqs;
	void *loadedFile;

	if (callbackOrFlags == -1)
	{
		lqs.ptrBigfileCdPos_UNUSED = bigfilePtr;
		lqs.flags = 0;
		lqs.type_UNUSED = LT_DRAM;
		lqs.subfileIndex = subfileIndex;
		lqs.ptrDestination = LOAD_ReadFile_ex(bigfilePtr, LT_GETADDR, subfileIndex, ptrDestination, sizePtr, NULL);
		lqs.size_UNUSED = *sizePtr;
		lqs.callbackFuncPtr = NULL;

		LOAD_DramFileCallback(&lqs);

		// NOTE(aalhendi): DRAM relocation has advanced this slot past its header.
		return lqs.ptrDestination;
	}

	if (callbackOrFlags == -2)
	{
		loadedFile = LOAD_ReadFile_ex(bigfilePtr, LT_GETADDR, subfileIndex, NULL, sizePtr, LOAD_DramFileCallback);
		data.currSlot.ptrDestination = loadedFile;
		*(void **)ptrDestination = loadedFile;
		return loadedFile;
	}

	return LOAD_ReadFile_ex(bigfilePtr, LT_GETADDR, subfileIndex, ptrDestination, sizePtr, LOAD_DramFileCallback);
}

void LOAD_VramFileCallback(struct LoadQueueSlot *lqs)
{
	RECT rect;
	struct VramHeader *vh = lqs->ptrDestination;

	if (vh != NULL)
	{
		// Packed TIMs start with the container tag and record lengths.
		if (*(int *)vh == 0x20)
		{
			int size;
			vh = (struct VramHeader *)((int *)vh + 1);
			size = *(int *)vh;
			vh = (struct VramHeader *)((int *)vh + 1);
			while (size != 0)
			{
				rect.x = vh->rect.x;
				rect.y = vh->rect.y;
				rect.w = vh->rect.w;
				rect.h = vh->rect.h;
				LoadImage(&rect, VRAMHEADER_GETPIXLES(vh));
				vh = (struct VramHeader *)((int *)vh + (size >> 2));
				size = *(int *)vh;
				vh = (struct VramHeader *)((int *)vh + 1);
			}
		}
		else
		{
			rect.x = vh->rect.x;
			rect.y = vh->rect.y;
			rect.w = vh->rect.w;
			rect.h = vh->rect.h;
			LoadImage(&rect, VRAMHEADER_GETPIXLES(vh));
		}
	}

	// LOAD_NextQueuedFile waits 3 vsync frames before releasing the queue.
	GAME_LOAD_VRAM_FINISHED = GAME_TRACKER->frameTimer_VsyncCallback;
}

void *LOAD_VramFile(void *bigfilePtr, int subfileIndex, void *ptrDestination, u32 *sizePtr, int callbackOrFlags)
{
	struct LoadQueueSlot lqs;
	void *loadedFile;

	if (ptrDestination == NULL)
	{
		MEMPACK_PushState();
	}

	if (callbackOrFlags == -1)
	{
		lqs.ptrBigfileCdPos_UNUSED = bigfilePtr;
		lqs.flags = 0;
		lqs.type_UNUSED = LT_VRAM;
		lqs.subfileIndex = subfileIndex;
		lqs.ptrDestination = LOAD_ReadFile_ex(bigfilePtr, LT_VRAM, subfileIndex, ptrDestination, sizePtr, NULL);
		lqs.size_UNUSED = *sizePtr;
		lqs.callbackFuncPtr = NULL;

		LOAD_VramFileCallback(&lqs);

		VSync(2);
		GAME_LOAD_VRAM_FINISHED = 0;

		if (ptrDestination == NULL)
		{
			MEMPACK_PopState();
		}

		return lqs.ptrDestination;
	}

	if (callbackOrFlags == -2)
	{
		loadedFile = LOAD_ReadFile_ex(bigfilePtr, LT_VRAM, subfileIndex, NULL, sizePtr, LOAD_VramFileCallback);
		data.currSlot.ptrDestination = loadedFile;
		*(void **)ptrDestination = loadedFile;
		return loadedFile;
	}

	return LOAD_ReadFile_ex(bigfilePtr, LT_VRAM, subfileIndex, ptrDestination, sizePtr, LOAD_VramFileCallback);
}

void LOAD_ReadFileASyncCallback(u8 result, u8 *unused)
{
	(void)unused;
	CdReadCallback(0);
	result &= 0xff;

	if (result == CdlComplete)
	{
		struct LoadQueueSlot *lqs = &data.currSlot;
#if defined(CTR_NATIVE)
		if ((lqs->flags & LT_MEMPACK) != 0)
#else
		if ((lqs->flags & LT_SETADDR) != 0)
#endif
		{
			MEMPACK_ReallocMem(lqs->size_UNUSED);
		}

		if (sdata->callbackCdReadSuccess != NULL)
		{
			sdata->callbackCdReadSuccess(lqs);
		}
	}

	// CdlDiskError
	else
	{
#if defined(CTR_NATIVE)
		if ((data.currSlot.flags & LT_MEMPACK) != 0)
#else
		if ((data.currSlot.flags & LT_SETADDR) != 0)
#endif
		{
			// undo allocation, try again
			MEMPACK_PopState();
		}

		sdata->queueRetry = 1;
	}
}

void *LOAD_ReadFile_ex(struct BigHeader *bigfile, u32 loadType, int subfileIndex, void *ptrDst, u32 *sizePtr, void (*callback)(struct LoadQueueSlot *))
{
	b32 commandSuccess;
	CdlLOC cdLoc;
	u8 paramOutput[8];
	void *originalDst = ptrDst;
	int sectorSize;
	// NOTE(aalhendi): The initial success state stays live across CD setup on GCC 2.8.
	register b32 readComplete = 1;
	struct BigEntry *entry;

	(void)loadType;
	CDSYS_SetMode_StreamData();

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): CTR_NATIVE preserves existing queues that pass 0 for the
	// default bigfile; retail callers are expected to pass the real pointer.
	if (bigfile == NULL)
	{
		bigfile = sdata->ptrBigfile1;
	}
#endif

	// get size and offset of subfile
	entry = BIG_GETENTRY((struct BigHeader *)((char *)bigfile + subfileIndex * sizeof(struct BigEntry)));
	// NOTE(aalhendi): Publish size before reading offset; sizePtr may alias the entry.
	*sizePtr = entry->size;
	commandSuccess = 0;

	CdIntToPos(bigfile->cdpos + entry->offset, &cdLoc);

	// If no address given, then find one.
	if (ptrDst == NULL)
	{
		struct LoadQueueSlot *lqs = &data.currSlot;
#if defined(CTR_NATIVE)
		lqs->flags |= LT_MEMPACK;
#else
		lqs->flags |= LT_SETADDR;
#endif

		// allocate room for all sectors,
		// remove alignment before next Read
		sectorSize = ((*sizePtr + LOAD_CD_DATA_SECTOR_ROUND_MASK) >> LOAD_CD_DATA_SECTOR_SHIFT) << LOAD_CD_DATA_SECTOR_SHIFT;
		ptrDst = (void *)MEMPACK_AllocMem(sectorSize, GAME_LOAD_FILE_LABEL);
		if (ptrDst == NULL)
		{
			return (void *)commandSuccess;
		}
	}
	else
	{
		struct LoadQueueSlot *lqs = &data.currSlot;
#if defined(CTR_NATIVE)
		lqs->flags &= ~LT_MEMPACK;
#else
		lqs->flags &= ~LT_SETADDR;
#endif
	}

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): native CD reads can call back before wrapper callers store
	// the returned pointer back into data.currSlot.
	data.currSlot.ptrDestination = ptrDst;
	data.currSlot.size_UNUSED = *sizePtr;
#endif

	while ((commandSuccess == 0) || (readComplete == 0))
	{
		commandSuccess = CdControl(CdlSetloc, (u8 *)&cdLoc, &paramOutput[0]);

		if (callback != NULL)
		{
			sdata->callbackCdReadSuccess = callback;
			CdReadCallback(LOAD_ReadFileASyncCallback);
		}
		else
		{
			sdata->callbackCdReadSuccess = NULL;
			CdReadCallback(NULL);
		}

		commandSuccess &= CdRead((*sizePtr + LOAD_CD_DATA_SECTOR_ROUND_MASK) >> LOAD_CD_DATA_SECTOR_SHIFT, ptrDst, CdlModeSpeed);

		if (callback == NULL)
		{
			// Wait for all sectors to finish
			readComplete = CdReadSync(0, (u8 *)0x0) == 0;
		}
	}

	if ((callback == NULL) && (originalDst == NULL))
	{
		MEMPACK_ReallocMem(*sizePtr);
	}

	return ptrDst;
}

// Used for XNF and only the XNF
void *LOAD_XnfFile(char *filename, void *ptrDestination, int *size)
{
	CdlFILE cdlFile;
	u8 buf[8];
	void *originalDst = ptrDestination;

	LOAD_StringToUpper(filename);
	CDSYS_SetMode_StreamData();

	if (CdSearchFile(&cdlFile, filename) == 0)
	{
		return ptrDestination;
	}

	*size = cdlFile.size;

	if (ptrDestination == NULL)
	{
		// allocate room for all sectors,
		// remove alignment before next Read
		int sectorSize = ((cdlFile.size + LOAD_CD_DATA_SECTOR_ROUND_MASK) >> LOAD_CD_DATA_SECTOR_SHIFT) << LOAD_CD_DATA_SECTOR_SHIFT;
		ptrDestination = MEMPACK_AllocMem(sectorSize, filename);
		if (ptrDestination == NULL)
		{
			return NULL;
		}
	}

	CdControl(CdlSetloc, (u8 *)&cdlFile, buf);

	if (CdRead((cdlFile.size + LOAD_CD_DATA_SECTOR_ROUND_MASK) >> LOAD_CD_DATA_SECTOR_SHIFT, ptrDestination, CdlModeSpeed) == 0)
	{
		return 0;
	}

	if (CdReadSync(0, 0))
	{
		return 0;
	}

	if (originalDst == NULL)
	{
		MEMPACK_ReallocMem(cdlFile.size);
	}

	return ptrDestination;
}

int LOAD_FindFile(char *filename, CdlFILE *cdlFile)
{
	if ((filename == 0) || (cdlFile == 0))
	{
		return 0;
	}

	CDSYS_SetMode_StreamData();
	LOAD_StringToUpper(filename);

	return (CdSearchFile(cdlFile, filename) != 0);
}
