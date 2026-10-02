#include <common.h>

#ifndef HOWL_GARAGE_SOUND_IDS
#define HOWL_GARAGE_SOUND_IDS (sdata->garageSoundIDs)
#endif
#ifndef HOWL_GARAGE_RNG_READ
#define HOWL_GARAGE_RNG_READ (sdata->audioRNG)
#endif
#ifndef HOWL_GARAGE_RNG_WRITE
#define HOWL_GARAGE_RNG_WRITE (sdata->audioRNG)
#endif

void Garage_Init(void)
{
	struct GarageFX *garageSounds;
	s32 i;

	// erase backup, keep music, stop all fx
	howl_StopAudio(1, 0, 1);

	for (i = 0; i < 8; i++)
	{
		garageSounds = &GAME_GARAGE_SOUND_POOL[i];
		garageSounds->gsp_curr = GSP_GONE;
		garageSounds->gsp_prev = GSP_GONE;
		garageSounds->volume = 0;
		garageSounds->LR = 0;
		garageSounds->soundIDCount = 0;
	}
	return;
}

void Garage_Enter(int charId)
{
	struct GarageFX *garageSounds;
	// NOTE(aalhendi): Keep the pool address in v0 until its two array iterators are initialized.
	register struct GarageFX *garageBase CTR_PSX_REGISTER("$2");
	u8 *soundIDs;

	struct GarageFX *countSlot;
	s32 i;
	s32 charRight;
	s32 charLeft;
	s32 volume;
	// NOTE(aalhendi): Keep the count argument in a0 across the signed volume reload.
	register u32 *arg CTR_PSX_REGISTER("$4");
	s32 LR;
	s32 gone;
	if (charId >= 8)
		return;
	// NOTE(aalhendi): Retail wraps the additions, then clamps only the adjacent IDs.
	charLeft = CTR_MipsSubLo(charId, 1);
	charRight = CTR_MipsAddLo(charId, 1);
	if (charLeft < 0)
		charLeft = 7;
	if (charRight >= 8)
		charRight = 0;
	i = 0;
	gone = GSP_GONE;
	garageBase = GAME_GARAGE_SOUND_POOL;
	garageSounds = garageBase;
	soundIDs = HOWL_GARAGE_SOUND_IDS;
	// NOTE(aalhendi): Retain the independent ID iterator and its absolute address construction.
	CTR_PSX_KEEP_VALUE_RELAXED(soundIDs);
	countSlot = garageSounds;
	for (; i < 8; garageSounds++, soundIDs++, i++, countSlot++)
	{
		// NOTE(aalhendi): Retain the struct base so field stores keep their retail offsets.
		CTR_PSX_KEEP_VALUE_RELAXED(garageSounds);
		if (i == charId)
		{
			garageSounds->gsp_curr = GSP_CENTER;
			garageSounds->gsp_prev = gone;
			garageSounds->volume = 0;
			LR = 0x80;
			garageSounds->LR = LR;
			if (*soundIDs != 0)
			{
				arg = &countSlot->soundIDCount;
				volume = garageSounds->volume;
				// NOTE(aalhendi): The retail word combines the signed volume low word with pan/distortion.
				OtherFX_RecycleNew(arg, *soundIDs, ((u32)volume << 16) | 0x8080);
				continue;
			}
		}
		else if (i == charLeft)
		{
			garageSounds->gsp_curr = GSP_LEFT;
			garageSounds->gsp_prev = gone;
			garageSounds->volume = 0;
			garageSounds->LR = 0x3c;
			if (*soundIDs != 0)
			{
				arg = &countSlot->soundIDCount;
				volume = garageSounds->volume;
				OtherFX_RecycleNew(arg, *soundIDs, ((u32)volume << 16) | 0x803c);
				continue;
			}
		}
		else if (i == charRight)
		{
			garageSounds->gsp_curr = GSP_RIGHT;
			garageSounds->gsp_prev = gone;
			garageSounds->volume = 0;
			garageSounds->LR = 0xc3;
			if (*soundIDs != 0)
			{
				arg = &countSlot->soundIDCount;
				volume = garageSounds->volume;
				OtherFX_RecycleNew(arg, *soundIDs, ((u32)volume << 16) | 0x80c3);
				continue;
			}
		}
		else
		{
			garageSounds->gsp_curr = gone;
			garageSounds->gsp_prev = gone;
			garageSounds->volume = 0;
			LR = 0x80;
			garageSounds->LR = LR;
		}
		garageSounds->soundIDCount = 0;
	}
}

void Garage_PlayFX(u32 soundId, int charId)
{
	if (charId < PINSTRIPE)
	{
		u32 volumeFlags;
		u32 lrFlags;
		// NOTE(aalhendi): Test the full ID before the final call truncates it to 16 bits.
		if (soundId == 0xf6)
		{
			u32 rng = ((HOWL_GARAGE_RNG_READ >> 3) + HOWL_GARAGE_RNG_READ * 0x20000000) * 5 + 1;
			HOWL_GARAGE_RNG_WRITE = rng;
			soundId = (rng % 3) + 0xf3;
		}
		volumeFlags = (u32)(u8)GAME_GARAGE_SOUND_POOL[charId].volume << 16;
		lrFlags = (u8)GAME_GARAGE_SOUND_POOL[charId].LR | 0x8000;
		OtherFX_Play_LowLevel(soundId & 0xffff, 1, volumeFlags | lrFlags);
	}
}

void Garage_LerpFX(void)
{
	int i = 0;
	struct GarageFX *base = GAME_GARAGE_SOUND_POOL;
	struct GarageFX *garageSounds;
	struct GarageFX *soundHandleEntry;
	register u32 *soundIDCountRef CTR_PSX_REGISTER("$19");

	// NOTE(aalhendi): Keep the initial handle address available before the entry base.
	soundIDCountRef = &base->soundIDCount;
	soundHandleEntry = base;
	garageSounds = base;
	CTR_PSX_DEPEND_VALUE(garageSounds, soundIDCountRef);

	for (; i < 8; ++soundHandleEntry, ++i, ++garageSounds)
	{
		u8 *soundID;
		int targetVolume, targetLR;
		u8 garageSoundPos;

		CTR_PSX_KEEP_VALUE_RELAXED(garageSounds);
		garageSoundPos = garageSounds->gsp_curr;

		if (garageSoundPos == GSP_CENTER)
		{
			targetVolume = 0xff;
			targetLR = 0x80;
		}
		else if (garageSoundPos == GSP_LEFT)
		{
			targetVolume = 100;
			targetLR = 0x3c;
		}
		else if (garageSoundPos == GSP_RIGHT)
		{
			targetVolume = 100;
			targetLR = 0xc3;
		}
		else
		{
			targetVolume = 0;
			targetLR = garageSounds->LR;
		}

		if (targetLR == garageSounds->LR && targetVolume == garageSounds->volume)
		{
			continue;
		}

		// NOTE(aalhendi): Each halfword store truncates before the signed clamp comparison.
		if (targetVolume != garageSounds->volume)
		{
			if (garageSounds->volume < targetVolume)
			{
				garageSounds->volume += 8;
				if (garageSounds->volume > targetVolume)
					garageSounds->volume = targetVolume;
			}
			else
			{
				garageSounds->volume -= 8;
				if (garageSounds->volume < targetVolume)
					garageSounds->volume = targetVolume;
			}
		}

		if (targetLR != garageSounds->LR)
		{
			if (garageSounds->LR < targetLR)
			{
				garageSounds->LR += 2;
				if (garageSounds->LR > targetLR)
					garageSounds->LR = targetLR;
			}
			else
			{
				garageSounds->LR -= 2;
				if (garageSounds->LR < targetLR)
					garageSounds->LR = targetLR;
			}
		}

		CTR_PSX_MEMORY_BARRIER();
		{
			u8 *soundIDs = HOWL_GARAGE_SOUND_IDS;
			CTR_PSX_KEEP_VALUE(soundIDs);
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(soundID, soundIDs, i);
		}
		if (*soundID != 0)
		{
			// NOTE(aalhendi): Retail packs both signed halfwords without byte masks.
			u32 volumeFlags = (u32)CTR_MipsSll(garageSounds->volume, 16);
			u32 panFlags = (u32)(s32)garageSounds->LR | 0x8000;
			OtherFX_RecycleNew(&soundHandleEntry->soundIDCount, *soundID, volumeFlags | panFlags);
		}

		if (targetLR == garageSounds->LR && targetVolume == garageSounds->volume)
		{
			garageSoundPos = garageSounds->gsp_curr;
			garageSounds->gsp_prev = garageSoundPos;
			if (garageSoundPos == GSP_GONE)
			{
				OtherFX_RecycleMute(&soundHandleEntry->soundIDCount);
			}
		}
	}
}

void Garage_MoveLR(int desiredId)
{
	struct GarageFX *garageSounds;
	s32 i;
	s32 charRight;
	s32 charLeft;
	s32 value;
	if (desiredId > 7)
		return;
	// NOTE(aalhendi): Retail wraps the additions, then clamps only the adjacent IDs.
	charLeft = CTR_MipsSubLo(desiredId, 1);
	charRight = CTR_MipsAddLo(desiredId, 1);
	if (charLeft < 0)
		charLeft = 7;
	if (charRight >= 8)
		charRight = 0;
	for (i = 0; i < 8; i++)
	{
		garageSounds = &GAME_GARAGE_SOUND_POOL[i];
		if (i == desiredId)
			garageSounds->gsp_curr = GSP_CENTER;
		else if (i == charLeft)
		{
			if (garageSounds->gsp_curr == GSP_GONE)
			{
				value = 0x3c;
				garageSounds->LR = value;
			}
			value = GSP_LEFT;
			garageSounds->gsp_curr = value;
		}
		else if (i == charRight)
		{
			if (garageSounds->gsp_curr == GSP_GONE)
			{
				value = 0xc3;
				garageSounds->LR = value;
			}
			value = GSP_RIGHT;
			garageSounds->gsp_curr = value;
		}
		else
			garageSounds->gsp_curr = GSP_GONE;
	}
}

void Garage_Leave(void)
{
	int i;
	struct GarageFX *garageSounds = GAME_GARAGE_SOUND_POOL;

	for (i = 0; i < 8; i++)
	{
		garageSounds[i].gsp_curr = GSP_GONE;
	}

	return;
}
