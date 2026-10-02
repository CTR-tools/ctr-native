#ifndef CTR_REFRESH_CARD_UNKNOWN3_RETAIL_SYMBOLS_H
#define CTR_REFRESH_CARD_UNKNOWN3_RETAIL_SYMBOLS_H

#define REFRESHCARD_SCREEN_TEXT      ctr_refreshCardScreenText
#define REFRESHCARD_MC_START         ctr_refreshCardStart
#define REFRESHCARD_ERROR_FLAG       ctr_refreshCardError
#define REFRESHCARD_DATA_OUTDATED    ctr_refreshCardDataOutdated
#define REFRESHCARD_ACTION_MODE      ctr_refreshCardAction
#define REFRESHCARD_UNK_8008D928     ctr_refreshCardUnk8008d928
#define REFRESHCARD_HUMAN_GHOST      ctr_refreshCardHumanGhost
#define REFRESHCARD_UNK_8008D964     ctr_refreshCardUnk8008d964
#define REFRESHCARD_UNK_8008D95C     ctr_refreshCardUnk8008d95c
#define REFRESHCARD_PROFILES_CHECKED ctr_refreshCardProfilesChecked
#define REFRESHCARD_SLOTS_ICON       ctr_refreshCardSlotsIcon
#define REFRESHCARD_GHOST_FILENAME   ctr_refreshCardFinalName
#include "../../retail_bindings.h"

// NOTE(aalhendi): Individual resident symbols preserve GCC 2.8.1 GP scheduling.
// These aliases use the shared storage widths; they define no new storage.
extern s16 ctr_refreshCardScreenText asm("1294($28)");

extern s16 ctr_refreshCardStart asm("1292($28)");
extern b16 ctr_refreshCardError asm("2584($28)");
extern b16 ctr_refreshCardDataOutdated asm("2520($28)");
extern s16 ctr_refreshCardAction asm("2572($28)");
extern b16 ctr_refreshCardUnk8008d928 asm("2492($28)");
extern b16 ctr_refreshCardHumanGhost asm("2540($28)");
extern b16 ctr_refreshCardUnk8008d964 asm("2552($28)");
extern b16 ctr_refreshCardUnk8008d95c asm("2544($28)");
extern b16 ctr_refreshCardProfilesChecked asm("2556($28)");
extern u8 ctr_refreshCardSlotsIcon[0x38] asm("data+20568");
extern char ctr_refreshCardFinalName[0x20] asm("sdata_static+49944");
#endif
