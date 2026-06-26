#include <common.h>

enum
{
	GHOST_OP_POSITION = 0x80,
	GHOST_OP_ANIMATION,
	GHOST_OP_BOOST,
	GHOST_OP_INSTANCE,
	GHOST_OP_IDLE,
};
typedef u8 GhostOpcode;

CTR_STATIC_ASSERT(sizeof(GhostOpcode) == 0x1);
CTR_STATIC_ASSERT(GHOST_OP_POSITION == 0x80);
CTR_STATIC_ASSERT(GHOST_OP_IDLE == 0x84);

static const u8 GHOST_SIZE_POSITION = 11;
static const u8 GHOST_SIZE_ANIMATION = 3;
static const u8 GHOST_SIZE_BOOST = 6;
static const u8 GHOST_SIZE_INSTANCE = 2;
static const u8 GHOST_SIZE_IDLE = 1;
static const u8 GHOST_SIZE_VELOCITY = 5;

#define GHOST_IS_OPCODE(b) ((u8)((b) + 0x80) < 5)
#define Ghost_ReadBE16(p)  ((u16)((p)[0] << 8 | (p)[1]))

internal s16 Ghost_LerpRot12(s16 curr, s16 next, u16 t)
{
	s32 delta = ((s32)next - (s32)curr) & 0xFFF;
	if (delta > 0x7FF)
	{
		delta -= 0x1000;
	}
	return (curr + ((delta * t) >> 0xC)) & 0xFFF;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80026ed8-0x80027838.
void GhostReplay_ThTick(struct Thread *t)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *d = t->object;
	struct GhostTape *tape = d->ghostTape;
	struct Instance *inst = d->instSelf;

	inst->scale = (SVec3){.x = 0xccc, .y = 0xccc, .z = 0xccc};

	// 6-second timer != 0, and ghost made by human
	if ((sdata->ghostOverflowTextTimer != 0) && (d->ghostID == 0))
	{
		s32 color = 0xFFFF8004;
		if (sdata->ghostOverflowTextTimer & 1)
		{
			color = 0xFFFF8003;
		}

		DecalFont_DrawLine(sdata->lngStrings[LNG_GHOST_DATA_OVERFLOW], 0x100, 0x28, 2, color);
		DecalFont_DrawLine(sdata->lngStrings[LNG_CAN_NOT_SAVE_GHOST_DATA], 0x100, 0x32, 2, color);

		sdata->ghostOverflowTextTimer--;
	}

	if ((sdata->boolGhostsDrawing == 0) || ((gGT->gameMode1 & DEBUG_MENU) != 0) || (tape->ptrEnd == tape->ptrStart) || (d->ghostBoolInit == 0))
	{
		inst->flags |= HIDE_MODEL;
		return;
	}

	if (d->reserves > 0)
	{
		d->reserves -= gGT->elapsedTimeMS;
		if (d->reserves < 0)
		{
			d->reserves = 0;
		}
	}

	if ((gGT->trafficLightsTimer < 1) && (d->ghostBoolStarted == 0))
	{
		d->ghostBoolStarted = 1;
		tape->packetID = -1;
	}

	inst->alphaScale = 0xa00;

	inst->flags &= ~(HIDE_MODEL | DRAW_TRANSPARENT);
	inst->flags |= GHOST_DRAW_TRANSPARENT;

	s32 timeInRace = tape->timeElapsedInRace >= 0 ? tape->timeElapsedInRace : 0;

	struct GhostPacket *packet = &tape->packets[0];

	// flush and rewrite cached GhostPackets array
	if (tape->timeInPacket32 <= timeInRace)
	{
		s16 opcodePos = 0;
		u8 *packetPtr = tape->ptrCurr;
		u8 *packetEndChain = tape->ptrCurr;
		SVec3 tmpPos = {0};

		tape->packetID = -1;
		tape->timeInPacket01 = tape->timeInPacket32_backup;

		// advance two POSITION opcodes, combining
		// with velocity data to build the packet cache
		while (opcodePos < 2)
		{
			// reached end of tape
			if (tape->ptrEnd <= (void *)packetPtr)
			{
				struct GhostHeader *gh = tape->gh;

				d->ySpeed = gh->ySpeed;
				d->actionsFlagSet &= ~ACTION_BOT;
				d->speedApprox = gh->speedApprox;

				BOTS_Driver_Convert(d);
				BOTS_ThTick_Drive(t);

				d->actionsFlagSet |= ACTION_RACE_FINISHED;
				t->flags |= THREAD_FLAG_DISABLE_COLLISION;
				return;
			}

			u8 opcode = packetPtr[0];
			if (GHOST_IS_OPCODE(opcode))
			{
				switch (opcode)
				{
				case GHOST_OP_POSITION:
					for (s32 i = 0; i < 3; ++i)
					{
						u16 rawValue = Ghost_ReadBE16(&packetPtr[1 + i * 2]);
						tmpPos.v[i] = (s16)(((s32)((u32)rawValue << 0x10)) >> 0xd);
					}
					packet->pos = tmpPos;

					packet->rot.x = 0;
					packet->rot.y = (u16)packetPtr[9] << 4;
					packet->rot.z = (u16)packetPtr[10] << 4;

					if (opcodePos == 1)
					{
						s32 bigEndianTime = Ghost_ReadBE16(&packetPtr[7]);
						tape->ptrCurr = packetPtr;

						tape->timeInPacket32_backup += bigEndianTime;
						tape->timeInPacket32 += bigEndianTime;
					}

					opcodePos++;

					packet->bufferPacket = packetEndChain;
					packetPtr += GHOST_SIZE_POSITION;
					packetEndChain = packetPtr;

					packet++;

					break;

				case GHOST_OP_ANIMATION:
					packetPtr += GHOST_SIZE_ANIMATION;
					break;

				case GHOST_OP_BOOST:
					packetPtr += GHOST_SIZE_BOOST;
					break;

				case GHOST_OP_INSTANCE:
					packetPtr += GHOST_SIZE_INSTANCE;
					break;

				case GHOST_OP_IDLE:
					packet->pos = tmpPos;
					packet[0].rot = packet[-1].rot;

					packet->bufferPacket = packetEndChain;
					packetPtr += GHOST_SIZE_IDLE;
					packetEndChain = packetPtr;

					packet++;
					break;
				}
			}

			// velocity data (no opcode byte)
			else
			{
				for (s32 i = 0; i < 3; ++i)
				{
					tmpPos.v[i] += (s16)((s8)packetPtr[i]) * 8;
				}
				packet->pos = tmpPos;

				packet->rot.x = 0;
				packet->rot.y = packetPtr[3] << 4;
				packet->rot.z = packetPtr[4] << 4;

				packet->bufferPacket = packetEndChain;
				packetPtr += GHOST_SIZE_VELOCITY;
				packetEndChain = packetPtr;

				packet++;
			}
		}

		tape->numPacketsInArray = ((u32)packet - (u32)&tape->packets[0]) >> 4;

		tape->numPacketsInArray -= 1;

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

	s32 scaledNum = (timeInRace - tape->timeInPacket01) * tape->numPacketsInArray * 0x1000;

	// 0% = 0, 100% = 0x1000
	s32 scaledPacketIdx = scaledNum / tape->timeBetweenPackets;
	s32 packetIdx = scaledPacketIdx >> 0xc;
	u16 lerp4096 = (u16)(scaledPacketIdx & 0xfff);

	if (tape->numPacketsInArray <= packetIdx)
	{
		packetIdx = tape->numPacketsInArray - 1;
		lerp4096 = 0;
	}

	// Ptrs to current and next packets for better readability
	struct GhostPacket *currPacket = &tape->packets[packetIdx];
	struct GhostPacket *nextPacket = &tape->packets[packetIdx + 1];

	s32 vel[3];
	vel[0] = (s32)nextPacket->pos.x - (s32)currPacket->pos.x;
	vel[1] = (s32)nextPacket->pos.y - (s32)currPacket->pos.y;
	vel[2] = (s32)nextPacket->pos.z - (s32)currPacket->pos.z;

	inst->matrix.t[0] = currPacket->pos.x + ((vel[0] * lerp4096) >> 0xC);
	inst->matrix.t[1] = currPacket->pos.y + ((vel[1] * lerp4096) >> 0xC);
	inst->matrix.t[2] = currPacket->pos.z + ((vel[2] * lerp4096) >> 0xC);

	SVec3 local_rot = {
	    .x = Ghost_LerpRot12(currPacket->rot.x, nextPacket->rot.x, lerp4096),
	    .y = Ghost_LerpRot12(currPacket->rot.y, nextPacket->rot.y, lerp4096),
	    .z = Ghost_LerpRot12(currPacket->rot.z, nextPacket->rot.z, lerp4096),
	};

	ConvertRotToMatrix(&inst->matrix, &local_rot);

	d->posCurr.x = inst->matrix.t[0] << 8;
	d->posCurr.y = inst->matrix.t[1] << 8;
	d->posCurr.z = inst->matrix.t[2] << 8;

	d->rotCurr.x = local_rot.x;
	d->rotCurr.y = local_rot.y;
	d->rotCurr.z = local_rot.z;

	u8 *buffer = tape->packets[packetIdx].bufferPacket;

	while (tape->packetID < packetIdx)
	{
		if (tape->ptrEnd <= (void *)buffer)
		{
			break;
		}

		u8 opcode = buffer[0];

		if (!GHOST_IS_OPCODE(opcode))
		{
			buffer += GHOST_SIZE_VELOCITY;
			tape->packetID++;
		}

		else
		{
			switch (opcode)
			{
			case GHOST_OP_POSITION:
				buffer += GHOST_SIZE_POSITION;
				tape->packetID++;
				break;

			case GHOST_OP_ANIMATION:
			{
				s32 numAnimFrames = INSTANCE_GetNumAnimFrames(inst, buffer[1]);
				inst->animIndex = (numAnimFrames < 1) ? 0 : buffer[1];

				s32 maxFrame = INSTANCE_GetNumAnimFrames(inst, inst->animIndex) - 1;
				if (buffer[2] != 0)
				{
					if (buffer[2] < maxFrame)
					{
						inst->animFrame = buffer[2];
					}
					else
					{
						inst->animFrame = maxFrame;
					}
				}
				else if (maxFrame > 0)
				{
					inst->animFrame = maxFrame;
				}
				else
				{
					inst->animFrame = 0;
				}

				buffer += GHOST_SIZE_ANIMATION;
			}
			break;

			case GHOST_OP_BOOST:
				if (gGT->trafficLightsTimer < 1 && ((gGT->gameMode1 & START_OF_RACE) == 0) && (RaceFlag_IsFullyOnScreen() == 0))
				{
					VehFire_Increment(d, Ghost_ReadBE16(&buffer[1]), buffer[3], Ghost_ReadBE16(&buffer[4]));
				}
				buffer += GHOST_SIZE_BOOST;
				break;

			case GHOST_OP_INSTANCE:
				inst->flags &= ~SPLIT_LINE;
				if (buffer[1] != 0)
				{
					inst->flags |= SPLIT_LINE;
				}
				buffer += GHOST_SIZE_INSTANCE;
				break;

			case GHOST_OP_IDLE:
				buffer += GHOST_SIZE_IDLE;
				tape->packetID++;
				break;
			}
		}
	}

	if (gGT->trafficLightsTimer < 1)
	{
		tape->timeElapsedInRace += gGT->elapsedTimeMS;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80027838-0x80027b88.
void GhostReplay_Init1(void)
{
	struct GameTracker *gGT = sdata->gGT;

	sdata->boolCanSaveGhost = 0;
	sdata->boolGhostsDrawing = 0;

	// only continue if you're in time trial, not main menu, and not cutscene
	if ((gGT->gameMode1 & GAME_MODE_TIME_TRIAL_GAMEPLAY_MASK) != TIME_TRIAL)
	{
		return;
	}

	struct GhostHeader *gh = MEMPACK_AllocMem(0x3e00);
	char *recordBuffer = GHOSTHEADER_GETRECORDBUFFER(gh);
	sdata->GhostRecording.ptrGhost = gh;
	sdata->GhostRecording.ptrStartOffset = &recordBuffer[0];
	sdata->GhostRecording.ptrEndOffset = &recordBuffer[0x3DD4];

	// ALWAYS initialize ghost threads, even if gh == 0,
	// or else the "Ghost Too Big" text will never play.
	// 0: human ghost, 1: N Tropy / Oxide ghost
	for (s32 i = 0; i < 2; i++)
	{
		struct GhostTape *tape = MEMPACK_AllocMem(0x268);
		sdata->ptrGhostTape[i] = tape;

		if (i == 0)
		{
			gh = sdata->ptrGhostTapePlaying;
		}
		else
		{
			s32 timeTrialFlags = sdata->gameProgress.highScoreTracks[gGT->levelID].timeTrialFlags;
			void **pointers = ST1_GETPOINTERS(gGT->level1->ptrSpawnType1);

			gh = ((timeTrialFlags & TT_NTROPY_BEATEN) != 0) ? pointers[ST1_NOXIDE] : pointers[ST1_NTROPY];
		}

		recordBuffer = GHOSTHEADER_GETRECORDBUFFER(gh);

		tape->gh = gh;
		tape->gh_again = gh;
		tape->ptrStart = &recordBuffer[0];
		tape->constDEADC0ED = 0xDEADC0ED;
		tape->ptrEnd = &recordBuffer[gh->size];

		if (i == 1)
		{
			gGT->timeToBeatInTimeTrial_ForCurrentEvent = gh->timeElapsedInRace;
		}
	}

	for (s32 i = 0; i < 2; i++)
	{
		struct Thread *t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(4, NONE, LARGE, GHOST), GhostReplay_ThTick, sdata->s_ghost, 0);

		t->modelIndex = DYNAMIC_GHOST;
		t->flags |= THREAD_FLAG_DISABLE_COLLISION;

		struct Driver *ghostDriver = t->object;
		memset(ghostDriver, 0, 0x638);
		ghostDriver->ghostID = i;
		ghostDriver->driverID = i + 1;
		ghostDriver->ghostBoolInit = 0;
		ghostDriver->ghostTape = sdata->ptrGhostTape[i];

		s32 charID = data.characterIDs[i + 1];
		struct Model *model = VehBirth_GetModelByName(data.MetaDataCharacters[charID].name_Debug);
		struct Instance *inst = INSTANCE_Birth3D(model, model->name, t);
		t->inst = inst;

		struct Model *wake = gGT->modelPtr[STATIC_WAKE];
		if (wake)
		{
			struct Instance *wakeInst = INSTANCE_Birth3D(wake, wake->name, 0);
			ghostDriver->wakeInst = wakeInst;

			if (wakeInst != 0)
			{
				wakeInst->flags |= HIDE_MODEL | ANIM_LOOP;
			}
		}

		inst->depthBiasSecondary = 0xc;
		inst->flags |= OWNER_PUSHBUFFER_GATE;
		ghostDriver->instSelf = inst;
		VehBirth_TireSprites(t);
		VehBirth_SetConsts(ghostDriver);

		ghostDriver->actionsFlagSet |= ACTION_BOT; // AI driver
		ghostDriver->wheelSprites = ICONGROUP_GETICONS(gGT->iconGroup[0xc]);

		// NOTE(aalhendi): GhostReplay_Init2 owns retail activation/tick.
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80027b88-0x80027df4.
void GhostReplay_Init2(void)
{
	struct GameTracker *gGT = sdata->gGT;

	for (struct Thread *thread = gGT->threadBuckets[GHOST].thread; thread != NULL; thread = thread->siblingThread)
	{
		struct Driver *driver = thread->object;
		if (driver == NULL)
		{
			continue;
		}

		struct GhostTape *tape = driver->ghostTape;
		if (tape->ptrEnd == tape->ptrStart)
		{
			continue;
		}

		s32 ghostID = driver->ghostID;
		if (ghostID == 0)
		{
			if (sdata->boolReplayHumanGhost == 0)
			{
				continue;
			}
		}
		else
		{
			if (ghostID != 1)
			{
				continue;
			}

			s32 timeTrialFlags = sdata->gameProgress.highScoreTracks[gGT->levelID].timeTrialFlags;
			if ((timeTrialFlags & TT_NTROPY_OPEN) == 0)
			{
				continue;
			}
		}

		tape->timeElapsedInRace = 0;
		tape->timeInPacket32_backup = 0;
		tape->unk20 = 0;
		tape->timeInPacket32 = 0;
		tape->timeInPacket01 = 0;
		tape->ptrCurr = tape->ptrStart;

		sdata->boolGhostsDrawing = 1;
		driver->ghostBoolInit = 1;
		driver->ghostBoolStarted = 0;

		s32 characterIndex = ghostID + 1;
		if (ghostID != 0)
		{
			s32 timeTrialFlags = sdata->gameProgress.highScoreTracks[gGT->levelID].timeTrialFlags;
			if ((timeTrialFlags & TT_NTROPY_BEATEN) != 0)
			{
				characterIndex = ghostID + 2;
			}
		}

		s32 characterID = data.characterIDs[characterIndex];
		struct Model *model = VehBirth_GetModelByName(data.MetaDataCharacters[characterID].name_Debug);

		driver->wheelSize = (characterID != NITROS_OXIDE) ? 0xccc : 0;

		struct Instance *inst = driver->instSelf;
		char *name = (ghostID != 0) ? sdata->s_ghost1 : sdata->s_ghost0;

		INSTANCE_Birth(inst, model, name, inst->thread, 7);
		GhostReplay_ThTick(thread);

		// NOTE(aalhendi): written in retail. never read?
		tape->unk2 = tape->unk1;
		tape->unk4 = tape->unk3;
		tape->unk20 = 0;
	}
}
