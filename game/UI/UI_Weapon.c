#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800507e0-0x80050af8.
// Draw weapon and wumpa fruit in HUD
void UI_Weapon_DrawSelf(s16 posX, s16 posY, s16 scale, struct Driver *d)

{
	u32 currChar;
	int itemID;
	int iconID;
	struct GameTracker *gGT;
	SVec2 pos;

	// beat 7360

	gGT = sdata->gGT;
	itemID = d->heldItemID;

	// If you do have "no weapon icon"
	if (itemID == 0xf)
	{
		return;
	}

	// If you are not shuffling through weapon roulette
	if (itemID != 0x10)
	{
		iconID = itemID + 5;

		// character ID
		currChar = data.characterIDs[d->driverID];

		// if mask item
		if (itemID == 7)
		{
			// Crash, Coco, Pura, Polar, NO Penta
			u32 maskBits = 0xc9;

			// This is a bad guy, change icon to Uka
			if (((maskBits >> currChar) & 1) == 0)
			{
				iconID = 0x32;
			}
		}

		if ((d->numWumpas >= 10) &&

		    // TNT, Potion, Shield
		    (((u32)(itemID - 3) < 2) || (itemID == 6)))
		{
			iconID = itemID + 0x11;
		}

		// make weapon flicker
		if (((d->noItemTimer) != 0) && ((gGT->timer & 1) == 0))
		{
			return;
		}

		// If this weapon has a quantity (3 missiles)
		if (d->numHeldItems != 0)
		{
			// Get the ascii character to represent the quantity
			// of weapon that you have (3 missiles)
			sdata->s_spacebar[0] = d->numHeldItems + '0';

			// Draw the number near the weapon icon to show how many
			DecalFont_DrawLine(sdata->s_spacebar, (int)posX, (int)posY, 2, 4);
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
				itemID = itemID % 0xc;

				// replace spring with turbo
				if (itemID == 5)
				{
					goto LAB_800508ec;
				}
			}

			// if Battle Mode
			else
			{
				itemID = itemID % 0xe;

				// replace spring
				if (itemID == 5)
				{
				LAB_800508ec:
					itemID = 0;
				}

				// replace clock
				else if (itemID == 8)
				{
					itemID = 1;
				}

				// replace warpball
				else if (itemID == 9)
				{
					itemID = 3;
				}
			}

			// only change icon once per 2 frames,
			// take advantage of unused padding
		}

		// if timer is not finished
		if (d->PickupTimeboxHUD.cooldown != 0)
		{
			UI_Lerp2D_HUD(pos.v, d->PickupTimeboxHUD.startX, d->PickupTimeboxHUD.startY, (int)posX, (int)posY, d->PickupTimeboxHUD.cooldown, 5);

			// subtract one from timer
			d->PickupTimeboxHUD.cooldown--;
		}

		iconID = itemID + 5;

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
void UI_Weapon_DrawBG(s16 param_1, s16 param_2, s16 param_3, struct Driver *d)
{
	int iVar1;
	int iVar2;
	int i;
	struct GameTracker *gGT = sdata->gGT;

	// reduce frame timer until it hits zero (unused?)
	if (d->BattleHUD.juicedUpCooldown != 0)
	{
		d->BattleHUD.juicedUpCooldown--;
	}

	iVar2 = (int)param_3;

	// wumpaShineTheta (given to sine)
	sdata->wumpaShineTheta += 0x100;

	iVar1 = iVar2 * 0xd000 >> 0x10;

	for (i = 0; i < 2; i++)
	{
		UI_WeaponBG_DrawShine(

		    // Weapon Roulette background (shine)
		    gGT->ptrIcons[0xc4 / 4],

		    (int)param_1, (int)param_2,

		    // pointer to PrimMem struct
		    &gGT->backBuffer->primMem,

		    // pointer to OTMem
		    gGT->pushBuffer[d->driverID].ptrOT,

		    2 + i, iVar2, iVar1, 0xff0000);
	}

	return;
}
