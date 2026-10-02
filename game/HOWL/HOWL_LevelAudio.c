#include <common.h>
#include <ctr_gte_transfer.h>

#ifndef GAME_LEVEL_AUDIO_TRACKER
#define GAME_LEVEL_AUDIO_TRACKER (sdata->gGT)
#endif
#ifndef GAME_LEVEL_AUDIO_FADES
#define GAME_LEVEL_AUDIO_FADES (sdata->SoundFadeInput)
#endif
#ifndef GAME_LEVEL_AUDIO_RNG
#define GAME_LEVEL_AUDIO_RNG (sdata->audioRNG)
#endif
#ifndef GAME_LEVEL_AUDIO_AMBIENT_SOUNDS
#define GAME_LEVEL_AUDIO_AMBIENT_SOUNDS (data.levAmbientSound)
#endif
#ifndef GAME_LEVEL_AUDIO_INVALID_SPAWN
#define GAME_LEVEL_AUDIO_INVALID_SPAWN (sdata->audioDefaults[6])
#endif
#ifndef GAME_LEVEL_AUDIO_TRACKER_LOAD_AFTER
#define GAME_LEVEL_AUDIO_TRACKER_LOAD_AFTER(result, dependency) ((void)(dependency), (result) = GAME_LEVEL_AUDIO_TRACKER)
#endif
#ifndef GAME_LEVEL_AUDIO_CAMERA_ECHO
#define GAME_LEVEL_AUDIO_CAMERA_ECHO(result, tracker, offset, index) ((void)(offset), (result) = (tracker)->cameraDC[index].ptrQuadBlock->quadFlags)
#endif
#ifndef GAME_LEVEL_AUDIO_CAMERA_QUAD_LOAD
#define GAME_LEVEL_AUDIO_CAMERA_QUAD_LOAD(result, tracker, offset, index) ((void)(offset), (result) = (tracker)->cameraDC[index].ptrQuadBlock)
#endif

int GTE_GetSquaredDistance(s16 *pos1, s16 *pos2)
{
	s32 squares[3];
	s32 dy, dz;
	s32 x2, y2;
	// NOTE(aalhendi): MAC reads remain in a3 while earlier results reload from the stack.
	register s32 value CTR_PSX_REGISTER("$7");
	s32 z2;

	squares[0] = pos1[0] - pos2[0];
	dy = pos1[1] - pos2[1];
	dz = pos1[2] - pos2[2];
	squares[1] = dy;
	squares[2] = dz;
	CTR_GteLoadDataWord(&squares[0], 9);
	CTR_GteLoadDataWord(&squares[1], 10);
	CTR_GteLoadDataWord(&squares[2], 11);
	CTR_GteLoadDelay();
	gte_sqr0();
	value = (s32)MFC2(25);
	CTR_PSX_GTE_READ_DELAY();
	squares[0] = value;
	value = (s32)MFC2(26);
	CTR_PSX_GTE_READ_DELAY();
	squares[1] = value;
	value = (s32)MFC2(27);
	// NOTE(aalhendi): Keep the stack reads and final MAC copy in the retail delay slots.
	CTR_PSX_RELOAD(squares[0]);
	CTR_PSX_RELOAD(squares[1]);
	x2 = squares[0];
	y2 = squares[1];
	CTR_PSX_COPY_VALUE(z2, value);
	squares[2] = value;
	return CTR_MipsAddLo(CTR_MipsAddLo(x2, y2), z2);
}

void CalculateVolumeFromDistance(u32 *soundIDCount, u32 soundID, int distance)
{
	u32 volume;

	if (distance >= 6000)
	{
		if (*soundIDCount != 0)
		{
			OtherFX_Stop1(*soundIDCount);
			*soundIDCount = 0;
		}
		return;
	}

	if ((*soundIDCount != 0) && ((*soundIDCount & 0xffff) != soundID))
	{
		OtherFX_Stop1(*soundIDCount);
		*soundIDCount = 0;
	}

	if (distance > 300)
	{
		volume = VehCalc_MapToRange(distance, 300, 6000, 0xff, 0);
	}
	else
	{
		volume = 0xff;
	}

	if (soundID != (u32)-1)
	{
		u32 handle = *soundIDCount;
		if (handle == 0)
		{
			*soundIDCount = OtherFX_Play_LowLevel(soundID & 0xffff, 0, HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, volume, 0));
		}
		else
		{
			// NOTE(aalhendi): Retail keeps packed modify flags in a1.
			register u32 modifyFlags CTR_PSX_REGISTER("$5") = volume & 0xff;
			if (soundID == 0x89)
			{
				s32 distort;
				distort = GAME_LEVEL_AUDIO_TRACKER->frameTimer_VsyncCallback;
				// NOTE(aalhendi): The volume shift follows the frame load.
				CTR_PSX_DEPEND_VALUE(modifyFlags, distort);
				modifyFlags <<= 16;
				CTR_PSX_DEPEND_VALUE(distort, modifyFlags);
				distort = ((u32)distort >> 2 & 0x7f) - 0x40;
				if (distort < 0)
				{
					distort = -distort;
				}

				distort += 100;
				modifyFlags |= ((u32)distort & 0xff) << 8;
				modifyFlags |= HOWL_SFX_LR_CENTER;
			}
			else
			{
				modifyFlags = (modifyFlags << 16) | HOWL_SFX_CENTER_NO_DISTORTION;
			}

			OtherFX_Modify(handle, modifyFlags);
		}
	}
}

void PlayWarppadSound(u32 distance)
{
	CalculateVolumeFromDistance((u32 *)&GAME_LEVEL_AUDIO_FADES[0].soundID_soundCount, 0x98, distance);
}

void Level_SoundLoopSet(int *soundIDCount, u32 soundID, u32 volume)
{
	if (volume == 0)
	{
		if (*soundIDCount != 0)
		{
			OtherFX_Stop1(*soundIDCount);
			*soundIDCount = 0;
		}
	}
	else if (*soundIDCount == 0)
	{
		*soundIDCount = OtherFX_Play_LowLevel(soundID & 0xffff, 0, HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, volume, 0));
	}
	else
	{
		OtherFX_Modify(*soundIDCount, HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, volume, 0));
	}
}

void Level_SoundLoopFade(struct SoundFadeInput *fade, u32 soundID, int desiredVolume, int fadeStep)
{
	// NOTE(aalhendi): Preserve the initial volume load in v0 before the equality exit.
	register s32 initialVolume CTR_PSX_REGISTER("$2") = fade->currentVolume;
	s32 currentVolume;
	b32 clamped;

	if (initialVolume == desiredVolume)
	{
		return;
	}

	currentVolume = initialVolume;
	fade->desiredVolume = desiredVolume;

	if (currentVolume < desiredVolume)
	{
		fade->currentVolume = CTR_MipsAddLo(currentVolume, fadeStep);
		clamped = desiredVolume < CTR_MipsAddLo(currentVolume, fadeStep);
	}
	else
	{
		if (currentVolume <= desiredVolume)
		{
			goto updateSound;
		}

		fade->currentVolume = CTR_MipsSubLo(currentVolume, fadeStep);
		clamped = CTR_MipsSubLo(currentVolume, fadeStep) < desiredVolume;
	}

	if (clamped)
	{
		fade->currentVolume = desiredVolume;
	}

updateSound:
	Level_SoundLoopSet(&fade->soundID_soundCount, soundID, fade->currentVolume);
}

void Level_RandomFX(int *cooldown, u32 incomingSoundID, int baseCooldown, u32 randomRange, int volumeScale)
{
	// NOTE(aalhendi): The sound argument stays in t0 through the cooldown read.
	register u32 soundID CTR_PSX_REGISTER("$8") = incomingSoundID;
	int cooldownValue;
	CTR_PSX_KEEP_VALUE_RELAXED(soundID);
	cooldownValue = *cooldown;
	if (cooldownValue > 0)
	{
		*cooldown = cooldownValue - 1;
		// NOTE(aalhendi): Retail reloads the stored cooldown before the zero test.
		CTR_PSX_RELOAD(*cooldown);
		cooldownValue = *cooldown;
	}
	if (cooldownValue == 0)
	{
		u32 firstRng = ((GAME_LEVEL_AUDIO_RNG >> 3) + GAME_LEVEL_AUDIO_RNG * 0x20000000U) * 5U + 1U;
		GAME_LEVEL_AUDIO_RNG = firstRng;
		OtherFX_Play_LowLevel(soundID & 0xffff, 0,
		                      HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, (firstRng % 100 + 100) * (u32)volumeScale >> 8, 1));
		{
			u32 rng = ((GAME_LEVEL_AUDIO_RNG >> 3) + GAME_LEVEL_AUDIO_RNG * 0x20000000U) * 5U + 1U;
			GAME_LEVEL_AUDIO_RNG = rng;
			*cooldown = (s32)(rng % randomRange + (u32)baseCooldown);
		}
	}
}

void Level_AmbientSound(void)
{
	struct GameTracker *gGT = GAME_LEVEL_AUDIO_TRACKER;
	struct Level *level = gGT->level1;
	u32 soundID;
	u32 levelID = gGT->levelID;
	s32 closestDistance[2];
	s32 closestCoord[2];
	s32 soundSlot;
	s32 coordIndex;
	s32 playerIndex;
	// NOTE(aalhendi): Retail records closest coordinates; native callers consume only distance.
	(void)closestCoord;

	if (levelID >= 0x19 || gGT->numPlyrCurrGame >= 3)
	{
		return;
	}

	switch (levelID)
	{
	case 6:
	{
		// NOTE(aalhendi): Retail keeps the drop flag in a1 across the terrain scan.
		register s32 playDrops CTR_PSX_REGISTER("$5") = 0;
		s32 playLoop = 0;
		for (soundSlot = 0; soundSlot < gGT->numPlyrCurrGame; soundSlot++)
		{
			struct Driver *driver = gGT->drivers[soundSlot];
			u8 terrain = driver->currentTerrain;
			s32 sound;
			if (terrain == 0)
			{
				playDrops = 1;
			}
			if (terrain == 1)
			{
				playDrops = 1;
			}
			if (terrain == 11)
			{
				playDrops = 1;
			}
			// NOTE(aalhendi): Ambient sound uses the signed halfword at +0x34.
			sound = (s16)driver->terrainMeta2->bot.fields.unk_0x34_0;
			if (sound != -1)
			{
				if (sound == 0x87)
				{
					playLoop = 1;
				}
			}
		}
		if (playDrops)
		{
			Level_RandomFX(&GAME_LEVEL_AUDIO_FADES[0].unk, 0x86, 6, 0x5a, 0xff);
		}
		if (playLoop)
		{
			Level_SoundLoopFade(&GAME_LEVEL_AUDIO_FADES[1], 0x87, 0xff, 8);
		}
		else
		{
			Level_SoundLoopFade(&GAME_LEVEL_AUDIO_FADES[1], 0x87, 0, 8);
		}
		return;
	}
	case 8:
	{
		s32 playFirstLoop = 0;
		s32 playSecondLoop = 0;
		for (soundSlot = 0; soundSlot < gGT->numPlyrCurrGame; soundSlot++)
		{
			struct Driver *driver = gGT->drivers[soundSlot];
			s32 sound = (s16)driver->terrainMeta2->bot.fields.unk_0x34_0;
			// NOTE(aalhendi): Keep the retail driver load alive through the sound test.
			CTR_PSX_OBSERVE_VALUE(driver);
			if (sound != -1)
			{
				if (sound == 0x88)
				{
					playFirstLoop = 1;
				}
				if (sound == 0x8b)
				{
					playSecondLoop = 1;
				}
			}
		}
		if (playFirstLoop)
		{
			Level_SoundLoopFade(&GAME_LEVEL_AUDIO_FADES[0], 0x88, 0xff, 8);
		}
		else
		{
			Level_SoundLoopFade(&GAME_LEVEL_AUDIO_FADES[0], 0x88, 0, 8);
		}
		if (playSecondLoop)
		{
			Level_SoundLoopFade(&GAME_LEVEL_AUDIO_FADES[1], 0x8b, 0xff, 4);
		}
		else
		{
			Level_SoundLoopFade(&GAME_LEVEL_AUDIO_FADES[1], 0x8b, 0, 4);
		}
		return;
	}
	}
	for (soundSlot = 0; soundSlot < 2; soundSlot++)
	{
		closestDistance[soundSlot] = 0x7fffffff;
		closestCoord[soundSlot] = -1;
	}
	for (soundSlot = 0; soundSlot < 2; soundSlot++)
	{
		// NOTE(aalhendi): Retail retains the spawn index while the GTE helper runs.
		register s32 spawnIndex CTR_PSX_REGISTER("$23");
		s32 spawnCount;
		soundID = GAME_LEVEL_AUDIO_AMBIENT_SOUNDS[soundSlot + levelID * 2];
		if (soundID == 0)
		{
			continue;
		}
		spawnCount = GAME_LEVEL_AUDIO_TRACKER->level1->numSpawnType2;
		// NOTE(aalhendi): The count load precedes the index add in its load delay.
		CTR_PSX_OBSERVE_VALUE(spawnCount);
		spawnIndex = soundSlot + 5;
		// NOTE(aalhendi): Preserve the index rather than carrying a byte offset.
		CTR_PSX_KEEP_VALUE(spawnIndex);
		if (spawnIndex < spawnCount)
		{
			struct SpawnType2 *spawn = &level->ptrSpawnType2[spawnIndex];
			if (spawn->numCoords > 9)
			{
				goto invalidSpawn;
			}
			for (coordIndex = 0; coordIndex < spawn->numCoords; coordIndex++)
			{
				for (playerIndex = 0; playerIndex < GAME_LEVEL_AUDIO_TRACKER->numPlyrCurrGame; playerIndex++)
				{
					s32 distance = GTE_GetSquaredDistance(CTR_VECTOR_DATA(&GAME_LEVEL_AUDIO_TRACKER->pushBuffer[playerIndex].pos),
					                                      CTR_VECTOR_DATA(&level->ptrSpawnType2[spawnIndex].coords.positions[coordIndex]));
					if (distance < closestDistance[soundSlot])
					{
						closestDistance[soundSlot] = distance;
						closestCoord[soundSlot] = coordIndex;
					}
				}
			}
			{
				s32 distance = SquareRoot0_stub(closestDistance[soundSlot]);
				if (soundSlot == 0)
				{
					if (levelID == 9)
					{
						s32 volume = VehCalc_MapToRange(distance, 300, 6000, 0xff, 0);
						Level_RandomFX(&GAME_LEVEL_AUDIO_FADES[0].unk, 0x86, 6, 0x5a, volume);
					}
					else
					{
						CalculateVolumeFromDistance((u32 *)&GAME_LEVEL_AUDIO_FADES[0].soundID_soundCount, soundID, distance);
					}
				}
				else if (levelID == 3)
				{
					s32 volume = VehCalc_MapToRange(distance, 300, 6000, 0xff, 0);
					Level_RandomFX(&GAME_LEVEL_AUDIO_FADES[1].unk, 0x85, 6, 0x5a, volume);
				}
				else
				{
					CalculateVolumeFromDistance((u32 *)&GAME_LEVEL_AUDIO_FADES[1].soundID_soundCount, soundID, distance);
				}
			}
		}
		else
		{
		invalidSpawn:
			if (GAME_LEVEL_AUDIO_INVALID_SPAWN == 0)
			{
				GAME_LEVEL_AUDIO_INVALID_SPAWN = 1;
			}
		}
	}
}

void PlaySound3D(u32 soundID, struct Instance *inst)
{
	s32 dir[4][3];
	u32 distance[4];
	u32 closestDistance = 9000;
	int closestCamera = 0;
	int i;
	int lr;
	u32 volume;

	for (i = 0; i < GAME_LEVEL_AUDIO_TRACKER->numPlyrCurrGame; i++)
	{
		s32 *direction = dir[i];
		struct GameTracker *gGT = GAME_LEVEL_AUDIO_TRACKER;
		direction[0] = CTR_MipsSubLo(inst->matrix.t[0], gGT->pushBuffer[i].pos.x);
		direction[1] = CTR_MipsSubLo(inst->matrix.t[1], gGT->pushBuffer[i].pos.y);
		direction[2] = CTR_MipsSubLo(inst->matrix.t[2], gGT->pushBuffer[i].pos.z);
		distance[i] = GTE_GetSquaredLength(direction);
		distance[i] = SquareRoot0_stub(distance[i]);
		if (distance[i] < closestDistance)
		{
			closestDistance = distance[i];
			closestCamera = i;
		}
	}
	if (closestDistance == 9000)
	{
		return;
	}
	GTE_AudioLR_Inst(&GAME_LEVEL_AUDIO_TRACKER->pushBuffer[closestCamera].matrix_Camera, dir[closestCamera]);
	lr = CTR_MipsAddLo(ratan2(dir[closestCamera][0], CTR_MipsNegLo(dir[closestCamera][2])), 0x800);
	lr = CTR_MipsSra(CTR_MipsNegLo(CTR_MipsSll(lr, 20)), 23);
	if (lr >= 0x81)
	{
		lr = 0x100 - lr;
	}
	else if (lr <= -0x81)
	{
		lr = -0x100 - lr;
	}
	lr += 0x80;
	if (lr < 0)
	{
		lr = 0;
	}
	else if (lr > 0xff)
	{
		lr = 0xff;
	}
	if (closestDistance >= 301)
	{
		volume = VehCalc_MapToRange(closestDistance, 300, 9000, 0xff, 0);
	}
	else
	{
		volume = 0xff;
	}
	{
		u32 cameraOffset = closestCamera * sizeof(struct CameraDC);
		struct GameTracker *gGT;
		// NOTE(aalhendi): Echo occupies the indexed tracker register after its load.
		register u32 echo CTR_PSX_REGISTER("$3");
		u32 packedVolume;
		u32 packedLR;
		u32 filteredID;
		GAME_LEVEL_AUDIO_TRACKER_LOAD_AFTER(gGT, cameraOffset);
		CTR_PSX_DEPEND_VALUE(soundID, gGT);
		filteredID = soundID & 0xffff;
		CTR_PSX_DEPEND_VALUE(cameraOffset, filteredID);
		GAME_LEVEL_AUDIO_CAMERA_ECHO(echo, gGT, cameraOffset, closestCamera);
		packedVolume = (volume & 0xff) << HOWL_SFX_VOLUME_SHIFT;
		packedLR = HowlSfx_Pack(lr, HOWL_SFX_DISTORTION_NONE, 0, 0);
		OtherFX_Play_LowLevel(filteredID, 1, ((echo & QUADBLOCK_FLAG_ENGINE_ECHO) << HOWL_SFX_ECHO_SHIFT) | packedVolume | packedLR);
	}
}

void PlaySound3D_Flags(u32 *flags, u32 soundID, struct Instance *inst)
{
	s32 dir[4][3];
	u32 distance[4];
	u32 closestDistance;
	s32 closestCamera = 0;
	s32 i;
	s32 lr;
	u32 volume;

	if ((*flags != 0) && ((*flags & 0xffff) != soundID))
	{
		OtherFX_Stop1(*flags);
		*flags = 0;
	}
	if (soundID == (u32)-1)
	{
		return;
	}
	closestDistance = 9000;
	for (i = 0; i < GAME_LEVEL_AUDIO_TRACKER->numPlyrCurrGame; i++)
	{
		s32 *direction = dir[i];
		struct GameTracker *gGT = GAME_LEVEL_AUDIO_TRACKER;
		direction[0] = CTR_MipsSubLo(inst->matrix.t[0], gGT->pushBuffer[i].pos.x);
		direction[1] = CTR_MipsSubLo(inst->matrix.t[1], gGT->pushBuffer[i].pos.y);
		direction[2] = CTR_MipsSubLo(inst->matrix.t[2], gGT->pushBuffer[i].pos.z);
		distance[i] = GTE_GetSquaredLength(direction);
		distance[i] = SquareRoot0_stub(distance[i]);
		if (distance[i] < closestDistance)
		{
			closestDistance = distance[i];
			closestCamera = i;
		}
	}
	if (closestDistance == 9000)
	{
		return;
	}
	GTE_AudioLR_Inst(&GAME_LEVEL_AUDIO_TRACKER->pushBuffer[closestCamera].matrix_Camera, dir[closestCamera]);
	lr = CTR_MipsAddLo(ratan2(dir[closestCamera][0], CTR_MipsNegLo(dir[closestCamera][2])), 0x800);
	lr = CTR_MipsSra(CTR_MipsNegLo(CTR_MipsSll(lr, 20)), 23);
	if (lr >= 0x81)
	{
		lr = 0x100 - lr;
	}
	else if (lr <= -0x81)
	{
		lr = -0x100 - lr;
	}
	lr += 0x80;
	if (lr < 0)
	{
		lr = 0;
	}
	else if (lr > 0xff)
	{
		lr = 0xff;
	}
	if (closestDistance >= 301)
	{
		volume = VehCalc_MapToRange(closestDistance, 300, 9000, 0xff, 0);
	}
	else
	{
		volume = 0xff;
	}
	{
		u32 soundHandle = *flags;
		if (soundHandle == 0)
		{
			u32 cameraOffset = closestCamera * sizeof(struct CameraDC);
			struct GameTracker *gGT;
			u32 echo;
			register u32 packedVolume CTR_PSX_REGISTER("$2");
			register u32 packedLR CTR_PSX_REGISTER("$6");
			u32 filteredID;
			// NOTE(aalhendi): Reuse the original sound ID in retail's naturally saved s8.
			register u32 sourceSound CTR_PSX_REGISTER("$30") = soundID;
			GAME_LEVEL_AUDIO_TRACKER_LOAD_AFTER(gGT, cameraOffset);
			// NOTE(aalhendi): The sound mask fills the tracker pointer load delay.
			CTR_PSX_DEPEND_VALUE(sourceSound, gGT);
			filteredID = sourceSound & 0xffff;
			CTR_PSX_DEPEND_VALUE(cameraOffset, filteredID);
			GAME_LEVEL_AUDIO_CAMERA_ECHO(echo, gGT, cameraOffset, closestCamera);
			CTR_PSX_DEPEND_VALUE(volume, echo);
			packedVolume = (volume & 0xff) << HOWL_SFX_VOLUME_SHIFT;
			CTR_PSX_DEPEND_VALUE(lr, packedVolume);
			packedLR = HowlSfx_Pack(lr, HOWL_SFX_DISTORTION_NONE, 0, 0);
			*flags = OtherFX_Play_LowLevel(filteredID, 0, ((echo & QUADBLOCK_FLAG_ENGINE_ECHO) << HOWL_SFX_ECHO_SHIFT) | packedVolume | packedLR);
		}
		else
		{
			u32 cameraOffset = closestCamera * sizeof(struct CameraDC);
			struct GameTracker *gGT;
			u32 echo;
			register u32 packedVolume CTR_PSX_REGISTER("$2");
			register u32 packedLR CTR_PSX_REGISTER("$5");
			struct QuadBlock *quad;
			GAME_LEVEL_AUDIO_TRACKER_LOAD_AFTER(gGT, cameraOffset);
			packedLR = lr & 0xff;
			GAME_LEVEL_AUDIO_CAMERA_QUAD_LOAD(quad, gGT, cameraOffset, closestCamera);
			// NOTE(aalhendi): Pack distortion in the quad pointer load delay.
			packedLR |= HOWL_SFX_DISTORTION_NONE << HOWL_SFX_DISTORTION_SHIFT;
			echo = quad->quadFlags;
			CTR_PSX_DEPEND_VALUE(volume, echo);
			packedVolume = (volume & 0xff) << HOWL_SFX_VOLUME_SHIFT;
			OtherFX_Modify(soundHandle, ((echo & QUADBLOCK_FLAG_ENGINE_ECHO) << HOWL_SFX_ECHO_SHIFT) | packedVolume | packedLR);
		}
	}
}
