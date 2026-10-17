#include <common.h>

void MM_Title_MenuUpdate(void)
{
	u16 seenDemo;
	s32 cutsceneLev;
	s16 demoDriverIndex;
	struct RectMenu *mainMenu;
	const SVec2 *menuPos;
	struct TransitionMeta *transition;
	register MainMenuState *mainMenuState CTR_PSX_REGISTER("$2");
	register MainMenuState scrapbookState CTR_PSX_REGISTER("$3");

	// 0 - watching Crash + C-T-R letters animation
	// 1 - in the main menu
	// 2 - leaving main menu
	// 3 - coming back to main menu after exiting another menu

	// If main menu is in focus
	if (MM_TITLE_MENU_STATE == TITLE_MENU_STATE_IN_MENU)
	{
		// no transitioning action is needed,
		// skip to end of function
		goto END_FUNCTION;
	}

	if (MM_TITLE_MENU_STATE < TITLE_MENU_STATE_EXITING)
	{
		if (MM_TITLE_MENU_STATE == TITLE_MENU_STATE_INTRO)
		{
			goto HANDLE_INTRO;
		}
		goto END_FUNCTION;
	}

	if (MM_TITLE_MENU_STATE == TITLE_MENU_STATE_EXITING)
	{
		goto HANDLE_EXITING;
	}
	if (MM_TITLE_MENU_STATE == TITLE_MENU_STATE_RETURNING)
	{
		goto HANDLE_RETURNING;
	}
	goto END_FUNCTION;

HANDLE_INTRO:
	// assume main menu state = 0,
	// if you are transitioning in

	// if not done watching C-T-R letters
	if ((s16)MM_TITLE_INTRO_FRAME < TITLE_INTRO_MENU_READY_FRAME)
	{
		MM_TITLE_MENU_TRANSITION_FRAME = MM_TITLE_TRANSITION_DURATION;

		// end function
		goto END_FUNCTION;
	}

	MM_MENU_MAIN.state &= ~(DISABLE_INPUT_ALLOW_FUNCPTRS);
	MM_MENU_MAIN.state |= EXECUTE_FUNCPTR;

	MM_TransitionInOut(MM_TITLE_TRANSITIONS, MM_TITLE_MENU_TRANSITION_FRAME, MM_TITLE_TRANSITION_STEP);

	// If the animation is not done
	if (MM_TITLE_MENU_TRANSITION_FRAME != 0)
	{
		// decrease amount of time remaining in animation
		MM_TITLE_MENU_TRANSITION_FRAME -= 1;
		goto END_FUNCTION;
	}

	// you are now in main menu
	MM_TITLE_MENU_STATE = TITLE_MENU_STATE_IN_MENU;
	goto END_FUNCTION;

HANDLE_EXITING:
	// assume title menu state = TITLE_MENU_STATE_EXITING
	// If you are transitioning out

	MM_TransitionInOut(MM_TITLE_TRANSITIONS, MM_TITLE_MENU_TRANSITION_FRAME, MM_TITLE_TRANSITION_STEP);

	// Increment frame timer, increase time left in "fade-in"
	// animation, which plays it in reverse, as "fade-out"
	MM_TITLE_MENU_TRANSITION_FRAME += 1;

	// If the "fade-out" animation is not over, skip the switch statement
	if (MM_TITLE_MENU_TRANSITION_FRAME <= MM_TITLE_TRANSITION_DURATION)
	{
		goto END_FUNCTION;
	}

	// If you are transitioning out of the menu,
	// and if the "fade-out" animation is done,
	// time to figure out where you're going next
	switch (MM_DESIRED_MENU_INDEX)
	{
	// adventure character selection
	case MM_EXIT_ROUTE_ADV_NEW:

		GAMEPROG_NewProfile_InsideAdv(&GAME_ADV_PROGRESS);

		// The 1P/2P choice happens before this route, but the profile screen that
		// writes the new save runs later, after the main menu has already reset
		// numPlyrNextGame, so the racer count has to be captured here.
		sdata->advMultiplayer.numPlayers = GAME_TRACKER->numPlyrNextGame;
		sdata->advMultiplayer.characterID2 = ADV_MULTIPLAYER_NO_CHARACTER;

		MM_ADV_PROFILE_INDEX = 0xffff;

	        // go to adventure character select screen
	        MM_MAIN_MENU_STATE = MAIN_MENU_ADVENTURE;

	        MM_Title_CameraReset();
	        MM_Title_KillThread();

		if (g_config.extendedAdventureCharacterSelect)
		{
		  // The normal Adventure path loads the garage. Keep the main-menu level
		  // active instead so its full character roster can be selected first.
		  if (!g_config.multiplayerAdventure)
		  {
		    GAME_TRACKER->numPlyrNextGame = 1;
		  }
		  sdata->ptrDesiredMenu = &D230.menuCharacterSelect;
		  MM_Characters_RestoreIDs();
		}
	        else
	        {
	          MainRaceTrack_RequestLoad(ADVENTURE_GARAGE);
	        }
		break;

	// adventure save/load
	case MM_EXIT_ROUTE_ADV_LOAD:

		// Go to save/load
		MM_DESIRED_MENU = &MM_MENU_FOUR_ADV_PROFILES;

		MM_Title_CameraReset();
		SelectProfile_ToggleMode(SELECT_PROFILE_SCREEN_ADV_LOAD);
		break;

	// regular character selection screen
	case MM_EXIT_ROUTE_CHARACTER_SELECT:

		MM_Title_CameraReset();
		MM_Title_KillThread();

		// return to character selection
		MM_DESIRED_MENU = &MM_MENU_CHARACTER_SELECT;

		MM_Characters_RestoreIDs();
		break;

	// high score menu
	case MM_EXIT_ROUTE_HIGH_SCORE:

		MM_Title_CameraReset();
		MM_HighScore_Init();

		// Go to high score menu
		MM_DESIRED_MENU = &MM_MENU_HIGH_SCORES;
		break;

	// demo mode
	case MM_EXIT_ROUTE_DEMO:

		MM_Title_CameraReset();
		MM_Title_KillThread();

		GAME_TRACKER->gameMode1 &= ~(BATTLE_MODE | ADVENTURE_MODE | TIME_TRIAL | ADVENTURE_ARENA | ARCADE_MODE | ADVENTURE_CUP);
		GAME_TRACKER->gameMode2 &= ~(CUP_ANY_KIND);

		// enable Arcade Mode
		GAME_TRACKER->gameMode1 |= ARCADE_MODE;

		// If you have not viewed Oxide cutscene yet
		if (GAME_TRACKER->boolSeenOxideIntro == 0)
		{
			GAME_TRACKER->boolSeenOxideIntro = 1;
			cutsceneLev = INTRO_RACE_TODAY;
		}

		// If you've already seen Oxide Cutscene
		else
		{
			// enable Demo Mode
			GAME_TRACKER->boolDemoMode = 1;

			// number of times you've seen Demo Mode
			seenDemo = MM_DEMO_MODE_INDEX;

			GAME_TRACKER->demoCountdownTimer = TITLE_DEMO_RACE_FRAMES;

			for (demoDriverIndex = 0; demoDriverIndex < TITLE_DEMO_DRIVER_COUNT; demoDriverIndex++)
			{
				GAME_CHARACTER_IDS[demoDriverIndex] = seenDemo++;
				GAME_CHARACTER_IDS[demoDriverIndex] &= TITLE_DEMO_INDEX_MASK;
			}

			// set number of players to 1
			GAME_TRACKER->numPlyrNextGame = 1;

			// get trackID from demo mode index,
			// in order of Single Race track selection
			cutsceneLev = MM_ARCADE_TRACKS[MM_DEMO_MODE_INDEX & TITLE_DEMO_INDEX_MASK].levID;

			// increment counter
			MM_DEMO_MODE_INDEX += 1;
		}
		goto LAB_800abfc0;

	// developer hack: straight into a 1P arcade race as Crash in Crash Cove
	case MM_EXIT_ROUTE_QUICK_RACE:

		MM_Title_CameraReset();
		MM_Title_KillThread();

		cutsceneLev = CRASH_COVE;
		MM_QuickRace_SetupRace();
		goto LAB_800abfc0;

	// scrapbook
	case MM_EXIT_ROUTE_SCRAPBOOK:

		MM_Title_CameraReset();
		MM_Title_KillThread();

		// go to scrapbook
		mainMenuState = &MM_MAIN_MENU_STATE;
		scrapbookState = MAIN_MENU_SCRAPBOOK;
		*mainMenuState = scrapbookState;

		cutsceneLev = SCRAPBOOK;
	LAB_800abfc0:

		// Load level
		MainRaceTrack_RequestLoad(cutsceneLev);

		// make main menu disappear
		RECTMENU_Hide(&MM_MENU_MAIN);
	}
	goto END_FUNCTION;

HANDLE_RETURNING:
	// assume title menu state = TITLE_MENU_STATE_RETURNING
	// if you are returning from another menu
	MM_TransitionInOut(MM_TITLE_TRANSITIONS, MM_TITLE_MENU_TRANSITION_FRAME, MM_TITLE_TRANSITION_STEP);

	// If "fade-in" animation from other menu is not done
	if (MM_TITLE_MENU_TRANSITION_FRAME != 0)
	{
		MM_TITLE_MENU_TRANSITION_FRAME -= 1;
		goto END_FUNCTION;
	}

	// you are now in main menu
	MM_TITLE_MENU_STATE = TITLE_MENU_STATE_IN_MENU;

END_FUNCTION:
	mainMenu = &MM_MENU_MAIN;
	menuPos = MM_TITLE_MENU_LAYOUT.menuPos;
	transition = MM_TITLE_TRANSITIONS;

	mainMenu->posX_curr = menuPos[TITLE_MENU_POS_MAIN].x + transition[TITLE_TRANSITION_MAIN].currX;
	mainMenu->posY_curr = menuPos[TITLE_MENU_POS_MAIN].y + transition[TITLE_TRANSITION_MAIN].currY;
	MM_MENU_ADVENTURE.posX_curr = menuPos[TITLE_MENU_POS_ADVENTURE].x + transition[TITLE_TRANSITION_ADVENTURE].currX;
	MM_MENU_ADVENTURE.posY_curr = menuPos[TITLE_MENU_POS_ADVENTURE].y + transition[TITLE_TRANSITION_ADVENTURE].currY;
	MM_MENU_RACE_TYPE.posX_curr = menuPos[TITLE_MENU_POS_RACE_TYPE].x + transition[TITLE_TRANSITION_RACE_TYPE].currX;
	MM_MENU_RACE_TYPE.posY_curr = menuPos[TITLE_MENU_POS_RACE_TYPE].y + transition[TITLE_TRANSITION_RACE_TYPE].currY;
	MM_MENU_PLAYERS_1P2P.posX_curr = menuPos[TITLE_MENU_POS_PLAYERS].x + transition[TITLE_TRANSITION_PLAYERS].currX;
	MM_MENU_PLAYERS_1P2P.posY_curr = menuPos[TITLE_MENU_POS_PLAYERS].y + transition[TITLE_TRANSITION_PLAYERS].currY;
	MM_MENU_PLAYERS_2P3P4P.posX_curr = menuPos[TITLE_MENU_POS_PLAYERS].x + transition[TITLE_TRANSITION_PLAYERS].currX;
	MM_MENU_PLAYERS_2P3P4P.posY_curr = menuPos[TITLE_MENU_POS_PLAYERS].y + transition[TITLE_TRANSITION_PLAYERS].currY;
	MM_MENU_DIFFICULTY.posX_curr = menuPos[TITLE_MENU_POS_DIFFICULTY].x + transition[TITLE_TRANSITION_DIFFICULTY].currX;
	MM_MENU_DIFFICULTY.posY_curr = menuPos[TITLE_MENU_POS_DIFFICULTY].y + transition[TITLE_TRANSITION_DIFFICULTY].currY;

	// if you're entering menu for first time in
	// Crash + C-T-R animation cutscene
	if (MM_TITLE_MENU_STATE != TITLE_MENU_STATE_INTRO)
	{
		MM_TITLE_CAMERA_POS.x = MM_TITLE_MENU_LAYOUT.baseCameraPos.x + transition[TITLE_TRANSITION_CAMERA_XY].currX;
		MM_TITLE_CAMERA_POS.y = MM_TITLE_MENU_LAYOUT.baseCameraPos.y + transition[TITLE_TRANSITION_CAMERA_XY].currY;
		MM_TITLE_CAMERA_POS.z = MM_TITLE_MENU_LAYOUT.baseCameraPos.z + transition[TITLE_TRANSITION_CAMERA_Z].currX;
	}
	else
	{
		MM_TITLE_CAMERA_POS.x = MM_TITLE_MENU_LAYOUT.baseCameraPos.x;
		MM_TITLE_CAMERA_POS.y = MM_TITLE_MENU_LAYOUT.baseCameraPos.y;
		MM_TITLE_CAMERA_POS.z = MM_TITLE_MENU_LAYOUT.baseCameraPos.z;
	}
}

void MM_Title_KillThread(void)
{
	struct GameTracker *gGT;
	struct Title *title;
	s16 instanceIndex;

	if (                               // if "title" object exists
	    (MM_TITLE_OBJECT != NULL) && ( // if you are in main menu
	                                     (GAME_TRACKER->gameMode1 & MAIN_MENU) != 0))
	{
		// destroy title instances
		for (instanceIndex = 0; (u16)instanceIndex < TITLE_INSTANCE_COUNT; instanceIndex++)
		{
			INSTANCE_Death(MM_TITLE_OBJECT->i[(s16)instanceIndex]);
		}

		title = MM_TITLE_OBJECT;
		MM_TITLE_OBJECT = NULL;
		gGT = GAME_TRACKER;
		title->t->flags |= THREAD_FLAG_DEAD;

		// CameraDC, it must be zero to follow you
		gGT->cameraDC[0].cameraMode = 0;
		gGT->pushBuffer[0].distanceToScreen_CURR = TITLE_DEFAULT_DISTANCE_TO_SCREEN;
	}
}

void MM_Title_SetTrophyDPP(void)
{
	struct Title *title;
	register u32 secondaryFlags CTR_PSX_REGISTER("$6");
	register struct Instance *secondary CTR_PSX_REGISTER("$5");
	register struct Instance *primary CTR_PSX_REGISTER("$4");
	register u32 drawFlags CTR_PSX_REGISTER("$2");
	register u32 primaryFlags CTR_PSX_REGISTER("$3");

	title = MM_TITLE_OBJECT;

	if (title == NULL)
	{
		return;
	}

	secondary = title->i[2];
	secondaryFlags = INST_GETIDPP(secondary)->instFlags;
	primary = title->i[1];
	if ((secondaryFlags & PUSHBUFFER_EXISTS) != 0)
	{
		return;
	}

	drawFlags = secondaryFlags | ~DRAW_SUCCESSFUL;
	primaryFlags = INST_GETIDPP(primary)->instFlags;
	primaryFlags &= drawFlags;
	INST_GETIDPP(primary)->instFlags = primaryFlags;
	INST_GETIDPP(primary)->otRangeNormal = INST_GETIDPP(secondary)->otRangeNormal;
	INST_GETIDPP(primary)->otRangeSecondary = INST_GETIDPP(secondary)->otRangeSecondary;
	INST_GETIDPP(primary)->depthOffset[0] = INST_GETIDPP(secondary)->depthOffset[0];
	INST_GETIDPP(primary)->depthOffset[1] = INST_GETIDPP(secondary)->depthOffset[1];
}

void MM_Title_CameraMove(struct Title *title, s16 frameIndex)
{
	s32 result;
	const s16 *cameraPath;
	struct GameTracker *gGT;
	struct PushBuffer *pushBuffer;

	// after frame 0xe6, make the intro models transition from the center
	// of the screen, to the left of the screen, over the course of 15 frames
	result = RaceFlag_MoveModels((s16)(MM_TITLE_INTRO_FRAME - TITLE_INTRO_MENU_READY_FRAME), TITLE_CAMERA_MOVE_FRAMES);
	cameraPath = (const s16 *)&MM_TITLE_CAMERA_PATH[frameIndex];
	gGT = GAME_TRACKER;
	pushBuffer = &gGT->pushBuffer[0];

	pushBuffer->pos.x = title->cameraPosOffset.x + (cameraPath[0] + (s16)((MM_TITLE_CAMERA_POS.x * result) >> 0xc));
	pushBuffer->pos.y = title->cameraPosOffset.y + (cameraPath[1] + (s16)((MM_TITLE_CAMERA_POS.y * result) >> 0xc));
	pushBuffer->pos.z = title->cameraPosOffset.z + (cameraPath[2] + (s16)((MM_TITLE_CAMERA_POS.z * result) >> 0xc));

	cameraPath += 3;
	pushBuffer->rot.x = cameraPath[0] + (s16)((MM_TITLE_CAMERA_ROT.x * result) >> 0xc);
	pushBuffer->rot.y = cameraPath[1] + (s16)((MM_TITLE_CAMERA_ROT.y * result) >> 0xc);
	pushBuffer->rot.z = cameraPath[2] + (s16)((MM_TITLE_CAMERA_ROT.z * result) >> 0xc);
}

static inline void MM_Title_RotMatrixMul(const SVec3 *input, VECTOR *mac)
{
	CTR_GteLoadSVec3V0(input);
	CTR_GteLoadDelay();
	gte_rtv0();
	CTR_GteStoreMAC(&mac->vx);
}

static inline void MM_Title_UpdateTrophySpecLight(struct Instance *titleInst)
{
	struct GameTracker *gGT;
	struct PushBuffer *pb;
	MATRIX matrix;
	VECTOR lightMac;
	VECTOR viewMac;
	SVec3 view;
	SVec3 rot;
	SVec3 *viewPtr;
	register u32 gteValue CTR_PSX_REGISTER("$7");
	register u16 lightX CTR_PSX_REGISTER("$6");
	register u16 viewX CTR_PSX_REGISTER("$2");
	register u16 viewY CTR_PSX_REGISTER("$4");
	register u16 lightZ CTR_PSX_REGISTER("$3");
	register u16 viewZ CTR_PSX_REGISTER("$5");
	register u16 lightY CTR_PSX_REGISTER("$2");
	register s32 viewMacValue CTR_PSX_REGISTER("$7");

	gGT = GAME_TRACKER;
	pb = &gGT->pushBuffer[0];
	viewPtr = &view;
	rot.x = -pb->rot.x;
	rot.y = -pb->rot.y;
	rot.z = -pb->rot.z;
	ConvertRotToMatrix_Transpose(&matrix, &rot);

	gteValue = (u32)TITLE_SPEC_LIGHT_Y << 16;
	MTC2(gteValue, 0);
	gteValue = 0;
	MTC2(gteValue, 1);
	CTR_GteLoadDelay();
	gte_rtv0();
	CTR_GteStoreMAC(&lightMac.vx);

	titleInst->specLightX = (s8)lightMac.vx;
	titleInst->reflectionRGBA = (u32)lightMac.vz;

	view.x = titleInst->matrix.t[0] - pb->pos.x;
	view.y = titleInst->matrix.t[1] - pb->pos.y;
	view.z = titleInst->matrix.t[2] - pb->pos.z;
	MATH_VectorNormalize(viewPtr);
	CTR_GteLoadSVec3V0(viewPtr);
	CTR_GteLoadDelay();
	gte_rtv0();

	viewMacValue = (s32)MFC2(25);
	CTR_GteRegisterReadDelay();
	viewMac.vx = viewMacValue;
	viewMacValue = (s32)MFC2(26);
	CTR_GteRegisterReadDelay();
	viewMac.vy = viewMacValue;
	viewMacValue = (s32)MFC2(27);

	lightX = (u16)lightMac.vx;
	CTR_PSX_KEEP_VALUE(lightX);
	viewX = (u16)viewMac.vx;
	viewY = (u16)viewMac.vy;
	lightZ = (u16)lightMac.vz;
	CTR_PSX_KEEP_VALUE(lightZ);
	viewMac.vz = viewMacValue;
	viewZ = (u16)viewMac.vz;
	view.x = (s16)(lightX + viewX);
	view.z = (s16)(lightZ + viewZ);
	lightY = (u16)lightMac.vy;
	view.y = (s16)(lightY + viewY);

	INST_GETIDPP(titleInst)->halfVector.x = view.x;
	INST_GETIDPP(titleInst)->halfVector.y = view.y;
	INST_GETIDPP(titleInst)->halfVector.z = view.z;
}

u16 MM_Title_ThTick(struct Thread *title)
{
	struct Instance **titleInstances;
	struct Instance *titleInst;
	struct Title *ptrTitle;
	register s32 timer CTR_PSX_REGISTER("$20");
	register s16 index CTR_PSX_REGISTER("$19");
	struct TitleSoundCue *sounds;
	s32 timerSigned;
	s16 animFram;

	// frame counters
	timer = MM_TITLE_INTRO_FRAME;
	ptrTitle = (struct Title *)title->object;

	// If you press Cross, Circle, Triangle, or Square
	if ((MM_GAME_BUTTON_TAPS[0] & TITLE_INTRO_SKIP_INPUT) != 0)
	{
		// clear gamepad input (for menus)
		RECTMENU_ClearInput();

		// set frame to 1000, skip the animation
		MM_TITLE_INTRO_FRAME = TITLE_INTRO_SKIP_FRAME;
	}

	// cap at 230
	if ((s16)timer > TITLE_INTRO_MENU_READY_FRAME)
	{
		timer = TITLE_INTRO_MENU_READY_FRAME;
	}
	index = 0;
	sounds = MM_TITLE_SOUNDS;
	CTR_PSX_CLOBBER("$16");
	timerSigned = (s16)timer;

	// play queued title sounds
	for (; (u16)index < TITLE_SOUND_COUNT; index++)
	{
		if (sounds[(s16)index].frameToPlay == timerSigned)
		{
			OtherFX_Play(sounds[(s16)index].soundID, 1);
		}
	}

	// loop through title instances
	titleInstances = ptrTitle->i;
	for (index = 0; (u16)index < TITLE_INSTANCE_COUNT; index++, titleInstances++)
	{
		// current instance
		titleInst = *titleInstances;

		titleInst->flags &= ~HIDE_MODEL;

		// the frame of title screen that each instance should start animation
		animFram = MM_TITLE_INSTANCES[(s16)index].animStartFrame;

		// set all instances to first animation
		titleInst->animIndex = 0;

		// set animation frame, based on what frame each instance should start
		titleInst->animFrame = (timer - animFram);

		// if instance has not started animation
		if ((titleInst->animFrame * 0x10000) < 0)
		{
			// keep instance 2 visible before its animation starts
			if ((s16)index != 2)
			{
				titleInst->flags |= HIDE_MODEL;
			}

			// set animFrame to zero
			titleInst->animFrame = 0;
		}

		if ((MM_TITLE_INSTANCES[(s16)index].isTrophy) != 0)
		{
			// if frame is anywhere in the two seconds
			// that the trophy is in the air
			if ((u32)(timer - TITLE_TROPHY_HIDE_START_FRAME) < TITLE_TROPHY_HIDE_FRAMES)
			{
				titleInst->flags |= HIDE_MODEL;
			}

			// otherwise
			else if (TITLE_TROPHY_ANIM_START_FRAME <= (s16)timer)
			{
				// play frame index, based on total animation frame
				titleInst->animFrame = timer - TITLE_TROPHY_ANIM_START_FRAME;

				// set animation to 1
				titleInst->animIndex = 1;
			}

			MM_Title_UpdateTrophySpecLight(titleInst);
		}
	}

	MM_Title_CameraMove(ptrTitle, (s16)timer);

	if ((s16)MM_TITLE_INTRO_FRAME < (TITLE_INTRO_END_FRAME + 1))
	{
		MM_TITLE_INTRO_FRAME++;
	}
	else
	{
		// animation is over
		MM_MENU_MAIN.state &= ~(DISABLE_INPUT_ALLOW_FUNCPTRS);
		MM_MENU_MAIN.state |= EXECUTE_FUNCPTR;
	}

	{
		register u32 introFramePage CTR_PSX_REGISTER("$3");

		CTR_PSX_LOAD_SYMBOL_PAGE(introFramePage, MM_TITLE_INTRO_FRAME_ASM_NAME);
		return CTR_PSX_PAGE_LVALUE(u16, introFramePage, MM_TITLE_INTRO_FRAME_PAGE_OFFSET, MM_TITLE_INTRO_FRAME);
	}
}

void MM_Title_Init(void)
{
	struct Instance *inst;
	struct Instance **titleInstanceSlot;
	struct Thread *t;
	struct Title *title;
	struct InstDrawPerPlayer *playerDraw;
	void **pointers;
	s16 instanceIndex;
	s16 playerIndex;

	if (
	    // if "title" object is nullptr
	    (MM_TITLE_OBJECT == NULL) &&

	    // if you are in main menu
	    ((GAME_TRACKER->gameMode1 & MAIN_MENU) != 0) &&

	    // You're not in transition between menus
	    (MM_TITLE_MENU_STATE != TITLE_MENU_STATE_EXITING) &&

	    // model ptr (Title blue Ring)
	    (GAME_TRACKER->modelPtr[STATIC_RINGTOP] != 0) &&

	    // IntroCam ptr exists
	    (GAME_TRACKER->level1->ptrSpawnType1->count > 2))
	{
		// freecam mode
		GAME_TRACKER->cameraDC[0].cameraMode = CAMERA_MODE_FREECAM;

		GAME_TRACKER->pushBuffer[0].distanceToScreen_CURR = TITLE_INTRO_DISTANCE_TO_SCREEN;

		pointers = ST1_GETPOINTERS(GAME_TRACKER->level1->ptrSpawnType1);

		// pointer to Intro Cam, to view Crash holding Trophy in main menu
		MM_TITLE_CAMERA_PATH = pointers[ST1_CAMERA_PATH];

		t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Title), NONE, MEDIUM, OTHER), MM_Title_ThTick, MM_TITLE_OBJECT_NAME, 0);

		title = t->object;

		MM_TITLE_OBJECT = title;

		memset(title, 0, sizeof(*title));

		titleInstanceSlot = title->i;

		title->t = t;

		// create title instances
		for (instanceIndex = 0; (u16)instanceIndex < TITLE_INSTANCE_COUNT; instanceIndex++, titleInstanceSlot++)
		{
			inst = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[MM_TITLE_INSTANCES[(s16)instanceIndex].modelID], MM_TITLE_OBJECT_NAME, t);

			// store instance
			*titleInstanceSlot = inst;

			if (MM_TITLE_INSTANCES[(s16)instanceIndex].isTrophy)
			{
				inst->flags |= VISIBLE_DURING_GAMEPLAY;
			}

			CTR_WriteU32AlignedLE(&inst->matrix.m[0][0], FP_ONE);
			CTR_WriteU32AlignedLE(&inst->matrix.m[0][2], 0);
			CTR_WriteU32AlignedLE(&inst->matrix.m[1][1], FP_ONE);
			CTR_WriteU32AlignedLE(&inst->matrix.m[2][0], 0);

			inst->matrix.m[2][2] = TITLE_MATRIX_SCALE;
			inst->matrix.m[1][1] = TITLE_MATRIX_SCALE;
			inst->matrix.m[0][0] = TITLE_MATRIX_SCALE;

			inst->matrix.t[2] = 0;
			inst->matrix.t[1] = 0;
			inst->matrix.t[0] = 0;

			inst->flags |= HIDE_MODEL;

			for (playerIndex = 1; playerIndex < GAME_TRACKER->numPlyrCurrGame; playerIndex++)
			{
				playerDraw = INST_GETIDPP(inst);
				playerDraw += (s16)playerIndex;
				playerDraw->pushBuffer = 0;
			}
		}

		MM_Title_CameraMove(title, 0);
	}
}

void MM_Title_CameraReset(void)
{
	struct Title *title = MM_TITLE_OBJECT;

	if (title == NULL)
	{
		return;
	}

	title->cameraPosOffset.x = TITLE_CAMERA_RESET_X;
}
