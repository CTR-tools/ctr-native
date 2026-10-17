#ifndef CTR_NATIVE_OVR_230_H
#define CTR_NATIVE_OVR_230_H

enum
{
	SCRAP_INIT = 0,
	SCRAP_LOAD = 1,
	SCRAP_PLAY = 2,
	SCRAP_STOP = 3,
	SCRAP_EXIT = 4,
};
typedef s32 ScrapbookState;

CTR_STATIC_ASSERT(sizeof(ScrapbookState) == 0x4);

enum TransitionState
{
	ENTERING_MENU,
	IN_MENU,
	EXITING_MENU,
};

CTR_STATIC_ASSERT(sizeof(enum TransitionState) == 0x4);

enum TitleMenuState
{
	TITLE_MENU_STATE_INTRO = 0,
	TITLE_MENU_STATE_IN_MENU = 1,
	TITLE_MENU_STATE_EXITING = 2,
	TITLE_MENU_STATE_RETURNING = 3,
};

CTR_STATIC_ASSERT(sizeof(enum TitleMenuState) == 0x4);

enum
{
	MM_EXIT_ROUTE_ADV_NEW = 0,
	MM_EXIT_ROUTE_ADV_LOAD = 1,
	MM_EXIT_ROUTE_CHARACTER_SELECT = 2,
	MM_EXIT_ROUTE_HIGH_SCORE = 3,
	MM_EXIT_ROUTE_DEMO = 4,
	MM_EXIT_ROUTE_SCRAPBOOK = 5,
	MM_EXIT_ROUTE_QUICK_RACE = 6,
};
typedef s32 MainMenuExitRoute;

enum TitleIntroFrames
{
	TITLE_INTRO_MENU_READY_FRAME = 230,
	TITLE_INTRO_TM_DRAW_MIN_FRAME = TITLE_INTRO_MENU_READY_FRAME - 1,
	TITLE_INTRO_END_FRAME = 245,
	TITLE_INTRO_SKIP_FRAME = 1000,
};

enum TitleIntroConstants
{
	TITLE_INSTANCE_COUNT = 6,
	TITLE_INSTANCE_COUNT_JPN = 7,
	TITLE_INTRO_SKIP_INPUT = BTN_CROSS_one | BTN_CIRCLE | BTN_SQUARE_one | BTN_TRIANGLE,
	TITLE_DEMO_IDLE_FRAMES = CTR_SECONDS_TO_FRAMES(30),
	TITLE_DEMO_RACE_FRAMES = CTR_SECONDS_TO_FRAMES(60),
	TITLE_DEFAULT_DISTANCE_TO_SCREEN = 0x100,
	TITLE_INITIAL_EVENT_TIME = 0x2a300,
	TITLE_CAMERA_MOVE_FRAMES = 0xf,
	TITLE_TROPHY_HIDE_START_FRAME = 138,
	TITLE_TROPHY_HIDE_FRAMES = 62,
	TITLE_TROPHY_ANIM_START_FRAME = 200,
	TITLE_MATRIX_SCALE = 0x5000,
	TITLE_SPEC_LIGHT_Y = 0x1000,
	TITLE_DEMO_DRIVER_COUNT = 8,
	TITLE_DEMO_INDEX_MASK = TITLE_DEMO_DRIVER_COUNT - 1,
	TITLE_INTRO_DISTANCE_TO_SCREEN = 450,
	TITLE_CAMERA_RESET_X = 2000,

	TITLE_SOUND_COUNT = 8,
};

CTR_STATIC_ASSERT(sizeof(MainMenuExitRoute) == 0x4);

enum ScrapbookConstants
{
	SCRAPBOOK_STREAM_FRAMES = 0x1148,
	SCRAPBOOK_VIDEO_WIDTH = 0x200,
	SCRAPBOOK_VIDEO_HEIGHT = 0xd0,
	SCRAPBOOK_FRAME_VBLANKS = 4,
	SCRAPBOOK_SKIP_INPUT = TITLE_INTRO_SKIP_INPUT | BTN_START,
};


enum MainMenuCheatConstants
{
	MM_CHEAT_SUCCESS_SFX = 0x67,
	MM_CHEAT_BUTTON_HISTORY_COUNT = 10,

	MM_CHEAT_COUNT = 0x17,
};

enum CharacterSelectDirection
{
	CHARACTER_SELECT_DIR_UP = 0,
	CHARACTER_SELECT_DIR_DOWN = 1,
	CHARACTER_SELECT_DIR_LEFT = 2,
	CHARACTER_SELECT_DIR_RIGHT = 3,
	CHARACTER_SELECT_DIRECTION_COUNT = 4,
};

enum CharacterSelectUnlockRequirement
{
	MM_CHARACTER_UNLOCK_ALWAYS = -1,
};


enum MainMenuFlowConstants
{
	MM_TRANSITION_SWISH_SFX = 0x65,
	MM_TRANSITION_SWISH_FRAME = 4,
	MM_DEFAULT_LAP_COUNT = 3,
	MM_ONE_LAP_CHEAT_COUNT = 1,
	MM_MENU_RESET_DONE_FRAMES = 0xf,
	MM_PLAYER_1P2P_SELECTABLE_ROWS = 2,
	MM_PLAYER_1P2P3P4P_SELECTABLE_ROWS = 4,
	MM_PLAYER_2P3P4P_SELECTABLE_ROWS = 3,
	MM_RACE_TYPE_SELECTABLE_ROWS = 2,
	MM_ADV_NEW_LOAD_ROUTE_COUNT = 2,
	MM_DIFFICULTY_COUNT = 3,
	MM_CUP_TRACK_COUNT = 4,
	MM_TITLE_TM_X = 0x10e,
	MM_TITLE_TM_Y = 0x9c,
	MM_TITLE_TM_OT_INDEX = 3,

	MM_MENU_RESET_COUNT = 9,
};


enum MainMenuCupSelectConstants
{
	MM_CUP_SELECT_INITIAL_TRANSITION_FRAMES = 0xc,
	MM_CUP_SELECT_LERP_FRAMES = 8,
	MM_CUP_SELECT_TRANSITION_OUT_DONE_FRAME = 12,
	MM_CUP_SELECT_DRIVER_SLOT_COUNT = 8,
	MM_CUP_SELECT_TITLE_META_INDEX = 4,
	MM_CUP_SELECT_TITLE_X_OFFSET = 0x100,
	MM_CUP_SELECT_TITLE_Y_OFFSET = 0x10,
	MM_CUP_SELECT_COLUMN_WIDTH = 200,
	MM_CUP_SELECT_ROW_HEIGHT = 0x54,
	MM_CUP_SELECT_NAME_X_OFFSET = 0xa2,
	MM_CUP_SELECT_NAME_Y_OFFSET = 0x44,
	MM_CUP_SELECT_CONTENT_X_OFFSET = 0x4e,
	MM_CUP_SELECT_CONTENT_Y_OFFSET = 0x29,
	MM_CUP_SELECT_STAR_ICON_GROUP = 5,
	MM_CUP_SELECT_STAR_ICON_ID = 0x37,
	MM_CUP_SELECT_STAR_COLUMN_BIAS = 0xca,
	MM_CUP_SELECT_STAR_X_OFFSET = -0x16,
	MM_CUP_SELECT_STAR_Y_OFFSET = 0x10,
	MM_CUP_SELECT_STAR_Y_STEP = 0x10,
	MM_CUP_SELECT_TRACK_X_STEP = 0x54,
	MM_CUP_SELECT_TRACK_Y_STEP = 0x23,
	MM_CUP_SELECT_HIGHLIGHT_X_OFFSET = -3,
	MM_CUP_SELECT_HIGHLIGHT_Y_OFFSET = -2,
	MM_CUP_SELECT_HIGHLIGHT_WIDTH = 174,
	MM_CUP_SELECT_HIGHLIGHT_HEIGHT = 74,
	MM_CUP_SELECT_BACKGROUND_X_OFFSET = -6,
	MM_CUP_SELECT_BACKGROUND_Y_OFFSET = -4,
	MM_CUP_SELECT_BACKGROUND_WIDTH = 180,
	MM_CUP_SELECT_BACKGROUND_HEIGHT = 78,
	MM_CUP_SELECT_FLASH_FRAME_BIT = 2,
	MM_CUP_SELECT_FLASH_COLOR_BIT = 4,
	MM_CUP_SELECT_TEXT_COLOR = JUSTIFY_CENTER | ORANGE,
};


// MainMenu
struct Title
{
	// 0x0
	struct Thread *t;

	// 0x4
	struct Instance *i[TITLE_INSTANCE_COUNT];

	// 0x1c
	SVec3 cameraPosOffset;

	// 0x22
	s16 UnusedPadding;

	// 0x24 -- size of struct
};

CTR_STATIC_ASSERT(sizeof(struct Title) == 0x24);

struct TitleInstanceMeta
{
	s16 modelID;
	s16 animStartFrame;

	// Stored as TITLE_INTRO_MENU_READY_FRAME for every row; no use site is known.
	s16 unusedMenuReadyFrame;

	s16 isTrophy;
};

CTR_STATIC_ASSERT(sizeof(struct TitleInstanceMeta) == 0x8);

enum MMVideoFlags
{
	MM_VIDEO_FLAG_RGB24 = 0x1,
	MM_VIDEO_FLAG_HAS_XA_AUDIO = 0x2,
	MM_VIDEO_FLAG_LOOP = 0x4,
	MM_VIDEO_FLAG_SCRAPBOOK = 0x8,
};


enum MMVideoConstants
{
	MM_VIDEO_VLC_WAIT_FRAMES = 10,
	MM_VIDEO_STALL_RECOVERY_FRAMES = 400,
	MM_VIDEO_STALLED_BACKLOC_FRAMES = 0x40,
	MM_VIDEO_DEFAULT_RING_SECTORS = 0x40,
	MM_VIDEO_BACKLOC_NONE = -1,
	MM_VIDEO_DCT_OUTPUT_TIMEOUT_POLLS = 40000,
	MM_VIDEO_CD_READY_POLL_INTERVAL = 0x28,
	MM_VIDEO_GPU_IDLE_TIMEOUT = 10000,
};



enum MainMenuTrackUnlockRequirement
{
	MM_TRACK_UNLOCK_ALWAYS = -1,
	MM_TRACK_UNLOCK_1P_ONLY = -2,
};


// array at 0x800b53b0
struct MainMenu_LevelRow
{
	// 0 - dingo canyon
	// 3 - crash cove
	// etc
	s16 levID;

	// texture that shows before video plays
	s16 videoThumbnail;

	// which black+white map draws
	s16 mapTextureID;

	// MM_TRACK_UNLOCK_ALWAYS for unlock by default,
	// otherwise has a flag for what is needed.
	// Retail marks Oxide Station as 1P-only; native can lift this restriction.
	s16 unlock;


	// BIGFILE entry index for this track's preview video
	s32 previewVideoFileIndex;

	// how long preview video plays before looping
	s16 previewVideoFrameCount;
	s16 pad;

	// Complete 0x10-byte structure
};

struct CharacterSelectMeta
{
	// 0x0
	s16 posX;
	s16 posY;

	// 0x4
	// up, down, left, right
	u8 nextIconByDirection[CHARACTER_SELECT_DIRECTION_COUNT];

	// 0x8
	s16 characterID;
	u16 unlockFlags;

	// 0xC -- size
};

struct CupDifficultyTables
{
	// 0x00
	s16 firstUnlockBit[4];

	// 0x08
	s16 stringIndex[4];

	// 0x10
	s16 speed[4];
};

struct CharacterSelectLayoutTables
{
	// 0x00
	s16 windowW[6];

	// 0x0c
	s16 windowH[6];

	// 0x18
	s16 driverPosZ[6];

	// 0x24
	s16 driverPosY[6];

	// 0x30
	s16 textY[6];
};

struct CharacterSelectPlayerState
{
	// 0x00
	s16 modelMoveDir[4];

	// 0x08
	s16 desiredCharacterID[4];

	// 0x10
	s16 currentCharacterID[4];

	// 0x18
	s16 angle[4];
};

struct CharacterSelectDriverModelConfig
{
	// 0x00
	SVec3 pos;
	s16 _pad_pos;

	// 0x08
	SVec3 rot;
	s16 moveFrames;

	// 0x10
	s32 slideDistance;
};

struct TimeTrialStarTables
{
	// 0x00
	u16 colorIndex[2];

	// 0x04
	u16 beatenFlagBit[2];
};

struct CupSelectStarTables
{
	// 0x00
	u16 colorIndex[4];

	// 0x08
	s16 winBitBase[4];
};

struct BattleSetupTables
{
	// 0x00
	u32 typeModeFlags[3];

	// 0x0c
	u8 timeLimitMinutes[4];

	// 0x10
	s8 lifeModeTimeLimitMinutes[4];

	// 0x14
	u8 lifeLimitValues[4];

	// 0x18
	u8 pointLimitValues[4];
};

struct HighScoreGhostStarTables
{
	// 0x00
	u16 colorIndex[2];

	// 0x04
	u16 beatenFlagBit[2];
};

struct LapCountMenuRow
{
	// 0x00
	u8 lapCount;

	// 0x01
	u8 padding;
};

struct HighScoreSelectionState
{
	// 0x00
	s16 targetTrack;

	// 0x02
	s16 targetRow;

	// 0x04
	s16 currentTrack;

	// 0x06
	s16 currentRow;
};

struct HighScoreTransitionState
{
	// 0x00
	s16 state;

	// 0x02
	s16 mainFrame;
	s16 trackFrame;
	s16 rowFrame;

	// 0x08
	// -1 for negative direction and 1 for positive direction.
	s16 activeHorizontalMove;
	s16 pendingHorizontalMove;

	// 0x0c
	s16 activeVerticalMove;
	s16 pendingVerticalMove;
};

struct MenuExitTransitionState
{
	// 0x00
	s16 state;

	// 0x02
	s16 startAfterExit;

	// 0x04
	s16 frame;
};

struct TrackSelectRuntimeState
{
	// 0x00
	s32 trackChangeFrames;

	// 0x04
	s16 currentTrack;

	// 0x06
	s16 trackChangeDirection;

	// 0x08
	s16 lapBoxOpen;

	// 0x0a
	struct MenuExitTransitionState transition;

	// 0x10
	s16 videoPreviewFrames;

	// 0x12
	s16 videoMemAllocated;

	// 0x14
	s16 videoStateCurr;

	// 0x16
	s16 videoStatePrev;
};

struct TrackSelectMapOffset
{
	s16 offsetX;
	s16 offsetY;
	u8 type;
	u8 pad;
};

struct TransitionMeta
{
	s16 distX;
	s16 distY;

	// change when each one starts to move
	s16 headStart;

	s16 currX;
	s16 currY;

	// 0xA -- size
};

enum TitleMenuPositionIndex
{
	TITLE_MENU_POS_MAIN = 0,
	TITLE_MENU_POS_ADVENTURE = 1,
	TITLE_MENU_POS_RACE_TYPE = 2,
	TITLE_MENU_POS_PLAYERS = 3,
	TITLE_MENU_POS_DIFFICULTY = 4,
	TITLE_MENU_POS_COUNT = 5,
};

enum TitleTransitionIndex
{
	TITLE_TRANSITION_MAIN = 0,
	TITLE_TRANSITION_ADVENTURE = 1,
	TITLE_TRANSITION_RACE_TYPE = 2,
	TITLE_TRANSITION_PLAYERS = 3,
	TITLE_TRANSITION_DIFFICULTY = 4,
	TITLE_TRANSITION_CAMERA_XY = 5,
	TITLE_TRANSITION_CAMERA_Z = 6,
	TITLE_TRANSITION_END = 7,
	TITLE_TRANSITION_COUNT = 8,
};

struct TitleMenuPositionLayout
{
	SVec2 menuPos[TITLE_MENU_POS_COUNT];
	SVec3 baseCameraPos;
	s16 pad_baseCameraPos;
};

struct TitleTransitionLayout
{
	s32 durationFrames;
	s16 step;
	s16 pad_step;
	struct TitleMenuPositionLayout menuLayout;
	struct TransitionMeta transition[TITLE_TRANSITION_COUNT];
	char padding[0x10];
};

struct BattleWeaponMenuItem
{
	u32 enabledWeaponFlag;
	s16 iconID;
	s16 pad_iconID;
};

CTR_STATIC_ASSERT(sizeof(struct BattleWeaponMenuItem) == 0x8);

struct TitleSoundCue
{
	s16 frameToPlay;
	u16 soundID;
};

CTR_STATIC_ASSERT(sizeof(struct TitleSoundCue) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct TitleSoundCue, frameToPlay) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct TitleSoundCue, soundID) == 0x2);

struct TitleCameraPathFrame
{
	SVec3 pos;
	SVec3 rot;
};

CTR_STATIC_ASSERT(sizeof(struct TitleCameraPathFrame) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct TitleCameraPathFrame, pos) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct TitleCameraPathFrame, rot) == 0x6);

struct MainMenuCheatCode
{
	s32 buttonCount;
	u32 buttons[MM_CHEAT_BUTTON_HISTORY_COUNT];
	void (*handler)(void);
};

CTR_STATIC_ASSERT(sizeof(struct MainMenuCheatCode) == 0x30);
CTR_STATIC_ASSERT(offsetof(struct MainMenuCheatCode, buttonCount) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct MainMenuCheatCode, buttons) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct MainMenuCheatCode, handler) == 0x2c);

// 800ab9f0
struct OverlayRDATA_230
{
	// (tag given by compiler, meaningless to game)
	// do not compile this "tag" in the struct when we're actually building the overlay
	s32 overlayTag;

	// 800ab9f4
	u32 jmpPtrs_Title_MenuUpdate[6];

	// 800aba0c
	// "title"
	char s_title[8];

	// 800aba14
	// 0,1,2,3,4,5,6,7
	u32 packedDefaultCharacterIDWords[2];

	// strings for the player numbers in the character select screen
	//

	// "4"
	char s_4[4];
	// "3"
	char s_3[4];
	// "2"
	char s_2[4];
	// "1"
	char s_1[4];

	// 800aba2c
	u32 jmpPtrs_Characters_MenuProc[6];

	// 800aba44
	char s_loaded_ghost_data[0x18];

	// 800aba6c
	u32 jmpPtrs_Battle_MenuProc[11];

	// 800aba88
	// " test.str 1"
	char s_teststr1[12];

	// 800aba94
	u32 jmpPtrs_Scrapbook_PlayMovie[5];
};

// 800b44e4
struct OverlayDATA_230
{
	// =========== Main Menu CONST =============

	struct MenuRow rowsMainMenuBasic[9];
	char padding800b450E[2];

	struct MenuRow rowsMainMenuWithScrapbook[10];
	struct MenuRow rowsQuitConfirm[3];
	struct RectMenu menuQuitConfirm;

	// 800b4540
	struct RectMenu menuMainMenu;

	// 800b456c
	struct MenuRow rowsPlayers1P2P[MM_PLAYER_1P2P_SELECTABLE_ROWS + 1];
	struct MenuRow rowsPlayers1P2P3P4P[MM_PLAYER_1P2P3P4P_SELECTABLE_ROWS + 1];

	// ????
	char padding800b4cf2[2];

	// 800b4580
	struct RectMenu menuPlayers1P2P;

	// 800b45ac
	struct MenuRow rowsPlayers2P3P4P[MM_PLAYER_2P3P4P_SELECTABLE_ROWS + 1];

	// 800b45c4
	struct RectMenu menuPlayers2P3P4P;

	// 800b45f0
	struct MenuRow rowsDifficulty[4];

	// 800b4608
	struct RectMenu menuDifficulty;

	// 800b4634
	struct MenuRow rowsRaceType[3];

	char padding800b4dba[2];

	// 800b4648
	struct RectMenu menuRaceType;

	// 800b4674
	struct MenuRow rowsAdventure[3];

	// ???
	char padding800b4dfa[2];

	// 800b4688
	struct RectMenu menuAdventure;


	// 800b46b4
	struct RectMenu menuCharacterSelect;

	// 800b46e0
	struct RectMenu menuTrackSelect;

	// 800b470c
	struct MenuRow rowsCupSelect[5];

	// ????
	char padding800b4f02[2];

	// 800b472c
	struct RectMenu menuCupSelect;

	// 800b4758
	struct RectMenu menuBattleWeapons;

	// 800b4784
	struct RectMenu menuHighScores;

	// 800b47b0
	struct RectMenu menuScrapbook;

	// 800b47dc
	// array of menu pointers
	struct RectMenu *arrayMenuPtrs[MM_MENU_RESET_COUNT];


	// 800B4800
	struct TitleInstanceMeta titleInstances[TITLE_INSTANCE_COUNT];

	// 800B4830
	SVec3 titleCameraPos;
	s16 _pad_titleCameraPos;
	SVec3 titleCameraRot;
	s16 _pad_titleCameraRot;

	// 800B4840
	// random stuff related to the title animation,
	// come up with better names later

	// Full block is 0x84 bytes

	// 800B4840
	struct TitleTransitionLayout titleTransition;


	// 800b48c4
	struct TitleSoundCue titleSounds[8];

	// 800b48e4
	struct MainMenuCheatCode cheats[MM_CHEAT_COUNT];

	// 800B4D04
	u32 cheatButtonHistory[MM_CHEAT_BUTTON_HISTORY_COUNT];

	// 800B4D2C
	struct CupDifficultyTables cupDifficulty;

	// ============= Character Select CONST ================

	// 800B4D44
	SVec2 characterSelectWindowPos[0xD];

	// pointer
	// 800b4d78
	SVec2 *characterSelectWindowPosByLayout[6];

	// 800B4D90
	struct CharacterSelectLayoutTables characterSelectLayout;

	// 800b4dcc
	struct CharacterSelectMeta characterSelectMeta1P2PLimited[0x10];

	// 800b4e80
	struct CharacterSelectMeta characterSelectMeta1P2P[0x10];

	// 800b4f34
	struct CharacterSelectMeta characterSelectMeta3P[0x10];

	// 800b4fe8
	struct CharacterSelectMeta characterSelectMeta4P[0x10];

	// 800b509C
	struct CharacterSelectMeta *characterSelectMetaByLayout[6];

	// 800b50B4
	// the character select menu has a different order for playable characters
	// this array contains the IDs used for each character in the character select menu
	// each member of the array corresponds to the character order used in the rest of the game, see enum Characters
	s16 characterMenuID[0x10];

	// 0x16 for transition meta array:
	// 16 character icons + title text + 4 kart screens + one terminator.

	// 800b50D4
	// 1P/2P mode
	struct TransitionMeta characterSelectTransition1P2P[0x16];

	// 0x2 byte padding
	s16 padding800b51A6;

	// 3P mode
	// 800b51A8
	struct TransitionMeta characterSelectTransition3P[0x16];

	// 0x2 byte padding
	s16 padding800B527A;

	// 4P mode
	// 800b527c
	struct TransitionMeta characterSelectTransition4P[0x16];

	// 0x2 byte padding
	s16 padding800B534E;

	// 800B5350
	struct TransitionMeta *characterSelectTransitionByPlayerCount[4];

	// 800B5360
	struct CharacterSelectDriverModelConfig characterSelectDriverModel;

	// 800b5374
	// points to s_1, s_2, s_3, s_4
	char *playerNumberStrings[4];

	// 800b5384
	u8 characterSelectFallbackDirection1[CHARACTER_SELECT_DIRECTION_COUNT];

	// 800b5388
	u8 characterSelectFallbackDirection2[CHARACTER_SELECT_DIRECTION_COUNT];

	// 800b538c
	Color characterSelect_Outline;

	// 800b5390
	Color characterSelect_NeutralColor;

	// 800b5394
	Color characterSelect_ChosenColor;

	// 800b5398
	char characterSelect_BlueRectColors[0x18];
	// u32 characterSelect_BlueRectColors[6];

	// =========== Track Select CONST ============

	// 800b53b0
	struct MainMenu_LevelRow arcadeTracks[0x12];

	// 800b54d0
	struct MainMenu_LevelRow battleTracks[0x7];

	// 800b5540
	union
	{
		struct TransitionMeta transitionMeta_trackSel[5];
		struct
		{
			struct TransitionMeta trackSelect_rowListTransition;
			struct TransitionMeta trackSelect_previewTransition;
			struct TransitionMeta trackSelect_lapMenuTransition;
			struct TransitionMeta trackSelect_titleTransition;
			struct TransitionMeta trackSelect_transitionEnd;
		} named;
	} trackTransitions;
	s16 padding_800B5572;

	// 800b5574
	struct LapCountMenuRow lapCountByRow[4];

	// 800b557c
	struct MenuRow rowsLapSel[4];

	// 800B5594
	struct RectMenu menuLapSel;

	// 800B55C0
	Color videoCol;

	// 800B55C4
	struct TimeTrialStarTables timeTrialStars;


	// 800b55cc
	struct TrackSelectMapOffset drawMapOffset[6];

	// ============== Cup Select ==================

	// 800b55f0
	struct TransitionMeta transitionMeta_cupSel[0x6];

	// 800b562c
	struct CupSelectStarTables cupSelectStars;

	// 800b563c
	Color cupSel_Color;

	// ============= Battle CONST ================

	// 800b5640
	struct TransitionMeta transitionMeta_battle[0xB];

	// 2 byte padding
	s16 padding800b56ae;

	// 800b56b0
	struct MenuRow rowsBattleType[4];

	// 800b56c8
	struct RectMenu menuBattleType;

	// 800b56f4
	struct MenuRow rowsBattleLengthLifeTime[4];

	// 800b570c
	struct RectMenu menuBattleLengthLifeTime;

	// 800b5738
	struct MenuRow rowsBattleLengthTimeTime[4];

	// 800b5750
	struct RectMenu menuBattleLengthTimeTime;

	// 800b577c
	struct MenuRow rowsBattleLengthPoints[4];

	// 800b5794
	struct RectMenu menuBattleLengthPoints;

	// 800b57c0
	struct MenuRow rowsBattleLengthLifeLife[4];

	// 800b57d8
	struct RectMenu menuBattleLengthLifeLife;

	// 800b5804
	struct MenuRow rowsBattleStartGame[2];

	// 800b5810
	struct RectMenu menuBattleStartGame;

	// 800b583c
	struct RectMenu *battleMenuArray[5];

	// 800b5850
	struct BattleWeaponMenuItem battleWeaponItems[11];

	// 800b58a8
	struct BattleSetupTables battleSetupTables;

	// 800b58c4
	Color battleWeaponEnabledColor;
	Color battleWeaponDisabledColor;
	Color battleWeaponPanelColor;

	// ================ High Score CONST ==================

	// 800b58d0
	struct TransitionMeta transitionMeta_HighScores[0xC];

	// 800b5948
	struct HighScoreGhostStarTables highScoreGhostStars;

	// 800b5950
	Color highscore_iconColor;

	// 800b5954
	struct MenuRow rowsHighScore[4];

	// 800b596c
	struct RectMenu menuHighScore;

	// 800B5998
	struct HighScoreSelectionState highScoreSelection;


	// 800b59a0
	struct Title *titleObj;

	// ============== Track Select DYN ====================

	// 800b59a4
	struct TrackSelectRuntimeState trackSelect;

	// ============== Cup Select DYN ========================

	// 800b59bc
	struct MenuExitTransitionState cupSelectTransition;

	// ============== Battle DYN ==========================

	// 800b59c2
	struct MenuExitTransitionState battleTransition;

	// =============== High Score DYN ===================

	// 800b59c8
	struct HighScoreTransitionState highScoreTransition;

	// =============== Scrapbook ===================

	// 800b59d8
	ScrapbookState scrapbookState;

	// ============= Character Select DYN ====================

	// 800b59dc
	s16 characterSelectWindowHeight;
	s16 pad_afterCharacterSelectWindowHeight;

	// 800b59e0
	MainMenuExitRoute desiredMenuIndex;

	// 800b59e4
	s16 characterSelectExitsForward;
	s16 pad_afterCharacterSelectExitsForward;

	// 800b59e8
	struct CharacterSelectPlayerState characterSelectPlayerState;

	// 800b5a08
	enum TransitionState characterSelectTransitionState;

	// 800b5a0c
	SVec2 *activeCharacterSelectWindowPos;

	// 800b5a10
	s32 characterSelectLayoutIndex;

	// 800b5a14
	s32 titleIntroFrame;

	// 800b5a18
	struct CharacterSelectMeta *activeCharacterSelectMeta;

	// 800b5a1c
	enum TitleMenuState titleMenuState;

	// 800b5a20
	// 0 = transitioning in, 1 = in focus/still, 2 = transitioning out
	enum TransitionState characterSelectMenuState;

	// 800b5a24
	s16 characterSelectModelMoveTimer[4];


	// 800b5a2c
	b32 characterSelectRosterExpanded;

	// 800b5a30
	u16 characterSelectWindowWidth;
	u16 padding800b5a32;

	// 800b5a34
	struct TitleCameraPathFrame *titleIntroCameraPath;

	// 800b5a38
	s16 characterSelectNameTextY;
	u16 padding800b5a3a;

	// 800b5a3c
	struct TransitionMeta *characterSelectTransitionMeta;

	// 800b5a40
	s32 titleMenuTransitionFrame;

	// 800b5a44
	s32 characterSelectTransitionFrame;

	// ================= Video RDATA ===========================

	// 800b5a48
	char s_SliceBuf[0xC];
	char s_VlcBuf[8];
	char s_RingBuf[8];

	// 800b5a64 MM_Video_DecDCToutCallbackFunc
};

// starts at 800b67ac
struct OVR_230_VideoBSS
{
	s32 loopStartBackloc;
	s32 loopEndBackloc;
	s32 loopWrapPending;
	s32 cdRetryState;

	// 800b67bc
	u16 finalSliceIndex;

	// 800b67be
	u16 sliceIndex;

	// 800b67c0
	u16 dctMode;

	// 800b67c2
	u16 drawNextFrame;

	// 800b67c4
	u16 endOfStream;
	s16 decodeState;

	// 800b67c8
	u16 vlcBufferIndex;

	// 800b67ca
	u16 dctOutBufferIndex;

	// 800b67cc
	s16 cdKickState;
	u16 stalledBacklocFrames;

	// 800b67d0
	u16 stallRecoveryFrames;
	u16 pad_stallRecoveryFrames;

	// 800b67d4
	s32 streamFrameCount;

	// 800b67d8
	s32 sectorFrameCount;
	s32 lastSectorFrameCount;
	s32 lastBackloc;
	s32 unused_0x38;

	// 800b67e8
	// 1 - IS_RGB24
	// 2 - has Audio
	// 4 - is looping
	// 8 - is Scrapbook (?)
	u32 flags;

	// 800b67ec
	s32 ringSectorCount;

	// 800b67f0
	volatile u32 dctOutputDone;

	s32 vlcBufferSize;

	char pad800b67f8[4];

	// 30,31...
	// 800b67fc
	u32 *in_Buf[2];

	s32 dctOutSliceSize;

	char pad800b6808[4];

	// 800b680c
	u32 *out_Buf[4];

	// 800b681c
	RECT slice;

	// 800b6824
	CdlLOC cdLocation1;

	// 800b6828
	CdlLOC cdLocation2;

	// 800b682c
	CdlLOC cdLocation3;

	// 800b6830
	CdlLOC *ptrCdLoc;
};

CTR_STATIC_ASSERT(sizeof(struct OVR_230_VideoBSS) == 0x88);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, loopStartBackloc) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, loopEndBackloc) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, loopWrapPending) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, cdRetryState) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, finalSliceIndex) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, sliceIndex) == 0x12);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, dctMode) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, dctOutBufferIndex) == 0x1e);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, stallRecoveryFrames) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, streamFrameCount) == 0x28);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, sectorFrameCount) == 0x2c);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, unused_0x38) == 0x38);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, ringSectorCount) == 0x40);
CTR_STATIC_ASSERT(offsetof(struct OVR_230_VideoBSS, dctOutputDone) == 0x44);

extern struct OverlayRDATA_230 R230;
extern struct OverlayDATA_230 D230;
extern struct OVR_230_VideoBSS V230;


CTR_STATIC_ASSERT(sizeof(struct MainMenu_LevelRow) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct MainMenu_LevelRow, previewVideoFileIndex) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct MainMenu_LevelRow, previewVideoFrameCount) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct OverlayRDATA_230, overlayTag) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct OverlayRDATA_230, s_title) == 0x1c);
CTR_STATIC_ASSERT(offsetof(struct OverlayRDATA_230, packedDefaultCharacterIDWords) == 0x24);
CTR_STATIC_ASSERT(sizeof(struct OverlayRDATA_230) == 0xb8);
CTR_STATIC_ASSERT(sizeof(struct CharacterSelectMeta) == 0xC);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectMeta, nextIconByDirection) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectMeta, characterID) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectMeta, unlockFlags) == 0xa);
CTR_STATIC_ASSERT(sizeof(struct CupDifficultyTables) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct CupDifficultyTables, firstUnlockBit) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct CupDifficultyTables, stringIndex) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct CupDifficultyTables, speed) == 0x10);
CTR_STATIC_ASSERT(sizeof(struct CharacterSelectLayoutTables) == 0x3c);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectLayoutTables, windowW) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectLayoutTables, windowH) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectLayoutTables, driverPosZ) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectLayoutTables, driverPosY) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectLayoutTables, textY) == 0x30);
CTR_STATIC_ASSERT(sizeof(struct CharacterSelectPlayerState) == 0x20);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectPlayerState, modelMoveDir) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectPlayerState, desiredCharacterID) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectPlayerState, currentCharacterID) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectPlayerState, angle) == 0x18);
CTR_STATIC_ASSERT(sizeof(struct CharacterSelectDriverModelConfig) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectDriverModelConfig, pos) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectDriverModelConfig, rot) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectDriverModelConfig, moveFrames) == 0xe);
CTR_STATIC_ASSERT(offsetof(struct CharacterSelectDriverModelConfig, slideDistance) == 0x10);
CTR_STATIC_ASSERT(sizeof(struct TimeTrialStarTables) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct TimeTrialStarTables, colorIndex) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct TimeTrialStarTables, beatenFlagBit) == 0x4);
CTR_STATIC_ASSERT(sizeof(struct CupSelectStarTables) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct CupSelectStarTables, colorIndex) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct CupSelectStarTables, winBitBase) == 0x8);
CTR_STATIC_ASSERT(sizeof(struct BattleSetupTables) == 0x1c);
CTR_STATIC_ASSERT(offsetof(struct BattleSetupTables, typeModeFlags) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct BattleSetupTables, timeLimitMinutes) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct BattleSetupTables, lifeModeTimeLimitMinutes) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct BattleSetupTables, lifeLimitValues) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct BattleSetupTables, pointLimitValues) == 0x18);
CTR_STATIC_ASSERT(sizeof(struct HighScoreGhostStarTables) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct HighScoreGhostStarTables, colorIndex) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct HighScoreGhostStarTables, beatenFlagBit) == 0x4);
CTR_STATIC_ASSERT(sizeof(struct LapCountMenuRow) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct LapCountMenuRow, lapCount) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct LapCountMenuRow, padding) == 0x1);
CTR_STATIC_ASSERT(sizeof(struct HighScoreSelectionState) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct HighScoreSelectionState, targetTrack) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct HighScoreSelectionState, targetRow) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct HighScoreSelectionState, currentTrack) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct HighScoreSelectionState, currentRow) == 0x6);
CTR_STATIC_ASSERT(sizeof(struct HighScoreTransitionState) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct HighScoreTransitionState, state) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct HighScoreTransitionState, mainFrame) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct HighScoreTransitionState, trackFrame) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct HighScoreTransitionState, rowFrame) == 0x6);
CTR_STATIC_ASSERT(offsetof(struct HighScoreTransitionState, activeHorizontalMove) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct HighScoreTransitionState, pendingHorizontalMove) == 0xa);
CTR_STATIC_ASSERT(offsetof(struct HighScoreTransitionState, activeVerticalMove) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct HighScoreTransitionState, pendingVerticalMove) == 0xe);
CTR_STATIC_ASSERT(sizeof(struct MenuExitTransitionState) == 0x6);
CTR_STATIC_ASSERT(offsetof(struct MenuExitTransitionState, state) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct MenuExitTransitionState, startAfterExit) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct MenuExitTransitionState, frame) == 0x4);
CTR_STATIC_ASSERT(sizeof(struct TrackSelectRuntimeState) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct TrackSelectRuntimeState, trackChangeFrames) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct TrackSelectRuntimeState, currentTrack) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct TrackSelectRuntimeState, trackChangeDirection) == 0x6);
CTR_STATIC_ASSERT(offsetof(struct TrackSelectRuntimeState, lapBoxOpen) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct TrackSelectRuntimeState, transition) == 0xa);
CTR_STATIC_ASSERT(offsetof(struct TrackSelectRuntimeState, videoPreviewFrames) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct TrackSelectRuntimeState, videoMemAllocated) == 0x12);
CTR_STATIC_ASSERT(offsetof(struct TrackSelectRuntimeState, videoStateCurr) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct TrackSelectRuntimeState, videoStatePrev) == 0x16);
CTR_STATIC_ASSERT(sizeof(struct TransitionMeta) == 0xA);
CTR_STATIC_ASSERT(sizeof(struct TitleMenuPositionLayout) == 0x1c);
CTR_STATIC_ASSERT(sizeof(struct TitleTransitionLayout) == 0x84);
#endif
