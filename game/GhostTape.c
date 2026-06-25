#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80027df4-0x80027e90.
void GhostTape_Start(void)
{
	struct GhostHeader *gh;
	struct Driver *d;
	struct GameTracker *gGT = sdata->gGT;

	d = gGT->drivers[0];

	// v1 - PizzaHut (June), Spyro2 (July)
	// v4 - Aug5, Aug14, Sep3, Retail

	gh = sdata->GhostRecording.ptrGhost;
	gh->version = -4;
	gh->levelID = gGT->levelID;
	gh->characterID = data.characterIDs[d->driverID];

	sdata->GhostRecording.VelX = 0;
	sdata->GhostRecording.VelY = 0;
	sdata->GhostRecording.VelZ = 0;

	sdata->GhostRecording.timeElapsedInRace = 0;
	sdata->boolGhostTooBigToSave = 0;
	sdata->ghostOverflowTextTimer = 0;
	sdata->boolCanSaveGhost = 1;

	sdata->GhostRecording.ptrCurrOffset = sdata->GhostRecording.ptrStartOffset;

	sdata->GhostRecording.countEightFrames = 0;
	sdata->GhostRecording.countSixteenFrames = 0;
	sdata->GhostRecording.timeOfLast80buffer = 0;
	sdata->GhostRecording.boostCooldown1E = 0;

	sdata->GhostRecording.animFrame = -1;
	sdata->GhostRecording.animIndex = -1;
	sdata->GhostRecording.instanceFlags = 0;

	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80027e90-0x80027f20
void GhostTape_End(void)
{
	struct Driver *d;
	struct GhostHeader *gh;
	struct GameTracker *gGT = sdata->gGT;

	// quit, if ghost cant be saved
	if (sdata->boolCanSaveGhost == 0)
	{
		return;
	}

	// dont save ghost twice
	sdata->boolCanSaveGhost = 0;

	// Write the last chunk of ghost data
	GhostTape_WriteMoves(1);

	d = gGT->drivers[0];
	gh = sdata->GhostRecording.ptrGhost;

	gh->ySpeed = d->ySpeed;
	gh->speedApprox = d->speedApprox;
	gh->timeElapsedInRace = d->timeElapsedInRace;
	gh->size = (u32)sdata->GhostRecording.ptrCurrOffset - (u32)sdata->GhostRecording.ptrStartOffset;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80027f20-0x8002838c.
void GhostTape_WriteMoves(s16 raceFinished)
{
	char *pbVar1;
	int iVar3;
	int iVar4;
	int iVar6;
	struct Instance *iVar7;
	struct Driver *iVar8;
	int iVar9;
	char local_10;
	char local_e;
	char local_c;
	struct GameTracker *gGT = sdata->gGT;
	u32 gameMode = gGT->gameMode1;

	if (raceFinished == 0)
	{
		// if you can not save ghost
		if (sdata->boolCanSaveGhost == 0)
		{
			return;
		}

		// if paused or [race ended while not yet in end-of-race menu]???
		if ((gameMode & 0x1f) != 0)
		{
			return;
		}

		// if traffic lights are not done counting down
		if (0 < gGT->trafficLightsTimer)
		{
			return;
		}

		// If you're in End-Of-Race menu
		if ((gameMode & 0x200000) != 0)
		{
			GhostTape_End();
			return;
		}
	}
	if (sdata->GhostRecording.boostCooldown1E != 0)
	{
		sdata->GhostRecording.boostCooldown1E--;
	}

	if (

	    // If race is just finished
	    (raceFinished != 0) ||

	    // This is true every 8 frames
	    ((sdata->GhostRecording.countEightFrames & 7) == 0))
	{
		iVar8 = gGT->threadBuckets[0].thread->object;

		// player instance
		iVar7 = iVar8->instSelf;

		// compress position (x, y, z) with bitshifting
		iVar4 = iVar7->matrix.t[0] >> 3;
		iVar3 = iVar7->matrix.t[1] >> 3;
		iVar6 = iVar7->matrix.t[2] >> 3;

		// get change in position (x, y, z)
		sdata->GhostRecording.VelX = (s16)iVar4 - sdata->GhostRecording.VelX;
		sdata->GhostRecording.VelY = (s16)iVar3 - sdata->GhostRecording.VelY;
		sdata->GhostRecording.VelZ = (s16)iVar6 - sdata->GhostRecording.VelZ;

		// Time elapsed since last 0x80 buffer
		iVar9 = sdata->GhostRecording.timeElapsedInRace - sdata->GhostRecording.timeOfLast80buffer;

		// get pointer to current recording char in buffer
		pbVar1 = sdata->GhostRecording.ptrCurrOffset;

		if (
		    // if animation frame changed
		    (sdata->GhostRecording.animFrame != iVar7->animFrame) ||

		    // if animation changed
		    (sdata->GhostRecording.animIndex != iVar7->animIndex))
		{
			sdata->GhostRecording.animFrame = iVar7->animFrame;
			sdata->GhostRecording.animIndex = iVar7->animIndex;

			pbVar1[0] = 0x81;
			pbVar1[1] = iVar7->animIndex;
			pbVar1[2] = iVar7->animFrame;
			pbVar1 += 3;
		}

		// If there is a change in instance flags,
		// determine if driver is split by water or mud
		if ((iVar7->flags & 0x2000) != (sdata->GhostRecording.instanceFlags & 0x2000))
		{
			// Record the instance flags
			// determine if driver is split by water or mud

			pbVar1[0] = 0x83;
			pbVar1[1] = (char)(iVar7->flags >> 0xd) & 1;
			pbVar1 += 2;
		}

		// If velocity is small enough for a compressed 5-char message
		if (
		    // If the race is not over
		    (raceFinished == 0) &&

		    // false once every 16 frames
		    ((sdata->GhostRecording.countSixteenFrames & 0x1f) != 0) &&

		    // If velX is small enough for one char
		    (sdata->GhostRecording.VelX < 0x80) && (-0x7c < sdata->GhostRecording.VelX) &&

		    // If velY is small enough for one char
		    (sdata->GhostRecording.VelY < 0x80) && (-0x7c < sdata->GhostRecording.VelY) &&

		    // If velZ is small enough for one char
		    (sdata->GhostRecording.VelZ < 0x80) && (-0x7c < sdata->GhostRecording.VelZ) &&

		    // if not a lot of time has passed
		    // since the last 0x80 buffer
		    (iVar9 < 0xff01))
		{
			// If there is no change in position
			if (((sdata->GhostRecording.VelX == 0) && (sdata->GhostRecording.VelY == 0)) && (sdata->GhostRecording.VelZ == 0))
			{
				// Record that you are doing nothing
				pbVar1[0] = 0x84;
				pbVar1 += 1;
			}

			// If you are moving
			else
			{
				// dont write opcode,
				// "no opcode" means "assume velocity"

				// Write velX to buffer
				pbVar1[0] = (char)sdata->GhostRecording.VelX;
				pbVar1[1] = (char)sdata->GhostRecording.VelY;
				pbVar1[2] = (char)sdata->GhostRecording.VelZ;
				pbVar1[3] = (char)(iVar8->rotCurr.y >> 4);
				pbVar1[4] = (char)(iVar8->rotCurr.z >> 4);
				pbVar1 += 5;
			}
		}

		// If velocity is too large,
		// If the race just ended
		// If you're in a 16-frame interval
		// write a longer message
		else
		{
			// 0x80-style chunks are 11 chars long (including 0x80)

			// Write to ghost recording buffer
			pbVar1[0] = 0x80;

			// flipping endians

			// Write 2-char X position
			pbVar1[1] = (char)(iVar4 >> 8);
			pbVar1[2] = (char)iVar4;

			// Write 2-char Y position
			pbVar1[3] = (char)(iVar3 >> 8);
			pbVar1[4] = (char)iVar3;

			// Write 2-char Z position
			pbVar1[5] = (char)(iVar6 >> 8);
			pbVar1[6] = (char)iVar6;

			// Write 2-char ???
			// related to time
			pbVar1[7] = (char)(iVar9 >> 8);
			pbVar1[8] = (char)iVar9;

			// Write 2-char rotation
			pbVar1[9] = (char)(iVar8->rotCurr.y >> 4);
			pbVar1[10] = (char)(iVar8->rotCurr.z >> 4);

			pbVar1 += 11;

			// Time of last 0x80 buffer
			sdata->GhostRecording.timeOfLast80buffer = sdata->GhostRecording.timeElapsedInRace;
		}

		// Make a copy of instance flags
		sdata->GhostRecording.instanceFlags = iVar7->flags;

		if (
		    // if offset of ghost-recording buffer exceeds
		    // the maximum size of a ghost that can be recorded
		    // (if you're one frame away from max capacity)
		    ((u32)sdata->GhostRecording.ptrEndOffset < (u32)pbVar1 + 0x40) &&

		    (sdata->boolCanSaveGhost = 0,

		     // If you're not in End-Of-Race menu
		     // (if you were, you'd be just in time to save the ghost)
		     (gameMode & 0x200000) == 0))
		{
			sdata->boolGhostTooBigToSave = 1;

			// set ghostOverflowTextTimer
			// to 180 frames (6 seconds 30fps)
			sdata->ghostOverflowTextTimer = 0xb4;
		}

		// Increment frame counter
		sdata->GhostRecording.countSixteenFrames++;

		// Save this frame's X, Y, Z positions,
		// so that they can be used next frame to
		// calculate velocity
		sdata->GhostRecording.VelX = (s16)iVar4;
		sdata->GhostRecording.VelY = (s16)iVar3;
		sdata->GhostRecording.VelZ = (s16)iVar6;

		// save incremeneted pointer
		sdata->GhostRecording.ptrCurrOffset = pbVar1;
	}

	// Increment frame counter
	sdata->GhostRecording.countEightFrames++;

	// Increment race timer by elapsed milliseconds per frame, ~32
	sdata->GhostRecording.timeElapsedInRace += gGT->elapsedTimeMS;
	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002838c-0x80028410.
void GhostTape_WriteBoosts(int addReserve, u8 type, int speedCap)
{
	char *puVar1;

	// quit, if ghost cant be saved
	if (sdata->boolCanSaveGhost == 0)
	{
		return;
	}

	puVar1 = sdata->GhostRecording.ptrCurrOffset;

	if ((type & TURBO_PAD) != 0)
	{
		if (sdata->GhostRecording.boostCooldown1E != 0)
		{
			return;
		}
		sdata->GhostRecording.boostCooldown1E = 0x1e;
	}

	// 0x82-style chunks are 6 bytes long (including 0x82)

	// Write to recording buffer
	puVar1[0] = 0x82;

	// big endian reserve
	puVar1[1] = (char)((u32)addReserve >> 8);
	puVar1[2] = (char)addReserve;

	// char, add type (increment or set)
	puVar1[3] = type;

	// big endian speedCcap
	puVar1[4] = (char)((u32)speedCap >> 8);
	puVar1[5] = (char)speedCap;

	sdata->GhostRecording.ptrCurrOffset += 6;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80028410-0x8002843c.
void GhostTape_Destroy()
{
	if (sdata->ptrGhostTapePlaying != 0)
	{
		MEMPACK_ClearHighMem();
		sdata->ptrGhostTapePlaying = 0;
	}
}
