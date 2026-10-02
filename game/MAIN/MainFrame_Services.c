#include <common.h>

#ifndef MAIN_FRAME_VIDEO_DST_X
#define MAIN_FRAME_VIDEO_DST_X (sdata->videoSTR_dst_vramX)
#define MAIN_FRAME_VIDEO_DST_Y (sdata->videoSTR_dst_vramY)
#endif

void MainFrame_TogglePauseAudio(b32 bool_pause)
{
	if (bool_pause != 0)
	{
		if (sdata->boolSoundPaused == 0)
		{
			OtherFX_Stop2(1);
			howl_PauseAudio();
			sdata->boolSoundPaused = 1;
			// NOTE(aalhendi): Keep the retail publication before the branch exit.
			CTR_PSX_OBSERVE_MEMORY(sdata->boolSoundPaused);
		}
	}
	else if (sdata->boolSoundPaused)
	{
		howl_StopAudio(0, 0, 1);
		howl_UnPauseAudio();
		sdata->boolSoundPaused = 0;
	}
	return;
}

void MainFrame_InitVideoSTR(u32 boolPlayVideoStr, RECT *r, s16 posX, s16 posY)
{
	s16 height;

#ifdef CTR_NATIVE
	if (r == NULL)
	{
		// NOTE(aalhendi): Native overlay 230 uses NULL to disable STR copy;
		// retail blindly copies the RECT fields in the PSX path below.
		sdata->videoSTR_src_vramRect.x = 0;
		sdata->videoSTR_src_vramRect.y = 0;
		sdata->videoSTR_src_vramRect.w = 0;
		height = 0;
	}
	else
#endif
	{
		sdata->videoSTR_src_vramRect.x = r->x;
		sdata->videoSTR_src_vramRect.y = r->y;
		sdata->videoSTR_src_vramRect.w = r->w;
		height = r->h;
	}

	sdata->boolPlayVideoSTR = boolPlayVideoStr;
	// NOTE(aalhendi): Retail publishes STR state before destination fields
	// and completes the final RECT halfword before returning.
	CTR_PSX_OBSERVE_MEMORY(sdata->boolPlayVideoSTR);
	MAIN_FRAME_VIDEO_DST_X = posX;
	MAIN_FRAME_VIDEO_DST_Y = posY;
	sdata->videoSTR_src_vramRect.h = height;
	CTR_PSX_OBSERVE_MEMORY(sdata->videoSTR_src_vramRect.h);
}

b32 MainFrame_HaveAllPads(s16 numPlyrNextGame)
{
	s16 allPads = true;
	s16 i;
	struct GamepadSystem *pads;

	if (sdata->Loading.stage != LOAD_IDLE)
	{
		return true;
	}
	if (numPlyrNextGame == 0)
	{
		goto noPads;
	}
	i = 0;
	if (numPlyrNextGame > 0)
	{
		pads = sdata->gGamepads;
		do
		{
			s16 plugged = false;
			if (allPads)
			{
				struct ControllerPacket *packet = pads->gamepad[i].ptrControllerPacket;
				if (packet != NULL)
				{
					plugged = packet->plugged == PLUGGED;
				}
			}
			allPads = plugged;
			i++;
		} while (i < numPlyrNextGame);
	}
	goto padsDone;
noPads:
	allPads = false;
padsDone:
	return allPads;
}

// Request Aku Hint; AH_MaskHint_Start begins it later.
// hintId:
//	0x00 - Welcome to Adventure Arena
//	0x01 - using a warp pad (part of welcome)
// 	0x02 - Need more trophies
//	0x03 - Need 4 trophies for Boss
//	0x04 - Need 4 keys for oxide
//	0x05 - Must have 1 Boss Key
//	0x06 - This is the load/save screen
// 	0x07 - Congrats on opening new area
//	0x12 - Must have 2 Boss Key
//	0x19 - Collect every crystal in arena
//	0x1a - CTR Token
//	0x1b - Gem Cups
//	0x1c - Must get 10 relics
//	0x1d - Relic
// interruptWarpPad:
//	0x00 - not interrupting a warppad load screen
// 	0x01 - interrupting (CTR, Relic, or Crystal hints)
void MainFrame_RequestMaskHint(s16 hintId, s16 interruptWarpPad)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *driver;

	if (((gGT->gameMode1 & PAUSE_ALL) == 0) && (sdata->AkuHint_RequestedHint == -1))
	{
		sdata->AkuAkuHintState = 1;

		// NOTE(aalhendi): Publish the callback after its hint fields are ready.
		driver = gGT->drivers[0];

		sdata->AkuHint_RequestedHint = hintId;
		sdata->AkuHint_boolInterruptWarppad = interruptWarpPad;
		driver->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_FreezeEndEvent_Init;
	}
	return;
}
