#include <common.h>

enum
{
	DOT_LIGHT_COUNT = 4,

	DOT_LIGHT_SCALE_1P = FP(1.0),
	DOT_LIGHT_SCALE_2P = 0xaaa,
	DOT_LIGHT_SCALE_3P4P = FP(0.5),

	DOT_LIGHT_STATE_OFF = 0,
	DOT_LIGHT_STATE_ON = 1,

	DOT_LIGHT_TIMER_SHOW_MIN = -0x3bf,
	DOT_LIGHT_TIMER_GREEN = 0,
	DOT_LIGHT_TIMER_RED3 = 0x3c0,
	DOT_LIGHT_TIMER_RED2 = 0x780,
	DOT_LIGHT_TIMER_RED1 = 0xb40,
	DOT_LIGHT_TIMER_HIDE = 0xf00,

	DOT_LIGHT_FX_COUNTDOWN = 0x45,
	DOT_LIGHT_FX_GREEN = 0x46,
	DOT_LIGHT_FX_FLAGS = 0,
};


void DotLights_Video(struct GameTracker *gGT, s32 red1, s32 red2, s32 red3, s32 green, s32 posY)
{
	s32 playerIndex;
	for (playerIndex = 0; playerIndex < gGT->numPlyrCurrGame; playerIndex++)
	{
		struct PushBuffer *pb = &gGT->pushBuffer[playerIndex];
		struct Icon *icon;
		s32 scale;
		s32 sizeX;
		s32 newPosX;
		s32 newPosY;
		// NOTE(aalhendi): Scale follows the global count; viewports and primitives belong to the passed tracker.
		if (GAME_TRACKER->numPlyrCurrGame < 3)
		{
			if (GAME_TRACKER->numPlyrCurrGame == 1)
			{
				scale = DOT_LIGHT_SCALE_1P;
			}
			else
			{
				scale = DOT_LIGHT_SCALE_2P;
			}
		}
		else
		{
			scale = DOT_LIGHT_SCALE_3P4P;
		}
		icon = gGT->trafficLights.all[0];
		sizeX = FP_Mult(icon->texLayout.u1 - icon->texLayout.u0, scale);
		newPosX = (pb->rect.w - sizeX * DOT_LIGHT_COUNT) / 2;
		newPosY = -FP_Mult(icon->texLayout.v2 - icon->texLayout.v0, scale) + FP_Mult((s16)(pb->rect.h / 3), posY);
		// NOTE(aalhendi): Separate calls retain live icon, back-buffer and OT reads between lights.
		DecalHUD_DrawPolyFT4(gGT->trafficLights.color.red[red1], newPosX, newPosY, &gGT->backBuffer->primMem, pb->ptrOT, 0, scale);
		DecalHUD_DrawPolyFT4(gGT->trafficLights.color.red[red2], newPosX + sizeX, newPosY, &gGT->backBuffer->primMem, pb->ptrOT, 0, scale);
		DecalHUD_DrawPolyFT4(gGT->trafficLights.color.red[red3], newPosX + sizeX * 2, newPosY, &gGT->backBuffer->primMem, pb->ptrOT, 0, scale);
		DecalHUD_DrawPolyFT4(gGT->trafficLights.color.green[green], newPosX + sizeX * 3, newPosY, &gGT->backBuffer->primMem, pb->ptrOT, 0, scale);
	}
}

void DotLights_AudioAndVideo(struct GameTracker *gGT)
{
	// NOTE(aalhendi): Countdown uses the global tracker; final publication rereads the passed tracker after drawing.
	struct GameTracker *tracker = GAME_TRACKER;
	s32 timer = tracker->trafficLightsTimer;
	if (timer >= DOT_LIGHT_TIMER_SHOW_MIN)
	{
		if (timer <= DOT_LIGHT_TIMER_GREEN)
		{
			if (GAME_TRAFFIC_LIGHT_PREVIOUS_TIMER > DOT_LIGHT_TIMER_GREEN)
			{
				OtherFX_Play(DOT_LIGHT_FX_GREEN, DOT_LIGHT_FX_FLAGS);
			}
			DotLights_Video(gGT, DOT_LIGHT_STATE_ON, DOT_LIGHT_STATE_ON, DOT_LIGHT_STATE_ON, DOT_LIGHT_STATE_ON,
			                (s32)(((u32)GAME_TRACKER->trafficLightsTimer + DOT_LIGHT_TIMER_RED3) << FRACTIONAL_BITS) / DOT_LIGHT_TIMER_RED3);
		}
		else if (timer <= DOT_LIGHT_TIMER_RED3)
		{
			if (GAME_TRAFFIC_LIGHT_PREVIOUS_TIMER > DOT_LIGHT_TIMER_RED3)
			{
				OtherFX_Play(DOT_LIGHT_FX_COUNTDOWN, DOT_LIGHT_FX_FLAGS);
			}
			DotLights_Video(gGT, DOT_LIGHT_STATE_ON, DOT_LIGHT_STATE_ON, DOT_LIGHT_STATE_ON, DOT_LIGHT_STATE_OFF, FP(1.0));
		}
		else if (timer <= DOT_LIGHT_TIMER_RED2)
		{
			if (GAME_TRAFFIC_LIGHT_PREVIOUS_TIMER > DOT_LIGHT_TIMER_RED2)
			{
				OtherFX_Play(DOT_LIGHT_FX_COUNTDOWN, DOT_LIGHT_FX_FLAGS);
			}
			DotLights_Video(gGT, DOT_LIGHT_STATE_ON, DOT_LIGHT_STATE_ON, DOT_LIGHT_STATE_OFF, DOT_LIGHT_STATE_OFF, FP(1.0));
		}
		else if (timer <= DOT_LIGHT_TIMER_RED1)
		{
			if (GAME_TRAFFIC_LIGHT_PREVIOUS_TIMER > DOT_LIGHT_TIMER_RED1)
			{
				OtherFX_Play(DOT_LIGHT_FX_COUNTDOWN, DOT_LIGHT_FX_FLAGS);
			}
			DotLights_Video(gGT, DOT_LIGHT_STATE_ON, DOT_LIGHT_STATE_OFF, DOT_LIGHT_STATE_OFF, DOT_LIGHT_STATE_OFF, FP(1.0));
		}
		else
		{
			DotLights_Video(gGT, DOT_LIGHT_STATE_OFF, DOT_LIGHT_STATE_OFF, DOT_LIGHT_STATE_OFF, DOT_LIGHT_STATE_OFF,
			                (s32)(((u32)DOT_LIGHT_TIMER_HIDE - (u32)tracker->trafficLightsTimer) << FRACTIONAL_BITS) / DOT_LIGHT_TIMER_RED3);
		}
	}
	GAME_TRAFFIC_LIGHT_PREVIOUS_TIMER = gGT->trafficLightsTimer;
}
