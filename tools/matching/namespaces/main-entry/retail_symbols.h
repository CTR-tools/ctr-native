#ifndef CTR_MAIN_ENTRY_RETAIL_SYMBOLS_H
#define CTR_MAIN_ENTRY_RETAIL_SYMBOLS_H

#include <common.h>

// NOTE(aalhendi): Retail main imports these resident GP-relative small-data
// slots directly rather than retaining the full sData base in a register.
extern s32 ctr_main_gameState;
extern LoadStage ctr_main_loadStage;
extern s16 ctr_main_levIdToLoad;
extern u32 ctr_main_addBits0;
extern u32 ctr_main_remBits0;
extern u32 ctr_main_addBits8;
extern u32 ctr_main_remBits8;
extern struct GameTracker *ctr_main_gGT;
extern struct GamepadSystem *ctr_main_gGS;
extern void *ctr_main_drawSyncPtr;
extern s32 ctr_main_vlcLoaded;
extern struct BigHeader *ctr_main_bigfile1;
extern char **ctr_main_lngStrings;
extern s16 ctr_main_boolDraw3DAdvMask;
extern s32 ctr_main_boolSoundPaused;

// NOTE(aalhendi): These resident accesses use absolute pages in retail.
// The level-name word view stays an incomplete array of a struct member so
// GCC emits an absolute load without hoisting it across levelID's store.
struct CtrMainLevelNameWord
{
	u32 word;
};

extern struct CtrMainLevelNameWord ctr_main_s_ndi[] asm("sdata_static+512");
extern s16 ctr_main_pause_state asm("sdata_static+202");
extern XAState ctr_main_xa_state asm("sdata_static+1948");
extern u16 ctr_main_frame_counter asm("sdata_static+2564");
extern s16 ctr_main_menu_state asm("sdata_static+2576");

#define MAIN_GAME_STATE     ctr_main_gameState
#define MAIN_LOAD_STAGE     ctr_main_loadStage
#define MAIN_LEV_ID_TO_LOAD ctr_main_levIdToLoad
#define MAIN_ADD_BITS_0     ctr_main_addBits0
#define MAIN_REM_BITS_0     ctr_main_remBits0
#define MAIN_ADD_BITS_8     ctr_main_addBits8
#define MAIN_REM_BITS_8     ctr_main_remBits8
#define MAIN_GT             ctr_main_gGT
#define MAIN_GS             ctr_main_gGS
#define MAIN_DRAW_SYNC_PTR  ctr_main_drawSyncPtr
#define MAIN_VLC_LOADED     ctr_main_vlcLoaded
#define MAIN_BIGFILE_1      ctr_main_bigfile1
#define MAIN_LNG_STRINGS    ctr_main_lngStrings
#define MAIN_ADV_MASK_3D    ctr_main_boolDraw3DAdvMask
#define MAIN_SOUND_PAUSED   ctr_main_boolSoundPaused
#define MAIN_S_NDI_WORD     (ctr_main_s_ndi[0].word)
#define MAIN_PAUSE_STATE    ctr_main_pause_state
#define MAIN_XA_STATE       ctr_main_xa_state
#define MAIN_FRAME_COUNTER  ctr_main_frame_counter
#define MAIN_MENU_STATE     ctr_main_menu_state

#endif
