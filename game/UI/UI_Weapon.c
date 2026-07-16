#include <common.h>

enum UIWeaponConstants
{
	UI_WEAPON_ITEM_TURBO = HELD_ITEM_TURBO,
	UI_WEAPON_ITEM_BOMB = HELD_ITEM_BOMB_1X,
	UI_WEAPON_ITEM_TNT = HELD_ITEM_TNT,
	UI_WEAPON_ITEM_POTION = HELD_ITEM_POTION,
	UI_WEAPON_ITEM_SPRING = HELD_ITEM_SPRING,
	UI_WEAPON_ITEM_SHIELD = HELD_ITEM_SHIELD,
	UI_WEAPON_ITEM_MASK = HELD_ITEM_MASK,
	UI_WEAPON_ITEM_CLOCK = HELD_ITEM_CLOCK,
	UI_WEAPON_ITEM_WARPBALL = HELD_ITEM_WARPBALL,
	UI_WEAPON_ITEM_NONE = HELD_ITEM_NONE,
	UI_WEAPON_ITEM_ROULETTE = HELD_ITEM_ROULETTE,
	UI_WEAPON_ICON_BASE = 5,
	UI_WEAPON_JUICED_ICON_BASE = 0x11,
	UI_WEAPON_MASK_GOOD_CHARACTER_BITS = 0xc9,
	UI_WEAPON_MASK_UKA_ICON = 0x32,
	UI_WEAPON_ROULETTE_RACING_COUNT = 0xc,
	UI_WEAPON_ROULETTE_BATTLE_COUNT = 0xe,
	UI_WEAPON_QUANTITY_FONT = 2,
	UI_WEAPON_QUANTITY_FLAGS = 4,
	UI_WEAPON_ROULETTE_LERP_FRAMES = 5,
	UI_WEAPON_BG_SHINE_THETA_STEP = 0x100,
	UI_WEAPON_BG_SHINE_SCALE_MUL = 0xd000,
	UI_WEAPON_BG_SHINE_SCALE_SHIFT = 0x10,
	UI_WEAPON_BG_SHINE_ICON_INDEX = 0x31,
	UI_WEAPON_BG_SHINE_COUNT = 2,
	UI_WEAPON_BG_SHINE_TRANSPARENCY_BASE = 2,
};

CTR_STATIC_ASSERT(UI_WEAPON_ITEM_TURBO == 0);
CTR_STATIC_ASSERT(UI_WEAPON_ITEM_BOMB == 1);
CTR_STATIC_ASSERT(UI_WEAPON_ITEM_TNT == 3);
CTR_STATIC_ASSERT(UI_WEAPON_ITEM_POTION == 4);
CTR_STATIC_ASSERT(UI_WEAPON_ITEM_SPRING == 5);
CTR_STATIC_ASSERT(UI_WEAPON_ITEM_SHIELD == 6);
CTR_STATIC_ASSERT(UI_WEAPON_ITEM_MASK == 7);
CTR_STATIC_ASSERT(UI_WEAPON_ITEM_CLOCK == 8);
CTR_STATIC_ASSERT(UI_WEAPON_ITEM_WARPBALL == 9);
CTR_STATIC_ASSERT(UI_WEAPON_ITEM_NONE == 0xf);
CTR_STATIC_ASSERT(UI_WEAPON_ITEM_ROULETTE == 0x10);
CTR_STATIC_ASSERT(UI_WEAPON_ICON_BASE == 5);
CTR_STATIC_ASSERT(UI_WEAPON_JUICED_ICON_BASE == 0x11);
CTR_STATIC_ASSERT(UI_WEAPON_MASK_GOOD_CHARACTER_BITS == 0xc9);
CTR_STATIC_ASSERT(UI_WEAPON_MASK_UKA_ICON == 0x32);
CTR_STATIC_ASSERT(UI_WEAPON_ROULETTE_RACING_COUNT == 0xc);
CTR_STATIC_ASSERT(UI_WEAPON_ROULETTE_BATTLE_COUNT == 0xe);
CTR_STATIC_ASSERT(UI_WEAPON_QUANTITY_FONT == 2);
CTR_STATIC_ASSERT(UI_WEAPON_QUANTITY_FLAGS == 4);
CTR_STATIC_ASSERT(UI_WEAPON_ROULETTE_LERP_FRAMES == 5);
CTR_STATIC_ASSERT(UI_WEAPON_BG_SHINE_THETA_STEP == 0x100);
CTR_STATIC_ASSERT(UI_WEAPON_BG_SHINE_SCALE_MUL == 0xd000);
CTR_STATIC_ASSERT(UI_WEAPON_BG_SHINE_SCALE_SHIFT == 0x10);
CTR_STATIC_ASSERT(UI_WEAPON_BG_SHINE_ICON_INDEX == 0x31);
CTR_STATIC_ASSERT(UI_WEAPON_BG_SHINE_COUNT == 2);
CTR_STATIC_ASSERT(UI_WEAPON_BG_SHINE_TRANSPARENCY_BASE == 2);

static const u32 UI_WEAPON_BG_SHINE_COLOR = 0xff0000u;

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800507e0-0x80050af8.
// Draw weapon and wumpa fruit in HUD
void UI_Weapon_DrawSelf(s16 posX, s16 posY, s16 scale, struct Driver *d)

{
	u32 characterID;
	int itemID;
	int iconID;
	SVec2 pos;

	struct GameTracker *gGT = sdata->gGT;
	itemID = d->heldItemID;

	// If you do have "no weapon icon"
	if (itemID == UI_WEAPON_ITEM_NONE)
	{
		return;
	}

	// If you are not shuffling through weapon roulette
	if (itemID != UI_WEAPON_ITEM_ROULETTE)
	{
		iconID = itemID + UI_WEAPON_ICON_BASE;
		sdata->s_spacebar[0] = d->numHeldItems + '0';

		// character ID
		characterID = data.characterIDs[d->driverID];

		// if mask item
		if (itemID == UI_WEAPON_ITEM_MASK)
		{
			// Crash, Coco, Pura, Polar, NO Penta
			u32 goodMaskCharacterBits = UI_WEAPON_MASK_GOOD_CHARACTER_BITS;

			// This is a bad guy, change icon to Uka
			if (((goodMaskCharacterBits >> characterID) & 1) == 0)
			{
				iconID = UI_WEAPON_MASK_UKA_ICON;
			}
		}

		if ((d->numWumpas >= DRIVER_WUMPA_JUICED_COUNT) &&

		    // TNT, Potion, Shield
		    (((u32)(itemID - UI_WEAPON_ITEM_TNT) < 2) || (itemID == UI_WEAPON_ITEM_SHIELD)))
		{
			iconID = itemID + UI_WEAPON_JUICED_ICON_BASE;
		}

		// make weapon flicker
		if (((d->noItemTimer) != 0) && ((gGT->timer & 1) == 0))
		{
			return;
		}

		// If this weapon has a quantity (3 missiles)
		if (d->numHeldItems != 0)
		{
			// Draw the number near the weapon icon to show how many
			DecalFont_DrawLine(sdata->s_spacebar, (int)posX, (int)posY, UI_WEAPON_QUANTITY_FONT, UI_WEAPON_QUANTITY_FLAGS);
		}
	}

	// if roulette shuffle
	else
	{
		itemID = 0;
		pos.x = posX;
		pos.y = posY;

		// If game is not paused
		if ((gGT->gameMode1 & PAUSE_ALL) == 0)
		{
			// random item
			itemID = rand();

			// If you're not in Battle Mode
			if ((gGT->gameMode1 & BATTLE_MODE) == 0)
			{
				itemID = itemID % UI_WEAPON_ROULETTE_RACING_COUNT;

				// replace spring with turbo
				if (itemID == UI_WEAPON_ITEM_SPRING)
				{
					goto LAB_800508ec;
				}
			}

			// if Battle Mode
			else
			{
				itemID = itemID % UI_WEAPON_ROULETTE_BATTLE_COUNT;

				// replace spring
				if (itemID == UI_WEAPON_ITEM_SPRING)
				{
				LAB_800508ec:
					itemID = UI_WEAPON_ITEM_TURBO;
				}

				// replace clock
				else if (itemID == UI_WEAPON_ITEM_CLOCK)
				{
					itemID = UI_WEAPON_ITEM_BOMB;
				}

				// replace warpball
				else if (itemID == UI_WEAPON_ITEM_WARPBALL)
				{
					itemID = UI_WEAPON_ITEM_TNT;
				}
			}

			// only change icon once per 2 frames,
			// take advantage of unused padding
		}

		// if timer is not finished
		if (d->PickupTimeboxHUD.cooldown != 0)
		{
			UI_Lerp2D_HUD(pos.v, d->PickupTimeboxHUD.startX, d->PickupTimeboxHUD.startY, (int)posX, (int)posY, d->PickupTimeboxHUD.cooldown,
			              UI_WEAPON_ROULETTE_LERP_FRAMES);

			// subtract one from timer
			d->PickupTimeboxHUD.cooldown--;
		}

		iconID = itemID + UI_WEAPON_ICON_BASE;

		posX = pos.x;
		posY = pos.y;
	}

	DecalHUD_DrawWeapon(
	    // pointer to icon, from array of icon pointers
	    gGT->ptrIcons[iconID],

	    (int)posX, (int)posY,

	    // PrimMem
	    &gGT->backBuffer->primMem,

	    // OTMem
	    gGT->pushBuffer_UI.ptrOT,

	    TRANS_50_DECAL, (int)scale, 1);

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050af8-0x80050c20.
void UI_Weapon_DrawBG(s16 posX, s16 posY, s16 scale, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;

	// reduce frame timer until it hits zero (unused?)
	if (d->BattleHUD.juicedUpCooldown != 0)
	{
		d->BattleHUD.juicedUpCooldown--;
	}

	int scaleInt = (int)scale;

	// wumpaShineTheta (given to sine)
	sdata->wumpaShineTheta += UI_WEAPON_BG_SHINE_THETA_STEP;

	int shineScale = scaleInt * UI_WEAPON_BG_SHINE_SCALE_MUL >> UI_WEAPON_BG_SHINE_SCALE_SHIFT;

	for (int i = 0; i < UI_WEAPON_BG_SHINE_COUNT; i++)
	{
		UI_WeaponBG_DrawShine(

		    // Weapon Roulette background (shine)
		    gGT->ptrIcons[UI_WEAPON_BG_SHINE_ICON_INDEX],

		    (int)posX, (int)posY,

		    // pointer to PrimMem struct
		    &gGT->backBuffer->primMem,

		    // pointer to OTMem
		    gGT->pushBuffer[d->driverID].ptrOT,

		    UI_WEAPON_BG_SHINE_TRANSPARENCY_BASE + i, scaleInt, shineScale, UI_WEAPON_BG_SHINE_COLOR);
	}

	return;
}
