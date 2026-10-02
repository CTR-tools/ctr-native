#ifndef CTR_SUBMIT_NAME_RETAIL_SYMBOLS_H
#define CTR_SUBMIT_NAME_RETAIL_SYMBOLS_H

#define SUBMIT_NAME_TYPE_TIMER     ctr_submitNameTypeTimer
#define SUBMIT_NAME_MODE           ctr_submitNameMode
#define SUBMIT_NAME_UNICODE        ctr_submitNameUnicode
#define SUBMIT_NAME_SOUND_INDEX    ctr_submitNameSoundIndex
#define SUBMIT_NAME_TAPS           ctr_submitNameTaps
#define SUBMIT_NAME_EDGE_COLOR     ctr_submitNameEdgeColor
#define SUBMIT_NAME_UNDERSCORE     ctr_submitNameUnderscore
#define SUBMIT_NAME_DESIRED_MENU   ctr_submitNameDesiredMenu
#define SUBMIT_NAME_MENU_GHOST     ctr_submitNameMenuGhost
#define SUBMIT_NAME_MENU_ADVENTURE ctr_submitNameMenuAdventure
#include "../../retail_bindings.h"

// NOTE(aalhendi): These aliases name canonical resident storage and define no data.
extern __typeof__(sdata->typeTimer) ctr_submitNameTypeTimer asm("2438($28)");
extern __typeof__(sdata->selectProfileState.submitNameMode) ctr_submitNameMode asm("2458($28)");
extern __typeof__(data.unicodeAscii) ctr_submitNameUnicode asm("data+21492");
extern __typeof__(data.soundIndexArray) ctr_submitNameSoundIndex asm("data+21572");
extern __typeof__(sdata->buttonTapPerPlayer) ctr_submitNameTaps asm("sdata_static+55844");
extern __typeof__(sdata->battleSetup_Color_UI_1) ctr_submitNameEdgeColor asm("sdata_static+1228");
// NOTE(aalhendi): Incomplete bounds preserve the address scheduling of this char[4] field.
extern char ctr_submitNameUnderscore[] asm("sdata_static+1344");
extern __typeof__(sdata->ptrDesiredMenu) ctr_submitNameDesiredMenu asm("2488($28)");
extern __typeof__(data.menuGhostSelection) ctr_submitNameMenuGhost asm("data+21012");
extern __typeof__(data.menuFourAdvProfiles) ctr_submitNameMenuAdventure asm("data+20968");
#endif
