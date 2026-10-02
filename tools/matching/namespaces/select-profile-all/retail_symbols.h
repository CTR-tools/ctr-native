#ifndef CTR_SELECT_PROFILE_ALL_RETAIL_SYMBOLS_H
#define CTR_SELECT_PROFILE_ALL_RETAIL_SYMBOLS_H

#define SELECT_PROFILE_MEMCARD_BUFFER       ctr_selectProfileMemcardBuffer
#define SELECT_PROFILE_MESSAGE_RECT_BYTES   ctr_selectProfileMessageRectBytes
#define SELECT_PROFILE_GHOST_TAPE           ctr_selectProfileGhostTape
#define SELECT_PROFILE_FREE_BYTES           ctr_selectProfileFreeBytes
#define SELECT_PROFILE_SAVE_CUP             ctr_selectProfileSaveCup
#define SELECT_PROFILE_UNK_8008D928         ctr_selectProfileUnk8008d928
#define SELECT_PROFILE_DATA_OUTDATED        ctr_selectProfileDataOutdated
#define SELECT_PROFILE_UNK_8008D95C         ctr_selectProfileUnk8008d95c
#define SELECT_PROFILE_UNK_8008D964         ctr_selectProfileUnk8008d964
#define SELECT_PROFILE_ADV_INDEX            ctr_selectProfileAdvIndex
#define SELECT_PROFILE_FRAME_COUNTER        ctr_selectProfileFrameCounter
#define SELECT_PROFILE_ERROR_FLAG           ctr_selectProfileErrorFlag
#define SELECT_PROFILE_FONT_HEIGHT          ctr_selectProfileFontHeight
#define SELECT_PROFILE_MESSAGE_SCREENS      ctr_selectProfileMessageScreens
#define SELECT_PROFILE_GHOST_TEXT           ctr_selectProfileGhostText
#define SELECT_PROFILE_ADV_TEXT             ctr_selectProfileAdvText
#define SELECT_PROFILE_MENU_SUBMIT_NAME     ctr_selectProfileMenuSubmitName
#define SELECT_PROFILE_MENU_QUEUE_LOAD_HUB  ctr_selectProfileMenuQueueLoadHub
#define SELECT_PROFILE_MENU_OVERWRITE_GHOST ctr_selectProfileMenuOverwriteGhost
#define SELECT_PROFILE_MENU_SAVE_GAME       ctr_selectProfileMenuSaveGame
#include "../select-profile-support/retail_symbols.h"

// NOTE(aalhendi): These typed aliases name canonical storage and define no data.
extern void *ctr_selectProfileMemcardBuffer asm("1288($28)");
extern char ctr_selectProfileMessageRectBytes[] asm("sdata_static+1336");
extern struct GhostHeader *ctr_selectProfileGhostTape asm("sdata_static+2024");
extern s32 ctr_selectProfileFreeBytes asm("sdata_static+2368");
extern b16 ctr_selectProfileSaveCup asm("2476($28)");
extern b16 ctr_selectProfileUnk8008d928 asm("2492($28)");
extern b16 ctr_selectProfileDataOutdated asm("2520($28)");
extern b16 ctr_selectProfileUnk8008d95c asm("2544($28)");
extern b16 ctr_selectProfileUnk8008d964 asm("2552($28)");
extern s16 ctr_selectProfileAdvIndex asm("2560($28)");
extern s32 ctr_selectProfileFrameCounter asm("2564($28)");
extern b16 ctr_selectProfileErrorFlag asm("2584($28)");
extern s16 ctr_selectProfileFontHeight[4] asm("data+6600");
extern s32 ctr_selectProfileMessageScreens[10] asm("data+20836");
extern s16 ctr_selectProfileGhostText[4] asm("data+21364");
extern s16 ctr_selectProfileAdvText[6] asm("data+21480");
extern struct RectMenu ctr_selectProfileMenuSubmitName asm("data+21100");
extern struct RectMenu ctr_selectProfileMenuQueueLoadHub asm("data+21144");
extern struct RectMenu ctr_selectProfileMenuOverwriteGhost asm("data+21436");
extern struct RectMenu ctr_selectProfileMenuSaveGame asm("data+20680");
#endif
