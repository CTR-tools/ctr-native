#ifndef CTR_SELECT_PROFILE_SUPPORT_RETAIL_SYMBOLS_H
#define CTR_SELECT_PROFILE_SUPPORT_RETAIL_SYMBOLS_H

#define SELECT_PROFILE_PADDED_FORMAT        ctr_selectProfilePaddedFormat
#define SELECT_PROFILE_INTEGER_FORMAT       ctr_selectProfileIntegerFormat
#define SELECT_PROFILE_SPIN_RATE            ctr_selectProfileSpinRate
#define SELECT_PROFILE_LOAD_SAVE_META       ctr_selectProfileLoadSaveMeta
#define SELECT_PROFILE_MENU_GREEN_LOAD_SAVE ctr_selectProfileMenuGreenLoadSave
#define SELECT_PROFILE_LOAD_SAVE_OBJECT     ctr_selectProfileLoadSaveObject
#define SELECT_PROFILE_LOAD_SAVE_ICONS      ctr_selectProfileLoadSaveIcons
#define SELECT_PROFILE_SPIN_OFFSET          ctr_selectProfileSpinOffset
#define SELECT_PROFILE_DESIRED_MENU         ctr_selectProfileDesiredMenu
#define SELECT_PROFILE_INSTANCE_NAME        ctr_selectProfileInstanceName
#define SELECT_PROFILE_RED_COLOR            ctr_selectProfileRedColor
#define SELECT_PROFILE_GHOST_COLOR          ctr_selectProfileGhostColor
#define SELECT_PROFILE_HIGHLIGHT_NORMAL     ctr_selectProfileHighlightNormal
#define SELECT_PROFILE_HIGHLIGHT_GREEN      ctr_selectProfileHighlightGreen
#define SELECT_PROFILE_GREY_COLOR           ctr_selectProfileGreyColor
#define SELECT_PROFILE_GREEN_COLOR          ctr_selectProfileGreenColor
#define SELECT_PROFILE_PERCENT_TEXT         ((char *)&ctr_selectProfilePercentText)
#define SELECT_PROFILE_MENU_ADVENTURE       ctr_selectProfileMenuAdventure
#define SELECT_PROFILE_MENU_GHOST           ctr_selectProfileMenuGhost
#define SELECT_PROFILE_MENU_WARNING         ctr_selectProfileMenuWarning
#define SELECT_PROFILE_MENU_OVERWRITE       ctr_selectProfileMenuOverwrite
#define SELECT_PROFILE_MODE                 ctr_selectProfileMode
#define SELECT_PROFILE_ACTION_ACTIVE        ctr_selectProfileActionActive
#define SELECT_PROFILE_EXIT_TO_PREVIOUS     ctr_selectProfileExitToPrevious
#define SELECT_PROFILE_ACTION_DONE          ctr_selectProfileActionDone
#define SELECT_PROFILE_OVERWRITE_PROMPT     ctr_selectProfileOverwritePrompt
#define SELECT_PROFILE_TIMEOUT_PROMPT       ctr_selectProfileTimeoutPrompt
#define SELECT_PROFILE_TIMER_SAVE_COMPLETE  ctr_selectProfileTimerSaveComplete
#define SELECT_PROFILE_ACTION               ctr_selectProfileAction
#define SELECT_PROFILE_SCREEN_TEXT          ctr_selectProfileScreenText
#define SELECT_PROFILE_BUTTON_TAP           (ctr_selectProfileButtonTap[0])
#define SELECT_PROFILE_ROW_BACKUP           ctr_selectProfileRowBackup
#include "../../retail_bindings.h"

// NOTE(aalhendi): These aliases name existing resident storage. Incomplete
// format-string bounds keep their addresses out of GCC's small-data class.
extern char ctr_selectProfilePaddedFormat[] asm("sdata_static+1304");
extern char ctr_selectProfileIntegerFormat[] asm("sdata_static+1312");
extern s16 ctr_selectProfileSpinRate[4] asm("sdata_static+1296");
extern __typeof__(data.MetaDataLoadSave) ctr_selectProfileLoadSaveMeta asm("data+21188");

extern struct RectMenu ctr_selectProfileMenuGreenLoadSave asm("data+20924");
// NOTE(aalhendi): Incomplete bounds expose address high/low scheduling to GCC;
// canonical storage remains char[16] for the name and s16[4] for the spin offsets.
extern char ctr_selectProfileInstanceName[] asm("rdata+5696");
extern s32 ctr_selectProfileLoadSaveObject asm("2440($28)");
extern struct SelectProfileLoadSaveIcon ctr_selectProfileLoadSaveIcons[12] asm("sdata_static+55860");
extern s16 ctr_selectProfileSpinOffset[] asm("data+21356");
extern struct RectMenu *ctr_selectProfileDesiredMenu asm("2488($28)");
extern s32 ctr_selectProfileRedColor asm("sdata_static+1328");
extern s32 ctr_selectProfileGhostColor asm("sdata_static+1332");
extern Color ctr_selectProfileHighlightNormal asm("sdata_static+2528");
extern Color ctr_selectProfileHighlightGreen asm("sdata_static+2524");
extern s32 ctr_selectProfileGreyColor asm("sdata_static+1316");
extern s32 ctr_selectProfileGreenColor asm("sdata_static+1320");
extern s32 ctr_selectProfilePercentText asm("sdata_static+1324");

// NOTE(aalhendi): Typed names for existing storage; these aliases define no storage.
extern struct RectMenu ctr_selectProfileMenuAdventure asm("data+20968");
extern struct RectMenu ctr_selectProfileMenuGhost asm("data+21012");
extern struct RectMenu ctr_selectProfileMenuWarning asm("data+21056");
extern struct RectMenu ctr_selectProfileMenuOverwrite asm("data+21392");
extern s16 ctr_selectProfileMode asm("2444($28)");
extern s16 ctr_selectProfileActionActive asm("2446($28)");
extern s16 ctr_selectProfileExitToPrevious asm("2448($28)");
extern s16 ctr_selectProfileActionDone asm("2450($28)");
extern s16 ctr_selectProfileOverwritePrompt asm("2452($28)");
extern s16 ctr_selectProfileTimeoutPrompt asm("2454($28)");
extern s16 ctr_selectProfileTimerSaveComplete asm("2456($28)");
extern s16 ctr_selectProfileAction asm("2572($28)");
extern s16 ctr_selectProfileScreenText asm("1294($28)");
// NOTE(aalhendi): Preserve the array shape so GCC retains its address across calls.
extern u32 ctr_selectProfileButtonTap[4] asm("sdata_static+55844");
extern s32 ctr_selectProfileRowBackup asm("sdata_static+2000");
#endif
