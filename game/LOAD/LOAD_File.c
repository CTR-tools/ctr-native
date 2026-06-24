#include <common.h>
#include <platform/native_reloc.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031c1c-0x80031c58.
void LOAD_StringToUpper(char *path)
{
	for (u8 *letter = (u8 *)path; *letter != 0; letter++)
	{
		// if lowercase letter
		if ((u32)(*letter - 0x61) < 0x1a)
		{
			// uppercase
			*letter -= 0x20;
		}
	}
}

#ifdef CTR_NATIVE
#include <platform/native_cd.h>
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8007c118-0x8007c208.
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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031c58-0x80031c78 for the retail path.
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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031c78-0x80031d30
void *LOAD_ReadDirectory(char *filename)
{
	CdlFILE cdlFile;
	char buf[8];

	CDSYS_SetMode_StreamData();

	if (CdSearchFile(&cdlFile, filename) == NULL)
		return NULL;

	struct BigHeader *bh = MEMPACK_AllocMem(0x4000 /*, filename*/);

	// Search for file on disc
	// Set Cd laser to file position
	// Read the bigfile header
	// Wait for read to end
	CdControl(CdlSetloc, &cdlFile, buf);
	if (CdRead(8, (u32 *)bh, 0x80) == 0)
		return NULL;

	if (CdReadSync(0, 0) != 0)
		return NULL;

	// Save position
	bh->cdpos = CdPosToInt(&cdlFile.pos);

	// undo allocation of 0x4000, only use "needed" size
	MEMPACK_ReallocMem(sizeof(struct BigHeader) + sizeof(struct BigEntry) * bh->numEntry);

	sdata->ptrBigfileCdPos_2 = bh;
	return bh;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 PS1 path 0x80031d30-0x80031e00.
void LOAD_DramFileCallback(struct LoadQueueSlot *lqs)
{
	char *fileBuf = lqs->ptrDestination;
	void (*callback)(struct LoadQueueSlot *) = lqs->callbackFuncPtr;

	if (fileBuf != NULL)
	{
		int ptrMapOffset = *(int *)&fileBuf[0];
		char *realFileBuf = &fileBuf[4];

		if (ptrMapOffset >= 0)
		{
			struct DramPointerMap *dpm = (struct DramPointerMap *)&realFileBuf[ptrMapOffset];

#ifdef CTR_RELOC64
			if (callback == LOAD_Callback_DriverModels || callback == (void (*)(struct LoadQueueSlot *))-2)
			{
				// arm64: rebuild the model pack into native-pointer structs
				// instead of the truncating in-place LOAD_RunPtrMap. The file
				// body stays resident (leaf data is pointed at directly), so no
				// realloc trim here. See platform/native_reloc.c. The -2
				// sentinel (LOAD_DriverMPK_SetPointer) loads individually-
				// loaded driver model packs (game/LOAD/LOAD_Assets.c) — same
				// MPK format, same transform.
				lqs->ptrDestination = Reloc64_ModelPack(realFileBuf, DRAM_GETOFFSETS(dpm), dpm->numBytes >> 2);
			}
			else
#endif
			{
				LOAD_RunPtrMap(realFileBuf, (int *)DRAM_GETOFFSETS(dpm), dpm->numBytes >> 2);

#if defined(CTR_NATIVE)
				if ((lqs->flags & LT_MEMPACK) != 0)
#else
				if ((lqs->flags & LT_SETADDR) != 0)
#endif
				{
					MEMPACK_ReallocMem(ptrMapOffset + 4);
				}

				lqs->ptrDestination = &fileBuf[4];
			}
		}
		else
		{
			lqs->flags |= LT_GETADDR;
			lqs->ptrDestination = &fileBuf[4];
		}
	}

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): CTR_NATIVE keeps host callback pointers and queue sentinels.
	if ((callback != NULL) && (callback != LOAD_DramFileCallback) && (callback != (void (*)(struct LoadQueueSlot *))-1) &&
	    (callback != (void (*)(struct LoadQueueSlot *))-2))
#else
	if ((callback != NULL) && (((u32)(uintptr_t)callback & 0xff000000) == 0x80000000))
#endif
	{
		callback(lqs);
	}

	sdata->queueReady = 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031e00-0x80031ee4.
void *LOAD_DramFile(void *bigfilePtr, int subfileIndex, void *ptrDestination, int *sizePtr, int callbackOrFlags)
{
	struct LoadQueueSlot lqs;
	void *loadedFile;

	if (callbackOrFlags == -1)
	{
		loadedFile = LOAD_ReadFile_ex(bigfilePtr, LT_GETADDR, subfileIndex, ptrDestination, sizePtr, NULL);

		lqs.ptrBigfileCdPos_UNUSED = bigfilePtr;
		lqs.flags = 0;
		lqs.type_UNUSED = LT_DRAM;
		lqs.subfileIndex = subfileIndex;
		lqs.ptrDestination = loadedFile;
		lqs.size_UNUSED = *sizePtr;
		lqs.callbackFuncPtr = NULL;

		LOAD_DramFileCallback(&lqs);

		return loadedFile;
	}

	if (callbackOrFlags == -2)
	{
		loadedFile = LOAD_ReadFile_ex(bigfilePtr, LT_GETADDR, subfileIndex, NULL, sizePtr, LOAD_DramFileCallback);

#ifdef CTR_RELOC64
		// arm64: LOAD_ReadFile_ex always returns the raw (pre-relocation) buffer
		// it allocated; on retail that's fine because LOAD_RunPtrMap relocates
		// in place, so "raw buffer + 4" (done by the caller later) still lands
		// on the relocated data. Reloc64_ModelPack instead rebuilds into a new
		// native allocation and stores it in data.currSlot.ptrDestination
		// (LOAD_DramFileCallback, this same callback chain) — pick that up
		// instead of the stale raw pointer.
		loadedFile = data.currSlot.ptrDestination;
#endif

		data.currSlot.ptrDestination = loadedFile;
		*(void **)ptrDestination = loadedFile;
		return loadedFile;
	}

	return LOAD_ReadFile_ex(bigfilePtr, LT_GETADDR, subfileIndex, ptrDestination, sizePtr, LOAD_DramFileCallback);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031ee4-0x80031fdc.
void LOAD_VramFileCallback(struct LoadQueueSlot *lqs)
{
	int *vramBuf = lqs->ptrDestination;

	struct VramHeader *vh = (struct VramHeader *)vramBuf;

	// if just one TIM
	if ((vramBuf != NULL) && (vramBuf[0] != 0x20))
	{
		LoadImage(&vh->rect, VRAMHEADER_GETPIXLES(vh));
	}

	// if multiple TIMs are packed together
	if ((vramBuf != NULL) && (vramBuf[0] == 0x20))
	{
		int size;
		vramBuf++;

		size = vramBuf[0];
		vh = (struct VramHeader *)&vramBuf[1];

		while (size != 0)
		{
			LoadImage(&vh->rect, VRAMHEADER_GETPIXLES(vh));

			// goto next
			vramBuf = (int *)((u8 *)vh + size);

			size = vramBuf[0];
			vh = (struct VramHeader *)&vramBuf[1];
		}
	}

	// LOAD_NextQueuedFile waits 3 vsync frames before releasing the queue.
	sdata->frameFinishedVRAM = sdata->gGT->frameTimer_VsyncCallback;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031fdc-0x80032110.
void *LOAD_VramFile(void *bigfilePtr, int subfileIndex, void *ptrDestination, int *sizePtr, int callbackOrFlags)
{
	struct LoadQueueSlot lqs;
	void *loadedFile;

	if (ptrDestination == NULL)
		MEMPACK_PushState();

	if (callbackOrFlags == -1)
	{
		loadedFile = LOAD_ReadFile_ex(bigfilePtr, LT_VRAM, subfileIndex, ptrDestination, sizePtr, NULL);

		lqs.ptrBigfileCdPos_UNUSED = bigfilePtr;
		lqs.flags = 0;
		lqs.type_UNUSED = LT_VRAM;
		lqs.subfileIndex = subfileIndex;
		lqs.ptrDestination = loadedFile;
		lqs.size_UNUSED = *sizePtr;
		lqs.callbackFuncPtr = NULL;

		LOAD_VramFileCallback(&lqs);

		VSync(2);
		sdata->frameFinishedVRAM = 0;

		if (ptrDestination == NULL)
			MEMPACK_PopState();

		return loadedFile;
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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80032110-0x800321b4.
void LOAD_ReadFileASyncCallback(CdlIntrResult result, u8 *unk)
{
	CdReadCallback(0);
	result &= 0xff;

	struct LoadQueueSlot *lqs = &data.currSlot;

	if (result == CdlComplete)
	{
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
		if ((lqs->flags & LT_MEMPACK) != 0)
#else
		if ((lqs->flags & LT_SETADDR) != 0)
#endif
		{
			// undo allocation, try again
			MEMPACK_ReallocMem(0);
		}

		sdata->queueRetry = 1;
	}
}

void *LOAD_ReadFile_ex(struct BigHeader *bigfile, u32 loadType, int subfileIndex, void *ptrDst, int *sizePtr, void (*callback)(struct LoadQueueSlot *))
{
	int uVar5;
	CdlLOC cdLoc;
	u8 paramOutput[8];
	void *originalDst;
	int sectorSize;
	int sectorCount;
	int readComplete;

	// NOTE(aalhendi): ASM-verified NTSC-U 926 PS1 path 0x800321b4-0x80032344.
	(void)loadType;
	CDSYS_SetMode_StreamData();

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): CTR_NATIVE preserves existing queues that pass 0 for the
	// default bigfile; retail callers are expected to pass the real pointer.
	if (bigfile == NULL)
		bigfile = sdata->ptrBigfile1;
#endif

	// get size and offset of subfile
	struct BigEntry *entry = BIG_GETENTRY(bigfile);
	int eSize = entry[subfileIndex].size;
	int eOffs = entry[subfileIndex].offset;

	*sizePtr = eSize;

	CdIntToPos(bigfile->cdpos + eOffs, &cdLoc);

	struct LoadQueueSlot *lqs = &data.currSlot;
	originalDst = ptrDst;
	sectorCount = (eSize + 0x7ffU) >> 0xb;
	readComplete = 1;

	// If no address given, then find one.
	if (ptrDst == NULL)
	{
#if defined(CTR_NATIVE)
		lqs->flags |= LT_MEMPACK;
#else
		lqs->flags |= LT_SETADDR;
#endif

		// allocate room for all sectors,
		// remove alignment before next Read
		sectorSize = sectorCount << 0xb;
		ptrDst = (void *)MEMPACK_AllocMem(sectorSize); // "FILE"
		if (ptrDst == NULL)
			return NULL;
	}
	else
	{
#if defined(CTR_NATIVE)
		lqs->flags &= ~LT_MEMPACK;
#else
		lqs->flags &= ~LT_SETADDR;
#endif
	}

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): native CD reads can call back before wrapper callers store
	// the returned pointer back into data.currSlot.
	lqs->ptrDestination = ptrDst;
	lqs->size_UNUSED = eSize;
#endif

	while (1)
	{
		uVar5 = CdControl(CdlSetloc, &cdLoc, &paramOutput[0]);

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

		uVar5 &= CdRead(sectorCount, ptrDst, 0x80);

		if (callback == NULL)
		{
			// Wait for all sectors to finish
			readComplete = CdReadSync(0, (u8 *)0x0) < 1;
		}

		// If either command failed, or sync read did not finish, retry.
		if ((uVar5 != 0) && (readComplete != 0))
			break;
	}

	if ((callback == NULL) && (originalDst == NULL))
	{
		MEMPACK_ReallocMem(*sizePtr);
	}

	return ptrDst;
}

// Used for XNF and only the XNF
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80032344-0x80032438
void *LOAD_XnfFile(char *filename, void *ptrDestination, int *size)
{
	CdlFILE cdlFile;

	LOAD_StringToUpper(filename);
	CDSYS_SetMode_StreamData();

	if (CdSearchFile(&cdlFile, filename) == 0)
		return 0;

	*size = cdlFile.size;

	int allocated = ptrDestination == NULL;
	if (allocated)
	{
		// allocate room for all sectors,
		// remove alignment before next Read
		int sectorSize = (cdlFile.size + 0x7ffU) & 0xfffff800;
		ptrDestination = MEMPACK_AllocMem(sectorSize /*, fileName*/);
		if (ptrDestination == NULL)
			return NULL;
	}

	char buf[8];
	CdControl(CdlSetloc, &cdlFile, buf);

	if (CdRead((cdlFile.size + 0x7ff) >> 0xb, ptrDestination, 0x80) == 0)
		return 0;

	if (CdReadSync(0, 0))
		return 0;

	if (allocated)
		MEMPACK_ReallocMem(cdlFile.size);

	return ptrDestination;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80032438-0x80032498
int LOAD_FindFile(char *filename, CdlFILE *cdlFile)
{
	if (filename == 0)
		return 0;
	if (cdlFile == 0)
		return 0;

	CDSYS_SetMode_StreamData();
	LOAD_StringToUpper(filename);

	return (CdSearchFile(cdlFile, filename) != 0);
}
