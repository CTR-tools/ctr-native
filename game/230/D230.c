#include <common.h>

#define CHEAT_N BTN_UP
#define CHEAT_U BTN_UP
#define CHEAT_S BTN_DOWN
#define CHEAT_D BTN_DOWN
#define CHEAT_W BTN_LEFT
#define CHEAT_L BTN_LEFT
#define CHEAT_E BTN_RIGHT
#define CHEAT_R BTN_RIGHT
#define CHEAT_A BTN_TRIANGLE
#define CHEAT_O BTN_CIRCLE
#define CHEAT_X BTN_CROSS_one

struct OverlayDATA_230 D230 CTR_PSX_MATCH_SECTION(".D230") =
    {
        // MAIN MENU CONSTS

    .rowsMainMenuBasic =
        {
            {0x4C, 7, 1, 0, 0},
            {0x4D, 0, 2, 1, 1},
            {0x4E, 1, 3, 2, 2},
            {0x4F, 2, 4, 3, 3},
            {0x50, 3, 5, 4, 4},
            {0x51, 4, 6, 5, 5},
            {0x0E, 5, 7, 6, 6},
            {0x003, 6, 0, 7, 7},
            {RECTMENU_STRING_NONE},
        },

    .rowsMainMenuWithScrapbook =
        {
            {0x4C, 8, 1, 0, 0},
            {0x4D, 0, 2, 1, 1},
            {0x4E, 1, 3, 2, 2},
            {0x4F, 2, 4, 3, 3},
            {0x50, 3, 5, 4, 4},
            {0x51, 4, 6, 5, 5},
            {0x234, 5, 7, 6, 6},
            {0x0E, 6, 8, 7, 7},
            {0x003, 7, 0, 8, 8},
            {RECTMENU_STRING_NONE},
        },

        .menuMainMenu =
            {
                .stringIndexTitle = RECTMENU_STRING_NONE,

                .posX_curr = 0x180,
                .posY_curr = 0x6c,

                .state = EXECUTE_FUNCPTR | CENTER_ON_COORDS,
                .rows = &D230.rowsMainMenuBasic[0],
                .funcPtr = MM_MenuProc_Main,

            },

        .rowsPlayers1P2P =
            {
                {0x54, 0, 1, 0, 0},
                {0x55, 0, 1, 1, 1},
                {RECTMENU_STRING_NONE},
            },

        .rowsPlayers1P2P3P4P =
            {
                {0x54, 0, 1, 0, 0},
                {0x55, 0, 2, 1, 1},
                {0x56, 1, 3, 2, 2},
                {0x57, 2, 3, 3, 3},
                {RECTMENU_STRING_NONE},
            },

        .menuPlayers1P2P =
            {
                .stringIndexTitle = LNG_PLAYERS,

                .state = CENTER_ON_X,
                .rows = &D230.rowsPlayers1P2P[0],
                .funcPtr = MM_MenuProc_1p2p,
            },

        // Either direction swaps between the two rows, so neither is a dead end.
        .rowsChaosPrompt =
            {
                {LNG_EXPANDED_CHAOS, 1, 1, 0, 0},
                {LNG_EXPANDED_REGULAR, 0, 0, 1, 1},
                {RECTMENU_STRING_NONE},
            },

        .menuChaosPrompt =
            {
                .stringIndexTitle = LNG_EXPANDED_MODE,

                .state = CENTER_ON_X,
                .rows = &D230.rowsChaosPrompt[0],
                .funcPtr = MM_MenuProc_Chaos,
            },

        .rowsPlayers2P3P4P =
            {
                {0x55, 0, 1, 0, 0},
                {0x56, 0, 2, 1, 1},
                {0x57, 1, 2, 2, 2},
                {RECTMENU_STRING_NONE},
            },

        .menuPlayers2P3P4P =
            {
                .stringIndexTitle = LNG_PLAYERS,

                .state = CENTER_ON_X,
                .rows = &D230.rowsPlayers2P3P4P[0],
                .funcPtr = MM_MenuProc_2p3p4p,
            },

        .rowsDifficulty =
            {
                {0x15A, 0, 1, 0, 0},
                {0x15B, 0, 2, 1, 1},
                {0x15C, 1, 2, 2, 2},
                {RECTMENU_STRING_NONE},
            },

        .menuDifficulty =
            {
                .stringIndexTitle = LNG_DIFFICULTY,

                .state = CENTER_ON_X,
                .rows = &D230.rowsDifficulty[0],
                .funcPtr = MM_MenuProc_Difficulty,
            },

        .rowsRaceType =
            {
                {0x15E, 0, 1, 0, 0},
                {0x15F, 0, 1, 1, 1},
                {RECTMENU_STRING_NONE},
            },

        .menuRaceType =
            {
                .stringIndexTitle = LNG_RACE_TYPE,

                .state = CENTER_ON_X,
                .rows = &D230.rowsRaceType[0],
                .funcPtr = MM_MenuProc_SingleCup,
            },

        .rowsAdventure =
            {
                {0x8d, 0, 1, 0, 0},
                {0x8e, 0, 1, 1, 1},
                {RECTMENU_STRING_NONE},
            },

        .menuAdventure =
            {
                .stringIndexTitle = RECTMENU_STRING_NONE,

                .state = CENTER_ON_X,
                .rows = &D230.rowsAdventure[0],
                .funcPtr = MM_MenuProc_NewLoad,
            },
        .menuCharacterSelect =
            {
                .stringIndexTitle = RECTMENU_STRING_NONE,

                .state = DISABLE_INPUT_ALLOW_FUNCPTRS,
                .funcPtr = MM_Characters_MenuProc,
            },

        .menuTrackSelect =
            {
                .stringIndexTitle = RECTMENU_STRING_NONE,

                .state = DISABLE_INPUT_ALLOW_FUNCPTRS,
                .funcPtr = MM_TrackSelect_MenuProc,
            },

        .rowsCupSelect = {{0, 0, 2, 0, 1}, {0, 1, 3, 0, 1}, {0, 0, 2, 2, 3}, {0, 1, 3, 2, 3}, {RECTMENU_STRING_NONE}},

        .menuCupSelect =
            {
                .stringIndexTitle = RECTMENU_STRING_NONE,

                .state = INVISIBLE | EXECUTE_FUNCPTR,
                .rows = &D230.rowsCupSelect[0],
                .funcPtr = MM_CupSelect_MenuProc,
            },

        .menuBattleWeapons =
            {
                .stringIndexTitle = RECTMENU_STRING_NONE,

                .state = DISABLE_INPUT_ALLOW_FUNCPTRS,
                .funcPtr = MM_Battle_MenuProc,
            },

        .menuHighScores =
            {
                .stringIndexTitle = RECTMENU_STRING_NONE,

                .state = DISABLE_INPUT_ALLOW_FUNCPTRS,
                .funcPtr = MM_HighScore_MenuProc,
            },

        .menuScrapbook =
            {
                .stringIndexTitle = RECTMENU_STRING_NONE,

                .state = DISABLE_INPUT_ALLOW_FUNCPTRS,
                .funcPtr = MM_Scrapbook_PlayMovie,
            },

        .arrayMenuPtrs = {&D230.menuMainMenu, &D230.menuPlayers1P2P, &D230.menuPlayers2P3P4P, &D230.menuAdventure, &D230.menuCharacterSelect,
                          &D230.menuTrackSelect, &D230.menuCupSelect, &D230.menuBattleWeapons, &D230.menuHighScores},

        .titleInstances =
            {
                {.modelID = 0x68, .animStartFrame = 0x4e, .unusedMenuReadyFrame = TITLE_INTRO_MENU_READY_FRAME, .isTrophy = 0},
                {.modelID = 0x66, .animStartFrame = 0, .unusedMenuReadyFrame = TITLE_INTRO_MENU_READY_FRAME, .isTrophy = 1},
                {.modelID = 0x67, .animStartFrame = 0x14, .unusedMenuReadyFrame = TITLE_INTRO_MENU_READY_FRAME, .isTrophy = 0},
                {.modelID = 0x69, .animStartFrame = 0x4e, .unusedMenuReadyFrame = TITLE_INTRO_MENU_READY_FRAME, .isTrophy = 0},
                {.modelID = 0x6a, .animStartFrame = 0x8c, .unusedMenuReadyFrame = TITLE_INTRO_MENU_READY_FRAME, .isTrophy = 0},
                {.modelID = 0x6b, .animStartFrame = 0xd7, .unusedMenuReadyFrame = TITLE_INTRO_MENU_READY_FRAME, .isTrophy = 0},
            },

        .titleCameraPos = {0x32, 0xFFE2, 0x64},
        .titleCameraRot = {0, 0xFF9C, 0},

        .titleTransition =
            {
                .durationFrames = 0xC,
                .step = 8,
                .menuLayout =
                    {
                        .menuPos =
                            {
                                {0x180, 0x6c},
                                {0, 0},
                                {0, 0},
                                {0, 0},
                                {0, 0},
                            },
                        .baseCameraPos = {0x32, 0xFFE2, 0x64},
                    },
                .transition =
                    {
                        {512, 0, 0, 0, 0},
                        {0, 0, 1, 0, 0},
                        {0, 0, 1, 0, 0},
                        {0, 0, 2, 0, 0},
                        {0, 0, 3, 0, 0},
                        {0x4B0, 0xFFE2, 0, 0, 0},
                        {0x64, 0xC8, 0, 0, 0},
                        {0, 0, -1, 0, 0},
                    },
            },

        .titleSounds =
            {
                {0x94, 0x9B},
                {0x9E, 0x9C},
                {0xAA, 0x9D},
                {0xB4, 0x9E},
                {0x6C, 0x9F},
                {0x49, 0xA0},
                {0x84, 0xA1},
                {0xC8, 0xA2},
            },

        .cheats =
            {
                {4, {CHEAT_S, CHEAT_O, CHEAT_A, CHEAT_R}, MainKillGame_LaunchSpyro2},
                {5, {CHEAT_S, CHEAT_E, CHEAT_E, CHEAT_D, CHEAT_S}, MM_Cheat_MaxWumpa},
                {7, {CHEAT_R, CHEAT_O, CHEAT_O, CHEAT_D, CHEAT_U, CHEAT_D, CHEAT_E}, MM_Cheat_UnlockRoo},
                {9, {CHEAT_L, CHEAT_A, CHEAT_R, CHEAT_D, CHEAT_R, CHEAT_O, CHEAT_L, CHEAT_L, CHEAT_S}, MM_Cheat_UnlockPapu},
                {7, {CHEAT_D, CHEAT_O, CHEAT_L, CHEAT_L, CHEAT_A, CHEAT_R, CHEAT_S}, MM_Cheat_UnlockJoe},
                {6, {CHEAT_L, CHEAT_E, CHEAT_A, CHEAT_D, CHEAT_E, CHEAT_D}, MM_Cheat_UnlockPinstripe},
                {9, {CHEAT_O, CHEAT_D, CHEAT_D, CHEAT_N, CHEAT_O, CHEAT_O, CHEAT_D, CHEAT_L, CHEAT_E}, MM_Cheat_UnlockFakeCrash},
                {8, {CHEAT_W, CHEAT_A, CHEAT_R, CHEAT_L, CHEAT_O, CHEAT_R, CHEAT_D, CHEAT_S}, MM_Cheat_InfiniteMasks},
                {7, {CHEAT_A, CHEAT_X, CHEAT_E, CHEAT_L, CHEAT_W, CHEAT_A, CHEAT_X}, MM_Cheat_MaxTurbos},
                {6, {CHEAT_U, CHEAT_N, CHEAT_S, CHEAT_E, CHEAT_E, CHEAT_N}, MM_Cheat_MaxInvisibility},
                {8, {CHEAT_U, CHEAT_N, CHEAT_L, CHEAT_E, CHEAT_A, CHEAT_D, CHEAT_E, CHEAT_D}, MM_Cheat_MaxEngine},
                {7, {CHEAT_S, CHEAT_E, CHEAT_A, CHEAT_S, CHEAT_W, CHEAT_A, CHEAT_N}, MM_Cheat_UnlockPenta},
                {7, {CHEAT_S, CHEAT_L, CHEAT_E, CHEAT_N, CHEAT_D, CHEAT_E, CHEAT_R}, MM_Cheat_UnlockTropy},
                {9, {CHEAT_U, CHEAT_N, CHEAT_D, CHEAT_E, CHEAT_R, CHEAT_W, CHEAT_E, CHEAT_A, CHEAT_R}, MM_Cheat_UnlockScrapbook},
                {7, {CHEAT_R, CHEAT_E, CHEAT_W, CHEAT_A, CHEAT_R, CHEAT_D, CHEAT_S}, MM_Cheat_UnlockTracks},
                {6, {CHEAT_E, CHEAT_N, CHEAT_D, CHEAT_N, CHEAT_E, CHEAT_O}, MM_Cheat_AdvDifficulty},
                {8, {CHEAT_S, CHEAT_L, CHEAT_E, CHEAT_D, CHEAT_R, CHEAT_O, CHEAT_A, CHEAT_D}, MM_Cheat_IcyTracks},
                {5, {CHEAT_A, CHEAT_R, CHEAT_R, CHEAT_O, CHEAT_W}, MM_Cheat_SuperTurboPads},
                {6, {CHEAT_D, CHEAT_E, CHEAT_L, CHEAT_U, CHEAT_X, CHEAT_E}, MM_Cheat_SuperHard},
                {7, {CHEAT_A, CHEAT_R, CHEAT_S, CHEAT_E, CHEAT_N, CHEAT_A, CHEAT_L}, MM_Cheat_MaxBombs},
                {10, {CHEAT_S, CHEAT_U, CHEAT_D, CHEAT_D, CHEAT_E, CHEAT_N, CHEAT_D, CHEAT_E, CHEAT_A, CHEAT_D}, MM_Cheat_OneLap},
                {5, {CHEAT_A, CHEAT_D, CHEAT_D, CHEAT_O, CHEAT_N}, MM_Cheat_TurboCounter},
                {4, {CHEAT_D, CHEAT_E, CHEAT_X, CHEAT_S}, MM_Cheat_EnableDevHacks},
            },

        .cheatButtonHistory = {0},

        .cupDifficulty =
            {
                .firstUnlockBit = {-1, 0xC, 0x10, 0},
                .stringIndex = {0x15A, 0x15B, 0x15C, 0},
                .speed = {0x50, 0xA0, 0xF0},
            },

        // CHARACTER SELECT CONSTS

        .characterSelectWindowPos =
            {
                // 1P full menu
                {.x = 0x7F, .y = 0xA},

                // 1P small menu
                {.x = 0x4D, .y = 0x1E},

                // 2P full menu
                {.x = 0xA, .y = 0xA},
                {.x = 0x110, .y = 0xA},

                // 2P small menu
                {.x = 0x1E, .y = 0x1E},
                {.x = 0x100, .y = 0x1E},

                // 3P menu
                {.x = 0x136, .y = 0x4},
                {.x = 0x136, .y = 0x4A},
                {.x = 0x136, .y = 0x90},

                // 4P menu
                {.x = 0x15, .y = 0x1},
                {.x = 0x13F, .y = 0x1},
                {.x = 0x15, .y = 0x95},
                {.x = 0x13F, .y = 0x95},
            },

        .characterSelectWindowPosByLayout =
            {
                // full menus, 1p2p3p4p
                &D230.characterSelectWindowPos[0],
                &D230.characterSelectWindowPos[2],
                &D230.characterSelectWindowPos[6],
                &D230.characterSelectWindowPos[9],

                // small menus, 1p2p
                &D230.characterSelectWindowPos[1],
                &D230.characterSelectWindowPos[4],
            },

        .characterSelectLayout =
            {
                .windowW = {0xF6, 0xD6, 0xA0, 0xA0, 0x15A, 0xD6},

                .windowH = {0x50, 0x50, 0x40, 0x40, 0x5C, 0x5C},

                .driverPosZ = {0xC8, 0xC8, 0xFa, 0xFa, 0xAA, 0xAA},

                .driverPosY = {0x28, 0x28, 0x28, 0x28, 0x28, 0x28},

                .textY = {0x46, 0x46, 0x36, 0x36, 0x52, 0x52},
            },

        .characterSelectMeta1P2PLimited =
            {// Crash
             {0x80, 0x80, {0, 4, 8, 1}, 0, 0xFFFF},
             // Cortex
             {0xC0, 0x80, {1, 5, 0, 2}, 1, 0xFFFF},
             // Tiny
             {0x100, 0x80, {2, 6, 1, 3}, 2, 0xFFFF},
             // Coco
             {0x140, 0x80, {3, 7, 2, 9}, 3, 0xFFFF},
             // N. Gin
             {0x80, 0xA7, {0, 12, 10, 5}, 4, 0xFFFF},
             // Dingo
             {0xC0, 0xA7, {1, 13, 4, 6}, 5, 0xFFFF},
             // Polar
             {0x100, 0xA7, {2, 14, 5, 7}, 6, 0xFFFF},
             // Pura
             {0x140, 0xA7, {3, 15, 6, 11}, 7, 0xFFFF},
             // N. Tropy
             {0x40, 0x80, {8, 10, 8, 0}, 12, 0x5},
             // Pinstripe
             {0x180, 0x80, {9, 11, 3, 9}, 8, 0xA},
             // Roo
             {0x40, 0xA7, {8, 10, 10, 4}, 10, 0x7},
             // Papu
             {0x180, 0xA7, {9, 11, 7, 11}, 9, 0x8},
             // Komodo Joe
             {0x80, 0xCE, {4, 12, 12, 13}, 11, 0x9},
             // Penta
             {0xC0, 0xCE, {5, 13, 12, 14}, 13, 0x6},
             // Fake Crash
             {0x100, 0xCE, {6, 15, 13, 15}, 14, 0xB},
             // Nitros Oxide
             {0x140, 0xCE, {7, 15, 14, 11}, NITROS_OXIDE, 0xFFFF}},

        .characterSelectMeta1P2P =
            {// Crash
             {0x80, 0x60, {0, 4, 8, 1}, 0, 0xFFFF},
             // Cortex
             {0xC0, 0x60, {1, 5, 0, 2}, 1, 0xFFFF},
             // Tiny
             {0x100, 0x60, {2, 6, 1, 3}, 2, 0xFFFF},
             // Coco
             {0x140, 0x60, {3, 7, 2, 9}, 3, 0xFFFF},
             // N. Gin
             {0x80, 0x87, {0, 12, 10, 5}, 4, 0xFFFF},
             // Dingo
             {0xC0, 0x87, {1, 13, 4, 6}, 5, 0xFFFF},
             // Polar
             {0x100, 0x87, {2, 14, 5, 7}, 6, 0xFFFF},
             // Pura
             {0x140, 0x87, {3, 15, 6, 11}, 7, 0xFFFF},
             // N. Tropy
             {0x40, 0x60, {8, 10, 8, 0}, 12, 0x5},
             // Pinstripe
             {0x180, 0x60, {9, 11, 3, 9}, 8, 0xA},
             // Roo
             {0x40, 0x87, {8, 10, 10, 4}, 10, 0x7},
             // Papu
             {0x180, 0x87, {9, 11, 7, 11}, 9, 0x8},
             // Komodo Joe
             {0x80, 0xAE, {4, 12, 12, 13}, 11, 0x9},
             // Penta
             {0xC0, 0xAE, {5, 13, 12, 14}, 13, 0x6},
             // Fake Crash
             {0x100, 0xAE, {6, 15, 13, 15}, 14, 0xB},
             // Nitros Oxide
             {0x140, 0xAE, {7, 15, 14, 11}, NITROS_OXIDE, 0xFFFF}},

        .characterSelectMeta3P =
            {// Crash
             {0x20, 0x47, {12, 4, 0, 1}, 0, 0xFFFF},
             // Cortex
             {0x60, 0x47, {13, 5, 0, 2}, 1, 0xFFFF},
             // Tiny
             {0xA0, 0x47, {14, 6, 1, 3}, 2, 0xFFFF},
             // Coco
             {0xE0, 0x47, {15, 7, 2, 3}, 3, 0xFFFF},
             // N. Gin
             {0x20, 0x6E, {0, 8, 4, 5}, 4, 0xFFFF},
             // Dingo
             {0x60, 0x6E, {1, 9, 4, 6}, 5, 0xFFFF},
             // Polar
             {0xA0, 0x6E, {2, 10, 5, 7}, 6, 0xFFFF},
             // Pura
             {0xE0, 0x6E, {3, 11, 6, 7}, 7, 0xFFFF},
             // N. Tropy
             {0x20, 0x95, {4, 8, 8, 9}, 12, 0x5},
             // Pinstripe
             {0x60, 0x95, {5, 9, 8, 10}, 8, 0xA},
             // Roo
             {0xA0, 0x95, {6, 10, 9, 11}, 10, 0x7},
             // Papu
             {0xE0, 0x95, {7, 11, 10, 11}, 9, 0x8},
             // Komodo Joe
             {0x20, 0x20, {12, 1, 12, 13}, 11, 0x9},
             // Penta
             {0x60, 0x20, {13, 2, 12, 14}, 13, 0x6},
             // Fake Crash
             {0xA0, 0x20, {14, 3, 13, 15}, 14, 0xB},
             // Nitros Oxide
             {0xE0, 0x20, {15, 3, 14, 15}, NITROS_OXIDE, 0xFFFF}},

        .characterSelectMeta4P =
            {// Crash
             {0x80, 0x47, {0, 4, 10, 1}, 0, 0xFFFF},
             // Cortex
             {0xC0, 0x47, {14, 5, 0, 2}, 1, 0xFFFF},
             // Tiny
             {0x100, 0x47, {15, 6, 1, 3}, 2, 0xFFFF},
             // Coco
             {0x140, 0x47, {3, 7, 2, 11}, 3, 0xFFFF},
             // N. Gin
             {0x80, 0x6E, {0, 4, 12, 5}, 4, 0xFFFF},
             // Dingo
             {0xC0, 0x6E, {1, 8, 4, 6}, 5, 0xFFFF},
             // Polar
             {0x100, 0x6E, {2, 9, 5, 7}, 6, 0xFFFF},
             // Pura
             {0x140, 0x6E, {3, 7, 6, 13}, 7, 0xFFFF},
             // N. Tropy
             {0xC0, 0x95, {5, 8, 8, 9}, 12, 0x5},
             // Fake Crash
             {0x100, 0x95, {6, 9, 8, 9}, 14, 0xB},
             // Roo
             {0x40, 0x47, {10, 12, 10, 0}, 10, 0x7},
             // Papu
             {0x180, 0x47, {11, 13, 3, 11}, 9, 0x8},
             // Komodo Joe
             {0x40, 0x6E, {10, 12, 12, 4}, 11, 0x9},
             // Pinstripe
             {0x180, 0x6E, {11, 13, 7, 13}, 8, 0xA},
             // Penta
             {0xC0, 0x20, {14, 1, 14, 15}, 13, 0x6},
             // Nitros Oxide
             {0x100, 0x20, {15, 2, 14, 15}, NITROS_OXIDE, 0xFFFF}},

        .characterSelectMetaByLayout = {&D230.characterSelectMeta1P2P[0], &D230.characterSelectMeta1P2P[0], &D230.characterSelectMeta3P[0],
                                        &D230.characterSelectMeta4P[0], &D230.characterSelectMeta1P2PLimited[0], &D230.characterSelectMeta1P2PLimited[0]},

        .characterMenuID =
            {
                -1,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
            },

        .characterSelectTransition1P2P =
            {
                {0, 0xC8, 6, 0, 0}, {0, 0xC8, 5, 0, 0}, {0, 0xC8, 4, 0, 0}, {0, 0xC8, 3, 0, 0}, {0, 0xC8, 5, 0, 0}, {0, 0xC8, 4, 0, 0}, {0, 0xC8, 3, 0, 0},
                {0, 0xC8, 2, 0, 0}, {0, 0xC8, 7, 0, 0}, {0, 0xC8, 1, 0, 0}, {0, 0xC8, 6, 0, 0}, {0, 0xC8, 0, 0, 0}, {0, 0xC8, 4, 0, 0}, {0, 0xC8, 3, 0, 0},
                {0, 0xC8, 2, 0, 0}, {0, 0xC8, 1, 0, 0}, {-512, 0, 0, 0, 0}, {512, 0, 3, 0, 0},  {512, 0, 1, 0, 0},  {512, 0, 7, 0, 0},  {512, 0, 5, 0, 0},  {0, 0, -1, 0, 0},
            },

        .characterSelectTransition3P =
            {
                {-512, 0, 2, 0, 0}, {-512, 0, 3, 0, 0}, {-512, 0, 4, 0, 0}, {-512, 0, 5, 0, 0}, {-512, 0, 1, 0, 0}, {-512, 0, 2, 0, 0}, {-512, 0, 3, 0, 0},
                {-512, 0, 4, 0, 0}, {-512, 0, 0, 0, 0}, {-512, 0, 1, 0, 0}, {-512, 0, 2, 0, 0}, {-512, 0, 3, 0, 0}, {-512, 0, 2, 0, 0}, {-512, 0, 3, 0, 0},
                {-512, 0, 4, 0, 0}, {-512, 0, 0, 0, 0}, {-512, 0, 1, 0, 0}, {512, 0, 5, 0, 0},  {512, 0, 3, 0, 0},  {512, 0, 1, 0, 0},  {512, 0, 5, 0, 0},  {0, 0, -1, 0, 0},
            },

        .characterSelectTransition4P =
            {
                {-512, 0, 1, 0, 0}, {-512, 0, 2, 0, 0}, {-512, 0, 3, 0, 0}, {-512, 0, 4, 0, 0}, {-512, 0, 2, 0, 0}, {-512, 0, 3, 0, 0}, {-512, 0, 4, 0, 0},
                {-512, 0, 5, 0, 0}, {-512, 0, 3, 0, 0}, {-512, 0, 4, 0, 0}, {-512, 0, 0, 0, 0}, {-512, 0, 5, 0, 0}, {-512, 0, 1, 0, 0}, {-512, 0, 6, 0, 0},
                {-512, 0, 2, 0, 0}, {-512, 0, 0, 0, 0}, {-512, 0, 1, 0, 0}, {512, 0, 3, 0, 0},  {512, 0, 1, 0, 0},  {512, 0, 7, 0, 0},  {512, 0, 5, 0, 0},  {0, 0, -1, 0, 0},
            },

        .characterSelectTransitionByPlayerCount = {&D230.characterSelectTransition1P2P[0], &D230.characterSelectTransition1P2P[0],
                                                   &D230.characterSelectTransition3P[0], &D230.characterSelectTransition4P[0]},

        .characterSelectDriverModel =
            {
                .pos = {0, 0x28, 0xFA},
                .rot = {0x800, 0, 0},
                .moveFrames = 4,
                .slideDistance = 0x8C,
            },

        .playerNumberStrings = {&R230.s_1[0], &R230.s_2[0], &R230.s_3[0], &R230.s_4[0]},

        .characterSelectFallbackDirection1 = {2, 2, 1, 1},

        .characterSelectFallbackDirection2 = {3, 3, 0, 0},

        .characterSelect_Outline = COLOR_CODE_PACKED_INIT(0),

        .characterSelect_NeutralColor = COLOR_CODE_PACKED_INIT(0x808080),

        .characterSelect_ChosenColor = COLOR_CODE_PACKED_INIT(0),

        .characterSelect_BlueRectColors = {0x1b, 0x6a, 0xcb, 0x00, 0x9b, 0xd2, 0xf2, 0x40, 0xb7, 0xe1, 0xed, 0x43,
                                           0x65, 0x4b, 0x03, 0x46, 0xa6, 0x83, 0x23, 0x50, 0xdf, 0xc8, 0x95, 0x64},

        // TRACK SELECT CONSTS

        .arcadeTracks =
            {
                // CRASH_COVE
                {3, 0x51, 0x6a, 0xFFFF, 0x1e4, 0x258},
                // ROO_TUBES
                {6, 0x5b, 0x7e, 0xFFFF, 0x1fa, 0x283},
                // TIGER_TEMPLE
                {4, 0x59, 0x7a, 0xFFFF, 0x1e5, 0x31f},
                // COCO_PARK
                {14, 0x43, 0x5c, 0xFFFF, 0x1ee, 0x2a1},
                // MYSTERY_CAVES
                {9, 0x4e, 0x64, 0xFFFF, 0x1e9, 0x39b},
                // BLIZZARD_BLUFF
                {2, 0x4f, 0x66, 0xFFFF, 0x1e3, 0x2cd},
                // SEWER_SPEEDWAY
                {8, 0x57, 0x76, 0xFFFF, 0x1e8, 0x374},
                // DINGO_CANYON
                {0, 0x53, 0x6e, 0xFFFF, 0x1e1, 0x2a3},
                // PAPU_PYRAMID
                {5, 0x5a, 0x7c, 0xFFFF, 0x1e6, 0x309},
                // DRAGON_MINES
                {1, 0x54, 0x70, 0xFFFF, 0x1e2, 0x285},
                // POLAR_PASS
                {12, 0x50, 0x68, 0xFFFF, 0x1ec, 0x4ca},
                // CORTEX_CASTLE
                {10, 0x4d, 0x62, 0xFFFF, 0x1ea, 0x41b},
                // TINY_ARENA
                {15, 0x44, 0x5e, 0xFFFF, 0x1ef, 0x58f},
                // HOT_AIR_SKYWAY
                {7, 0x4c, 0x60, 0xFFFF, 0x1e7, 0x4f9},
                // N_GIN_LABS
                {11, 0x52, 0x6c, 0xFFFF, 0x1eb, 0x448},
                // OXIDE_STATION
                {13, 0x58, 0x78, 0xFFFE, 0x1ed, 0x5da},
                // SLIDE_COLISEUM
                {16, 0x55, 0x72, 0xFFFF, 0x1f0, 0x39a},
                // TURBO_TRACK
                {17, 0x56, 0x74, 0x1, 0x1f1, 0x3d5},
            },

        .battleTracks =
            {
                // NITRO_COURT
                {18, 0x45, 0xFFFF, 0xFFFF, 0x1f2, 0x197},
                // RAMPAGE_RUINS
                {19, 0x46, 0xFFFF, 0xFFFF, 0x1f3, 0x19a},
                // PARKING_LOT
                {20, 0x47, 0xFFFF, 0x2, 0x1f4, 0x199},
                // SKULL_ROCK
                {21, 0x48, 0xFFFF, 0xFFFF, 0x1f5, 0x19a},
                // THE_NORTH_BOWL
                {22, 0x49, 0xFFFF, 0x3, 0x1f6, 0x19a},
                // ROCKY_ROAD
                {23, 0x4a, 0xFFFF, 0xFFFF, 0x1f7, 0x19a},
                // LAB_BASEMENT
                {24, 0x4b, 0xFFFF, 0x4, 0x1f8, 0x19a},
            },

        .trackTransitions = {{{-512, 0, 1, 0, 0}, {0, -200, 2, 0, 0}, {0, 200, 3, 0, 0}, {512, 0, 4, 0, 0}, {0, 0, -1, 0, 0}}},

        .lapCountByRow = {{3, 0}, {5, 0}, {7, 0}, {0, 0}},

        .rowsLapSel = {{0x9b, 0, 1, 0, 0}, {0x9c, 0, 2, 1, 1}, {0x9d, 1, 2, 2, 2}, {RECTMENU_STRING_NONE}},

        .menuLapSel =
            {
                .stringIndexTitle = LNG_LAPS,

                .posX_curr = 0x18C,
                .posY_curr = 0x7c,

                .state = USE_SMALL_FONT | CENTER_ON_X,
                .rows = &D230.rowsLapSel[0],
            },

        .videoCol = COLOR_CODE_PACKED_INIT(0x808080),

        .timeTrialStars =
            {
                .colorIndex = {0xE, 0x16},
                .beatenFlagBit = {0x1, 0x2},
            },

        .drawMapOffset = {{0, 0, 1}, {-2, 0, 3}, {2, 0, 3}, {0, 1, 3}, {0, -1, 3}, {12, 6, 2}},

        // CUP SELECT CONSTS

        .transitionMeta_cupSel = {{-256, -100, 5, 0, 0}, {256, -100, 4, 0, 0}, {-256, 100, 3, 0, 0}, {256, 100, 2, 0, 0}, {0, -100, 0, 0, 0}, {0, 0, -1, 0, 0}},

        .cupSelectStars =
            {
                .colorIndex = {TROPY_LIGHT_BLUE, PAPU_YELLOW, SILVER, 0},

                .winBitBase =
                    {
                        GAME_PROGRESS_CUP_CURRENT_WIN_FIRST_BIT,
                        GAME_PROGRESS_CUP_CURRENT_WIN_FIRST_BIT + GAME_PROGRESS_CUP_COUNT,
                        GAME_PROGRESS_CUP_CURRENT_WIN_FIRST_BIT + (GAME_PROGRESS_CUP_COUNT * 2),
                        0,
                    },
            },

        .cupSel_Color = COLOR_CODE_PACKED_INIT(0xC0C0C0),

        // BATTLE CONSTS

        .transitionMeta_battle = {{400, 0, 5, 0, 0},
                                  {-200, 0, 5, 0, 0},
                                  {400, 0, 4, 0, 0},
                                  {-200, 0, 4, 0, 0},
                                  {400, 0, 3, 0, 0},
                                  {-200, 0, 3, 0, 0},
                                  {400, 0, 2, 0, 0},
                                  {-200, 0, 2, 0, 0},
                                  {400, 0, 0, 0, 0},
                                  {0, -100, 0, 0, 0},
                                  {0, 0, -1, 0, 0}},

        .rowsBattleType = {{0x92, 0, 1, 0, 0}, {0x93, 0, 2, 1, 1}, {0x94, 1, 2, 2, 2}, {RECTMENU_STRING_NONE}},

        .menuBattleType = {.stringIndexTitle = RECTMENU_STRING_NONE,

                           .state = KEEP_INPUTS_IN_SUBMENU | CENTER_MENU_TEXT | USE_SMALL_FONT,
                           .rows = &D230.rowsBattleType[0],
                           .funcPtr = MM_Battle_CloseSubMenu},

        .rowsBattleLengthLifeTime = {{0xA4, 0, 1, 0, 0}, {0xA5, 0, 2, 1, 1}, {0xA6, 1, 2, 2, 2}, {RECTMENU_STRING_NONE}},

        .menuBattleLengthLifeTime = {.stringIndexTitle = RECTMENU_STRING_NONE,

                                     .state = KEEP_INPUTS_IN_SUBMENU | CENTER_MENU_TEXT | USE_SMALL_FONT,
                                     .rows = &D230.rowsBattleLengthLifeTime[0],
                                     .funcPtr = MM_Battle_CloseSubMenu},

        .rowsBattleLengthTimeTime = {{0xA7, 0, 1, 0, 0}, {0xA8, 0, 2, 1, 1}, {0xA9, 1, 2, 2, 2}, {RECTMENU_STRING_NONE}},

        .menuBattleLengthTimeTime = {.stringIndexTitle = RECTMENU_STRING_NONE,

                                     .state = KEEP_INPUTS_IN_SUBMENU | CENTER_MENU_TEXT | USE_SMALL_FONT,
                                     .rows = &D230.rowsBattleLengthTimeTime[0],
                                     .funcPtr = MM_Battle_CloseSubMenu},

        .rowsBattleLengthPoints = {{0xA1, 0, 1, 0, 0}, {0xA2, 0, 2, 1, 1}, {0xA3, 1, 2, 2, 2}, {RECTMENU_STRING_NONE}},

        .menuBattleLengthPoints = {.stringIndexTitle = RECTMENU_STRING_NONE,

                                   .state = KEEP_INPUTS_IN_SUBMENU | CENTER_MENU_TEXT | USE_SMALL_FONT,
                                   .rows = &D230.rowsBattleLengthPoints[0],
                                   .funcPtr = MM_Battle_CloseSubMenu},

        .rowsBattleLengthLifeLife = {{0x9e, 0, 1, 0, 0}, {0x9f, 0, 2, 1, 1}, {0xA0, 1, 2, 2, 2}, {RECTMENU_STRING_NONE}},

        .menuBattleLengthLifeLife = {.stringIndexTitle = RECTMENU_STRING_NONE,

                                     .state = KEEP_INPUTS_IN_SUBMENU | CENTER_MENU_TEXT | USE_SMALL_FONT,
                                     .rows = &D230.rowsBattleLengthLifeLife[0],
                                     .funcPtr = MM_Battle_CloseSubMenu},

        .rowsBattleStartGame = {{0xAE, 0, 0, 0, 0}, {RECTMENU_STRING_NONE}},

        .menuBattleStartGame = {.stringIndexTitle = RECTMENU_STRING_NONE,

                                .state = KEEP_INPUTS_IN_SUBMENU | CENTER_MENU_TEXT | USE_SMALL_FONT,
                                .rows = &D230.rowsBattleStartGame[0],
                                .funcPtr = MM_Battle_CloseSubMenu},

        .battleMenuArray = {&D230.menuBattleType, &D230.menuBattleLengthTimeTime, &D230.menuBattleLengthPoints, &D230.menuBattleLengthLifeLife,
                            &D230.menuBattleLengthLifeTime},

        .battleWeaponItems = {{0x1, 0x2},
                              {0x2, 0x6},
                              {0x4, 0x7},
                              {0x8, 0x8},
                              {0x10, 0x9},
                              {0x40, 0xB},
                              {0x80, 0xC},
                              {0x400, 0xF},
                              {0x800, 0x10},
                              {0x1000, 0x11},
                              {0x2000, 0x12}},

        .battleSetupTables =
            {
                .typeModeFlags = {POINT_LIMIT, TIME_LIMIT, LIFE_LIMIT},
                .timeLimitMinutes = {3, 6, 9},
                .lifeModeTimeLimitMinutes = {3, 6, -1},
                .lifeLimitValues = {3, 6, 9},
                .pointLimitValues = {5, 10, 15},
            },

        .battleWeaponEnabledColor = COLOR_CODE_PACKED_INIT(0x808080),
        .battleWeaponDisabledColor = COLOR_CODE_PACKED_INIT(0x101010),
        .battleWeaponPanelColor = COLOR_CODE_PACKED_INIT(0x806050),

        // HIGH SCORE CONSTS

        .transitionMeta_HighScores = {{0, -100, 0, 0, 0},
                                      {-256, 0, 6, 0, 0},
                                      {-256, 0, 5, 0, 0},
                                      {-256, 0, 4, 0, 0},
                                      {-256, 0, 3, 0, 0},
                                      {-256, 0, 2, 0, 0},
                                      {-256, 0, 1, 0, 0},
                                      {256, 0, 3, 0, 0},
                                      {256, 0, 2, 0, 0},
                                      {256, 0, 1, 0, 0},
                                      {256, 0, 0, 0, 0},
                                      {0, 0, -1, 0, 0}},

        .highScoreGhostStars =
            {
                .colorIndex = {0xE, 0x16},

                .beatenFlagBit = {0x1, 0x2},
            },

        .highscore_iconColor = COLOR_CODE_PACKED_INIT(0x808080),

        .rowsHighScore = {{0xAF, 0, 1, 0, 0}, {0xB1, 0, 2, 0, 0}, {0xB2, 1, 2, 0, 0}, {RECTMENU_STRING_NONE}},

        .menuHighScore = {.stringIndexTitle = RECTMENU_STRING_NONE,
                          .posX_curr = 0x17C,
                          .posY_curr = 0xAF,

                          .state = USE_SMALL_FONT | CENTER_ON_X,
                          .rows = &D230.rowsHighScore[0]},

        .highScoreSelection =
            {
                .targetTrack = 0,
                .targetRow = 0,
                .currentTrack = 0,
                .currentRow = 0,
            },
    .rowsQuitConfirm =
        {
            {0xd2, 1, 1, 0, 0},
            {0xd3, 0, 0, 1, 1},
            {-1},
        },

    .menuQuitConfirm =
        {
            .stringIndexTitle = -1,
            .state = 0x09,
            .rows = &D230.rowsQuitConfirm[0],
            .funcPtr = MM_MenuProc_QuitConfirm,
        },
        .titleObj = 0,

        // TRACK SELECT DYN
        .trackSelect =
            {
                .trackChangeFrames = 0,
                .currentTrack = 0,
                .trackChangeDirection = 0,
                .lapBoxOpen = 0,
                .transition =
                    {
                        .state = ENTERING_MENU,
                        .startAfterExit = 0,
                        .frame = 0,
                    },
                .videoPreviewFrames = 0,
                .videoMemAllocated = 0,
                .videoStateCurr = 0,
                .videoStatePrev = 0,
            },

        // CUP SELECT DYN
        .cupSelectTransition =
            {
                .state = ENTERING_MENU,
                .startAfterExit = 0,
                .frame = 0,
            },

        // BATTLE DYN
        .battleTransition =
            {
                .state = ENTERING_MENU,
                .startAfterExit = 0,
                .frame = 0,
            },

        // HIGHSCORE DYN
        .highScoreTransition =
            {
                .state = ENTERING_MENU,
                .mainFrame = 0,
                .trackFrame = 0,
                .rowFrame = 0,
                .activeHorizontalMove = 0,
                .pendingHorizontalMove = 0,
                .activeVerticalMove = 0,
                .pendingVerticalMove = 0,
            },

        // SCRAPBOOK
        .scrapbookState = SCRAP_INIT,

        // CHAR SELECT AND TITLE DYN
        .characterSelectWindowHeight = 0,
        .desiredMenuIndex = MM_EXIT_ROUTE_ADV_NEW,
        .characterSelectExitsForward = 0,

        .characterSelectPlayerState =
            {
                .modelMoveDir = {0, 0, 0, 0},
                .desiredCharacterID = {0, 0, 0, 0},
                .currentCharacterID = {0, 0, 0, 0},
                .angle = {0, 0, 0, 0},
            },
        .characterSelectTransitionState = ENTERING_MENU,
        .activeCharacterSelectWindowPos = 0,
        .characterSelectLayoutIndex = 0,

        .titleIntroFrame = 0,
        .activeCharacterSelectMeta = 0,
        .titleMenuState = TITLE_MENU_STATE_INTRO,
        .characterSelectMenuState = ENTERING_MENU,

        .characterSelectModelMoveTimer = {0, 0, 0, 0},
        .characterSelectRosterExpanded = 0,
        .characterSelectWindowWidth = 0,
        .titleIntroCameraPath = 0,
        .characterSelectNameTextY = 0,
        .characterSelectTransitionMeta = 0,
        .titleMenuTransitionFrame = 0,
        .characterSelectTransitionFrame = 0,

        .s_SliceBuf = "SliceBuf",
        .s_VlcBuf = "VlcBuf",
        .s_RingBuf = "RingBuf",
};

struct OVR_230_VideoBSS V230;

static struct OverlayDATA_230 s_d230InitialState;
static int s_d230InitialStateReady;

void OVR230_ResetRuntimeState(void)
{
	if (s_d230InitialStateReady == 0)
	{
		s_d230InitialState = D230;
		s_d230InitialStateReady = 1;
	}

	D230 = s_d230InitialState;
	memset(&V230, 0, sizeof(V230));
}

void OVR230_InitData(void)
{
	OVR230_ResetRuntimeState();
}

#undef CHEAT_N
#undef CHEAT_U
#undef CHEAT_S
#undef CHEAT_D
#undef CHEAT_W
#undef CHEAT_L
#undef CHEAT_E
#undef CHEAT_R
#undef CHEAT_A
#undef CHEAT_O
#undef CHEAT_X
