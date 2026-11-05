#include <common.h>
#include <platform/native_track_file.h>
#include <platform/native_track_library.h>
#include <platform/native_track_validate.h>
#include <stdio.h>

CTR_STATIC_ASSERT(NATIVE_TRACK_LEV_HEADER_MIN_BYTES == offsetof(struct Level, visMem) + sizeof(((struct Level *)0)->visMem));

static int LOAD_NativeTrackFile(struct LoadQueueSlot *slot)
{
	int vram = slot->type_UNUSED == LT_NATIVE_TRACK_VRM;
	enum NativeTrackFileRole role = vram ? NATIVE_TRACK_FILE_VRM : NATIVE_TRACK_FILE_LEV;
	struct NativeTrackFile *file = NativeTrackFile_Open(NativeTrackLibrary_Current(), slot->subfileIndex, role);
	if (file == NULL)
	{
		return 0;
	}

	size_t bytes = NativeTrackFile_SectorBytes(file);
	size_t size = NativeTrackFile_Size(file);
	if (bytes > (size_t)MEMPACK_GetFreeBytes())
	{
		NativeTrackFile_Close(file);
		return 0;
	}
	if (MEMPACK_PushState() >= MEMPACK_BOOKMARK_COUNT)
	{
		NativeTrackFile_Close(file);
		return 0;
	}
	void *buffer = MEMPACK_AllocMem((int)bytes, NULL);
	int ok = buffer != NULL && NativeTrackFile_Read(file, buffer, bytes);
	NativeTrackFile_Close(file);
	if (!ok)
	{
		MEMPACK_PopState();
		return 0;
	}

	if (!vram)
	{
		// Keep the LEV allocation while removing the temporary rollback bookmark.
		MEMPACK_PopState();
		buffer = MEMPACK_AllocMem((int)bytes, NULL);
		MEMPACK_ReallocMem((int)size);
	}
	slot->flags |= LT_MEMPACK;
	slot->ptrDestination = buffer;
	slot->size_UNUSED = (u32)size;
	if (vram)
	{
		LOAD_VramFileCallback(slot);
	}
	else
	{
		LOAD_DramFileCallback(slot);
	}
	return 1;
}

void LOAD_AppendQueue(struct BigHeader *bigfile, int type, int fileIndex, void *destinationPtr, void (*callback)(struct LoadQueueSlot *))
{
	if (sdata->queueLength >= LOAD_QUEUE_SLOT_COUNT)
	{
		return;
	}

	struct LoadQueueSlot *lqs = &sdata->queueSlots[(s32)sdata->queueLength];
	lqs->ptrBigfileCdPos_UNUSED = bigfile;
	lqs->flags = 0;
	lqs->type_UNUSED = type;
	lqs->subfileIndex = fileIndex;
	lqs->ptrDestination = destinationPtr;
	lqs->size_UNUSED = 0;
	lqs->callbackFuncPtr = callback;

	sdata->queueLength++;
}

void LOAD_CDRequestCallback(struct LoadQueueSlot *lqs)
{
	if (lqs->callbackFuncPtr != NULL)
	{
		lqs->callbackFuncPtr(lqs);
	}

	sdata->queueReady = 1;
}

void LOAD_NextQueuedFile()
{
	struct LoadQueueSlot *curr = &data.currSlot;

	if ((sdata->queueReady != 0) && (sdata->XA_State == 0) && (sdata->queueLength != 0))
	{
		sdata->queueReady = 0;

		if (sdata->queueRetry != 0)
		{
			sdata->queueRetry = 0;
		}
		else
		{
			*curr = sdata->queueSlots[0];

			for (int i = LOAD_QUEUE_FIRST_PENDING_SLOT; i < sdata->queueLength; i++)
			{
				sdata->queueSlots[i - 1] = sdata->queueSlots[i];
			}
		}

		switch (curr->type_UNUSED)
		{
		case LT_RAW:
			curr->ptrDestination = LOAD_ReadFile_ex(curr->ptrBigfileCdPos_UNUSED, LT_SETADDR, curr->subfileIndex, curr->ptrDestination, &curr->size_UNUSED,
			                                        LOAD_CDRequestCallback);
			break;

		case LT_DRAM:
			curr->ptrDestination =
			    LOAD_DramFile(curr->ptrBigfileCdPos_UNUSED, curr->subfileIndex, curr->ptrDestination, &curr->size_UNUSED, (int)(s32)curr->callbackFuncPtr);
			break;

		case LT_VRAM:
			curr->ptrDestination =
			    LOAD_VramFile(curr->ptrBigfileCdPos_UNUSED, curr->subfileIndex, curr->ptrDestination, &curr->size_UNUSED, (int)(s32)curr->callbackFuncPtr);
			break;

		case LT_NATIVE_TRACK_LEV:
		case LT_NATIVE_TRACK_VRM:
			if (!LOAD_NativeTrackFile(curr))
			{
				fprintf(stderr, "Could not load custom track %d (%s)\n", curr->subfileIndex,
				        curr->type_UNUSED == LT_NATIVE_TRACK_VRM ? "VRM" : "LEV");
				// Discard pending level files and return through the normal menu load.
				sdata->queueLength = 1;
				sdata->queueReady = 1;
				sdata->load_inProgress = 0;
				sdata->mainMenuState = MAIN_MENU_TRACK_SELECT;
				sdata->Loading.OnBegin.AddBitsConfig0 |= MAIN_MENU;
				MainRaceTrack_RequestLoad(MAIN_MENU_LEVEL);
			}
			break;
		}

		sdata->queueLength--;
	}

	if (sdata->frameFinishedVRAM != 0)
	{
		if ((u32)(sdata->gGT->frameTimer_VsyncCallback - sdata->frameFinishedVRAM) >= LOAD_QUEUE_VRAM_CALLBACK_DELAY_FRAMES)
		{
			if (curr->callbackFuncPtr != NULL)
			{
				curr->callbackFuncPtr(curr);
			}

			sdata->frameFinishedVRAM = 0;

			// Mempack allocations are marked with a host-only flag
			// where the retail path uses LT_SETADDR for the same ownership.
			if ((curr->flags & (LT_SETADDR | LT_MEMPACK)) != 0)
			{
				MEMPACK_PopState();
			}

			sdata->queueReady = 1;
		}
	}
}
