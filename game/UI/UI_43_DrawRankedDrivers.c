#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800524c4-0x80052f98.
// DriverIndex_GetDamageColor factors the duplicated inline color-timer logic.

// Draw player icons on left side of screen
// in Arcade mode and Boss mode, and draws
// icons in multiplayer on the midY axis (and warpball)

int DriverIndex_GetDamageColor(int iVar14)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *d = gGT->drivers[iVar14];

	int iVar12 = d->damageColorTimer;

	// make icon white
	int local_30 = 0x808080;

	// -30 to -1
	if (iVar12 < 0)
	{
		local_30 = 0;
		int strength = (iVar12 + 30) * 4;
		local_30 += strength;
		local_30 += (0xFF - strength) * 0x100;
		local_30 += strength * 0x10000;

		// one frame closer to zero
		d->damageColorTimer += 1;
	}

	// 30 to 1
	if (iVar12 > 0)
	{
		local_30 = 0;
		int strength = (30 - iVar12) * 4;
		local_30 += (0xFF - strength);
		local_30 += strength * 0x100;
		local_30 += strength * 0x10000;

		// one frame closer to zero
		d->damageColorTimer -= 1;
	}

	return local_30;
}

void UI_DrawRankedDrivers(void)
{
	u16 uVar1;
	char bVar2;
	int iVar3;
	int iVar4;
	struct GameTracker *gGT;
	s16 txtColor;
	u32 uVar7;
	int absPosRank;
	int iVar8;
	s16 *puVar9;
	s16 numRacersFinished;
	u16 uVar11;
	int iVar12;
	s16 *psVar13;
	int iVar14;
	struct Thread *warpballThread;
	struct Instance *warpballInst;
	int iVar15;
	u16 *puVar16;
	s16 *psVar17;
	u16 uVar18;
	Point pos;
	s16 local_44;
	s16 local_40;
	s16 local_3e;
	s16 local_3c;
	s16 local_38;
	s16 local_34;
	u32 local_30;
	u32 local_2c;

	gGT = sdata->gGT;

	int numPlyr = gGT->numPlyrCurrGame;

	if (numPlyr == 1)
	{
		// Number of racers that have finished race
		numRacersFinished = 0;

		// incremented when looping through player structures

		for (iVar14 = 0; iVar14 < 8; iVar14++)
		{
			if (data.rankIconsTransitionTimer[iVar14] == 0)
			{
				// player structure + 0x482 is your rank in the race
				// 0 = 1st place, 1 = 2nd place, 2 = 3rd place, etc
				data.rankIconsDesired[iVar14] = gGT->drivers[iVar14]->driverRank;
			}

			// if player structure pointer is not nullptr
			if ((gGT->drivers[iVar14] != 0) &&

			    // if racer finished the race
			    ((gGT->drivers[iVar14]->actionsFlagSet & 0x2000000) != 0))
			{
				// count how many racers have finished
				numRacersFinished++;
			}
		}

		// Default for Arcade: Show 4 racers
		iVar14 = 4;

		// If you're in a Boss Race
		if (gGT->gameMode1 < 0)
		{
			// Show 2 racers
			iVar14 = 2;
		}

		iVar15 = 0;

		// height to draw rank (this bitshifts later)
		iVar12 = 0x380000;

		for (iVar15 = 0; iVar15 < iVar14; iVar15++)
		{
			// make the text white by default
			txtColor = 4;

			// if racer has finished the race
			if (iVar15 < numRacersFinished)
			{
				// make the text red
				txtColor = 3;
			}

			// draw rank number: '1', '2', '3', '4'
			sdata->s_spacebar[0] = (char)iVar15 + '1';
			DecalFont_DrawLine(&sdata->s_spacebar[0], 0x34, iVar12 >> 0x10, 2, txtColor);

			// add to Y, which mekes it lower on screen
			iVar12 = iVar12 + 0x1b0000;
		}

		for (iVar14 = 0; iVar14 < 8; iVar14++)
		{
			s16 *curr = &data.rankIconsCurr[iVar14];
			s16 *des = &data.rankIconsDesired[iVar14];

			if (
			    // if player structure pointer is not nullptr
			    (gGT->drivers[iVar14] != 0) &&

			    // if you haven't gotten to the last driver
			    ((*des + 1) < 9))
			{
				local_30 = DriverIndex_GetDamageColor(iVar14);

				psVar13 = &data.rankIconsTransitionTimer[iVar14];

				// placeholder
				pos.x = -100;

				// icon not transitioning
				if (*psVar13 == 0)
				{
					// get absolute pos-rank of driver
					iVar12 = *des;

					// if current == desired
					if (iVar12 == *curr)
					{
						// if top positions

						if (iVar12 < 4)
						{
							pos.x = 0x14;
							pos.y = iVar12 * 0x1b + 0x39;
						}
						else
						{
							continue;
						}
					}
				}

				s16 iconScale = FP(1);

				int isTransitioning = (pos.x == -100);

				// === Icon Transitioning ===
				if (isTransitioning)
				{
					UI_Lerp2D_Angular((s16 *)&pos, *curr, *des, *psVar13);
				}

				UI_DrawDriverIcon(

				    gGT->ptrIcons[data.MetaDataCharacters[data.characterIDs[iVar14]].iconID],

				    pos.x, pos.y, &gGT->backBuffer->primMem,

				    gGT->pushBuffer_UI.ptrOT,

				    1, iconScale, local_30);

				if (isTransitioning)
				{
					psVar13[0]++;

					if (*psVar13 >= 5)
					{
						*psVar13 = 0;
						*curr = *des;
					}
				}
			}
		}
	}

	// if this is multiplayer
	else
	{
		puVar16 = &data.rankIconsTransitionTimer[0];
		iVar15 = 0;

		int totalNumDrivers;
		totalNumDrivers = numPlyr + gGT->numBotsNextGame;

		for (iVar14 = 0; iVar14 < totalNumDrivers; iVar14++)
		{
			// puVar5 increases by 4 for each iteration

			local_30 = DriverIndex_GetDamageColor(iVar14);

			uVar1 = *puVar16;

			if (
			    // if racer is in first lap and
			    (gGT->drivers[iVar14]->lapIndex == 0) &&

			    // racer crossed the startline backwards
			    // this is when race starts and you're behind the finish line
			    ((gGT->drivers[iVar14]->actionsFlagSet & 0x1000000) != 0))
			{
			LAB_80052b00:
				// icon posX is zero,
				// dont go to end of lap on the graph
				uVar18 = 0;
			}
			else
			{
				// length of track
				iVar3 = gGT->level1->ptr_restart_points[0].distToFinish * 8;
				iVar4 = iVar3 - gGT->drivers[iVar14]->distanceToFinish_curr;
				iVar3 = iVar3 / 0x1d1;

				// divide distanceToFinish by screen width
				uVar18 = iVar4 / iVar3;
				uVar18 = (u16)uVar18;

				if ((s16)uVar18 < 0)
					goto LAB_80052b00;
			}

			// posX
			iVar4 = uVar18;
			iVar3 = iVar4 - (s16)uVar1;

			if (iVar3 < 0)
			{
				iVar3 = -iVar3;
			}
			uVar7 = iVar3 / 0xe;
			if ((uVar7 & 0xffff) == 0)
			{
				uVar7 = 1;
			}
			iVar3 = uVar1 + uVar7;
			uVar11 = (u16)iVar3;
			if ((s16)uVar1 < iVar4)
			{
				bVar2 = iVar4 < (s16)iVar3;
			}
			else
			{
				iVar3 = uVar1 - uVar7;
				uVar11 = (u16)iVar3;
				bVar2 = (s16)iVar3 < iVar4;
			}
			if (bVar2)
			{
				uVar11 = uVar18;
			}
			iVar3 = uVar18 - uVar11;
			if (iVar3 < 0)
			{
				iVar3 = -iVar3;
			}

			// 400 = 0x191
			if (400 < iVar3)
			{
				uVar11 = uVar18;
			}

			int posX = uVar11 + 5;
			int posY = 0x66;

			DecalHUD_DrawPolyGT4(gGT->ptrIcons[data.MetaDataCharacters[data.characterIDs[iVar14]].iconID], posX, posY,

			                     // pointer to PrimMem struct
			                     &gGT->backBuffer->primMem,

			                     // pointer to OT memory
			                     gGT->pushBuffer_UI.ptrOT,

			                     // color data
			                     local_30, local_30, local_30, local_30,

			                     TRANS_50_DECAL, 0x9d8); // 0x9d8 = 8/13

			*puVar16 = uVar11;
			puVar16 = puVar16 + 1;
		}

		for (warpballThread = gGT->threadBuckets[TRACKING].thread; warpballThread != 0; warpballThread = warpballThread->siblingThread)
		{
			// Get Instance from Thread
			warpballInst = warpballThread->inst;

			// if not warpball, skip
			if (warpballInst->model->id != DYNAMIC_WARPBALL)
				continue;

			// pointer to path data
			struct CheckpointNode *cn = gGT->level1->ptr_restart_points;

			struct TrackerWeapon *tw = warpballInst->thread->object;

			iVar4 = ((intptr_t)tw->ptrNodeCurr - (intptr_t)cn) / (s32)sizeof(struct CheckpointNode);
			iVar12 = 0;

			if (((u32)(gGT->level1->cnt_restart_points - 1) >= 0xff) || (iVar4 < 0))
				continue;

			int pos[4];
			pos[0] = warpballInst->matrix.t[0];
			pos[1] = warpballInst->matrix.t[1];
			pos[2] = warpballInst->matrix.t[2];

			struct CheckpointNode *cn0 = &cn[iVar4];
			struct CheckpointNode *cn1 = &cn[cn0->nextIndex_forward];
			struct CheckpointNode *cn2 = &cn[cn1->nextIndex_forward];

			s16 vec1[4];
			vec1[0] = cn1->pos[0] - cn2->pos[0];
			vec1[1] = cn1->pos[1] - cn2->pos[1];
			vec1[2] = cn1->pos[2] - cn2->pos[2];
			MATH_VectorNormalize((VECTOR *)&vec1[0]);

			s16 vec2[4];
			vec2[0] = pos[0] - cn1->pos[0];
			vec2[1] = pos[1] - cn1->pos[1];
			vec2[2] = pos[2] - cn1->pos[2];

#if defined(CTR_NATIVE)
			// NOTE(aalhendi): Retail loads vec1 as GTE rot row 0, runs MVMVA
			// on vec2, then reads MAC1. Native computes the same unshifted dot.
			s64 projected = (s64)vec1[0] * vec2[0];
			projected += (s64)vec1[1] * vec2[1];
			projected += (s64)vec1[2] * vec2[2];
			iVar15 = (s32)projected;
#else
			// replace R11R12 and R13R21
			gte_ldsvrtrow0(&vec1[0]);

			// required s16
			gte_ldv0(&vec2[0]);

			gte_mvmva(0, 0, 0, 3, 0);

			// replace stMAC1
			gte_stlvnl0(&iVar15);
#endif

			iVar3 = cn1->distToFinish * 8 + (iVar15 >> 0xc);
			iVar15 = gGT->level1->ptr_restart_points[0].distToFinish * 8;
			iVar12 = iVar3 % iVar15;
			if (iVar12 == 0)
				continue;

#if 0
	  	if (uVar1 == 0) trap(0x1c00);
	  	if ((iVar15 == -1) && (iVar3 == -0x80000000)) trap(0x1800);
#endif

			iVar15 = gGT->level1->ptr_restart_points[0].distToFinish * 8;
			iVar12 = iVar15 - iVar12;
			iVar15 = iVar15 / 0x1d1;

#if 0
	  	if (iVar15 == 0) trap(0x1c00);
	  	if ((iVar15 == -1) && (iVar12 == -0x80000000)) trap(0x1800);
#endif

			int posX = (iVar12 / iVar15) + 5;
			int posY = 0x66;

			DecalHUD_DrawWeapon(
			    // warpball icon
			    gGT->ptrIcons[0xe], posX, posY,

			    // pointer to PrimMem struct
			    &gGT->backBuffer->primMem,

			    // pointer to OT memory
			    gGT->pushBuffer_UI.ptrOT,

			    TRANS_50_DECAL, 0x8aa, 1);
		}
	}
}
