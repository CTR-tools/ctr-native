#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002e7bc-0x8002e84c
int GTE_GetSquaredDistance(s16 *pos1, s16 *pos2)
{
	int dx = pos1[0] - pos2[0];
	int dy = pos1[1] - pos2[1];
	int dz = pos1[2] - pos2[2];

	MTC2(dx, 9);
	MTC2(dy, 10);
	MTC2(dz, 11);
	gte_sqr0();

	s32 x2 = MFC2(25);
	s32 y2 = MFC2(26);
	s32 z2 = MFC2(27);

	return CTR_MipsAddLo(CTR_MipsAddLo(x2, y2), z2);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002e84c-0x8002e994
void CalculateVolumeFromDistance(u32 *soundIDCount, u32 soundID, int distance)
{
	u32 volume;

	if (distance < 6000)
	{
		if ((*soundIDCount != 0) && ((*soundIDCount & 0xffff) != soundID))
		{
			OtherFX_Stop1(*soundIDCount);
			*soundIDCount = 0;
		}

		if (distance < 301)
		{
			volume = 0xff;
		}
		else
		{
			volume = VehCalc_MapToRange(distance, 300, 6000, 0xff, 0);
		}

		if (soundID != (u32)-1)
		{
			if (*soundIDCount == 0)
			{
				*soundIDCount = OtherFX_Play_LowLevel(soundID & 0xffff, 0, ((volume & 0xff) << 0x10) | 0x8080);
			}
			else
			{
				if (soundID == 0x89)
				{
					int distort = ((u32)sdata->gGT->frameTimer_VsyncCallback >> 2 & 0x7f) - 0x40;
					if (distort < 0)
					{
						distort = -distort;
					}

					volume = ((volume & 0xff) << 0x10) | (((distort + 100U) & 0xff) << 8) | 0x80;
				}
				else
				{
					volume = ((volume & 0xff) << 0x10) | 0x8080;
				}

				OtherFX_Modify(*soundIDCount, volume);
			}
		}
	}
	else if (*soundIDCount != 0)
	{
		OtherFX_Stop1(*soundIDCount);
		*soundIDCount = 0;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002e994-0x8002e9c0
void PlayWarppadSound(u32 distance)
{
	CalculateVolumeFromDistance((u32 *)&sdata->SoundFadeInput[0].soundID_soundCount, 0x98, distance);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002e9c0-0x8002ea44
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
		*soundIDCount = OtherFX_Play_LowLevel(soundID & 0xffff, 0, ((volume & 0xff) << 0x10) | 0x8080);
	}
	else
	{
		OtherFX_Modify(*soundIDCount, ((volume & 0xff) << 0x10) | 0x8080);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002ea44-0x8002eab8
void Level_SoundLoopFade(int *fade, u32 soundID, int desiredVolume, int fadeStep)
{
	int currentVolume = fade[2];
	b32 clamped;

	if (currentVolume == desiredVolume)
	{
		return;
	}

	fade[1] = desiredVolume;

	if (currentVolume < desiredVolume)
	{
		fade[2] = currentVolume + fadeStep;
		clamped = desiredVolume < currentVolume + fadeStep;
	}
	else
	{
		if (currentVolume <= desiredVolume)
		{
			goto updateSound;
		}

		fade[2] = currentVolume - fadeStep;
		clamped = currentVolume - fadeStep < desiredVolume;
	}

	if (clamped)
	{
		fade[2] = desiredVolume;
	}

updateSound:
	Level_SoundLoopSet(&fade[3], soundID, fade[2]);
}

static u32 Level_RandomFX_NextAudioRNG(void)
{
	sdata->audioRNG = ((sdata->audioRNG >> 3) + sdata->audioRNG * 0x20000000) * 5 + 1;
	return sdata->audioRNG;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002eab8-0x8002ebe4
void Level_RandomFX(int *cooldown, u32 soundID, int baseCooldown, u32 randomRange, int volumeScale)
{
	int cooldownValue = *cooldown;

	if (cooldownValue > 0)
	{
		*cooldown = cooldownValue - 1;
		cooldownValue = *cooldown;
	}

	if (cooldownValue == 0)
	{
		u32 rng = Level_RandomFX_NextAudioRNG();

		OtherFX_Play_LowLevel(soundID & 0xffff, 0, ((((rng % 100 + 100) * volumeScale >> 8) & 0xff) << 0x10) | 0x1008080);

		rng = Level_RandomFX_NextAudioRNG();
		*cooldown = rng % randomRange + baseCooldown;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002ebe4-0x8002f0dc
void Level_AmbientSound(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Level *level = gGT->level1;
	u32 levelID = gGT->levelID;
	int closestDistance[2];
	int closestIndex[2];

	if ((levelID >= 0x19) || ((u8)gGT->numPlyrCurrGame >= 3))
	{
		return;
	}

	if (levelID == 6)
	{
		b32 playDrops = false;
		b32 playLoop = false;

		for (int i = 0; i < (u8)gGT->numPlyrCurrGame; i++)
		{
			struct Driver *driver = gGT->drivers[i];
			u8 terrain = driver->currentTerrain;
			s16 sound = driver->terrainMeta2->sound;

			if ((terrain == 0) || (terrain == 1) || (terrain == 11))
			{
				playDrops = true;
			}

			if ((sound != -1) && (sound == 0x87))
			{
				playLoop = true;
			}
		}

		if (playDrops)
		{
			Level_RandomFX(&sdata->SoundFadeInput[0].unk, 0x86, 6, 0x5a, 0xff);
		}

		Level_SoundLoopFade((int *)&sdata->SoundFadeInput[1], 0x87, playLoop ? 0xff : 0, 8);
		return;
	}

	if (levelID == 8)
	{
		b32 playFirstLoop = false;
		b32 playSecondLoop = false;

		for (int i = 0; i < (u8)gGT->numPlyrCurrGame; i++)
		{
			s16 sound = gGT->drivers[i]->terrainMeta2->sound;

			if (sound != -1)
			{
				if (sound == 0x88)
				{
					playFirstLoop = true;
				}

				if (sound == 0x8b)
				{
					playSecondLoop = true;
				}
			}
		}

		Level_SoundLoopFade((int *)&sdata->SoundFadeInput[0], 0x88, playFirstLoop ? 0xff : 0, 8);
		Level_SoundLoopFade((int *)&sdata->SoundFadeInput[1], 0x8b, playSecondLoop ? 0xff : 0, 4);
		return;
	}

	for (int i = 0; i < 2; i++)
	{
		closestDistance[i] = 0x7fffffff;
		closestIndex[i] = -1;
	}

	for (int soundSlot = 0; soundSlot < 2; soundSlot++)
	{
		u32 soundID = data.levAmbientSound[levelID * 2 + soundSlot];
		int spawnIndex = soundSlot + 5;

		if (soundID == 0)
		{
			continue;
		}

		if (spawnIndex < level->numSpawnType2)
		{
			struct SpawnType2 *spawn = &level->ptrSpawnType2[spawnIndex];

			if (spawn->numCoords > 9)
			{
				goto invalidSpawn;
			}

			for (int coordIndex = 0; coordIndex < spawn->numCoords; coordIndex++)
			{
				SVec3 *coord = &spawn->positions[coordIndex];

				for (int playerIndex = 0; playerIndex < (u8)gGT->numPlyrCurrGame; playerIndex++)
				{
					int distance = GTE_GetSquaredDistance(gGT->pushBuffer[playerIndex].pos.v, coord->v);

					if (distance < closestDistance[soundSlot])
					{
						closestDistance[soundSlot] = distance;
						closestIndex[soundSlot] = coordIndex;
					}
				}
			}

			{
				int distance = SquareRoot0_stub(closestDistance[soundSlot]);

				if (soundSlot == 0)
				{
					if (levelID == 9)
					{
						int volume = VehCalc_MapToRange(distance, 300, 6000, 0xff, 0);
						Level_RandomFX(&sdata->SoundFadeInput[0].unk, 0x86, 6, 0x5a, volume);
					}
					else
					{
						CalculateVolumeFromDistance((u32 *)&sdata->SoundFadeInput[0].soundID_soundCount, soundID, distance);
					}
				}
				else if (levelID == 3)
				{
					int volume = VehCalc_MapToRange(distance, 300, 6000, 0xff, 0);
					Level_RandomFX(&sdata->SoundFadeInput[1].unk, 0x85, 6, 0x5a, volume);
				}
				else
				{
					CalculateVolumeFromDistance((u32 *)&sdata->SoundFadeInput[1].soundID_soundCount, soundID, distance);
				}
			}
		}
		else
		{
		invalidSpawn:
			if (sdata->audioDefaults[6] == 0)
			{
				sdata->audioDefaults[6] = 1;
			}
		}
	}
}

static u32 PlaySound3D_CalculateLR(s32 *dir)
{
	int angle = CTR_MipsAddLo(ratan2(dir[0], CTR_MipsNegLo(dir[2])), 0x800);
	int lr = CTR_MipsSra(CTR_MipsNegLo(CTR_MipsSll(angle, 20)), 23);

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
		return 0;
	}

	if (lr > 0xff)
	{
		return 0xff;
	}

	return lr;
}

static u32 PlaySound3D_BuildFlags(struct GameTracker *gGT, int cameraIndex, u32 distance, u32 lr)
{
	u32 volume;
	u32 echo = ((u32)gGT->cameraDC[cameraIndex].ptrQuadBlock->quadFlags & QUADBLOCK_FLAG_ENGINE_ECHO) << 0x18;

	if (distance < 301)
	{
		volume = 0xff;
	}
	else
	{
		volume = VehCalc_MapToRange(distance, 300, 9000, 0xff, 0);
	}

	return echo | ((volume & 0xff) << 0x10) | (lr & 0xff) | 0x8000;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002f0dc-0x8002f31c
void PlaySound3D(u32 soundID, struct Instance *inst)
{
	struct GameTracker *gGT = sdata->gGT;
	s32 dir[4][3];
	u32 distance[4];
	u32 closestDistance = 9000;
	int closestCamera = 0;

	for (int i = 0; i < (u8)gGT->numPlyrCurrGame; i++)
	{
		dir[i][0] = CTR_MipsSubLo(inst->matrix.t[0], gGT->pushBuffer[i].pos.x);
		dir[i][1] = CTR_MipsSubLo(inst->matrix.t[1], gGT->pushBuffer[i].pos.y);
		dir[i][2] = CTR_MipsSubLo(inst->matrix.t[2], gGT->pushBuffer[i].pos.z);

		distance[i] = GTE_GetSquaredLength(dir[i]);
		distance[i] = SquareRoot0_stub(distance[i]);

		if (distance[i] < closestDistance)
		{
			closestCamera = i;
			closestDistance = distance[i];
		}
	}

	if (closestDistance == 9000)
	{
		return;
	}

	GTE_AudioLR_Inst(&gGT->pushBuffer[closestCamera].matrix_Camera, dir[closestCamera]);

	OtherFX_Play_LowLevel(soundID & 0xffff, 1, PlaySound3D_BuildFlags(gGT, closestCamera, closestDistance, PlaySound3D_CalculateLR(dir[closestCamera])));
}

static u32 PlaySound3D_Flags_BuildFlags(struct GameTracker *gGT, int cameraIndex, u32 distance, u32 lr)
{
	u32 volume;
	u32 echo = ((u32)gGT->cameraDC[cameraIndex].ptrQuadBlock->quadFlags & QUADBLOCK_FLAG_ENGINE_ECHO) << 0x18;

	if (distance < 301)
	{
		volume = 0xff;
	}
	else
	{
		volume = VehCalc_MapToRange(distance, 300, 9000, 0xff, 0);
	}

	return echo | ((volume & 0xff) << 0x10) | (lr & 0xff) | 0x8000;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002f31c-0x8002f5f4
void PlaySound3D_Flags(u32 *flags, u32 soundID, struct Instance *inst)
{
	struct GameTracker *gGT = sdata->gGT;
	s32 dir[4][3];
	u32 distance[4];
	u32 closestDistance = 9000;
	int closestCamera = 0;
	u32 modifyFlags;

	if ((*flags != 0) && ((*flags & 0xffff) != soundID))
	{
		OtherFX_Stop1(*flags);
		*flags = 0;
	}

	if (soundID == (u32)-1)
	{
		return;
	}

	for (int i = 0; i < (u8)gGT->numPlyrCurrGame; i++)
	{
		dir[i][0] = CTR_MipsSubLo(inst->matrix.t[0], gGT->pushBuffer[i].pos.x);
		dir[i][1] = CTR_MipsSubLo(inst->matrix.t[1], gGT->pushBuffer[i].pos.y);
		dir[i][2] = CTR_MipsSubLo(inst->matrix.t[2], gGT->pushBuffer[i].pos.z);

		distance[i] = GTE_GetSquaredLength(dir[i]);
		distance[i] = SquareRoot0_stub(distance[i]);

		if (distance[i] < closestDistance)
		{
			closestCamera = i;
			closestDistance = distance[i];
		}
	}

	if (closestDistance == 9000)
	{
		return;
	}

	GTE_AudioLR_Inst(&gGT->pushBuffer[closestCamera].matrix_Camera, dir[closestCamera]);

	modifyFlags = PlaySound3D_Flags_BuildFlags(gGT, closestCamera, closestDistance, PlaySound3D_CalculateLR(dir[closestCamera]));
	if (*flags == 0)
	{
		*flags = OtherFX_Play_LowLevel(soundID & 0xffff, 0, modifyFlags);
	}
	else
	{
		OtherFX_Modify(*flags, modifyFlags);
	}
}
