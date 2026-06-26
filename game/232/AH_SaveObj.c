#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af3a4-0x800af3e4.
void AH_SaveObj_ThDestroy(struct Thread *t)
{
	struct SaveObj *save;
	save = t->object;

	if (save->inst != NULL)
	{
		INSTANCE_Death(save->inst);
		save->inst = NULL;
	}
	return;
}

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
	SVec3 desiredPos;
	SVec3 desiredRot;

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
		if (CHECK_ADV_BIT(sdata->advProgress.rewards, ADV_REWARD_HINT_SAVE_LOAD_SCREEN) == 0)
		{
			// Aku Hint "This is the load/save screen..."
			MainFrame_RequestMaskHint(ADV_MASK_HINT_ID_SAVE_LOAD_SCREEN, 0);
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
				struct SpawnPosRot *saveSpawn = gGT->level1->ptrSpawnType2_PosRot->posRot;

				// desired transition position (x,y,z)
				desiredPos.x = saveSpawn->pos.x + (s16)((int)saveInst->matrix.m[0][0] * 0x19 >> 7);
				desiredPos.y = saveSpawn->pos.y + (s16)((int)saveInst->matrix.m[1][0] * 0x19 >> 7);
				desiredPos.z = saveSpawn->pos.z + (s16)((int)saveInst->matrix.m[2][0] * 0x19 >> 7);

				// desired transition rotation (x,y,z)
				desiredRot.x = saveSpawn->rot.x + D232.saveObjCameraOffset[0];
				desiredRot.y = saveSpawn->rot.y + D232.saveObjCameraOffset[1];
				desiredRot.z = saveSpawn->rot.z + D232.saveObjCameraOffset[2];

				// VehBirth_NullThread is an empty function that does nothing
				driver->instSelf->thread->funcThTick = VehBirth_NullThread;

				// Set CameraDC's desired position and rotation,
				// then begin the transition by setting flag
				CAM_SetDesiredPosRot(&gGT->cameraDC[0], &desiredPos, &desiredRot);

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
#if defined(CTR_NATIVE)
				// NOTE(aalhendi): Retail hides the HUD before queueing a hub
				// load while levelID is temporarily MAIN_MENU_LEVEL. Native can
				// keep this save object ticking during that transition, so do not
				// restore the hub HUD/map until loading is idle again.
				if (sdata->Loading.stage == LOAD_IDLE)
				{
#endif
					gGT->hudFlags = save->hudFlagBackup;
#if defined(CTR_NATIVE)
				}
#endif
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
						    ((uVar6 & CAMERA_FLAG_TRANSITION_BACK) == 0) &&

						    // if there's no Menu active
						    (sdata->ptrActiveMenu == NULL))
						{
							// toggle flag to return, this either snaps back
							// or transitions back depending on & 0x200 (like 0x600 or 0xe00)
							gGT->cameraDC->flags = uVar6 | CAMERA_FLAG_TRANSITION_BACK;
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
			OtherFX_Play_LowLevel(0x99, 1, HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, uVar6, 0));

			// reset animation
			saveInst->animFrame = 0;
		}
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af7f0-0x800af9f8.
void AH_SaveObj_LInB(struct Instance *savInst)
{
	SVec3 rot;

	struct GameTracker *gGT = sdata->gGT;
	struct SpawnType2 *spawn;
	struct Thread *t;
	struct Instance *inst;
	struct SaveObj *save;

	// if this Instance's thread is not valid
	if (savInst->thread == NULL)
	{
		t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct SaveObj), NONE, SMALL, STATIC),

		                         AH_SaveObj_ThTick, R232.s_saveobj, 0);

		savInst->thread = t;

		// if the thread was built properly
		if (t != NULL)
		{
			save = t->object;

			t->inst = savInst;

			t->funcThDestroy = AH_SaveObj_ThDestroy;

			// initialize object
			save->flags = 0;

			save->scanlineFrame = 0;

			savInst->flags |= HIDE_MODEL;

			spawn = gGT->level1->ptrSpawnType2_PosRot;

			if (spawn == NULL)
			{
				save->inst = NULL;
			}
			else
			{
				struct SpawnPosRot *saveSpawn = spawn->posRot;
				inst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_SCAN], R232.s_scan, 0);
				save->inst = inst;

				// NOTE(aalhendi): Native low-RAM audit candidate only. Retail
				// uses this scan instance allocation before any null fallback;
				// keep unpatched until a valid hub/save repro proves failure.
				memcpy(&inst->matrix, &savInst->matrix, sizeof(inst->matrix));

				rot = saveSpawn->rot;

				ConvertRotToMatrix(&inst->matrix, &rot);

				inst->matrix.t[0] = saveSpawn->pos.x;
				inst->matrix.t[1] = saveSpawn->pos.y;
				inst->matrix.t[2] = saveSpawn->pos.z;

				inst->depthBiasNormal = 0xf8;
			}
		}
	}
	return;
}
