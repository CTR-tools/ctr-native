#include <common.h>
#include <widescreen.h>

enum
{
	DOT_LIGHT_COUNT = 4,
	DOT_LIGHT_GREEN_INDEX = 3,

	DOT_LIGHT_SCALE_1P = FP(1.0),
	DOT_LIGHT_SCALE_2P = 0xaaa,
	DOT_LIGHT_SCALE_3P4P = FP(0.5),

	DOT_LIGHT_GREEN_ICON_OFFSET = 2,

	DOT_LIGHT_TIMER_SHOW_MIN = -0x3bf,
	DOT_LIGHT_TIMER_GREEN = 0,
	DOT_LIGHT_TIMER_RED3 = 0x3c0,
	DOT_LIGHT_TIMER_RED2 = 0x780,
	DOT_LIGHT_TIMER_RED1 = 0xb40,
	DOT_LIGHT_TIMER_HIDE = 0xf00,

	DOT_LIGHT_TWEEN_RECIPROCAL_960 = -0x77777777,
	DOT_LIGHT_TWEEN_SHIFT = 9,

	DOT_LIGHT_FX_COUNTDOWN = 0x45,
	DOT_LIGHT_FX_GREEN = 0x46,
	DOT_LIGHT_FX_VOL = 0,
};

static const s32 s_dotLightThresholds[DOT_LIGHT_COUNT] = {
	DOT_LIGHT_TIMER_RED1, DOT_LIGHT_TIMER_RED2, DOT_LIGHT_TIMER_RED3, DOT_LIGHT_TIMER_GREEN,
};

static int DotLights_Scale(const struct GameTracker *gGT)
{
	if (gGT->numPlyrCurrGame == 1)
	{
		return DOT_LIGHT_SCALE_1P;
	}
	return gGT->numPlyrCurrGame == 2 ? DOT_LIGHT_SCALE_2P : DOT_LIGHT_SCALE_3P4P;
}

static void DotLights_Video(struct GameTracker *gGT, s32 timer, u32 players)
{
	if (gGT->numPlyrCurrGame == 0)
	{
		return;
	}

	const int scale = DotLights_Scale(gGT);
	for (s32 playerIndex = 0; playerIndex < gGT->numPlyrCurrGame; playerIndex++)
	{
		if ((players & (1u << playerIndex)) == 0)
		{
			continue;
		}
		struct PushBuffer *pb = &gGT->pushBuffer[playerIndex];

		RECT bounds;
		if (!DotLights_GetBounds(gGT, playerIndex, timer, &bounds))
		{
			continue;
		}
		const int sizeX = bounds.w / DOT_LIGHT_COUNT;
		const int rawWidth = FP_Mult(gGT->trafficLightIcon[0]->texLayout.u1 - gGT->trafficLightIcon[0]->texLayout.u0, scale);
		const int newPosX = bounds.x - pb->rect.x - Widescreen_CenteredEdgeShift(rawWidth);
		const int newPosY = bounds.y - pb->rect.y;

		for (s32 lightIndex = 0; lightIndex < DOT_LIGHT_COUNT; lightIndex++)
		{
			const int iconIndex = (timer <= s_dotLightThresholds[lightIndex]) +
			    DOT_LIGHT_GREEN_ICON_OFFSET * (lightIndex == DOT_LIGHT_GREEN_INDEX);
			DecalHUD_DrawPolyFT4(gGT->trafficLightIcon[iconIndex], newPosX + (sizeX * lightIndex), newPosY, &gGT->backBuffer->primMem, pb->ptrOT, 0, scale);
		}
	}
}


static s32 DotLights_TweenPos(s32 value)
{
	s32 quotient = (s32)((s64)((s64)value * DOT_LIGHT_TWEEN_RECIPROCAL_960) >> 0x20);
	return ((quotient + value) >> DOT_LIGHT_TWEEN_SHIFT) - (value >> 0x1f);
}

bool DotLights_GetBounds(struct GameTracker *gGT, int player, s32 timer, RECT *bounds)
{
	if (timer < DOT_LIGHT_TIMER_SHOW_MIN)
	{
		return false;
	}
	const struct PushBuffer *pb = &gGT->pushBuffer[player];
	const struct Icon *icon = gGT->trafficLightIcon[0];
	const int scale = DotLights_Scale(gGT);
	int posY = FP(1.0);
	if (timer <= DOT_LIGHT_TIMER_GREEN)
	{
		posY = DotLights_TweenPos((timer + DOT_LIGHT_TIMER_RED3) * FP(1.0));
	}
	else if (timer > DOT_LIGHT_TIMER_RED1)
	{
		posY = DotLights_TweenPos((DOT_LIGHT_TIMER_HIDE - timer) * FP(1.0));
	}
	const int width = FP_Mult(icon->texLayout.u1 - icon->texLayout.u0, scale);
	bounds->w = (width - 2 * Widescreen_CenteredEdgeShift(width)) * DOT_LIGHT_COUNT;
	bounds->h = FP_Mult(icon->texLayout.v2 - icon->texLayout.v0, scale);
	bounds->x = pb->rect.x + (pb->rect.w - bounds->w) / 2;
	bounds->y = pb->rect.y + FP_Mult(pb->rect.h / 3, posY) - bounds->h;
	return bounds->h > 0;
}

void DotLights_Sequence(struct GameTracker *gGT, s32 timer, s32 previous, u32 players)
{
	if (timer < DOT_LIGHT_TIMER_SHOW_MIN)
	{
		return;
	}
	// At most one cue per frame, even when a long frame skips several lights.
	for (int light = DOT_LIGHT_GREEN_INDEX; light >= 0; light--)
	{
		if (timer <= s_dotLightThresholds[light])
		{
			if (previous > s_dotLightThresholds[light])
			{
				OtherFX_Play(light == DOT_LIGHT_GREEN_INDEX ? DOT_LIGHT_FX_GREEN : DOT_LIGHT_FX_COUNTDOWN, DOT_LIGHT_FX_VOL);
			}
			break;
		}
	}
	DotLights_Video(gGT, timer, players);
}

void DotLights_AudioAndVideo(struct GameTracker *gGT)
{
	DotLights_Sequence(gGT, gGT->trafficLightsTimer, sdata->trafficLightsTimer_prevFrame, UINT32_MAX);
	sdata->trafficLightsTimer_prevFrame = gGT->trafficLightsTimer;
}
