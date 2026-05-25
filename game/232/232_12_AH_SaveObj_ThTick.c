#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af3e4-0x800af7f0.
void AH_SaveObj_ThTick(struct Thread *t)
{
	s16 sVar1;
	u16 uVar2;
	int dist;
	int distX;
	int distY;
	int distZ;
	u32 uVar6;
	int iVar7;
	s16 desiredPos[3];
	s16 desiredRot[3];

	struct GameTracker *gGT;
	struct Instance *driverInst;
	struct Instance *saveInst;
	struct SaveObj *save;
	struct Driver *driver;

	gGT = sdata->gGT;

	driver = gGT->drivers[0];

	saveInst = t->inst;

	driverInst = driver->instSelf;

	// Get difference in positions between instances
	distX = saveInst->matrix.t[0] - driverInst->matrix.t[0];
	distY = saveInst->matrix.t[1] - driverInst->matrix.t[1];
	distZ = saveInst->matrix.t[2] - driverInst->matrix.t[2];

	save = t->object;

	// get distance from player instance and thread object instance
	dist = distX * distX + distY * distY + distZ * distZ;

	if ((save->flags & 1) == 0)
	{
		if ( // If game is not paused
		    ((gGT->gameMode1 & PAUSE_ALL) != 0) ||

		    // If driver is too far away from screen
		    (0x8ffff < dist))
		{
			// dont trigger menu, but play
			// sound of the load/save screen
			goto LAB_800af72c;
		}

		// If hint is not unlocked
		if ((sdata->advProgress.rewards[3] & 0x10000000) == 0)
		{
			// Aku Hint "This is the load/save screen..."
			MainFrame_RequestMaskHint(6, 0);
		}

		if ((driver->speed < 0x80) &&

		    (
		        // Check if Aku Aku is giving a hint,
		        // 0 - speaking,
		        // 1 - gone

		        uVar6 = AH_MaskHint_boolCanSpawn(),

		        // if aku is gone
		        (uVar6 & 0xffff) != 0))
		{
			uVar2 = save->scanlineFrame - 1;
			save->scanlineFrame = uVar2;

			// if scanline goes past the top
			if ((int)(uVar2 << 0x10) < 0)
			{
				// desired transition position (x,y,z)
				desiredPos[0] = gGT->level1->ptrSpawnType2_PosRot->posCoords[0] + (s16)((int)saveInst->matrix.m[0][0] * 0x19 >> 7);
				desiredPos[1] = gGT->level1->ptrSpawnType2_PosRot->posCoords[1] + (s16)((int)saveInst->matrix.m[1][0] * 0x19 >> 7);
				desiredPos[2] = gGT->level1->ptrSpawnType2_PosRot->posCoords[2] + (s16)((int)saveInst->matrix.m[2][0] * 0x19 >> 7);

				// desired transition rotation (x,y,z)
				desiredRot[0] = gGT->level1->ptrSpawnType2_PosRot->posCoords[3] + D232.saveObjCameraOffset[0];
				desiredRot[1] = gGT->level1->ptrSpawnType2_PosRot->posCoords[4] + D232.saveObjCameraOffset[1];
				desiredRot[2] = gGT->level1->ptrSpawnType2_PosRot->posCoords[5] + D232.saveObjCameraOffset[2];

				// VehBirth_NullThread is an empty function that does nothing
				driver->instSelf->thread->funcThTick = VehBirth_NullThread;

				// Set CameraDC's desired position and rotation,
				// then begin the transition by setting flag
				CAM_SetDesiredPosRot(&gGT->cameraDC[0], (s16 *)&desiredPos, (s16 *)&desiredRot);

				GAMEPAD_JogCon2(driver, 0, 0);

				save->flags |= 1;

				// backup HUD flags while driver sees Save/Load screen
				save->hudFlagBackup = gGT->hudFlags;

				// clear HUD flags
				gGT->hudFlags = 0;
			}
			goto LAB_800af72c;
		}
	}
	else
	{
		if (driver->speed < 0x101)
		{
			// if camera is not transitioning
			if (((gGT->cameraDC->flags & 0x200) == 0) &&

			    ((save->flags & 4) == 0))
			{
				driverInst->thread->funcThTick = NULL;

				save->flags |= 4;
				gGT->hudFlags = save->hudFlagBackup;
			}

			// if camera is transitioning
			else
			{
				// Check the flag again
				uVar6 = gGT->cameraDC->flags;

				// if transition reached it's destination position
				if ((uVar6 & 0x800) != 0)
				{
					// if it is not time to return to player
					if ((save->flags & 2) == 0)
					{
						save->flags |= 2;

						SelectProfile_GetTrackID();

						// enable menu for green load/save screen
						RECTMENU_Show(&data.menuGreenLoadSave);
					}

					// if it is time to return to player
					else
					{
						if (
						    // if you aren't already returning to player
						    ((uVar6 & 0x400) == 0) &&

						    // if there's no Menu active
						    (sdata->ptrActiveMenu == NULL))
						{
							// toggle flag to return, this either snaps back
							// or transitions back depending on & 0x200 (like 0x600 or 0xe00)
							gGT->cameraDC->flags = uVar6 | 0x400;
						}
					}
				}
			}
			goto LAB_800af72c;
		}
		save->flags = 0;
	}

	save->scanlineFrame = 0xf;

LAB_800af72c:

	// first member of object is instance???
	saveInst = save->inst;

	if (saveInst != NULL)
	{
		sVar1 = saveInst->animFrame;

		iVar7 = INSTANCE_GetNumAnimFrames(saveInst, 0);

		// if animation is not finished
		if ((int)sVar1 < iVar7 - 1)
		{
			// increment animation frame
			saveInst->animFrame += 1;
		}

		// if animation is finished,
		// reset animation, and play sound
		else
		{
			// get square root to have "true" distance
			iVar7 = SquareRoot0_stub(dist);

			// multiply by 1.5f
			// mul 3, divide 2
			iVar7 = iVar7 * 3 >> 1;

			// play sound at max volume
			uVar6 = 0xff;

			// if distance is large
			if (300 < iVar7)
			{
				// Map the distance
				//	[300, 6000] (close, far)
				// to volume
				//	[0xff, 0] (loud, soft)
				uVar6 = VehCalc_MapToRange(iVar7, 300, 6000, 0xff, 0);
			}

			// Play save/load screen sound
			// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af7a8-0x800af7c0 for save/load screen loop SFX.
			OtherFX_Play_LowLevel(0x99, 1, (uVar6 & 0xff) << 0x10 | 0x8080);

			// reset animation
			saveInst->animFrame = 0;
		}
	}
	return;
}
