#include <common.h>

void GhostTape_Start(void)
{
	struct GhostRecording *recording = &GHOST_RECORDING;
	s16 characterID;
	// NOTE(aalhendi): These bindings retain retail's short-lived lookup registers.
	register s16 *characters CTR_PSX_REGISTER("v1");
	struct GhostHeader *gh = recording->ptrGhost;
	struct GameTracker *gGT = GAME_TRACKER;
	register struct Driver *d CTR_PSX_REGISTER("a2");

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): A failed player birth must not enable a recording whose
	// first sample would dereference a missing thread, driver or instance.
	if (gh == NULL || gGT->threadBuckets[PLAYER].thread == NULL || gGT->threadBuckets[PLAYER].thread->object == NULL ||
	    ((struct Driver *)gGT->threadBuckets[PLAYER].thread->object)->instSelf == NULL)
	{
		GHOST_CAN_SAVE = 0;
		return;
	}
#endif
	d = gGT->threadBuckets[PLAYER].thread->object;
	gh->levelID = gGT->levelID;
	// The header version is serialized as two little-endian bytes.
	((s8 *)&gh->version)[0] = GHOST_TAPE_VERSION_RETAIL;
	((s8 *)&gh->version)[1] = -1;
	characters = GAME_CHARACTER_IDS;
	characterID = characters[d->driverID];
	GHOST_TOO_BIG = 0;
	GHOST_OVERFLOW_TIMER = 0;
	GHOST_CAN_SAVE = 1;
	gh->characterID = characterID;
	recording->frameCount = 0;
	recording->sampleCount = 0;
	recording->animFrame = -1;
	recording->animIndex = -1;
	recording->timeOfLast80buffer = 0;
	recording->timeElapsedInRace = 0;
	recording->VelX = 0;
	recording->VelY = 0;
	recording->VelZ = 0;
	recording->instanceFlags = 0;
	recording->boostCooldown1E = 0;
	recording->ptrCurrOffset = recording->ptrStartOffset;
}


void GhostTape_End(void)
{
	struct Driver *d;
	struct GhostHeader *gh;

	// Overflowed or inactive recordings have no final packet to write.
	if (GHOST_CAN_SAVE == 0)
	{
		return;
	}

	d = GAME_TRACKER->threadBuckets[PLAYER].thread->object;
	// Write the last chunk of ghost data
	GhostTape_WriteMoves(1);

	gh = GHOST_RECORDING.ptrGhost;
	gh->size = (u16)(u32)GHOST_RECORDING.ptrCurrOffset - (u16)(u32)GHOST_RECORDING.ptrStartOffset;
	gh->ySpeed = d->ySpeed;
	gh->speedApprox = d->speedApprox;
	gh->timeElapsedInRace = GAME_TRACKER->drivers[0]->timeElapsedInRace;
	GHOST_CAN_SAVE = 0;
}


void GhostTape_WriteMoves(s16 raceFinished)
{
	s16 velocity[3];
	s16 position[3];
	struct Driver *driver;
	struct Instance *inst;
	s32 timeSincePositionPacket;

	if (raceFinished == 0)
	{
		struct GameTracker *gGT;
		u32 gameMode;
		if (GHOST_CAN_SAVE == 0)
		{
			return;
		}
		gGT = GAME_TRACKER;
		gameMode = gGT->gameMode1;
		if (gameMode & GAME_MODE_GHOST_RECORD_BLOCK_MASK)
		{
			return;
		}
		if (gGT->trafficLightsTimer > 0)
		{
			return;
		}
		if (gameMode & END_OF_RACE)
		{
			GhostTape_End();
			return;
		}
	}
	if (GHOST_RECORDING.boostCooldown1E != 0)
	{
		GHOST_RECORDING.boostCooldown1E--;
	}
	if (raceFinished || !(GHOST_RECORDING.frameCount & GHOST_RECORD_INTERVAL_MASK_8))
	{
		driver = GAME_TRACKER->threadBuckets[PLAYER].thread->object;
		inst = driver->instSelf;
		position[0] = inst->matrix.t[0] >> GHOST_RECORD_POSITION_SHIFT;
		position[1] = inst->matrix.t[1] >> GHOST_RECORD_POSITION_SHIFT;
		position[2] = inst->matrix.t[2] >> GHOST_RECORD_POSITION_SHIFT;
		velocity[0] = position[0] - GHOST_RECORDING.VelX;
		velocity[1] = position[1] - GHOST_RECORDING.VelY;
		velocity[2] = position[2] - GHOST_RECORDING.VelZ;
		timeSincePositionPacket = GHOST_RECORDING.timeElapsedInRace - GHOST_RECORDING.timeOfLast80buffer;

		if (GHOST_RECORDING.animFrame != inst->animFrame || GHOST_RECORDING.animIndex != inst->animIndex)
		{
			char *writeCursor;
			*GHOST_RECORDING.ptrCurrOffset++ = GHOST_OP_ANIMATION;
			writeCursor = GHOST_RECORDING.ptrCurrOffset;
			writeCursor[1] = (GHOST_RECORDING.animFrame = inst->animFrame);
			writeCursor[0] = (GHOST_RECORDING.animIndex = inst->animIndex);
			GHOST_RECORDING.ptrCurrOffset += GHOST_SIZE_ANIMATION - 1;
		}
		if ((inst->flags & GHOST_RECORD_INSTANCE_SPLIT_FLAG) != (GHOST_RECORDING.instanceFlags & GHOST_RECORD_INSTANCE_SPLIT_FLAG))
		{
			*GHOST_RECORDING.ptrCurrOffset++ = GHOST_OP_INSTANCE;
			*GHOST_RECORDING.ptrCurrOffset++ = (inst->flags >> GHOST_RECORD_INSTANCE_SPLIT_SHIFT) & 1;
		}

		// A full position anchors each group; small deltas fill its intermediate frames.
		if (raceFinished || !(GHOST_RECORDING.sampleCount & GHOST_RECORD_INTERVAL_MASK_32) || velocity[0] >= GHOST_RECORD_VELOCITY_MAX ||
		    velocity[0] <= GHOST_RECORD_VELOCITY_MIN_EXCLUSIVE || velocity[1] >= GHOST_RECORD_VELOCITY_MAX ||
		    velocity[1] <= GHOST_RECORD_VELOCITY_MIN_EXCLUSIVE || velocity[2] >= GHOST_RECORD_VELOCITY_MAX ||
		    velocity[2] <= GHOST_RECORD_VELOCITY_MIN_EXCLUSIVE || timeSincePositionPacket >= GHOST_RECORD_TIME_DELTA_MAX_EXCLUSIVE)
		{
			char *writeCursor;
			*GHOST_RECORDING.ptrCurrOffset++ = GHOST_OP_POSITION;
			writeCursor = GHOST_RECORDING.ptrCurrOffset;
			writeCursor[0] = (u16)position[0] >> 8;
			writeCursor[1] = ((u8 *)&position[0])[0];
			writeCursor[2] = (u16)position[1] >> 8;
			writeCursor[3] = ((u8 *)&position[1])[0];
			writeCursor[4] = (u16)position[2] >> 8;
			writeCursor[5] = ((u8 *)&position[2])[0];
			writeCursor[8] = (u16)driver->rotCurr.y >> GHOST_RECORD_ROTATION_SHIFT;
			writeCursor[9] = (u16)driver->rotCurr.z >> GHOST_RECORD_ROTATION_SHIFT;
			writeCursor[6] = timeSincePositionPacket >> 8;
			writeCursor[7] = timeSincePositionPacket;
			GHOST_RECORDING.timeOfLast80buffer = GHOST_RECORDING.timeElapsedInRace;
			GHOST_RECORDING.ptrCurrOffset += GHOST_SIZE_POSITION - 1;
		}
		else if (velocity[0] == 0 && velocity[1] == 0 && velocity[2] == 0)
		{
			*GHOST_RECORDING.ptrCurrOffset++ = GHOST_OP_IDLE;
		}
		else
		{
			char *writeCursor;
			// Velocity packets have no opcode; -128..-124 are reserved above.
			writeCursor = GHOST_RECORDING.ptrCurrOffset;
			writeCursor[0] = ((u8 *)&velocity[0])[0];
			writeCursor[1] = ((u8 *)&velocity[1])[0];
			writeCursor[2] = ((u8 *)&velocity[2])[0];
			writeCursor[3] = (u16)driver->rotCurr.y >> GHOST_RECORD_ROTATION_SHIFT;
			writeCursor[4] = (u16)driver->rotCurr.z >> GHOST_RECORD_ROTATION_SHIFT;
			GHOST_RECORDING.ptrCurrOffset += GHOST_SIZE_VELOCITY;
		}
		GHOST_RECORDING.VelX = position[0];
		GHOST_RECORDING.VelY = position[1];
		GHOST_RECORDING.VelZ = position[2];
		GHOST_RECORDING.instanceFlags = inst->flags;
		if ((u32)GHOST_RECORDING.ptrEndOffset < (u32)GHOST_RECORDING.ptrCurrOffset + GHOST_RECORD_BUFFER_END_GUARD)
		{
			GHOST_CAN_SAVE = 0;
			if (!(GAME_TRACKER->gameMode1 & END_OF_RACE))
			{
				GHOST_TOO_BIG = 1;
				GHOST_OVERFLOW_TIMER = GHOST_RECORD_OVERFLOW_TEXT_FRAMES;
			}
		}
		GHOST_RECORDING.sampleCount++;
	}
	GHOST_RECORDING.frameCount++;
	GHOST_RECORDING.timeElapsedInRace += GAME_TRACKER->elapsedTimeMS;
}


void GhostTape_WriteBoosts(s32 addReserve, u32 type, s32 speedCap)
{
	char *writeCursor;

	if (GHOST_CAN_SAVE == 0)
	{
		return;
	}

	if ((type & TURBO_PAD) != 0)
	{
		if (GHOST_RECORDING.boostCooldown1E != 0)
		{
			return;
		}
		GHOST_RECORDING.boostCooldown1E = GHOST_RECORD_BOOST_COOLDOWN_FRAMES;
	}

	// Boost payload: big-endian reserves, boost type, big-endian speed cap.
	*GHOST_RECORDING.ptrCurrOffset++ = GHOST_OP_BOOST;
	writeCursor = GHOST_RECORDING.ptrCurrOffset;

	writeCursor[0] = (char)(addReserve >> 8);
	writeCursor[1] = (char)addReserve;

	writeCursor[2] = type;

	writeCursor[3] = (char)(speedCap >> 8);
	writeCursor[4] = (char)speedCap;

	GHOST_RECORDING.ptrCurrOffset += GHOST_SIZE_BOOST - 1;
}


void GhostTape_Destroy(void)
{
	if (GHOST_PLAYING != 0)
	{
		MEMPACK_ClearHighMem();
		GHOST_PLAYING = 0;
	}
}
