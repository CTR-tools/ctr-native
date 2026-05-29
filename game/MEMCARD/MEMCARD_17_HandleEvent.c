#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003ddac-0x8003e238.
int MEMCARD_HandleEvent(void)
{
	int event;
	int stage;
	int offset;
	int size;
	u8 *writePtr;

	switch (sdata->memcard_stage)
	{
	case MC_STAGE_GETINFO:
		event = MEMCARD_GetNextSwEvent();

		if (event == MC_RETURN_IOE)
		{
			if ((sdata->memcardStatusFlags & 1) != 0)
			{
				sdata->memcard_stage = MC_STAGE_NEWCARD;
				MEMCARD_SkipEvents();

				while (_card_load(sdata->memcardSlot) != 1)
					;

				return MC_RETURN_PENDING;
			}

			if ((sdata->memcardStatusFlags & 2) == 0)
			{
				sdata->memoryCard_SizeRemaining = 0;
				event = MC_RETURN_UNFORMATTED;
			}
		}
		else if (event == MC_RETURN_NEWCARD)
		{
			MEMCARD_SkipEvents();

			while (_card_clear(sdata->memcardSlot) != 1)
				;

			event = MEMCARD_WaitForHwEvent();

			if (event == MC_RETURN_IOE)
			{
				sdata->memcard_stage = MC_STAGE_NEWCARD;
				MEMCARD_SkipEvents();

				while (_card_load(sdata->memcardSlot) != 1)
					;

				return MC_RETURN_PENDING;
			}
		}
		else if (event == MC_RETURN_PENDING)
		{
			return MC_RETURN_PENDING;
		}
		else
		{
			sdata->memcard_stage = MC_STAGE_IDLE;
			sdata->memoryCard_SizeRemaining = 0;
			return event;
		}

		sdata->memcard_stage = MC_STAGE_IDLE;
		return event;

	case MC_STAGE_NEWCARD:
		event = MEMCARD_GetNextSwEvent();

		if (event == MC_RETURN_IOE)
		{
			sdata->memcard_stage = MC_STAGE_IDLE;
			sdata->memcardStatusFlags = (sdata->memcardStatusFlags | 2) & ~1;
			MEMCARD_GetFreeBytes(sdata->memcardSlot);
			return MC_RETURN_NEWCARD;
		}

		if (event == MC_RETURN_NEWCARD)
		{
			sdata->memcard_stage = MC_STAGE_IDLE;
			sdata->memcardStatusFlags &= ~3;
			return MC_RETURN_UNFORMATTED;
		}

		if (event == MC_RETURN_PENDING)
			return MC_RETURN_PENDING;

		sdata->memcard_stage = MC_STAGE_IDLE;
		return event;

	case MC_STAGE_LOAD_PART0_START:
		event = MEMCARD_GetNextSwEvent();

		if (event == MC_RETURN_IOE)
		{
			sdata->memcard_stage = MC_STAGE_LOAD_PART2_READ;
			sdata->memcardIconSize = ((sdata->memcard_ptrStart[2] & 0xf) + 1) << 7;

			event = MEMCARD_ReadFile(sdata->memcardIconSize, sdata->memcardFileSize);

			if ((((sdata->memcardIconSize + sdata->memcardFileSize + 0x1fff) >> 13) < sdata->memcard_ptrStart[3]) &&
			    (((sdata->memcardIconSize + sdata->memcardFileSize * 2 + 0x1fff) >> 13) >= 2))
			{
				sdata->memcardStatusFlags &= ~4;
			}
			else
			{
				sdata->memcardStatusFlags |= 4;
			}

			return event;
		}

		if (event == MC_RETURN_PENDING)
			return MC_RETURN_PENDING;

		if (sdata->memcard_remainingAttempts > 0)
		{
			sdata->memcard_remainingAttempts--;
			return MEMCARD_ReadFile(0, 0x80);
		}

		MEMCARD_CloseFile();
		return event;

	case MC_STAGE_LOAD_PART2_READ:
	case MC_STAGE_LOAD_PART4_READ:
		event = MEMCARD_GetNextSwEvent();

		if (event == MC_RETURN_IOE)
		{
			sdata->crc16_checkpoint_byteIndex = 0;
			sdata->crc16_checkpoint_status = 0;
			sdata->memcard_stage++;

			if ((sdata->memcardStatusFlags & 8) == 0)
				return MC_RETURN_PENDING;

			goto ChecksumLoad;
		}

		if (event == MC_RETURN_PENDING)
			return MC_RETURN_PENDING;

		if (sdata->memcard_remainingAttempts > 0)
		{
			stage = sdata->memcard_stage;
			sdata->memcard_remainingAttempts--;
			return MEMCARD_ReadFile(sdata->memcardIconSize + (stage - MC_STAGE_LOAD_PART2_READ) * sdata->memcardFileSize, sdata->memcardFileSize);
		}

		MEMCARD_CloseFile();
		return event;

	case MC_STAGE_LOAD_PART3_CHECK:
	case MC_STAGE_LOAD_PART5_CHECK:
	ChecksumLoad:
		event = MEMCARD_ChecksumLoad(sdata->memcard_ptrStart, sdata->memcardFileSize);

		if (event != MC_RETURN_IOE)
		{
			if (event == MC_RETURN_PENDING)
				return MC_RETURN_PENDING;

			stage = sdata->memcard_stage;

			if (((sdata->memcardStatusFlags & 4) == 0) && (stage < MC_STAGE_LOAD_PART5_CHECK))
			{
				sdata->memcard_stage = stage + 1;
				return MEMCARD_ReadFile(sdata->memcardIconSize + (stage - MC_STAGE_LOAD_PART0_START) * sdata->memcardFileSize, sdata->memcardFileSize);
			}
		}

		MEMCARD_CloseFile();
		return event;

	case MC_STAGE_SAVE_PART0_START:
	case MC_STAGE_SAVE_PART1_ICON:
	case MC_STAGE_SAVE_PART2_WRITE:
		event = MEMCARD_GetNextSwEvent();

		if (event == MC_RETURN_IOE)
		{
			stage = sdata->memcard_stage;

			if ((stage != MC_STAGE_SAVE_PART0_START) && ((stage > MC_STAGE_SAVE_PART1_ICON) || ((sdata->memcardStatusFlags & 4) != 0)))
			{
				MEMCARD_CloseFile();
				MEMCARD_GetFreeBytes(sdata->memcardSlot);
				return MC_RETURN_IOE;
			}

			sdata->memcard_stage = stage + 1;
			offset = sdata->memcardIconSize + (stage - MC_STAGE_SAVE_PART0_START) * sdata->memcardFileSize;
			return MEMCARD_WriteFile(offset, sdata->memcard_ptrStart, sdata->memcardFileSize);
		}

		if (event == MC_RETURN_PENDING)
			return MC_RETURN_PENDING;

		if (sdata->memcard_remainingAttempts > 0)
		{
			stage = sdata->memcard_stage;
			sdata->memcard_remainingAttempts--;

			if (stage == MC_STAGE_SAVE_PART0_START)
			{
				return MEMCARD_WriteFile(0, (u8 *)&data.memcardIcon_PsyqHand[0], sdata->memcardIconSize);
			}

			offset = sdata->memcardIconSize + (stage - MC_STAGE_SAVE_PART1_ICON) * sdata->memcardFileSize;
			writePtr = sdata->memcard_ptrStart;
			size = sdata->memcardFileSize;
			return MEMCARD_WriteFile(offset, writePtr, size);
		}

		MEMCARD_CloseFile();
		return event;

	case MC_STAGE_ERASE_FAIL:
		sdata->memcard_stage = MC_STAGE_IDLE;
		return MC_RETURN_TIMEOUT;

	case MC_STAGE_ERASE_PASS:
		sdata->memcard_stage = MC_STAGE_IDLE;
		return MC_RETURN_IOE;

	default:
		return MC_RETURN_TIMEOUT;
	}
}
