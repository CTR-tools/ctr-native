#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002406c-0x800242b8.
void DotLights_Video(struct GameTracker *gGT, int red1, int red2, int red3, int green, int posY)
{
	struct Icon *icon;
	struct PushBuffer *pb;
	int iconState[4];
	int newPosX;
	int newPosY;
	int playerIndex;
	int lightIndex;
	int scale;
	int sizeX;

	if (gGT->numPlyrCurrGame == 0)
	{
		return;
	}

	iconState[0] = red1;
	iconState[1] = red2;
	iconState[2] = red3;
	iconState[3] = green;

	for (playerIndex = 0; playerIndex < gGT->numPlyrCurrGame; playerIndex++)
	{
		pb = &gGT->pushBuffer[playerIndex];

		scale = FP(0.5);
		if (gGT->numPlyrCurrGame == 1)
		{
			scale = FP(1.0);
		}
		else if (gGT->numPlyrCurrGame == 2)
		{
			scale = 0xaaa;
		}

		icon = gGT->trafficLightIcon[0];
		sizeX = FP_Mult(icon->texLayout.u1 - icon->texLayout.u0, scale);

		newPosX = (pb->rect.w - (sizeX * 4)) / 2;
		newPosY = FP_Mult(pb->rect.h / 3, posY) - FP_Mult(icon->texLayout.v2 - icon->texLayout.v0, scale);

		for (lightIndex = 0; lightIndex < 4; lightIndex++)
		{
			DecalHUD_DrawPolyFT4(gGT->trafficLightIcon[iconState[lightIndex] + (2 * (lightIndex == 3))], newPosX + (sizeX * lightIndex), newPosY,
			                     &gGT->backBuffer->primMem, pb->ptrOT, 0, scale);
		}
	}
}


static int DotLights_TweenPos(int value)
{
	int quotient = (int)((s64)((s64)value * -0x77777777) >> 0x20);
	return ((quotient + value) >> 9) - (value >> 0x1f);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800242b8-0x80024464.
void DotLights_AudioAndVideo(struct GameTracker *gGT)
{
	int timer = gGT->trafficLightsTimer;
	int red1;
	int red2;
	int red3;
	int green;
	int posY;

	if (timer >= -0x3bf)
	{
		if (timer < 1)
		{
			if (sdata->trafficLightsTimer_prevFrame > 0)
			{
				OtherFX_Play(0x46, 0);
			}

			red1 = 1;
			red2 = 1;
			red3 = 1;
			green = 1;
			posY = DotLights_TweenPos((gGT->trafficLightsTimer + 0x3c0) * 0x1000);
		}
		else if (timer < 0x3c1)
		{
			if (sdata->trafficLightsTimer_prevFrame > 0x3c0)
			{
				OtherFX_Play(0x45, 0);
			}

			red1 = 1;
			red2 = 1;
			red3 = 1;
			green = 0;
			posY = 0x1000;
		}
		else if (timer < 0x781)
		{
			if (sdata->trafficLightsTimer_prevFrame > 0x780)
			{
				OtherFX_Play(0x45, 0);
			}

			red1 = 1;
			red2 = 1;
			red3 = 0;
			green = 0;
			posY = 0x1000;
		}
		else if (timer < 0xb41)
		{
			if (sdata->trafficLightsTimer_prevFrame > 0xb40)
			{
				OtherFX_Play(0x45, 0);
			}

			red1 = 1;
			red2 = 0;
			red3 = 0;
			green = 0;
			posY = 0x1000;
		}
		else
		{
			red1 = 0;
			red2 = 0;
			red3 = 0;
			green = 0;
			posY = DotLights_TweenPos((0xf00 - gGT->trafficLightsTimer) * 0x1000);
		}

		DotLights_Video(gGT, red1, red2, red3, green, posY);
	}

	sdata->trafficLightsTimer_prevFrame = gGT->trafficLightsTimer;
}
