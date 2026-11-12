#include <common.h>
#include <ctr_gte_transfer.h>

#define RACE_FLAG_MATRIX data.matrixTitleFlag
#define RACE_FLAG_WAVE   data.checkerFlagVariables
#define RACE_FLAG_TRIG   data.trigApprox
#define RACE_FLAG_CAN_DRAW sdata->RaceFlag_CanDraw

// NOTE(aalhendi): The strip cursor starts one word before its first vertex.
// Reserve that word inside native scratch RAM; PSX keeps its hardware addresses.
#define RACE_FLAG_SCREEN_BASE CTR_SCRATCHPAD_PTR(u32, 4)

#define RACE_FLAG_LOADING_STAGE sdata->Loading.stage

static b32 s_raceFlagLoadingFadeActive;
static s32 s_raceFlagMenuFadeElapsed;

void RaceFlag_BeginMenuFade(void)
{
	if (g_config.loadingScreen == LOADING_SCREEN_FADE)
	{
		s_raceFlagLoadingFadeActive = 1;
		s_raceFlagMenuFadeElapsed = 0;
	}
}

enum RaceFlagScratchConstants
{
	RACE_FLAG_SCREEN_ROWS = 10,
	RACE_FLAG_SCREEN_POINTS_PER_ROW = 3,
	RACE_FLAG_SCREEN_POINTS_PER_COLUMN = RACE_FLAG_SCREEN_ROWS * RACE_FLAG_SCREEN_POINTS_PER_ROW,
};

enum RaceFlagConstants
{
	RACE_FLAG_POSITION_ONSCREEN = 0,
	RACE_FLAG_POSITION_OFFSCREEN = 5000,
	RACE_FLAG_POSITION_OFFSCREEN_LEFT = -5000,
	RACE_FLAG_OFFSCREEN_CHECK_BIAS = 4999,
	RACE_FLAG_OFFSCREEN_CHECK_WIDTH = 9999,
	RACE_FLAG_DRAW_ORDER_AFTER_FLAG = 1,
	RACE_FLAG_DRAW_ORDER_BEFORE_FLAG = -1,
	RACE_FLAG_DRAW_ORDER_DONE = 0,
	RACE_FLAG_TRANSITION_ONSCREEN = 0,
	RACE_FLAG_TRANSITION_OFFSCREEN = 2,
	RACE_FLAG_TRANSITION_BEGIN_ONSCREEN = 1,
	RACE_FLAG_TRANSITION_BEGIN_OFFSCREEN = 2,
	RACE_FLAG_TRANSITION_ONSCREEN_SPEED = 300,
	RACE_FLAG_TRANSITION_OFFSCREEN_SPEED_MAX = 1000,
	RACE_FLAG_TRANSITION_OFFSCREEN_ACCEL_SCALE = 10,
	RACE_FLAG_TRANSITION_ONSCREEN_SNAP_DISTANCE = 8,
	RACE_FLAG_TRANSITION_ONSCREEN_POSITION_SHIFT = 3,
	RACE_FLAG_TRANSITION_OFFSCREEN_POSITION_SHIFT = 2,
	RACE_FLAG_TRANSITION_TIME_SHIFT = 5,
	RACE_FLAG_LOADING_FADE_DURATION_MS = 100,
	RACE_FLAG_LOADING_IDLE_SLIDE_LIMIT = -1000,
	RACE_FLAG_LOADING_IDLE_SLIDE_STEP = 0x28,
	RACE_FLAG_LOADING_OFFSCREEN_X = 0x23c,
	RACE_FLAG_LOADING_CENTER_X = 0x100,
	RACE_FLAG_LOADING_Y = 0x6c,
	RACE_FLAG_LOADING_FONT_SIZE = 1,
	RACE_FLAG_LOADING_TEXT_FLAGS = 0,
	RACE_FLAG_LOADING_FIRST_CENTER_FRAME = 4,
	RACE_FLAG_LOADING_LAST_CENTER_FRAME = 0x4a,
	RACE_FLAG_LOADING_EXIT_BASE_FRAME = 0x4b,
	RACE_FLAG_LOADING_LAST_VISIBLE_FRAME = 0x4f,
	RACE_FLAG_LOADING_ANIM_RESET_FRAME = 0x50,
	RACE_FLAG_LOADING_LETTER_SPEED_X = 0x3c,
	RACE_FLAG_LOADING_NEXT_LETTER_START_X = 0xf0,
	RACE_FLAG_LOADING_LETTER_FRAME_STEP = 4,
	RACE_FLAG_LOADING_GLYPH_EXTENDED_MAX = 4,
	RACE_FLAG_LOADING_FRAME_TIME_SHIFT = 5,
	RACE_FLAG_LOADING_MIN_FRAME_ADVANCE = 1,
	RACE_FLAG_LOADING_REPEAT_STAGE_FIRST = 6,
	RACE_FLAG_LOADING_REPEAT_STAGE_COUNT = 2,
	RACE_FLAG_TRIG_TABLE_MASK = 0x3ff,
	RACE_FLAG_TRIG_HIGH_HALF_MASK = 0x400,
	RACE_FLAG_TRIG_NEGATE_MASK = 0x800,
};

static s32 RaceFlag_LoadingFadeStep(s32 remaining)
{
	s32 step = RACE_FLAG_POSITION_OFFSCREEN * GAME_TRACKER->elapsedTimeMS / RACE_FLAG_LOADING_FADE_DURATION_MS;
	if (step < 1)
	{
		step = 1;
	}
	return step < remaining ? step : remaining;
}


s32 RaceFlag_MoveModels(s32 frameIndex, s32 numFrames)
{
	// Move intro logos and character-selection models along the same eased curve.

	s16 midpoint;
	s32 result;
	// NOTE(aalhendi): Retain the angle's argument-register lifetime across scaling and division.
	register s32 angle CTR_PSX_REGISTER("$4");

	if (frameIndex < 0)
	{
		return 0;
	}

	if (frameIndex > numFrames)
	{
		return 0x1000;
	}

	// NOTE(aalhendi): The midpoint narrows before either half of the transition.
	midpoint = numFrames / 2;

	// if more than half done
	if (frameIndex >= midpoint)
	{
		angle = frameIndex - midpoint;
		angle *= 0x400;
		result = MATH_Sin(angle / midpoint) / 2 + 0x800;
	}
	// if less than half done
	else
	{
		angle = midpoint - frameIndex;
		angle *= 0x400;
		result = 0x800 - MATH_Sin(angle / midpoint) / 2;
	}
	return result;
}


b32 RaceFlag_IsFullyOnScreen(void)
{
	// return true if flag is fully on screen
	// return false if flag is not fully on screen
	return (sdata->RaceFlag_Position == RACE_FLAG_POSITION_ONSCREEN);
}


b32 RaceFlag_IsFullyOffScreen(void)
{
	// return false, "not true", if flag is < 5000, partially on-screen
	// return true, "not false", if flag is >= 5000, fully off-screen
	return ((((u32)(u16)sdata->RaceFlag_Position + RACE_FLAG_OFFSCREEN_CHECK_BIAS) & 0xffff) < RACE_FLAG_OFFSCREEN_CHECK_WIDTH) ^ 1;
}


b32 RaceFlag_IsTransitioning(void)
{
	int pos = sdata->RaceFlag_Position;

	return
	    // if checkered flag is not fully on-screen and not fully off-screen
	    (pos != RACE_FLAG_POSITION_ONSCREEN) && (pos != RACE_FLAG_POSITION_OFFSCREEN_LEFT) && (pos != RACE_FLAG_POSITION_OFFSCREEN) &&

	    // is allowed to render
	    ((GAME_TRACKER->renderFlags & RENDER_FLAG_CHECKERED_FLAG) != 0);
}


void RaceFlag_SetDrawOrder(b32 drawAfterFlag)
{
	// NOTE(aalhendi): Individual volatile stores keep these writes out of return delay slots.
	if (drawAfterFlag)
	{
		*(volatile s16 *)&sdata->RaceFlag_DrawOrder = RACE_FLAG_DRAW_ORDER_AFTER_FLAG;
	}
	else
	{
		*(volatile s16 *)&sdata->RaceFlag_DrawOrder = RACE_FLAG_DRAW_ORDER_BEFORE_FLAG;
	}
}


void RaceFlag_BeginTransition(s32 direction)
{
	// Begin Transition on-screen
	if (direction == RACE_FLAG_TRANSITION_BEGIN_ONSCREEN)
	{
		sdata->RaceFlag_LoadingTextAnimFrame = -1;

		sdata->RaceFlag_Position = RACE_FLAG_POSITION_OFFSCREEN;

		*(volatile s32 *)&sdata->RaceFlag_AnimationType = RACE_FLAG_TRANSITION_ONSCREEN;
	}

	// Begin Transition off-screen
	else if (direction == RACE_FLAG_TRANSITION_BEGIN_OFFSCREEN)
	{
		RaceFlag_SetDrawOrder(0);

		sdata->RaceFlag_Position = RACE_FLAG_POSITION_ONSCREEN;

		sdata->RaceFlag_AnimationType = direction;
	}

	// enable loading screen's checkered flag
	GAME_TRACKER->renderFlags |= RENDER_FLAG_CHECKERED_FLAG;
}


void RaceFlag_SetFullyOnScreen(void)
{
	struct GameTracker *gGT = GAME_TRACKER;
	// NOTE(aalhendi): Keep the pending flags separate from the transition-state stores.
	register u32 flags CTR_PSX_REGISTER("$3") = gGT->renderFlags;
	sdata->RaceFlag_Position = RACE_FLAG_POSITION_ONSCREEN;
	sdata->RaceFlag_AnimationType = RACE_FLAG_TRANSITION_ONSCREEN;
	sdata->RaceFlag_LoadingTextAnimFrame = -1;

	// enable loading screen's checkered flag
	gGT->renderFlags = flags | RENDER_FLAG_CHECKERED_FLAG;
}


void RaceFlag_SetFullyOffScreen(void)
{
	struct GameTracker *gGT = GAME_TRACKER;
	u32 flags;
	sdata->RaceFlag_LoadingTextAnimFrame = -1;

	// flag is now fully off-screen
	sdata->RaceFlag_Position = RACE_FLAG_POSITION_OFFSCREEN;
	flags = gGT->renderFlags;
	sdata->RaceFlag_AnimationType = RACE_FLAG_TRANSITION_ONSCREEN;

	// disable loading screen's checkered flag
	gGT->renderFlags = flags & ~RENDER_FLAG_CHECKERED_FLAG;
}


void RaceFlag_SetCanDraw(s16 canDraw)
{
	*(volatile s16 *)&sdata->RaceFlag_CanDraw = canDraw;
}


s32 RaceFlag_GetCanDraw(void)
{
	return sdata->RaceFlag_CanDraw;
}


u32 *RaceFlag_GetOT(void)
{
	s32 drawAfterFlag;

	if (sdata->RaceFlag_DrawInitialized == 0)
		sdata->RaceFlag_DrawInitialized = 1;

	// NOTE(aalhendi): Keep the draw-order comparison value live across the transition dispatch.
	drawAfterFlag = RACE_FLAG_DRAW_ORDER_AFTER_FLAG;
	switch ((u32)sdata->RaceFlag_AnimationType)
	{
	case RACE_FLAG_TRANSITION_ONSCREEN:
	{
		u16 position;
		if (sdata->RaceFlag_Position < 0)
			sdata->RaceFlag_Position = RACE_FLAG_POSITION_OFFSCREEN;
		position = sdata->RaceFlag_Position;
		sdata->RaceFlag_TransitionSpeed = RACE_FLAG_TRANSITION_ONSCREEN_SPEED;
		if ((s16)position > 0)
		{
			struct GameTracker *gGT;
			if ((s16)position < RACE_FLAG_TRANSITION_ONSCREEN_SNAP_DISTANCE)
				*(volatile s16 *)&sdata->RaceFlag_Position = RACE_FLAG_POSITION_ONSCREEN;
			else
			{
				s32 step = (((s16)position >> RACE_FLAG_TRANSITION_ONSCREEN_POSITION_SHIFT) * GAME_TRACKER->elapsedTimeMS) >> RACE_FLAG_TRANSITION_TIME_SHIFT;
				if (g_config.loadingScreen == LOADING_SCREEN_FADE && s_raceFlagLoadingFadeActive)
				{
					step = RaceFlag_LoadingFadeStep((s16)position);
				}
				if (step <= 0)
					step = 1;
				sdata->RaceFlag_Position = position - step;
			}
			gGT = GAME_TRACKER;
			return gGT->otSwapchainDB[gGT->swapchainIndex];
		}
		else
		{
			// NOTE(aalhendi): Classification retains its signed word while the update uses the raw halfword.
			register s32 order CTR_PSX_REGISTER("$4") = sdata->RaceFlag_DrawOrder;
			u16 rawOrder = sdata->RaceFlag_DrawOrder;
			*(volatile s16 *)&sdata->RaceFlag_Position = RACE_FLAG_POSITION_ONSCREEN;
			if (order == drawAfterFlag)
			{
				struct GameTracker *gGT = GAME_TRACKER;
				return gGT->otSwapchainDB[gGT->swapchainIndex];
			}
			if (order == RACE_FLAG_DRAW_ORDER_BEFORE_FLAG)
				*(volatile s16 *)&sdata->RaceFlag_DrawOrder = rawOrder + 1;
			else
				return GAME_TRACKER->pushBuffer[0].ptrOT + 0x3ff;
		}
		break;
	}
	case 1:
		break;
	case RACE_FLAG_TRANSITION_OFFSCREEN:
	{
		s16 speed = sdata->RaceFlag_TransitionSpeed;
		u16 rawSpeed = sdata->RaceFlag_TransitionSpeed;
		s16 position;
		u16 rawPosition;
		if (speed < RACE_FLAG_TRANSITION_OFFSCREEN_SPEED_MAX)
			sdata->RaceFlag_TransitionSpeed =
			    rawSpeed + ((GAME_TRACKER->elapsedTimeMS * RACE_FLAG_TRANSITION_OFFSCREEN_ACCEL_SCALE) >> RACE_FLAG_TRANSITION_TIME_SHIFT);
		position = sdata->RaceFlag_Position;
		rawPosition = sdata->RaceFlag_Position;
		if (position > RACE_FLAG_POSITION_OFFSCREEN_LEFT)
		{
			u16 nextSpeed = sdata->RaceFlag_TransitionSpeed;
			s32 step = (((s16)nextSpeed >> RACE_FLAG_TRANSITION_OFFSCREEN_POSITION_SHIFT) * GAME_TRACKER->elapsedTimeMS) >> RACE_FLAG_TRANSITION_TIME_SHIFT;
			if (g_config.loadingScreen == LOADING_SCREEN_FADE && s_raceFlagLoadingFadeActive)
			{
				step = RaceFlag_LoadingFadeStep(position - RACE_FLAG_POSITION_OFFSCREEN_LEFT);
			}
			*(volatile s16 *)&sdata->RaceFlag_Position =
			    rawPosition - step;
		}
		else
		{
			struct GameTracker *gGT = GAME_TRACKER;
			u32 flags;
			sdata->RaceFlag_Position = RACE_FLAG_POSITION_OFFSCREEN;
			flags = gGT->renderFlags;
			sdata->RaceFlag_AnimationType = RACE_FLAG_TRANSITION_ONSCREEN;
			gGT->renderFlags = flags & ~RENDER_FLAG_CHECKERED_FLAG;
		}
		break;
	}
	}
	{
		struct GameTracker *gGT = GAME_TRACKER;
		return gGT->otSwapchainDB[gGT->swapchainIndex];
	}
}


void RaceFlag_ResetTextAnim(void)
{
	*(volatile s32 *)&sdata->RaceFlag_LoadingTextAnimFrame = -1;
}


void RaceFlag_DrawLoadingString(void)
{
	// NOTE(aalhendi): The tracker is short-lived; the byte index survives both font calls.
	register struct GameTracker *gGT CTR_PSX_REGISTER("$3") = GAME_TRACKER;
	s32 loadingTextBytes;
	s32 letterAnimFrame;
	s32 letterX;
	s32 glyphByteCount;
	register s32 textByteIndex CTR_PSX_REGISTER("$18");
	char *loadingText;
	s32 nextLetterStartX;
	s32 drawX;
	s32 textWidth;
	u32 *oldOT;
	u8 glyph[3];

	// NOTE(aalhendi): Font loops look ahead before checking the glyph count.
	// Keep that byte inside initialized storage for either one- or two-byte glyphs.
	glyph[1] = 0;
	glyph[2] = 0;

	// Draw loading text into the active swapchain's ordering table.
	oldOT = (u32 *)gGT->pushBuffer_UI.ptrOT;

	gGT->pushBuffer_UI.ptrOT = gGT->otSwapchainDB[gGT->swapchainIndex];
	loadingText = GAME_LANGUAGE_STRINGS[LNG_LOADING];

	// get length of "LOADING..." string
	loadingTextBytes = strlen(loadingText);

	textWidth = DecalFont_GetLineWidth(loadingText, RACE_FLAG_LOADING_FONT_SIZE);

	// loop counter
	textByteIndex = 0;

	// if game is not loading
	if (RACE_FLAG_LOADING_STAGE == LOAD_IDLE)
	{
		s32 transition = sdata->RaceFlag_Transition;
		b32 atLimit = transition <= RACE_FLAG_LOADING_IDLE_SLIDE_LIMIT;
		if (!atLimit)
		{
			// NOTE(aalhendi): Keep the decision live until the update so width narrowing cannot fill its branch delay slot.
			CTR_PSX_KEEP_VALUE(atLimit);
			*(volatile s32 *)&sdata->RaceFlag_Transition = transition - RACE_FLAG_LOADING_IDLE_SLIDE_STEP;
		}
	}
	else
	{
		sdata->RaceFlag_Transition = 0;
	}

	drawX = (u16)sdata->RaceFlag_Transition - (s16)textWidth / 2;

	letterAnimFrame = sdata->RaceFlag_LoadingTextAnimFrame;

	if (0 < loadingTextBytes)
	{
		nextLetterStartX = letterAnimFrame * -RACE_FLAG_LOADING_LETTER_SPEED_X + RACE_FLAG_LOADING_OFFSCREEN_X;

		// for each byte in the localized "LOADING..." string
		do
		{
			if (letterAnimFrame >= 0)
			{
				letterX = nextLetterStartX;
				if (letterAnimFrame > RACE_FLAG_LOADING_FIRST_CENTER_FRAME)
				{
					if (letterAnimFrame > RACE_FLAG_LOADING_LAST_CENTER_FRAME)
					{
						if (letterAnimFrame > RACE_FLAG_LOADING_LAST_VISIBLE_FRAME)
							goto DrawLetterOffscreen;
						letterX = (RACE_FLAG_LOADING_EXIT_BASE_FRAME - letterAnimFrame) * RACE_FLAG_LOADING_LETTER_SPEED_X + RACE_FLAG_LOADING_CENTER_X;
					}
					else
						letterX = RACE_FLAG_LOADING_CENTER_X;
				}
			}
			else
			{
			DrawLetterOffscreen:
				letterX = RACE_FLAG_LOADING_OFFSCREEN_X;
			}
			glyph[0] = *loadingText++;
			glyphByteCount = 1;
			if (glyph[0] < RACE_FLAG_LOADING_GLYPH_EXTENDED_MAX)
			{
				glyph[1] = *loadingText;
				loadingText += glyphByteCount;

				// increment loop counter
				textByteIndex = textByteIndex + glyphByteCount;

				glyphByteCount = 2;
			}
			if ((s16)letterX != RACE_FLAG_LOADING_OFFSCREEN_X && g_config.loadingScreen != LOADING_SCREEN_NO_TEXT)
			{
				DecalFont_DrawLineStrlen((char *)glyph, glyphByteCount, (s16)(drawX + letterX), RACE_FLAG_LOADING_Y, RACE_FLAG_LOADING_FONT_SIZE,
				                         RACE_FLAG_LOADING_TEXT_FLAGS);
			}

			letterX = DecalFont_GetLineWidthStrlen((char *)glyph, glyphByteCount, RACE_FLAG_LOADING_FONT_SIZE);

			drawX = drawX + letterX;
			nextLetterStartX = nextLetterStartX + RACE_FLAG_LOADING_NEXT_LETTER_START_X;

			// increment loop counter
			textByteIndex = textByteIndex + 1;

			// treat all letters with 4 frame difference
			letterAnimFrame = letterAnimFrame - RACE_FLAG_LOADING_LETTER_FRAME_STEP;

		} while (textByteIndex < loadingTextBytes);
	}

	// NOTE(aalhendi): Font calls may change the tracker; restore the saved OT through the current one.
	gGT = GAME_TRACKER;
	gGT->pushBuffer_UI.ptrOT = (u32 *)oldOT;

	if (letterAnimFrame < RACE_FLAG_LOADING_ANIM_RESET_FRAME)
	{
		int frameAdvance = gGT->elapsedTimeMS >> RACE_FLAG_LOADING_FRAME_TIME_SHIFT;

		if (frameAdvance < RACE_FLAG_LOADING_MIN_FRAME_ADVANCE)
		{
			frameAdvance = RACE_FLAG_LOADING_MIN_FRAME_ADVANCE;
		}

		sdata->RaceFlag_LoadingTextAnimFrame += frameAdvance;
	}

	else
	{
		if ((u32)(RACE_FLAG_LOADING_STAGE - RACE_FLAG_LOADING_REPEAT_STAGE_FIRST) < RACE_FLAG_LOADING_REPEAT_STAGE_COUNT)
		{
			*(volatile s32 *)&sdata->RaceFlag_LoadingTextAnimFrame = 0;
		}
		else
			sdata->RaceFlag_LoadingTextAnimFrame = -1;
	}
	return;
}

static inline s32 RaceFlag_DecodeSine(u32 angle, s32 sine)
{
	// Select the packed sine/cosine halfword, then reflect the second half-cycle.
	if ((angle & RACE_FLAG_TRIG_HIGH_HALF_MASK) == 0)
	{
		sine = (u32)sine << 0x10;
	}

	sine = sine >> 0x10;

	if ((angle & RACE_FLAG_TRIG_NEGATE_MASK) != 0)
	{
		// make negative
		sine = -sine;
	}
	return sine;
}

static inline s32 RaceFlag_Sin(u32 angle)
{
	const struct TrigTable *table = RACE_FLAG_TRIG;
	return RaceFlag_DecodeSine(angle, (s32)CTR_ReadU32AlignedLE(&table[angle & RACE_FLAG_TRIG_TABLE_MASK]));
}

// NOTE(aalhendi): The first wave advances by elapsed time; columns use a fixed 64-tick step.
static inline s32 RaceFlag_StepWave(s32 *wave, s32 advance, s32 *light)
{
	u32 phase, angle;
	// NOTE(aalhendi): Retain the packed sample's register lifetime before decoding either halfword.
	register s32 sine CTR_PSX_REGISTER("$4");
	s32 height;
	wave[4] = (s32)((u32)wave[4] + advance);
	phase = wave[4] >> 5;
	if (phase > 4095)
	{
		wave[4] &= 0x1ffff;
		phase = wave[4] >> 5;
		angle = (u32)wave[0] + 512;
		wave[0] = (s32)angle;
		{
			const struct TrigTable *table = RACE_FLAG_TRIG;
			sine = (s32)CTR_ReadU32AlignedLE(&table[angle & RACE_FLAG_TRIG_TABLE_MASK]);
			CTR_PSX_OBSERVE_VALUE(sine);
			sine = RaceFlag_DecodeSine(angle, sine);
		}
		wave[1] = ((sine + 4095) * 32 >> 13) + 150;
		angle = (u32)wave[2] + 200;
		wave[2] = (s32)angle;
		{
			const struct TrigTable *table = RACE_FLAG_TRIG;
			sine = (s32)CTR_ReadU32AlignedLE(&table[angle & RACE_FLAG_TRIG_TABLE_MASK]);
			CTR_PSX_OBSERVE_VALUE(sine);
			sine = RaceFlag_DecodeSine(angle, sine);
		}
		wave[3] = ((sine + 4095) * 64 >> 13) + 180;
	}
	// NOTE(aalhendi): Keep sampling and biasing separate for the inline result's retail lifetime.
	height = RaceFlag_Sin(phase);
	height += 4095;
	phase += 3200;
	*light = RaceFlag_Sin(phase) + 4095;
	return height;
}

static inline u32 RaceFlag_ScreenSignMask(void)
{
	// NOTE(aalhendi): This inline boundary keeps GCC 2.8.1 from reassociating
	// the packed clipping expressions and lets it load the mask after the column bases.
	return 0x80008000u;
}

static void RaceFlag_DrawFade(u32 *ot, s32 position)
{
	typedef struct
	{
		u32 tag;
		u32 tpage;
		POLY_F4 quad;
	} RaceFlagFadePacket;
	struct PrimMem *primMem = &GAME_TRACKER->backBuffer->primMem;
	RaceFlagFadePacket *packet = primMem->cursor;
	int strength;

	if (position < 0)
	{
		position = -position;
	}
	if (position >= RACE_FLAG_POSITION_OFFSCREEN)
	{
		return;
	}
	if ((u8 *)(packet + 1) > (u8 *)primMem->guardEnd)
	{
		return;
	}
	strength = (RACE_FLAG_POSITION_OFFSCREEN - position) * 255 / RACE_FLAG_POSITION_OFFSCREEN;
	setlen(packet, 7);
	packet->tpage = 0xe1000e40;
	packet->quad.tag = 0;
	packet->quad.code = 0x2a;
	packet->quad.r0 = strength;
	packet->quad.g0 = strength;
	packet->quad.b0 = strength;
	packet->quad.x0 = 0;
	packet->quad.y0 = 0;
	packet->quad.x1 = 512;
	packet->quad.y1 = 0;
	packet->quad.x2 = 0;
	packet->quad.y2 = 240;
	packet->quad.x3 = 512;
	packet->quad.y3 = 240;
	AddPrim(ot, packet);
	primMem->cursor = packet + 1;
}

static void RaceFlag_DrawFlag(u32 *ot, s16 flagPosition)
{
	SVECTOR positions[3];
	s32 columnWave[5];
	u8 colors[2];
	s32 leftLight;
	s32 *firstWave;
	s32 columnPhase;
	// NOTE(aalhendi): These non-emitting bindings preserve retail loop/packet lifetimes;
	// native builds use ordinary locals. GTE transfers remain at the SDK boundary.
	register s32 row CTR_PSX_REGISTER("$14");
	s32 column;
	s32 point;
	s32 firstPoint;
	s32 segment;
	s32 toggle;
	s32 depth;
	s32 sine;
	s32 height;
	register s32 rightLight CTR_PSX_REGISTER("$25");
	u32 rippleAngle;
	register u32 *writeScreen CTR_PSX_REGISTER("$9");
	u32 *readScreen;
	s32 packedRight;
	register POLY_G4 *poly CTR_PSX_REGISTER("$8");
	CTR_GteSetRotMatrix(&RACE_FLAG_MATRIX);
	CTR_GteSetTransMatrix(&RACE_FLAG_MATRIX);
	CTR_GteSetGeomOffset(256, 120);
	gte_SetGeomScreen(256);
	firstWave = RACE_FLAG_WAVE;
	poly = 0;
	writeScreen = RACE_FLAG_SCREEN_BASE;
	toggle = 1;
	columnPhase = sdata->RaceFlag_ElapsedTime >> 5;
	rippleAngle = columnPhase;
	height = RaceFlag_StepWave(firstWave, firstWave[3] * GAME_TRACKER->elapsedTimeMS, &leftLight);
	depth = ((height * RACE_FLAG_WAVE[1]) >> 13) + 640;
	positions[0].vy = -910;
	positions[1].vy = -816;
	positions[2].vy = -722;
	flagPosition = (-3006) - flagPosition;
	positions[0].vx = flagPosition;
	positions[1].vx = flagPosition;
	positions[2].vx = flagPosition;
	for (row = 0; row < RACE_FLAG_SCREEN_ROWS; ++row)
	{
		for (firstPoint = 0; firstPoint < RACE_FLAG_SCREEN_POINTS_PER_ROW; ++firstPoint)
		{
			sine = RaceFlag_Sin(rippleAngle) + 4095;
			positions[firstPoint].vz = depth + ((sine * 32) >> 13);
			rippleAngle += 300;
		}

		CTR_GteLoadPositionsV0V1V2(positions, &positions[1], &positions[2]);
		gte_rtpt();
		positions[0].vy += 282;
		positions[1].vy += 282;
		positions[2].vy = positions[2].vy + 282;
		CTR_GteStorePositionsXY(writeScreen, writeScreen + 1, writeScreen + 2);
		writeScreen += 3;
	}

	rightLight = leftLight;
	columnWave[0] = RACE_FLAG_WAVE[0];
	columnWave[1] = RACE_FLAG_WAVE[1];
	columnWave[2] = RACE_FLAG_WAVE[2];
	columnWave[3] = RACE_FLAG_WAVE[3];
	columnWave[4] = RACE_FLAG_WAVE[4];

	for (column = 1; column < 35; ++column)
	{
		{
			// Alternate two 30-vertex scratch banks; keep the byte offset separate from the cursor base.
			register u8 *columnBase CTR_PSX_REGISTER("$3") = (u8 *)(RACE_FLAG_SCREEN_BASE - 1);
			register s32 byteOffset CTR_PSX_REGISTER("$2") = toggle * (RACE_FLAG_SCREEN_POINTS_PER_COLUMN * 4);
			writeScreen = (u32 *)(columnBase + byteOffset);
		}
		toggle ^= 1;
		readScreen = RACE_FLAG_SCREEN_BASE + (toggle * RACE_FLAG_SCREEN_POINTS_PER_COLUMN);
		columnPhase += 256;
		rippleAngle = columnPhase;
		height = RaceFlag_StepWave(columnWave, columnWave[3] * 64, &leftLight);
		depth = ((height * columnWave[1]) >> 13) + 640;
		positions[0].vy = -910;
		positions[1].vy = -816;
		positions[2].vy = -722;
		positions[0].vx += 100;
		positions[1].vx += 100;
		positions[2].vx += 100;
		for (point = 0; point < RACE_FLAG_SCREEN_POINTS_PER_ROW; ++point)
		{
			sine = RaceFlag_DecodeSine(rippleAngle, (s32)CTR_ReadU32AlignedLE(&RACE_FLAG_TRIG[rippleAngle & RACE_FLAG_TRIG_TABLE_MASK])) + 4095;
			positions[point].vz = depth + ((sine * 32) >> 13);
			rippleAngle += 300;
		}

		CTR_GteLoadPositionsV0V1V2(positions, &positions[1], &positions[2]);
		gte_rtpt();
		segment = 0;
		row = 0;
		do
		{
			s32 darkRight;
			s32 weightedRight;
			s32 shadeScale;
			register s32 inverseRight CTR_PSX_REGISTER("$10");
			++row;
			if (row < RACE_FLAG_SCREEN_ROWS + 1)
			{
				CTR_GteStorePositionsXY(writeScreen + 1, writeScreen + 2, writeScreen + 3);
				if (row < RACE_FLAG_SCREEN_ROWS)
				{
					for (point = 0; point < RACE_FLAG_SCREEN_POINTS_PER_ROW; ++point)
					{
						sine = RaceFlag_DecodeSine(rippleAngle, (s32)CTR_ReadU32AlignedLE(&RACE_FLAG_TRIG[rippleAngle & RACE_FLAG_TRIG_TABLE_MASK])) + 4095;
						positions[point].vz = depth + ((sine * 32) >> 13);
						rippleAngle += 300;
					}

					positions[0].vy += 282;
					positions[1].vy += 282;
					positions[2].vy += 282;
					CTR_GteLoadPositionsV0V1V2(positions, &positions[1], &positions[2]);
					gte_rtpt();
				}
			}
			{
				register s32 quadPoint CTR_PSX_REGISTER("$11");
				quadPoint = 0;
				if (segment == 0)
				{
					quadPoint = 1;
					++writeScreen;
				}

				weightedRight = rightLight * 105;

				shadeScale = 8192;
				inverseRight = shadeScale - rightLight;
				darkRight = (weightedRight + (inverseRight * 160)) >> 13;
				do
				{
					const u32 screenMaxXY = (216u << 16) | 512u;
					// Keep packed-word subtraction: an X borrow affects the Y halfword too.
					if (((((((readScreen[0] & (readScreen[1] & RaceFlag_ScreenSignMask())) & RaceFlag_ScreenSignMask()) & writeScreen[0]) &
					       RaceFlag_ScreenSignMask()) &
					      writeScreen[1]) == 0) &&
					    (((((((screenMaxXY - readScreen[0]) & ((screenMaxXY - readScreen[1]) & RaceFlag_ScreenSignMask())) & RaceFlag_ScreenSignMask()) &
					        (screenMaxXY - writeScreen[0])) &
					       RaceFlag_ScreenSignMask()) &
					      (screenMaxXY - writeScreen[1])) == 0))
					{
						struct DB *back = GAME_TRACKER->backBuffer;
						POLY_G4 *next = back->primMem.cursor;
						register u32 packedLeft CTR_PSX_REGISTER("$3");
						u32 packed;
						if (next <= ((POLY_G4 *)back->primMem.guardEnd))
						{
							poly = next;
							back->primMem.cursor = next + 1;
						}

						// NOTE(aalhendi): Retail reuses the last packet on exhaustion, which can
						// create a cyclic OT link. Native stops before relinking that packet.
						else
						{
							return;
						}
						if (!poly)
						{
							return;
						}
						if (((column >> 2) + (segment >> 2)) & 1)
						{
							s32 left = *((volatile s32 *)(&leftLight));
							colors[0] = darkRight;
							colors[1] = ((left * 105) + ((shadeScale - left) * 160)) >> 13;
						}
						else
						{
							s32 left = *((volatile s32 *)(&leftLight));
							colors[0] = ((rightLight * 130) + (inverseRight * 255)) >> 13;
							colors[1] = ((left * 130) + ((shadeScale - left) * 255)) >> 13;
						}
						CtrGpu_WritePackedXY(&poly->x0, *(readScreen++));
						CtrGpu_WritePackedXY(&poly->x2, *readScreen);
						CtrGpu_WritePackedXY(&poly->x1, *(writeScreen++));
						CtrGpu_WritePackedXY(&poly->x3, *writeScreen);
						packedLeft = (colors[0] | (colors[0] << 16)) | (colors[0] << 8);
						CTR_WriteU32AlignedLE(&poly->r0, packedLeft);
						CTR_WriteU32AlignedLE(&poly->r2, packedLeft);
						packedRight = (colors[1] | (colors[1] << 16)) | (colors[1] << 8);
						setlen(poly, 8);
						packed = packedRight;
						setcode(poly, 0x38);
						CTR_WriteU32AlignedLE(&poly->r1, packed);
						CTR_WriteU32AlignedLE(&poly->r3, packed);
						// NOTE(aalhendi): Preserve both tag length bytes while linking through native GPU tokens.
						{
							u32 addressMask = 0xffffffu;
							register u32 lengthMask CTR_PSX_REGISTER("$4") = 0xff000000u;
							poly->tag = (poly->tag & lengthMask) | (*ot & addressMask);
							*ot = (*ot & lengthMask) | (CtrGpu_PrimToOTLink24(poly) & addressMask);
						}
					}
					else
					{
						++readScreen;
						++writeScreen;
					}
					++quadPoint;
					++segment;
				} while (quadPoint < RACE_FLAG_SCREEN_POINTS_PER_ROW);
			}
		} while (row < RACE_FLAG_SCREEN_ROWS);
		rightLight = leftLight;
	}

	sdata->RaceFlag_ElapsedTime = (s32)(((u32)sdata->RaceFlag_ElapsedTime) + (GAME_TRACKER->elapsedTimeMS * 100));
}

void RaceFlag_DrawSelf(void)
{
	u32 *ot;
	if (!RACE_FLAG_CAN_DRAW)
	{
		return;
	}
	if (RACE_FLAG_LOADING_STAGE != LOAD_IDLE)
	{
		s_raceFlagLoadingFadeActive = 1;
		s_raceFlagMenuFadeElapsed = 0;
	}
	else if (g_config.loadingScreen == LOADING_SCREEN_FADE && s_raceFlagLoadingFadeActive &&
	         GAME_TRACKER->levelID == MAIN_MENU_LEVEL)
	{
		s_raceFlagMenuFadeElapsed += GAME_TRACKER->elapsedTimeMS;
		if (s_raceFlagMenuFadeElapsed >= RACE_FLAG_LOADING_FADE_DURATION_MS)
		{
			s_raceFlagLoadingFadeActive = 0;
		}
		else
		{
			RaceFlag_DrawFade(GAME_TRACKER->otSwapchainDB[GAME_TRACKER->swapchainIndex],
			                  s_raceFlagMenuFadeElapsed * RACE_FLAG_POSITION_OFFSCREEN / RACE_FLAG_LOADING_FADE_DURATION_MS);
			RaceFlag_DrawFlag(GAME_TRACKER->pushBuffer[0].ptrOT + 0x3ff, RACE_FLAG_POSITION_ONSCREEN);
			return;
		}
	}
	else if (sdata->RaceFlag_AnimationType != RACE_FLAG_TRANSITION_OFFSCREEN &&
	         !((sdata->mainGameState == 1 || sdata->mainGameState == 2) && GAME_TRACKER->levelID != MAIN_MENU_LEVEL))
	{
		s_raceFlagLoadingFadeActive = 0;
	}
	if (g_config.loadingScreen == LOADING_SCREEN_FADE && s_raceFlagLoadingFadeActive)
	{
		// Advance the transition, but always draw the fade in front of the UI.
		RaceFlag_GetOT();
		RaceFlag_DrawFade(GAME_TRACKER->otSwapchainDB[GAME_TRACKER->swapchainIndex], sdata->RaceFlag_Position);
		// The main menu uses the flag as its background.
		if (RACE_FLAG_LOADING_STAGE == LOAD_REQUESTED && (GAME_TRACKER->gameMode1 & MAIN_MENU) != 0)
		{
			RaceFlag_DrawFlag(GAME_TRACKER->pushBuffer[0].ptrOT + 0x3ff, RACE_FLAG_POSITION_ONSCREEN);
		}
		if (RACE_FLAG_LOADING_STAGE == LOAD_IDLE && RaceFlag_IsFullyOffScreen())
		{
			s_raceFlagLoadingFadeActive = 0;
		}
		return;
	}
	if (sdata->RaceFlag_LoadingTextAnimFrame < 0)
	{
		s32 stage = RACE_FLAG_LOADING_STAGE;
		b32 tooEarly = stage < 6;

		if ((!tooEarly) && (stage < 8))
		{
			sdata->RaceFlag_LoadingTextAnimFrame = 0;
		}
		if (sdata->RaceFlag_LoadingTextAnimFrame < 0)
		{
			goto SkipText;
		}
	}
	RaceFlag_DrawLoadingString();
SkipText:
	*((volatile s32 *)(&sdata->RaceFlag_CopyLoadStage)) = RACE_FLAG_LOADING_STAGE;

	ot = RaceFlag_GetOT();
	RaceFlag_DrawFlag(ot, sdata->RaceFlag_Position);
}
