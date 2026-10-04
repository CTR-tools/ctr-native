#include <common.h>

enum
{
	UI_LAP_TIME_LAPS_PER_PLAYER = 7,
	UI_LAP_TIME_TICKS_PER_SECOND = 0x3c0,
	UI_LAP_TIME_TICKS_PER_TENTH_SECOND = 96,
	UI_LAP_TIME_TICKS_PER_TEN_SECONDS = 0x2580,
	UI_LAP_TIME_TICKS_PER_MINUTE = 0xe100,
	UI_LAP_TIME_MAX_MINUTES = 9,
	UI_LAP_TIME_SECONDS_TENS_MOD = 6,
	UI_LAP_TIME_DECIMAL_BASE = 10,
	UI_LAP_TIME_CENTISECOND_SCALE = 100,

	UI_REWARD_WUMPA_SHINE_CENTER = 0x80,
	UI_REWARD_WUMPA_SHINE_SHIFT = 4,
	UI_REWARD_PICKUP_ROT_SLOW = 0x40,
	UI_REWARD_PICKUP_ROT_FAST = 0x80,
	// NOTE(aalhendi): Retail reads the four adjacent HUD bytes as one word:
	// only the HUD-visible bit and zero demo byte decide visibility.
	UI_REWARD_HUD_VISIBLE_WORD_MASK = 0xff0100,
	UI_REWARD_HUD_VISIBLE_WORD_VALUE = 0x100,
	UI_REWARD_FADE_VISIBLE_MIN = 0xfff,
	UI_REWARD_CTR_LETTER_BASE_SCALE = 0x800,
	UI_REWARD_CTR_LETTER_NEGATIVE_SCALE_BIAS = 0x401,
	UI_REWARD_CTR_LETTER_ROT_SHIFT = 10,
	UI_REWARD_CTR_LETTER_ROT_STEP = 0x200,
};

#define UI_REWARD_PICKUP_COLOR 0xffff0000u

void UI_SaveLapTime(s32 lapIndex, s32 lapTime, s16 driverID)
{
	s32 *minuteSlots = GAME_LAP_MINUTE_UNITS;
	s32 playerLapIndex = ((s32)driverID * UI_LAP_TIME_LAPS_PER_PLAYER) + lapIndex;
	s32 numMinutes;
	s32 tenSeconds;
	s32 seconds;

	numMinutes = (minuteSlots[playerLapIndex] = lapTime / UI_LAP_TIME_TICKS_PER_MINUTE);

	if (UI_LAP_TIME_MAX_MINUTES < numMinutes)
	{
		minuteSlots[playerLapIndex] = UI_LAP_TIME_MAX_MINUTES;
		GAME_LAP_SECOND_TENS[playerLapIndex] = UI_LAP_TIME_SECONDS_TENS_MOD - 1;
		GAME_LAP_SECOND_UNITS[playerLapIndex] = UI_LAP_TIME_DECIMAL_BASE - 1;
		GAME_LAP_CENTISECOND_TENS[playerLapIndex] = UI_LAP_TIME_DECIMAL_BASE - 1;
		GAME_LAP_CENTISECOND_UNITS[playerLapIndex] = UI_LAP_TIME_DECIMAL_BASE - 1;
		return;
	}

	// NOTE(aalhendi): Carry each quotient into the next digit. Scaling ticks
	// by ten first would wrap before division and change the retail result.
	tenSeconds = lapTime / UI_LAP_TIME_TICKS_PER_TEN_SECONDS;
	GAME_LAP_SECOND_TENS[playerLapIndex] = tenSeconds - numMinutes * UI_LAP_TIME_SECONDS_TENS_MOD;
	GAME_LAP_SECOND_UNITS[playerLapIndex] = (seconds = lapTime / UI_LAP_TIME_TICKS_PER_SECOND) - tenSeconds * UI_LAP_TIME_DECIMAL_BASE;
	GAME_LAP_CENTISECOND_TENS[playerLapIndex] = lapTime / UI_LAP_TIME_TICKS_PER_TENTH_SECOND - seconds * UI_LAP_TIME_DECIMAL_BASE;
	// NOTE(aalhendi): Keep the hundredfold product modulo 32 bits, then divide
	// as signed. The direct constant also lets old GCC synthesize shift/adds.
	GAME_LAP_CENTISECOND_UNITS[playerLapIndex] =
	    ((s32)((u32)lapTime * UI_LAP_TIME_CENTISECOND_SCALE) / UI_LAP_TIME_TICKS_PER_SECOND) % UI_LAP_TIME_DECIMAL_BASE;
}

void UI_ThTick_CountPickup(struct Thread *bucket)
{
	struct UiElement3D *obj = bucket->object;
	struct Instance *inst = bucket->inst;
	MATRIX *mat;
	u32 drawOtagState;

	{
		struct GameTracker *gGT = GAME_TRACKER;

		inst->colorRGBA = UI_REWARD_PICKUP_COLOR;

		if ((gGT->numPlyrCurrGame == 1) && (inst->model->id != STATIC_TIME_CRATE_01))
		{
			if (gGT->drivers[0]->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
			{
				inst->alphaScale = ((s32)CTR_ReadU16LE(&sdata->wumpaShineResult) - UI_REWARD_WUMPA_SHINE_CENTER) * (1 << UI_REWARD_WUMPA_SHINE_SHIFT);
			}
			else
			{
				inst->alphaScale = 0;
			}
		}
	}

	if (inst->model->id == STATIC_TIME_CRATE_01)
	{
		obj->rot.y = (u16)obj->rot.y + UI_REWARD_PICKUP_ROT_SLOW;
	}
	else
	{
		obj->rot.y = (u16)obj->rot.y + UI_REWARD_PICKUP_ROT_FAST;
	}

	mat = &inst->matrix;
	ConvertRotToMatrix(mat, &obj->rot);
	MatrixRotate(mat, &obj->m, mat);

	drawOtagState = CTR_ReadU32LE(&GAME_TRACKER->bool_DrawOTag_InProgress);
	if ((drawOtagState & UI_REWARD_HUD_VISIBLE_WORD_MASK) != UI_REWARD_HUD_VISIBLE_WORD_VALUE)
	{
		inst->flags |= HIDE_MODEL;
	}
	else
	{
		inst->flags &= ~HIDE_MODEL;
	}
}

void UI_ThTick_Reward(struct Thread *bucket)
{
	struct Instance *inst = bucket->inst;
	struct UiElement3D *obj = bucket->object;
	struct GameTracker *gGT;
	MATRIX *mat;
	u32 drawOtagState;

	obj->rot.y += UI_REWARD_PICKUP_ROT_SLOW;
	Vector_SpecLightSpin2D(inst, &obj->rot, &obj->lightDir);

	mat = &inst->matrix;
	ConvertRotToMatrix(mat, &obj->rot);
	MatrixRotate(mat, &obj->m, mat);

	gGT = GAME_TRACKER;
	drawOtagState = CTR_ReadU32LE(&gGT->bool_DrawOTag_InProgress);
	if (((drawOtagState & UI_REWARD_HUD_VISIBLE_WORD_MASK) != UI_REWARD_HUD_VISIBLE_WORD_VALUE) ||
	    (gGT->pushBuffer_UI.fadeFromBlack_currentValue <= UI_REWARD_FADE_VISIBLE_MIN))
	{
		inst->flags |= HIDE_MODEL;
	}
	else
	{
		inst->flags &= ~HIDE_MODEL;
	}
}

void UI_ThTick_CtrLetters(struct Thread *bucket)
{
	struct Instance *inst = bucket->inst;
	struct UiElement3D *obj = bucket->object;
	SVec3 rot;
	MATRIX *mat;

	obj->rot.y += UI_REWARD_PICKUP_ROT_SLOW;

	Vector_SpecLightSpin2D(inst, &obj->rot, &obj->lightDir);

	if (((GAME_TRACKER->gameMode1 & END_OF_RACE) != 0) && RaceFlag_IsTransitioning())
	{
		inst->scale.x = 0;
		inst->scale.y = 0;
		inst->scale.z = 0;
	}

	// NOTE(aalhendi): Initialize the complete rotation in each branch; old
	// GCC merges the zero-Z tail before preparing the matrix-call arguments.
	if (inst->scale.x != UI_REWARD_CTR_LETTER_BASE_SCALE)
	{
		s32 endOfRaceTransition;
		rot.x = 0;
		endOfRaceTransition = (s32)inst->scale.x - UI_REWARD_CTR_LETTER_BASE_SCALE;
		if (endOfRaceTransition < 0)
		{
			endOfRaceTransition = (s32)inst->scale.x - UI_REWARD_CTR_LETTER_NEGATIVE_SCALE_BIAS;
		}
		rot.y = ((endOfRaceTransition >> UI_REWARD_CTR_LETTER_ROT_SHIFT) + 1) * UI_REWARD_CTR_LETTER_ROT_STEP;
		rot.z = 0;
	}
	else
	{
		rot.x = 0;
		rot.y = 0;
		rot.z = 0;
	}

	mat = &inst->matrix;
	ConvertRotToMatrix(mat, &rot);

	MatrixRotate(mat, &obj->m, mat);
}

void UI_ThTick_big1(struct Thread *bucket)
{
	struct UiElement3D *obj = bucket->object;
	struct Instance *inst = bucket->inst;
	struct GameTracker *gGT;
	u16 scale;
	u32 drawOtagState;

	// NOTE(aalhendi): Scale is sampled for each diagonal store group. Clear
	// individual halfwords so negative scale never fills an off-diagonal.
	scale = (u16)obj->scale;
	inst->matrix.m[0][1] = 0;
	inst->matrix.m[0][2] = 0;
	inst->matrix.m[1][0] = 0;
	inst->matrix.m[0][0] = scale;

	scale = (u16)obj->scale;
	inst->matrix.m[1][2] = 0;
	inst->matrix.m[2][0] = 0;
	inst->matrix.m[2][1] = 0;
	inst->matrix.m[1][1] = scale;

	scale = (u16)obj->scale;
	inst->matrix.m[2][2] = scale;

	MatrixRotate(&inst->matrix, &obj->m, &inst->matrix);

	gGT = GAME_TRACKER;
	drawOtagState = CTR_ReadU32LE(&gGT->bool_DrawOTag_InProgress);
	if ((drawOtagState & UI_REWARD_HUD_VISIBLE_WORD_MASK) != UI_REWARD_HUD_VISIBLE_WORD_VALUE)
	{
		inst->flags |= HIDE_MODEL;
	}
	else
	{
		inst->flags &= ~HIDE_MODEL;
	}
}
