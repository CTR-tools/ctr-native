#include <common.h>

void UI_Weapon_DrawBG(s16 posX, s16 posY, s16 scale, struct Driver *d)
{
	SVec2 position;
	s32 shineScale;
	u16 nextPhase;
	s32 firstPosX;

	position.x = posX;
	position.y = posY;

	if (d->BattleHUD.juicedUpCooldown != 0)
	{
		d->BattleHUD.juicedUpCooldown = (s32)((u32)d->BattleHUD.juicedUpCooldown - 1u);
	}

	shineScale = (s32)scale * UI_WEAPON_BG_SHINE_SCALE_MUL >> UI_WEAPON_BG_SHINE_SCALE_SHIFT;

	// NOTE(aalhendi): Retail advances only the phase word's low half.
	// Sample and promote the first X coordinate before publishing that phase.
	nextPhase = *(CtrPackedU16 *)&sdata->wumpaShineTheta;
	firstPosX = position.x;
	nextPhase += UI_WEAPON_BG_SHINE_THETA_STEP;
	*(CtrPackedU16 *)&sdata->wumpaShineTheta = nextPhase;

	UI_WeaponBG_DrawShine(GAME_TRACKER->ptrIcons[UI_WEAPON_BG_SHINE_ICON_INDEX], firstPosX, position.y, &GAME_TRACKER->backBuffer->primMem,
	                      GAME_TRACKER->pushBuffer[d->driverID].ptrOT, UI_WEAPON_BG_SHINE_TRANSPARENCY_BASE, scale, (s16)shineScale,
	                      MakeColorPacked(0, 0, 0xff));

	// NOTE(aalhendi): Reload the tracker and driver ID after the first shine call.
	UI_WeaponBG_DrawShine(GAME_TRACKER->ptrIcons[UI_WEAPON_BG_SHINE_ICON_INDEX], position.x, position.y, &GAME_TRACKER->backBuffer->primMem,
	                      GAME_TRACKER->pushBuffer[d->driverID].ptrOT, UI_WEAPON_BG_SHINE_TRANSPARENCY_BASE + 1, scale, (s16)shineScale,
	                      MakeColorPacked(0, 0, 0xff));
}
