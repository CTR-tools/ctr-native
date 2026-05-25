#include <common.h>

#ifdef REBUILD_PC
#define BOTS_KILLPLANE_TINY_ARENA_NAME "asphalt2"
#else
#define BOTS_KILLPLANE_TINY_ARENA_NAME rdata.s_asphalt2_thisAppearsTwice
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013a70-0x80013c18.
void BOTS_Killplane(struct Thread *botThread)
{
	s16 i;
	char boolOverride;
	u8 currNav;
	u8 backCount;
	u8 override;
	struct NavFrame *frame;
	struct Driver *bot;

	// get object from thread
	bot = botThread->object;

	boolOverride = false;

	// check for Tiny Arena
	if (strcmp(sdata->gGT->levelName, BOTS_KILLPLANE_TINY_ARENA_NAME) == 0)
	{
		// edge-case override?
		switch (bot->unknown_lap_related[1])
		{
		case 0x94:
			override = 0x84;
			break;
		case 0xa0:
			override = 0x80;
			break;
		default:
			override = 0xff;
		}

		if (override != 0xff)
		{
			// pointer to nav point
			frame = bot->botData.botNavFrame;

			// goBackCount
			backCount = frame->goBackCount;
			boolOverride = (backCount < (override - 1));

			while ((boolOverride || (override + 1 < backCount)))
			{
				// nav path index
				i = bot->botData.botPath;

				// go back to previous point
				frame -= 1;

				// if this is less than address of first nav point
				if (frame < sdata->NavPath_ptrNavFrameArray[i])
				{
					// go to last nav point
					frame = &sdata->NavPath_ptrHeader[i]->last[-1];
				}

				backCount = frame->goBackCount;
				boolOverride = (backCount < (override - 1));
			}
			bot->botData.botNavFrame = frame;
			boolOverride = true;
		}
	}

	// if not Tiny Arena, or goBackCount didn't happen
	if (!boolOverride)
	{
		// pointer to navFrame
		frame = bot->botData.botNavFrame;

		// current nav point (player turned AI)
		currNav = bot->unknown_lap_related[1];

		// goBackCount
		backCount = frame->goBackCount;

		while ((backCount == currNav || ((frame->flags & 0x4000) != 0)))
		{
			// nav path index
			i = bot->botData.botPath;

			// go back one navFrame
			frame -= 1;

			// if you go back to far
			if (frame < sdata->NavPath_ptrNavFrameArray[i])
			{
				// loop back to last navFrame
				frame = &sdata->NavPath_ptrHeader[i]->last[-1];
			}
			backCount = frame->goBackCount;
			currNav = bot->unknown_lap_related[1];
		}
		// save ptr to nav frame
		bot->botData.botNavFrame = frame;
	}

	BOTS_MaskGrab(botThread);
	return;
}

#undef BOTS_KILLPLANE_TINY_ARENA_NAME
