#include <common.h>

#if defined(CTR_NATIVE)
// NOTE(aalhendi): Allocation labels live in resident read-only data on PSX;
// native does not load that executable region as live string storage.
#define GHOST_RECORD_BUFFER_NAME "ghost record buffer"
#define GHOST_TAPE_NAME          "ghost tape"
#else
#define GHOST_RECORD_BUFFER_NAME rdata.s_ghost_record_buffer
#define GHOST_TAPE_NAME          rdata.s_GhostTape_
#endif

internal inline s32 Ghost_LerpRot12(s32 curr, s32 next, s32 t)
{
	s32 delta = (next - curr) & 0xfff;
	if (delta > 0x7ff)
	{
		delta -= 0x1000;
	}
	return (curr + ((delta * t) >> 0xc)) & 0xfff;
}

void GhostReplay_ThTick(struct Thread *t)
{
	struct Driver *d = t->object;
	struct GhostTape *tape;
	struct Instance *inst;
	SVec3 tmpPos, local_rot;
	s32 vel[3];
	s32 timeInRace, scaledNum, packetIdx, lerp4096;
	struct GhostPacket *packet, *currPacket, *nextPacket;
	u8 *buffer;

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail checks the driver only after dereferencing it.
	// Native must reject incomplete pool objects before reading their fields.
	if (d == NULL)
	{
		return;
	}
#endif
	tape = d->ghostTape;
	inst = d->instSelf;
#if defined(CTR_NATIVE)
	if (inst == NULL)
	{
		return;
	}
	if (tape == NULL)
	{
		goto hidden;
	}
	// NOTE(aalhendi): Valid tapes anchor their first position before any delta.
	// Initialize native scratch state too, rather than expose stale stack data.
	tmpPos.x = tmpPos.y = tmpPos.z = 0;
#endif

	inst->scale.x = inst->scale.y = inst->scale.z = 0xccc;

	// The human replay thread also owns the six-second recording warning.
	if (GHOST_OVERFLOW_TIMER != 0 && d->ghostID == 0)
	{
		s16 color = 0x8004;
		if (GHOST_OVERFLOW_TIMER & 1)
		{
			color = 0x8003;
		}
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_GHOST_DATA_OVERFLOW], 0x100, 0x28, 2, color);
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_CAN_NOT_SAVE_GHOST_DATA], 0x100, 0x32, 2, color);
		GHOST_OVERFLOW_TIMER--;
	}

	if (!GHOST_DRAWING || (GAME_TRACKER->gameMode1 & DEBUG_MENU) || d == NULL || d->ghostTape->ptrEnd == d->ghostTape->ptrStart || !d->ghostBoolInit)
	{
		goto hidden;
	}

	d->reserves = (u16)d->reserves - (u16)GAME_TRACKER->elapsedTimeMS;
	if (d->reserves < 0)
	{
		d->reserves = 0;
	}

	if (GAME_TRACKER->trafficLightsTimer < 1 && !d->ghostBoolStarted)
	{
		d->ghostBoolStarted = 1;
		d->ghostTape->packetID = -1;
	}

	inst->alphaScale = 0xa00;
	inst->flags &= ~HIDE_MODEL;
	inst->flags &= ~(DRAW_TRANSPARENT | GHOST_DRAW_TRANSPARENT);
	inst->flags |= GHOST_DRAW_TRANSPARENT;
	timeInRace = tape->timeElapsedInRace;
	if (timeInRace < 0)
	{
		timeInRace = 0;
	}

	packet = tape->packets;
	if (tape->timeInPacket32 <= timeInRace)
	{
		s16 opcodePos = 0;
		u8 *packetEndChain;
		buffer = tape->ptrCurr;
		packetEndChain = buffer;

		tape->packetID = -1;
		tape->timeInPacket01 = tape->timeInPacket32_backup;

		// Each cache spans two absolute positions and their intervening deltas.
		do
		{
			u8 opcode;
			if (tape->ptrEnd <= (void *)buffer)
			{
				goto finished;
			}

			opcode = *buffer++;
			if (GHOST_IS_OPCODE(opcode))
			{
				switch (opcode)
				{
				case GHOST_OP_POSITION:
					packet->pos.x = tmpPos.x = (s16)Ghost_ReadBE16(buffer) * 8;
					packet->pos.y = tmpPos.y = (s16)Ghost_ReadBE16(buffer + 2) * 8;
					packet->pos.z = tmpPos.z = (s16)Ghost_ReadBE16(buffer + 4) * 8;
					packet->rot.x = 0;
					packet->rot.y = buffer[8] << 4;
					packet->rot.z = buffer[9] << 4;

					if (opcodePos == 1)
					{
						s16 elapsed = Ghost_ReadBE16(buffer + 6);
						tape->ptrCurr = buffer - 1;
						tape->timeInPacket32 = (tape->timeInPacket32_backup += elapsed);
					}
					opcodePos++;
					buffer += GHOST_SIZE_POSITION - 1;
					packet->bufferPacket = packetEndChain;
					packetEndChain = buffer;
					packet++;
					break;

				case GHOST_OP_ANIMATION:
					buffer += GHOST_SIZE_ANIMATION - 1;
					break;
				case GHOST_OP_BOOST:
					buffer += GHOST_SIZE_BOOST - 1;
					break;
				case GHOST_OP_INSTANCE:
					buffer += GHOST_SIZE_INSTANCE - 1;
					break;
				case GHOST_OP_IDLE:
					packet->pos.x = tmpPos.x;
					packet->pos.y = tmpPos.y;
					packet->pos.z = tmpPos.z;
					packet->rot.x = packet[-1].rot.x;
					packet->rot.y = packet[-1].rot.y;
					packet->rot.z = packet[-1].rot.z;
					packet->bufferPacket = packetEndChain;
					packetEndChain = buffer;
					packet++;
					break;
				}
			}
			else
			{
				packet->pos.x = (tmpPos.x += (s8)opcode * 8);
				packet->pos.y = (tmpPos.y += (s8)buffer[0] * 8);
				packet->pos.z = (tmpPos.z += (s8)buffer[1] * 8);
				packet->rot.x = 0;
				packet->rot.y = buffer[2] << 4;
				packet->rot.z = buffer[3] << 4;
				packet->bufferPacket = packetEndChain;
				buffer += GHOST_SIZE_VELOCITY - 1;
				packetEndChain = buffer;
				packet++;
			}
		}

		while (opcodePos < 2);

		tape->numPacketsInArray = packet - tape->packets - 1;
		if (tape->numPacketsInArray < 0)
		{
			tape->numPacketsInArray = 1;
		}
		tape->timeBetweenPackets = tape->timeInPacket32 - tape->timeInPacket01;
		if (tape->timeBetweenPackets == 0)
		{
			tape->timeBetweenPackets = 1;
		}
	}

	scaledNum = (s32)(((u32)timeInRace - tape->timeInPacket01) * tape->numPacketsInArray << 12);
	lerp4096 = scaledNum / tape->timeBetweenPackets;
	packetIdx = lerp4096 >> 12;
	lerp4096 &= 0xfff;
	if (tape->numPacketsInArray <= packetIdx)
	{
		packetIdx = tape->numPacketsInArray - 1;
		lerp4096 = 0;
	}

	currPacket = &tape->packets[packetIdx];
	nextPacket = currPacket + 1;
	vel[0] = nextPacket->pos.x - currPacket->pos.x;
	vel[1] = nextPacket->pos.y - currPacket->pos.y;
	vel[2] = nextPacket->pos.z - currPacket->pos.z;

	inst->matrix.t[0] = currPacket->pos.x + ((vel[0] * lerp4096) >> 12);
	inst->matrix.t[1] = currPacket->pos.y + ((vel[1] * lerp4096) >> 12);
	inst->matrix.t[2] = currPacket->pos.z + ((vel[2] * lerp4096) >> 12);
	local_rot.x = Ghost_LerpRot12(currPacket->rot.x, nextPacket->rot.x, lerp4096);
	local_rot.y = Ghost_LerpRot12(currPacket->rot.y, nextPacket->rot.y, lerp4096);
	local_rot.z = Ghost_LerpRot12(currPacket->rot.z, nextPacket->rot.z, lerp4096);
	ConvertRotToMatrix(&inst->matrix, &local_rot);

	d->posCurr.x = (u32)inst->matrix.t[0] << 8;
	d->posCurr.y = (u32)inst->matrix.t[1] << 8;
	d->posCurr.z = (u32)inst->matrix.t[2] << 8;
	d->rotCurr.x = local_rot.x;
	d->rotCurr.y = local_rot.y;
	d->rotCurr.z = local_rot.z;

	if (tape->packetID < packetIdx)
	{
		buffer = tape->packets[packetIdx].bufferPacket;
		do
		{
			u8 opcode;
			if (tape->ptrEnd <= (void *)buffer)
			{
				break;
			}
			opcode = *buffer++;
			if (GHOST_IS_OPCODE(opcode))
			{
				switch (opcode)
				{
				case GHOST_OP_POSITION:
					buffer += GHOST_SIZE_POSITION - 1;
					tape->packetID++;
					break;
				case GHOST_OP_ANIMATION:
				{
					s32 maxFrame;
					u8 *animation = buffer;
					if ((s32)VehFrameInst_GetNumAnimFrames(inst, buffer[0]) < 1)
					{
						inst->animIndex = 0;
					}
					else
					{
						inst->animIndex = buffer[0];
					}
					maxFrame = VehFrameInst_GetNumAnimFrames(inst, inst->animIndex) - 1;
					// NOTE(aalhendi): Preserve retail's repeated clamp evaluation and
					// vehicle animation lookup, including the empty-animation case.
					inst->animFrame = ((animation[1] != 0 ? animation[1] : 0) < maxFrame) ? (animation[1] != 0 ? animation[1] : 0)
					                                                                      : VehFrameInst_GetNumAnimFrames(inst, inst->animIndex) - 1;
					buffer += GHOST_SIZE_ANIMATION - 1;
					break;
				}
				case GHOST_OP_BOOST:
				{
					s32 reserves = (s16)Ghost_ReadBE16(buffer);
					s32 speedCap = (s16)Ghost_ReadBE16(buffer + 3);
					if (GAME_TRACKER->trafficLightsTimer < 1 && !(GAME_TRACKER->gameMode1 & START_OF_RACE) && !RaceFlag_IsFullyOnScreen())
					{
						VehFire_Increment(d, reserves, buffer[2], speedCap);
					}
					buffer += GHOST_SIZE_BOOST - 1;
					break;
				}
				case GHOST_OP_INSTANCE:
					inst->flags &= ~SPLIT_LINE;
					if (buffer[0])
					{
						inst->flags |= SPLIT_LINE;
					}
					buffer += GHOST_SIZE_INSTANCE - 1;
					break;
				case GHOST_OP_IDLE:
					tape->packetID++;
					break;
				}
			}
			else
			{
				buffer += GHOST_SIZE_VELOCITY - 1;
				tape->packetID++;
			}
		} while (tape->packetID < packetIdx);
	}
	if (GAME_TRACKER->trafficLightsTimer < 1)
	{
		tape->timeElapsedInRace += GAME_TRACKER->elapsedTimeMS;
	}
	return;

finished:
{
	struct GhostHeader *gh = tape->gh;
	d->ySpeed = gh->ySpeed;
	d->speedApprox = gh->speedApprox;
	d->actionsFlagSet &= ~ACTION_BOT;
	BOTS_Driver_Convert(d);
	BOTS_ThTick_Drive(t);
	d->actionsFlagSet |= ACTION_RACE_FINISHED;
	t->flags |= THREAD_FLAG_DISABLE_COLLISION;
	return;
}

hidden:
	inst->flags |= HIDE_MODEL;
}


void GhostReplay_Init1(void)
{
	s16 i;

	GHOST_CAN_SAVE = 0;
	GHOST_DRAWING = 0;

	// Only a time trial allocates recording and replay storage.
	if ((GAME_TRACKER->gameMode1 & GAME_MODE_TIME_TRIAL_GAMEPLAY_MASK) != TIME_TRIAL)
	{
		return;
	}

	GHOST_RECORDING.ptrGhost = MEMPACK_AllocMem(GHOST_RECORD_BUFFER_SIZE, GHOST_RECORD_BUFFER_NAME);
	GHOST_RECORDING.ptrStartOffset = GHOSTHEADER_GETRECORDBUFFER(GHOST_RECORDING.ptrGhost);
	GHOST_RECORDING.ptrEndOffset = (char *)GHOST_RECORDING.ptrGhost + 0x3dfc;

	// Human and staff replay objects exist even when their tape is empty.
	for (i = 0; i < 2; i++)
	{
		struct GhostTape *tape;
		GHOST_TAPES[i] = MEMPACK_AllocMem(sizeof(struct GhostTape), GHOST_TAPE_NAME);
		tape = GHOST_TAPES[i];

		switch (i)
		{
		case 0:
			tape->gh_again = GHOST_PLAYING;
			break;
		case 1:
			if (GAME_SAVE.progress.highScoreTracks[GAME_TRACKER->levelID].timeTrialFlags & TT_NTROPY_BEATEN)
			{
				tape->gh_again = (ST1_GETPOINTERS(GAME_TRACKER->level1->ptrSpawnType1))[ST1_NOXIDE];
			}
			else
			{
				tape->gh_again = (ST1_GETPOINTERS(GAME_TRACKER->level1->ptrSpawnType1))[ST1_NTROPY];
			}
			break;
		}

		tape->gh = tape->gh_again;
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): No saved human ghost is normal on a fresh time trial.
		// PSX can read its mapped low memory; native represents it as an empty
		// tape while retaining the thread that displays recording warnings.
		if (tape->gh == NULL)
		{
			tape->ptrStart = NULL;
			tape->ptrEnd = NULL;
			tape->constDEADC0ED = 0xDEADC0ED;
			if (i == 1)
			{
				GAME_TRACKER->timeToBeatInTimeTrial_ForCurrentEvent = 0;
			}
			continue;
		}
#endif
		tape->ptrStart = GHOSTHEADER_GETRECORDBUFFER(tape->gh);
		tape->ptrEnd = (char *)tape->ptrStart + tape->gh->size;
		tape->constDEADC0ED = 0xDEADC0ED;

		if (i == 1)
		{
			GAME_TRACKER->timeToBeatInTimeTrial_ForCurrentEvent = tape->gh->timeElapsedInRace;
		}
	}

	for (i = 0; i < 2; i++)
	{
		struct Driver *ghostDriver;
		struct GhostTape *tape;
		struct Model *model;
		// NOTE(aalhendi): Keep retail's model lookup register across the wake test.
		register struct Model *wake CTR_PSX_REGISTER("v0");
		struct Instance *inst;
		struct Thread *t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(4, NONE, LARGE, GHOST), GhostReplay_ThTick, GHOST_NAME, 0);

#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Pool exhaustion may return no thread or instance.
		// Leave incomplete ghosts inactive; Init2 and ThTick skip them.
		if (t == NULL)
		{
			continue;
		}
#endif
		t->modelIndex = DYNAMIC_GHOST;
		t->flags |= THREAD_FLAG_DISABLE_COLLISION;

		ghostDriver = t->object;
		memset(ghostDriver, 0, sizeof(*ghostDriver));
		tape = GHOST_TAPES[i];
		ghostDriver->ghostID = i;
		ghostDriver->ghostBoolInit = 0;
		ghostDriver->ghostTape = tape;
		ghostDriver->driverID = i + 1;

		model = VehBirth_GetModelByName(GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[i + 1]].name_Debug);
#if defined(CTR_NATIVE)
		if (model == NULL)
		{
			continue;
		}
#endif
		inst = INSTANCE_Birth3D(model, model->name, t);
		t->inst = inst;
#if defined(CTR_NATIVE)
		if (inst == NULL)
		{
			continue;
		}
#endif

		wake = GAME_TRACKER->modelPtr[STATIC_WAKE];
		if (wake)
		{
			struct Instance *wakeInst = INSTANCE_Birth3D(wake, wake->name, 0);
			ghostDriver->wakeInst = wakeInst;

			if (wakeInst != 0)
			{
				wakeInst->flags |= HIDE_MODEL | ANIM_LOOP;
			}
		}

		inst = t->inst;
		inst->depthBiasSecondary = 0xc;
		inst->flags |= OWNER_PUSHBUFFER_GATE;
		ghostDriver->instSelf = inst;
		VehBirth_TireSprites(t);
		VehBirth_SetConsts(ghostDriver);

		ghostDriver->wheelSprites = ICONGROUP_GETICONS(GAME_TRACKER->iconGroup[0xc]);
		ghostDriver->actionsFlagSet |= ACTION_BOT; // AI driver

		// Activation waits for GhostReplay_Init2, after both ghosts are allocated.
	}
}


void GhostReplay_Init2(void)
{
	struct Thread *thread;

	for (thread = GAME_TRACKER->threadBuckets[GHOST].thread; thread != NULL; thread = thread->siblingThread)
	{
		struct GhostTape *tape;
		s16 characterIndex;
		struct Model *model;
		struct Instance *inst;
		char *name;
		struct Driver *driver = thread->object;
		if (driver == NULL)
		{
			continue;
		}
#if defined(CTR_NATIVE)
		if (driver->ghostTape == NULL || driver->instSelf == NULL)
		{
			continue;
		}
#endif

		if (driver->ghostTape->ptrEnd == driver->ghostTape->ptrStart)
		{
			continue;
		}

		if (!((driver->ghostID == 0 && GHOST_REPLAY_HUMAN) ||
		      (driver->ghostID == 1 && (GAME_SAVE.progress.highScoreTracks[GAME_TRACKER->levelID].timeTrialFlags & TT_NTROPY_OPEN))))
		{
			continue;
		}

		tape = driver->ghostTape;
		inst = driver->instSelf;
		tape->timeElapsedInRace = 0;
		tape->timeInPacket32_backup = 0;
		tape->unk20 = 0;
		tape->timeInPacket32 = 0;
		tape->timeInPacket01 = 0;
		tape->ptrCurr = tape->ptrStart;

		GHOST_DRAWING = 1;
		driver->ghostBoolInit = 1;
		driver->ghostBoolStarted = 0;

		characterIndex = (u16)driver->ghostID + 1;
		if ((u16)driver->ghostID != 0)
		{
			if (GAME_SAVE.progress.highScoreTracks[GAME_TRACKER->levelID].timeTrialFlags & TT_NTROPY_BEATEN)
			{
				characterIndex = (u16)driver->ghostID + 2;
			}
		}

		model = VehBirth_GetModelByName(GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[characterIndex]].name_Debug);
#if defined(CTR_NATIVE)
		if (model == NULL)
		{
			driver->ghostBoolInit = 0;
			inst->flags |= HIDE_MODEL;
			continue;
		}
#endif
		driver->wheelSize = (GAME_CHARACTER_IDS[characterIndex] != NITROS_OXIDE) ? 0xccc : 0;
		if (driver->ghostID != 0)
		{
			name = GHOST_NAME_STAFF;
		}
		else
		{
			name = GHOST_NAME_HUMAN;
		}
		INSTANCE_Birth(inst, model, name, inst->thread, 7);
		GhostReplay_ThTick(thread);

		// NOTE(aalhendi): Retail retains these cache snapshots after the first tick.
		tape->unk2.x = tape->unk1.x;
		tape->unk2.y = tape->unk1.y;
		tape->unk2.z = tape->unk1.z;
		tape->unk4.x = tape->unk3.x;
		tape->unk4.y = tape->unk3.y;
		tape->unk4.z = tape->unk3.z;
		tape->unk20 = 0;
	}
}
