#define MM_GAME_TRACKER_PAGE_VALUE                        0U
#define MM_GAME_TRACKER_PAGE_OFFSET                       0
#define MM_OVERLAY_PAGE_VALUE                             0U
#define MM_REQUEST_LEVEL                                  MainRaceTrack_RequestLoad
#define MM_BIGFILE_HEADER                                 (sdata_static.ptrBigfileCdPos_2)
#define MM_GAME_UNLOCKS                                   (sdata->gameSave.progress.unlocks)
#define MM_CHARACTER_IDS_BACKUP                           (sdata->characterIDs_backup)
#define MM_DEFAULT_CHARACTER_ID_WORDS                     (R230.packedDefaultCharacterIDWords)
#define MM_TITLE_OBJECT                                   (D230.titleObj)
#define MM_TITLE_INTRO_FRAME                              (D230.titleIntroFrame)
#define MM_TITLE_INTRO_FRAME_ASM_NAME                     ""
#define MM_TITLE_INTRO_FRAME_PAGE_OFFSET                  0
#define MM_TITLE_CAMERA_POS                               (D230.titleCameraPos)
#define MM_TITLE_CAMERA_ROT                               (D230.titleCameraRot)
#define MM_TITLE_CAMERA_PATH                              (D230.titleIntroCameraPath)
#define MM_TITLE_SOUNDS                                   (D230.titleSounds)
#define MM_TITLE_INSTANCES                                (D230.titleInstances)
#define MM_MENU_MAIN                                      (D230.menuMainMenu)
#define MM_GAME_BUTTON_TAPS                               (sdata->buttonTapPerPlayer)
#define MM_TITLE_MENU_STATE                               (D230.titleMenuState)
#define MM_TITLE_OBJECT_NAME                              (R230.s_title)
#define MM_MENU_PLAYERS_1P2P                              (D230.menuPlayers1P2P)
#define MM_MENU_PLAYERS_2P3P4P                            (D230.menuPlayers2P3P4P)
#define MM_MENU_DIFFICULTY                                (D230.menuDifficulty)
#define MM_MENU_RACE_TYPE                                 (D230.menuRaceType)
#define MM_MENU_ADVENTURE                                 (D230.menuAdventure)
#define MM_MENU_POINTERS                                  (D230.arrayMenuPtrs)
#define MM_MENU_CHARACTER_SELECT                          (D230.menuCharacterSelect)
#define MM_MENU_HIGH_SCORES                               (D230.menuHighScores)
#define MM_MENU_SCRAPBOOK                                 (D230.menuScrapbook)
#define MM_TITLE_TRANSITION_DURATION                      (D230.titleTransition.durationFrames)
#define MM_TITLE_TRANSITION_STEP                          (D230.titleTransition.step)
#define MM_TITLE_MENU_LAYOUT                              (D230.titleTransition.menuLayout)
#define MM_TITLE_TRANSITIONS                              (D230.titleTransition.transition)
#define MM_ARCADE_TRACKS                                  (D230.arcadeTracks)
#define MM_ARCADE_TRACKS_ASM_NAME                         ""
#define MM_DESIRED_MENU_INDEX                             (D230.desiredMenuIndex)
#define MM_TITLE_MENU_TRANSITION_FRAME                    (D230.titleMenuTransitionFrame)
#define MM_ADV_PROFILE_INDEX                              (sdata->advProfileIndex)
#define MM_MAIN_MENU_STATE                                (sdata->mainMenuState)
#define MM_DEMO_MODE_INDEX                                (sdata->demoModeIndex)
#define MM_DESIRED_MENU                                   (sdata->ptrDesiredMenu)
#define MM_ACTIVE_MENU                                    (sdata_static.ptrActiveMenu)
#define MM_MENU_FOUR_ADV_PROFILES                         (data.menuFourAdvProfiles)
#define MM_CHEATS                                         (D230.cheats)
#define MM_CHEAT_BUTTON_HISTORY                           (D230.cheatButtonHistory)
#define MM_COLOR_POINTERS                                 (data.ptrColor)
#define MM_COLOR_POINTERS_ASM_NAME                        ""
#define MM_FRAME_COUNTER                                  (sdata->frameCounter)
#define MM_FRAME_COUNTER_ASM_NAME                         ""
#define MM_FRAME_COUNTER_PAGE_OFFSET                      0
#define MM_TRIG_APPROX                                    (data.trigApprox)
#define MM_ACTIVE_CHARACTER_SELECT_META                   (D230.activeCharacterSelectMeta)
#define MM_CHARACTER_METADATA_PAGE_VALUE                  0U
#define MM_CHARACTER_SELECT_DESIRED_IDS                   (D230.characterSelectPlayerState.desiredCharacterID)
#define MM_CHARACTER_SELECT_DESIRED_IDS_ASM_NAME          ""
#define MM_ACTIVE_CHARACTER_SELECT_WINDOW_POS             (D230.activeCharacterSelectWindowPos)
#define MM_ACTIVE_CHARACTER_SELECT_WINDOW_POS_ASM_NAME    ""
#define MM_CHARACTER_SELECT_TRANSITION_META               (D230.characterSelectTransitionMeta)
#define MM_CHARACTER_SELECT_TRANSITION_META_ASM_NAME      ""
#define MM_CHARACTER_SELECT_TRANSITION_POINTER_PAGE_VALUE 0U
#define MM_CHARACTER_SELECT_WINDOW_WIDTH                  (D230.characterSelectWindowWidth)
#define MM_CHARACTER_SELECT_WINDOW_HEIGHT                 (D230.characterSelectWindowHeight)
#define MM_CHARACTER_SELECT_NAME_TEXT_Y                   (D230.characterSelectNameTextY)
#define MM_CHARACTER_SELECT_CURRENT_IDS                   (D230.characterSelectPlayerState.currentCharacterID)
#define MM_CHARACTER_SELECT_CURRENT_IDS_ASM_NAME          ""
#define MM_CHARACTER_SELECT_DRIVER_POS                    (D230.characterSelectDriverModel.pos)
#define MM_CHARACTER_SELECT_DRIVER_POS_ASM_NAME           ""
#define MM_CHARACTER_SELECT_DRIVER_POS_PAGE_OFFSET        0
#define MM_CHARACTER_SELECT_DRIVER_ROT                    (D230.characterSelectDriverModel.rot)
#define MM_CHARACTER_SELECT_DRIVER_ROT_ASM_NAME           ""
#define MM_CHARACTER_SELECT_DRIVER_ROT_PAGE_OFFSET        0
#define MM_CHARACTER_SELECT_MOVE_TIMERS                   (D230.characterSelectModelMoveTimer)
#define MM_CHARACTER_SELECT_MOVE_FRAMES                   (D230.characterSelectDriverModel.moveFrames)
#define MM_CHARACTER_SELECT_MOVE_DIR                      (D230.characterSelectPlayerState.modelMoveDir)
#define MM_CHARACTER_SELECT_MOVE_DIR_ASM_NAME             ""
#define MM_CHARACTER_SELECT_SLIDE_DISTANCE                (D230.characterSelectDriverModel.slideDistance)
#define MM_CHARACTER_SELECT_ANGLE                         (D230.characterSelectPlayerState.angle)
#define MM_CHARACTER_SELECT_ANGLE_ASM_NAME                ""
#define MM_CHARACTER_SELECT_ROSTER_EXPANDED               (D230.characterSelectRosterExpanded)
#define MM_CHARACTER_SELECT_LAYOUT_INDEX                  (D230.characterSelectLayoutIndex)
#define MM_CHARACTER_SELECT_META_1P2P                     (D230.characterSelectMeta1P2P)
#define MM_CHARACTER_SELECT_WINDOW_POS_BY_LAYOUT          (D230.characterSelectWindowPosByLayout)
#define MM_CHARACTER_SELECT_META_BY_LAYOUT                (D230.characterSelectMetaByLayout)
#define MM_CHARACTER_SELECT_LAYOUT_WINDOW_W               (D230.characterSelectLayout.windowW)
#define MM_CHARACTER_SELECT_LAYOUT_WINDOW_H               (D230.characterSelectLayout.windowH)
#define MM_CHARACTER_SELECT_LAYOUT_DRIVER_POS_Z           (D230.characterSelectLayout.driverPosZ)
#define MM_CHARACTER_SELECT_LAYOUT_DRIVER_POS_Y           (D230.characterSelectLayout.driverPosY)
#define MM_CHARACTER_SELECT_LAYOUT_TEXT_Y                 (D230.characterSelectLayout.textY)
#define MM_CHARACTER_SELECT_TRANSITION_BY_PLAYER_COUNT    (D230.characterSelectTransitionByPlayerCount)
#define MM_ROWS_MAIN_WITH_SCRAPBOOK                       (D230.rowsMainMenuWithScrapbook)
#define MM_ROWS_PLAYERS_1P2P                              (D230.rowsPlayers1P2P)
#define MM_ROWS_PLAYERS_1P2P3P4P                          (D230.rowsPlayers1P2P3P4P)
#define MM_ROWS_PLAYERS_2P3P4P                            (D230.rowsPlayers2P3P4P)
#define MM_CUP_DIFFICULTY_FIRST_UNLOCK_BIT                (D230.cupDifficulty.firstUnlockBit)
#define MM_CUP_DIFFICULTY_STRING_INDEX                    (D230.cupDifficulty.stringIndex)
#define MM_CUP_DIFFICULTY_STRING_INDEX_ASM_NAME           ""
#define MM_CUP_DIFFICULTY_SPEED                           (D230.cupDifficulty.speed)
#define MM_LANGUAGE_STRINGS_PAGE_OFFSET                   0
#define MM_CHARACTER_SELECT_TRANSITION_STATE              (D230.characterSelectTransitionState)
#define MM_CHARACTER_SELECT_FLAGS                         (sdata->characterSelectFlags)
#define MM_CHARACTER_SELECT_TRANSITION_FRAME              (D230.characterSelectTransitionFrame)
#define MM_CHARACTER_SELECT_MENU_STATE                    (D230.characterSelectMenuState)
#define MM_CHARACTER_MENU_ID                              (D230.characterMenuID)
#define MM_CHARACTER_SELECT_EXITS_FORWARD                 (D230.characterSelectExitsForward)
#define MM_MENU_CUP_SELECT                                (D230.menuCupSelect)
#define MM_MENU_TRACK_SELECT                              (D230.menuTrackSelect)
#define MM_CHARACTER_SELECT_FALLBACK_1                    (D230.characterSelectFallbackDirection1)
#define MM_CHARACTER_SELECT_FALLBACK_2                    (D230.characterSelectFallbackDirection2)
#define MM_CHARACTER_SELECT_FALLBACK_1_REPEAT             (D230.characterSelectFallbackDirection1)
#define MM_CHARACTER_SELECT_FALLBACK_2_REPEAT             (D230.characterSelectFallbackDirection2)
#define MM_PLAYER_NUMBER_STRINGS                          (D230.playerNumberStrings)
#define MM_CHARACTER_SELECT_OUTLINE_COLOR                 (D230.characterSelect_Outline)
#define MM_CHARACTER_SELECT_NEUTRAL_COLOR                 (D230.characterSelect_NeutralColor)
#define MM_CHARACTER_SELECT_CHOSEN_COLOR                  (D230.characterSelect_ChosenColor)
#define MM_CHARACTER_SELECT_BLUE_RECT_COLORS              (D230.characterSelect_BlueRectColors)
#define MM_TRACK_VIDEO_PREVIEW_FRAMES                     (D230.trackSelect.videoPreviewFrames)
#define MM_TRACK_VIDEO_MEM_ALLOCATED                      (D230.trackSelect.videoMemAllocated)
#define MM_TRACK_VIDEO_STATE_CURR                         (D230.trackSelect.videoStateCurr)
#define MM_TRACK_VIDEO_STATE_PREV                         (D230.trackSelect.videoStatePrev)
#define MM_VIDEO_STR_SRC_X                                (sdata_static.videoSTR_src_vramRect.x)
#define MM_VIDEO_STR_DST_X                                (sdata_static.videoSTR_dst_vramX)
#define MM_VIDEO_STR_DST_Y                                (sdata_static.videoSTR_dst_vramY)
#define MM_VIDEO_STR_SRC_RECT_ASM_NAME                    ""
#define MM_VIDEO_STR_SRC_RECT_PAGE_OFFSET                 0
#define MM_VIDEO_COLOR                                    (D230.videoCol)
#define MM_RECTMENU_DRAW_POLY_GT4                         RECTMENU_DrawPolyGT4
#define MM_CUP_SELECT_DRAW_TRACK(icon, posX, posY, primMem, ot, color) \
	RECTMENU_DrawPolyGT4((icon), (posX), (posY), (primMem), (ot), (color), (color), (color), (color), 0, FP(0.5))
#define MM_TRACK_LAP_BOX_OPEN                            (D230.trackSelect.lapBoxOpen)
#define MM_TRACK_LAP_BOX_OPEN_PAGE_OFFSET                0
#define MM_TRACK_TRANSITION_STATE                        (D230.trackSelect.transition.state)
#define MM_TRACK_TRANSITION_STATE_PAGE_OFFSET            0
#define MM_TRACK_TRANSITION_FRAME                        (D230.trackSelect.transition.frame)
#define MM_TRACK_TRANSITION_FRAME_PAGE_OFFSET            0
#define MM_TRACK_CURRENT_TRACK                           (D230.trackSelect.currentTrack)
#define MM_TRACK_SEL_BACKUP                              (sdata_static.trackSelBackup)
#define MM_BATTLE_TRACKS                                 (D230.battleTracks)
#define MM_TRACK_CHANGE_FRAMES                           (D230.trackSelect.trackChangeFrames)
#define MM_TRACK_CHANGE_FRAMES_PAGE_OFFSET               0
#define MM_TRACK_CHANGE_DIRECTION                        (D230.trackSelect.trackChangeDirection)
#define MM_TRACK_TRANSITION_START_AFTER_EXIT             (D230.trackSelect.transition.startAfterExit)
#define MM_TRACK_TRANSITION_START_AFTER_EXIT_PAGE_OFFSET 0
#define MM_TRACK_TRANSITIONS                             (D230.trackTransitions.transitionMeta_trackSel)
#define MM_TRACK_PREVIEW_Y                               (MM_TRACK_TRANSITIONS[1].currY)
#define MM_MENU_LAP_SELECT                               (D230.menuLapSel)
#define MM_MENU_BATTLE_WEAPONS                           (D230.menuBattleWeapons)
#define MM_LAP_COUNT_BY_ROW                              (D230.lapCountByRow)
#define MM_TIME_TRIAL_STAR_COLORS                        (D230.timeTrialStars.colorIndex)
#define MM_TIME_TRIAL_STAR_FLAGS                         (D230.timeTrialStars.beatenFlagBit)
#define MM_TRACK_DRAW_MAP_OFFSET                         (D230.drawMapOffset)
#define MM_CUP_TRANSITIONS                               (D230.transitionMeta_cupSel)
#define MM_CUP_TRANSITIONS_ASM_NAME                      ""
#define MM_CUP_STAR_COLORS                               (D230.cupSelectStars.colorIndex)
#define MM_CUP_STAR_COLORS_ASM_NAME                      ""
#define MM_CUP_STAR_WIN_BITS                             (D230.cupSelectStars.winBitBase)
#define MM_CUP_STAR_WIN_BITS_ASM_NAME                    ""
#define MM_CUP_COLOR                                     (D230.cupSel_Color)
#define MM_CUP_TRANSITION_STATE                          (D230.cupSelectTransition.state)
#define MM_CUP_TRANSITION_START_AFTER_EXIT               (D230.cupSelectTransition.startAfterExit)
#define MM_CUP_TRANSITION_FRAME                          (D230.cupSelectTransition.frame)
#define MM_BATTLE_TRANSITIONS                            (D230.transitionMeta_battle)
#define MM_BATTLE_TRANSITION_STATE                       (D230.battleTransition.state)
#define MM_BATTLE_TRANSITION_START_AFTER_EXIT            (D230.battleTransition.startAfterExit)
#define MM_BATTLE_TRANSITION_FRAME                       (D230.battleTransition.frame)
#define MM_BATTLE_MENU_ARRAY                             (D230.battleMenuArray)
#define MM_BATTLE_SETTINGS                               (sdata_static.battleSettings)
#define MM_MENU_BATTLE_TYPE                              (D230.menuBattleType)
#define MM_MENU_BATTLE_LENGTH_LIFE_TIME                  (D230.menuBattleLengthLifeTime)
#define MM_MENU_BATTLE_LENGTH_POINTS                     (D230.menuBattleLengthPoints)
#define MM_MENU_BATTLE_LENGTH_TIME_TIME                  (D230.menuBattleLengthTimeTime)
#define MM_MENU_BATTLE_LENGTH_LIFE_LIFE                  (D230.menuBattleLengthLifeLife)
#define MM_MENU_BATTLE_START_GAME                        (D230.menuBattleStartGame)
#define MM_BATTLE_WEAPON_ITEMS                           (D230.battleWeaponItems)
#define MM_BATTLE_WEAPON_ITEMS_ASM_NAME                  ""
#define MM_BATTLE_TYPE_MODE_FLAGS                        (D230.battleSetupTables.typeModeFlags)
#define MM_BATTLE_TIME_LIMIT_MINUTES                     (D230.battleSetupTables.timeLimitMinutes)
#define MM_BATTLE_LIFE_TIME_LIMIT_MINUTES                (D230.battleSetupTables.lifeModeTimeLimitMinutes)
#define MM_BATTLE_LIFE_LIMIT_VALUES                      (D230.battleSetupTables.lifeLimitValues)
#define MM_BATTLE_POINT_LIMIT_VALUES                     (D230.battleSetupTables.pointLimitValues)
#define MM_BATTLE_WEAPON_ENABLED_COLOR                   (D230.battleWeaponEnabledColor)
#define MM_BATTLE_WEAPON_DISABLED_COLOR                  (D230.battleWeaponDisabledColor)
#define MM_BATTLE_WEAPON_PANEL_COLOR                     (D230.battleWeaponPanelColor)
#define MM_BATTLE_EXPAND_MENU                            (sdata_static.battleSetupExpandMenu)
#define MM_BATTLE_ROW_HIGHLIGHTED                        (sdata_static.battleSetupRowHighlighted)
#define MM_BATTLE_ROW_HIGHLIGHTED_PAGE_OFFSET            0
#define MM_BATTLE_ROW_HIGHLIGHTED_UPDATE                 (sdata_static.battleSetupRowHighlighted)
#define MM_BATTLE_ROW_HIGHLIGHTED_UNSIGNED               ((u16)sdata_static.battleSetupRowHighlighted)
#define MM_BATTLE_WEAPON_HIGHLIGHTED                     (sdata_static.battleSetupWeaponHighlighted)
#define MM_BATTLE_WEAPON_HIGHLIGHTED_PAGE_OFFSET         0
#define MM_BATTLE_TEAM_OF_EACH_PLAYER                    (sdata_static.teamOfEachPlayer)
#define MM_HIGHSCORE_MENU                                (D230.menuHighScore)
#define MM_HIGHSCORE_TARGET_TRACK                        (D230.highScoreSelection.targetTrack)
#define MM_HIGHSCORE_TRANSITION_STATE                    (D230.highScoreTransition.state)
#define MM_HIGHSCORE_TRANSITION_MAIN_FRAME               (D230.highScoreTransition.mainFrame)
#define MM_HIGHSCORE_TRANSITION_TRACK_FRAME              (D230.highScoreTransition.trackFrame)
#define MM_HIGHSCORE_TRANSITION_ROW_FRAME                (D230.highScoreTransition.rowFrame)
#define MM_HIGHSCORE_TRANSITION_ROW_FRAME_ASM_NAME       ""
#define MM_HIGHSCORE_TRANSITION_ROW_FRAME_PAGE_OFFSET    0
#define MM_HIGHSCORE_ACTIVE_HORIZONTAL_MOVE              (D230.highScoreTransition.activeHorizontalMove)
#define MM_HIGHSCORE_PENDING_HORIZONTAL_MOVE             (D230.highScoreTransition.pendingHorizontalMove)
#define MM_HIGHSCORE_ACTIVE_VERTICAL_MOVE                (D230.highScoreTransition.activeVerticalMove)
#define MM_HIGHSCORE_PENDING_VERTICAL_MOVE               (D230.highScoreTransition.pendingVerticalMove)
#define MM_HIGHSCORE_TARGET_ROW                          (D230.highScoreSelection.targetRow)
#define MM_HIGHSCORE_CURRENT_TRACK                       (D230.highScoreSelection.currentTrack)
#define MM_HIGHSCORE_CURRENT_ROW                         (D230.highScoreSelection.currentRow)
#define MM_HIGHSCORE_ICON_COLOR                          (D230.highscore_iconColor)
#define MM_HIGHSCORE_TRANSITIONS                         (D230.transitionMeta_HighScores)
#define MM_HIGHSCORE_GHOST_STAR_COLORS                   (D230.highScoreGhostStars.colorIndex)
#define MM_HIGHSCORE_GHOST_STAR_COLORS_ASM_NAME          ""
#define MM_HIGHSCORE_GHOST_STAR_FLAGS                    (D230.highScoreGhostStars.beatenFlagBit)
#define MM_HIGHSCORE_GHOST_STAR_FLAGS_ASM_NAME           ""
#define MM_MENU_GHOST_SELECTION                          (data.menuGhostSelection)
#define MM_MENU_QUEUE_LOAD_TRACK                         (data.menuQueueLoadTrack)
#define MM_LOADED_GHOST_DATA                             (R230.s_loaded_ghost_data)
#define MM_ALLOC_HIGH_MEM                                MEMPACK_AllocHighMem
#define MM_LEVEL_METADATA                                (data.metaDataLEV)
#define MM_LEVEL_METADATA_PAGE_VALUE                     0U
#define MM_ARCADE_CUPS                                   (data.ArcadeCups)
#define MM_ARCADE_CUPS_ASM_NAME                          ""
#define MM_REPLAY_HUMAN_GHOST                            (sdata_static.boolReplayHumanGhost)
#define MM_ERROR_MESSAGE_POS_INDEX                       (sdata_static.errorMessagePosIndex)
#define MM_GHOST_TAPE_PLAYING                            (sdata_static.ptrGhostTapePlaying)
#define MM_USELESS_LAP_ROW_COPY                          (sdata_static.uselessLapRowCopy)
#define MM_MENU_ROW_HIGHLIGHT_NORMAL_PAGE_OFFSET         0
#define MM_DECALHUD_DRAW_POLY_GT4(icon, x, y, primMem, ot, c0, c1, c2, c3, transparency, scale)                                                         \
	DecalHUD_DrawPolyGT4((icon), (x), (y), (primMem), (ot), ColorCode_Load(&(c0)), ColorCode_Load(&(c1)), ColorCode_Load(&(c2)), ColorCode_Load(&(c3)), \
	                     (transparency), (scale))
#define MM_DECALHUD_ARROW_2D(icon, posX, posY, primMem, ot, color0, color1, color2, color3, transparency, scale, rotation)                     \
	DecalHUD_Arrow2D((icon), (posX), (posY), (primMem), (ot), ColorCode_Load(&(color0)), ColorCode_Load(&(color1)), ColorCode_Load(&(color2)), \
	                 ColorCode_Load(&(color3)), (transparency), (scale), (rotation))

#define MM_DRAW_CLEAR_BOX               CTR_Box_DrawClearBox
#define MM_DRAW_SOLID_BOX_WITH_PRIM_MEM CTR_Box_DrawSolidBox
#define MM_DRAW_LINE_WIDE_FLAGS         DecalFont_DrawLine

#include "230/R230.c"
#include "230/D230.c"
#include "230/MM_Battle.c"
#include "230/MM_ConfigMenu.c"
#include "230/MM_ControlsConfig.c"
#include "230/MM_MenuFlow.c"
#include "230/MM_QuickRace.c"
#include "230/MM_Title.c"
#include "230/MM_CheatCodes.c"
#include "230/MM_OxideModel.c"
#include "230/MM_Characters.c"
#include "230/MM_TrackSelect.c"
#include "230/MM_CupSelect.c"
#include "230/MM_HighScore.c"
#include "230/MM_Scrapbook.c"
#include "230/MM_Video.c"
