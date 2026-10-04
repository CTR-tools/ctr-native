#include <common.h>

enum UIWeaponDrawSelfConstants
{
	UI_WEAPON_ICON_BASE = 5,
	UI_WEAPON_JUICED_ICON_BASE = 0x11,
	UI_WEAPON_MASK_UKA_ICON = 0x32,
	UI_WEAPON_ROULETTE_RACING_COUNT = 0xc,
	UI_WEAPON_ROULETTE_BATTLE_COUNT = 0xe,
	UI_WEAPON_ROULETTE_LERP_FRAMES = 5,
};

void UI_Weapon_DrawSelf(s16 posX, s16 posY, s16 scale, struct Driver *d)
{
	s32 iconID;
	SVec2 position;

	if (d->heldItemID == HELD_ITEM_ROULETTE)
	{
		position.x = posX;
		position.y = posY;
		iconID = HELD_ITEM_TURBO;
		if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) == 0)
		{
			if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) != 0)
			{
				iconID = rand() % UI_WEAPON_ROULETTE_BATTLE_COUNT;
				if (iconID == HELD_ITEM_SPRING)
				{
					iconID = HELD_ITEM_TURBO;
				}
				else if (iconID == HELD_ITEM_CLOCK)
				{
					iconID = HELD_ITEM_BOMB_1X;
				}
				else if (iconID == HELD_ITEM_WARPBALL)
				{
					iconID = HELD_ITEM_TNT;
				}
			}
			else
			{
				iconID = rand() % UI_WEAPON_ROULETTE_RACING_COUNT;
				if (iconID == HELD_ITEM_SPRING)
				{
					iconID = HELD_ITEM_TURBO;
				}
			}
		}
		if (d->PickupTimeboxHUD.cooldown != 0)
		{
			UI_Lerp2D_HUD(CTR_VECTOR_DATA(&position), d->PickupTimeboxHUD.startX, d->PickupTimeboxHUD.startY, posX, posY, d->PickupTimeboxHUD.cooldown,
			              UI_WEAPON_ROULETTE_LERP_FRAMES);
			// NOTE(aalhendi): Retail decrements the full word with wrapping arithmetic.
			d->PickupTimeboxHUD.cooldown = (s32)((u32)d->PickupTimeboxHUD.cooldown - 1u);
		}
		DecalHUD_DrawWeapon(GAME_TRACKER->ptrIcons[iconID + UI_WEAPON_ICON_BASE], position.x, position.y, &GAME_TRACKER->backBuffer->primMem,
		                    GAME_TRACKER->pushBuffer_UI.ptrOT, TRANS_50_DECAL, scale, 1);
	}
	else
	{
		char *quantityText;
		if (d->heldItemID == HELD_ITEM_NONE)
		{
			return;
		}
		iconID = d->heldItemID + UI_WEAPON_ICON_BASE;
		// NOTE(aalhendi): Publish the quantity byte even when this frame flickers off.
		GAME_SDATA_NAME(s_spacebar)[0] = d->numHeldItems + '0';
		quantityText = GAME_SDATA_NAME(s_spacebar);
		if (d->heldItemID == HELD_ITEM_MASK)
		{
			s32 characterID = GAME_CHARACTER_IDS[d->driverID];

			// Only Crash, Coco, Polar and Pura use the Aku Aku icon; Penta uses Uka Uka.
			if (characterID != CRASH_BANDICOOT && characterID != COCO_BANDICOOT && characterID != POLAR && characterID != PURA)
			{
				iconID = UI_WEAPON_MASK_UKA_ICON;
			}
		}
		if (d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT &&
		    (d->heldItemID == HELD_ITEM_TNT || d->heldItemID == HELD_ITEM_POTION || d->heldItemID == HELD_ITEM_SHIELD))
		{
			iconID = d->heldItemID + UI_WEAPON_JUICED_ICON_BASE;
		}
		if (d->noItemTimer != 0 && (GAME_TRACKER->timer & 1) == 0)
		{
			return;
		}
		if (d->numHeldItems != 0)
		{
			DecalFont_DrawLine(quantityText, posX, posY, FONT_SMALL, WHITE);
		}
		DecalHUD_DrawWeapon(GAME_TRACKER->ptrIcons[iconID], posX, posY, &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT, TRANS_50_DECAL,
		                    scale, 1);
	}
}
