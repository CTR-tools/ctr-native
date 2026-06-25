#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800abcac-0x800ac178.
void MM_Title_MenuUpdate(void)
{
	struct GameTracker *gGT = sdata->gGT;
	u16 seenDemo;
	s16 cutsceneLev;
	int i;

	// 0 - watching Crash + C-T-R letters animation
	// 1 - in the main menu
	// 2 - leaving main menu
	// 3 - coming back to main menu after exiting another menu

	// If main menu is in focus
	if (D230.MM_State == 1)
	{
		// no transitioning action is needed,
		// skip to end of function
		goto END_FUNCTION;
	}

	// If you aren't in main menu

	// if not transitioning out
	if (D230.MM_State < 2)
	{
		// If your state is less than 2, and
		// not 1, then it must be 0 by default

		// If not transitioning in
		if (D230.MM_State != 0)
		{
			// error, just skip everything
			goto END_FUNCTION;
		}

		// assume main menu state = 0,
		// if you are transitioning in

		// if not done watching C-T-R letters
		if (D230.timerInTitle < 230)
		{
			D230.countMeta0xD = D230.title_numFrameTotal;

			// end function
			goto END_FUNCTION;
		}

		D230.menuMainMenu.state &= ~(DISABLE_INPUT_ALLOW_FUNCPTRS);
		D230.menuMainMenu.state |= EXECUTE_FUNCPTR;

		MM_TransitionInOut(&D230.transitionMeta_Menu[0], D230.countMeta0xD, D230.title_numTransition);

		// If the animation ends
		if (D230.countMeta0xD == 0)
		{
			// you are now in main menu
			D230.MM_State = 1;

			// no further transitioning is needed,
			// skip to end of function
			goto END_FUNCTION;
		}

	LAB_800ac004:

		// decrease amount of time remaining in animation
		D230.countMeta0xD -= 1;
		goto END_FUNCTION;
	}

	// If not transitioning out
	if (D230.MM_State != 2)
	{
		// if you are not returning from another menu,
		// so either in main menu or watching C-T-R trophy animation
		if (D230.MM_State != 3)
		{
			// no further action is needed
			goto END_FUNCTION;
		}

		// assume D230.MM_State = 3
		// if you are returning from another menu
		MM_TransitionInOut(&D230.transitionMeta_Menu[0], D230.countMeta0xD, D230.title_numTransition);

		// If "fade-in" animation from other menu is done
		if (D230.countMeta0xD == 0)
		{
			// you are now in main menu
			D230.MM_State = 1;

			// end the function
			goto END_FUNCTION;
		}

		// If you're transitioning from another menu,
		// and the animation is not done, loop back and
		// check again if the transition is done
		goto LAB_800ac004;
	}

	// assume D230.MM_State = 2
	// If you are transitioning out

	MM_TransitionInOut(&D230.transitionMeta_Menu[0], D230.countMeta0xD, D230.title_numTransition);

	// Increment frame timer, increase time left in "fade-in"
	// animation, which plays it in reverse, as "fade-out"
	D230.countMeta0xD += 1;

	// If the "fade-out" animation is not over, skip "switch" statemenet
	if (D230.countMeta0xD <= D230.title_numFrameTotal)
	{
		goto END_FUNCTION;
	}

	// If you are transitioning out of the menu,
	// and if the "fade-out" animation is done,
	// time to figure out where you're going next
	MM_Title_CameraReset();

	switch (D230.desiredMenuIndex)
	{
	// adventure character selection
	case 0:

		MM_Title_KillThread();
		GAMEPROG_NewProfile_InsideAdv(&sdata->advProgress);

		sdata->advProfileIndex = 0xffff;

		// go to adventure character select screen
		sdata->mainMenuState = MAIN_MENU_ADVENTURE;

		MainRaceTrack_RequestLoad(ADVENTURE_GARAGE);
		break;

	// adventure save/load
	case 1:

		// Go to save/load
		sdata->ptrDesiredMenu = &data.menuFourAdvProfiles;

		SelectProfile_ToggleMode(0x10);
		break;

	// regular character selection screen
	case 2:

		MM_Title_KillThread();

		// return to character selection
		sdata->ptrDesiredMenu = &D230.menuCharacterSelect;

		MM_Characters_RestoreIDs();
		break;

	// high score menu
	case 3:

		MM_HighScore_Init();

		// Go to high score menu
		sdata->ptrDesiredMenu = &D230.menuHighScores;
		break;

	// demo mode
	case 4:

		MM_Title_KillThread();

		gGT->gameMode1 &= ~(BATTLE_MODE | ADVENTURE_MODE | TIME_TRIAL | ADVENTURE_ARENA | ARCADE_MODE | ADVENTURE_CUP);
		gGT->gameMode2 &= ~(CUP_ANY_KIND);

		// enable Arcade Mode
		gGT->gameMode1 |= ARCADE_MODE;

		// If you have not viewed Oxide cutscene yet
		if (gGT->boolSeenOxideIntro == 0)
		{
			gGT->boolSeenOxideIntro = 1;
			cutsceneLev = INTRO_RACE_TODAY;
		}

		// If you've already seen Oxide Cutscene
		else
		{
			// enable Demo Mode
			gGT->boolDemoMode = 1;

			// set number of players to 1
			gGT->numPlyrCurrGame = 1;

			// 60 seconds
			gGT->demoCountdownTimer = 1800;

			// number of times you've seen Demo Mode,
			seenDemo = sdata->demoModeIndex;

			for (i = 0; i < 8; i++)
			{
				data.characterIDs[i] = (seenDemo + i) & 7;
			}

			// get trackID from demo mode index,
			// in order of Single Race track selection
			cutsceneLev = D230.arcadeTracks[seenDemo & 7].levID;

			// increment counter
			sdata->demoModeIndex = seenDemo + 1;
		}
		goto LAB_800abfc0;

	// scrapbook
	case 5:

		MM_Title_KillThread();

		// go to scrapbook
		sdata->mainMenuState = MAIN_MENU_SCRAPBOOK;

		cutsceneLev = SCRAPBOOK;
	LAB_800abfc0:

		// Load level
		MainRaceTrack_RequestLoad(cutsceneLev);

		// make main menu disappear
		RECTMENU_Hide(&D230.menuMainMenu);
	}

END_FUNCTION:

	// if you're entering menu for first time in
	// Crash + C-T-R animation cutscene
	if (D230.MM_State == 0)
	{
		D230.titleCameraPos = D230.title_camPos;
	}
	else
	{
		D230.titleCameraPos.x = D230.title_camPos.x + D230.transitionMeta_Menu[5].currX;
		D230.titleCameraPos.y = D230.title_camPos.y + D230.transitionMeta_Menu[5].currY;
		D230.titleCameraPos.z = D230.title_camPos.z + D230.transitionMeta_Menu[6].currX;
	}

	D230.menuMainMenu.posX_curr = D230.title_mainPosX + D230.transitionMeta_Menu[0].currX;
	D230.menuMainMenu.posY_curr = D230.title_mainPosY + D230.transitionMeta_Menu[0].currY;
	D230.menuAdventure.posX_curr = D230.title_advPosX + D230.transitionMeta_Menu[1].currX;
	D230.menuAdventure.posY_curr = D230.title_advPosY + D230.transitionMeta_Menu[1].currY;
	D230.menuRaceType.posX_curr = D230.title_racePosX + D230.transitionMeta_Menu[2].currX;
	D230.menuRaceType.posY_curr = D230.title_racePosY + D230.transitionMeta_Menu[2].currY;
	D230.menuPlayers1P2P.posX_curr = D230.title_plyrPosX + D230.transitionMeta_Menu[3].currX;
	D230.menuPlayers1P2P.posY_curr = D230.title_plyrPosY + D230.transitionMeta_Menu[3].currY;
	D230.menuPlayers2P3P4P.posX_curr = D230.title_plyrPosX + D230.transitionMeta_Menu[3].currX;
	D230.menuPlayers2P3P4P.posY_curr = D230.title_plyrPosY + D230.transitionMeta_Menu[3].currY;
	D230.menuDifficulty.posX_curr = D230.title_diffPosX + D230.transitionMeta_Menu[4].currX;
	D230.menuDifficulty.posY_curr = D230.title_diffPosY + D230.transitionMeta_Menu[4].currY;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ac94c-0x800ac9fc.
void MM_Title_KillThread(void)
{
	char n;
	struct GameTracker *gGT = sdata->gGT;
	struct Title *title = D230.titleObj;

	if (                     // if "title" object exists
	    (title != NULL) && ( // if you are in main menu
	                           (gGT->gameMode1 & MAIN_MENU) != 0))
	{
		// destroy six instances
		for (n = 0; n < 6; n++)
		{
			INSTANCE_Death(title->i[n]);
		}

		title->t->flags |= THREAD_FLAG_DEAD;
		D230.titleObj = NULL;

		// CameraDC, it must be zero to follow you
		gGT->cameraDC[0].transitionTo.rot.x = 0;
		gGT->pushBuffer[0].distanceToScreen_CURR = 0x100;
	}
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ac178-0x800ac1f0.
void MM_Title_SetTrophyDPP(void)
{
	u32 idpp2_b8;
	struct InstDrawPerPlayer *idpp1;
	struct InstDrawPerPlayer *idpp2;
	struct Title *title = D230.titleObj;
	int e4;
	int e8;
	int dc;

	if (title == NULL)
	{
		return;
	}

	idpp1 = INST_GETIDPP(title->i[1]); // "title"
	idpp2 = INST_GETIDPP(title->i[2]); // another "title"

	idpp2_b8 = idpp2->instFlags;
	if ((idpp2_b8 & 0x100) != 0)
	{
		return;
	}

	idpp2_b8 |= 0xffffffbf;
	idpp1->instFlags &= idpp2_b8;

	e4 = idpp2->otRangeNormal;
	e8 = idpp2->otRangeSecondary;
	dc = *(int *)&idpp2->depthOffset[0];

	idpp1->otRangeNormal = e4;
	idpp1->otRangeSecondary = e8;
	*(int *)&idpp1->depthOffset[0] = dc;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ac1f0-0x800ac350.
void MM_Title_CameraMove(struct Title *title, int frameIndex)
{
	int result;
	s16 *posRot;
	struct GameTracker *gGT;

	// after frame 0xe6, make the intro models transition from the center
	// of the screen, to the left of the screen, over the course of 15 frames
	result = RaceFlag_MoveModels(D230.timerInTitle - 0xe6, 0xF);

	gGT = sdata->gGT;

	posRot = &D230.ptrIntroCam[frameIndex * 6];

	for (int i = 0; i < 3; i++)
	{
		// position XYZ
		gGT->pushBuffer[0].pos.v[i] = title->cameraPosOffset.v[i] + posRot[i] + (s16)((D230.titleCameraPos.v[i] * result) >> 0xc);

		// rotation XYZ
		gGT->pushBuffer[0].rot.v[i] = posRot[3 + i] + (s16)((D230.titleCameraRot.v[i] * result) >> 0xc);
	}
}

static void MM_Title_RotMatrixMul(MATRIX *matrix, const SVec3 *input, VECTOR *mac)
{
	gte_SetRotMatrix(matrix);
	CTR_GteLoadSVec3V0(input);
	gte_rtv0();
	CTR_GteStoreMAC(&mac->vx);
}

static void MM_Title_UpdateTrophySpecLight(struct Instance *titleInst)
{
	struct GameTracker *gGT = sdata->gGT;
	struct PushBuffer *pb = &gGT->pushBuffer[0];
	struct InstDrawPerPlayer *idpp = INST_GETIDPP(titleInst);
	MATRIX matrix;
	SVec3 rot;
	SVec3 light;
	SVec3 view;
	VECTOR lightMac;
	VECTOR viewMac;

	rot.x = -pb->rot.x;
	rot.y = -pb->rot.y;
	rot.z = -pb->rot.z;
	ConvertRotToMatrix_Transpose(&matrix, &rot);

	light.x = 0;
	light.y = 0x1000;
	light.z = 0;
	MM_Title_RotMatrixMul(&matrix, &light, &lightMac);

	titleInst->unk53 = (u8)lightMac.vx;
	titleInst->reflectionRGBA = (u32)lightMac.vz;

	view.x = titleInst->matrix.t[0] - pb->pos.x;
	view.y = titleInst->matrix.t[1] - pb->pos.y;
	view.z = titleInst->matrix.t[2] - pb->pos.z;
	MATH_VectorNormalize(&view);
	MM_Title_RotMatrixMul(&matrix, &view, &viewMac);

	idpp[0].halfVector.x = (s16)((u16)lightMac.vx + (u16)viewMac.vx);
	idpp[0].halfVector.y = (s16)((u16)lightMac.vy + (u16)viewMac.vy);
	idpp[0].halfVector.z = (s16)((u16)lightMac.vz + (u16)viewMac.vz);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800ac350-0x800ac6dc.
void MM_Title_ThTick(struct Thread *title)
{
	s16 animFram;
	struct Instance *titleInst;
	int i;
	int timer;
	struct Title *ptrTitle;

	// frame counters
	timer = D230.timerInTitle;

	// If you press Cross, Circle, Triangle, or Square
	if ((sdata->buttonTapPerPlayer[0] & 0x40070) != 0)
	{
		// clear gamepad input (for menus)
		RECTMENU_ClearInput();

		// set frame to 1000, skip the animation
		D230.timerInTitle = 1000;
	}

	// cap at 230
	if (timer > 230)
	{
		timer = 230;
	}

	// play 8 sounds, one on each frame
	for (i = 0; i < 8; i++)
	{
		if (D230.titleSounds[i].frameToPlay == timer)
		{
			// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac3e8-0x800ac400 for title queued SFX.
			OtherFX_Play(D230.titleSounds[i].soundID, 1);
		}
	}

	// copy pointer to title object
	ptrTitle = (struct Title *)title->object;

	// loop 6 times
	for (i = 0; i < 6; i++)
	{
		// current instance
		titleInst = ptrTitle->i[i];

		titleInst->flags &= ~HIDE_MODEL;

		// the frame of title screen that each instance should start animation
		animFram = D230.titleInstances[i].frameIndex_startMoving;

		// set all instances to first animation
		titleInst->animIndex = 0;

		// set animation frame, based on what frame each instance should start
		titleInst->animFrame = (timer - animFram);

		// if instance has not started animation
		if (((timer - animFram) * 0x10000) < 0)
		{
			// skip the trophy instance
			if (i != 2)
			{
				titleInst->flags |= HIDE_MODEL;
			}

			// set animFrame to zero
			titleInst->animFrame = 0;
		}

		if ((D230.titleInstances[i].boolTrophy) != 0)
		{
			// if frame is anywhere in the two seconds
			// that the trophy is in the air
			if ((u32)(timer - 138) < 62)
			{
				titleInst->flags |= HIDE_MODEL;
			}

			// otherwise
			else if (200 <= timer)
			{
				// play frame index, based on total animation frame
				titleInst->animFrame = timer - 200;

				// set animation to 1
				titleInst->animIndex = 1;
			}

			MM_Title_UpdateTrophySpecLight(titleInst);
		}
	}

	MM_Title_CameraMove(ptrTitle, timer);

	// increment frame counter
	timer = D230.timerInTitle + 1;

	if (245 < D230.timerInTitle)
	{
		// animation is over
		D230.menuMainMenu.state &= ~(DISABLE_INPUT_ALLOW_FUNCPTRS);
		D230.menuMainMenu.state |= EXECUTE_FUNCPTR;

		// dont increment index
		timer = D230.timerInTitle;
	}

	// write to index
	D230.timerInTitle = timer;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac6dc-0x800ac92c.
void MM_Title_Init(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Thread *t;
	struct Instance *inst;
	struct Title *title;
	char m, n;

	if (
	    // if "title" object is nullptr
	    (D230.titleObj == NULL) &&

	    // if you are in main menu
	    ((gGT->gameMode1 & MAIN_MENU) != 0) &&

	    // You're not in transition between menus
	    (D230.MM_State != 2) &&

	    // model ptr (Title blue Ring)
	    (gGT->modelPtr[STATIC_RINGTOP] != 0) &&

	    // IntroCam ptr exists
	    (gGT->level1->ptrSpawnType1->count > 2))
	{
		// freecam mode
		gGT->cameraDC[0].cameraMode = 3;

		gGT->pushBuffer[0].distanceToScreen_CURR = 450;

		void **pointers = ST1_GETPOINTERS(gGT->level1->ptrSpawnType1);

		// pointer to Intro Cam, to view Crash holding Trophy in main menu
		D230.ptrIntroCam = pointers[ST1_CAMERA_PATH];

		t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Title), // 0x24
		                                                   NONE, MEDIUM, OTHER),
		                         MM_Title_ThTick, 0, 0);

		title = t->object;

		D230.titleObj = title;

		memset(title, 0, 0x24);

		title->t = t;

		// create 6 instances
		for (n = 0; n < 6; n++)
		{
			inst = INSTANCE_Birth3D(gGT->modelPtr[D230.titleInstances[n].modelID], 0, t);

			// store instance
			title->i[n] = inst;

			if (D230.titleInstances[n].boolTrophy)
			{
				inst->flags |= VISIBLE_DURING_GAMEPLAY;
			}

			*(int *)&inst->matrix.m[0][0] = 0x5000;
			*(int *)&inst->matrix.m[0][2] = 0;
			*(int *)&inst->matrix.m[1][1] = 0x5000;
			*(int *)&inst->matrix.m[2][0] = 0;
			*(int *)&inst->matrix.m[2][2] = 0x5000;

			inst->matrix.t[0] = 0;
			inst->matrix.t[1] = 0;
			inst->matrix.t[2] = 0;

			inst->flags |= HIDE_MODEL;


			struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);
			for (m = 1; m < gGT->numPlyrCurrGame; m++)
			{
				idpp[m].pushBuffer = 0;
			}
		}

		MM_Title_CameraMove(title, 0);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac92c-0x800ac94c.
void MM_Title_CameraReset(void)
{
	struct Title *title = D230.titleObj;

	if (title == NULL)
	{
		return;
	}

	title->cameraPosOffset.x = 2000;
}
