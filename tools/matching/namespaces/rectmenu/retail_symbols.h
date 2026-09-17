#define RECTMENU_TIME_BUFFER      rectMenuTimeBuffer
#define RECTMENU_PLAYER_TAP       rectMenuPlayerTap
#define RECTMENU_PLAYER_HOLD      rectMenuPlayerHold
#define RECTMENU_FONT_HEIGHT      rectMenuFontHeight
#define RECTMENU_QUIP_PARAMS      rectMenuQuipParams
#define RECTMENU_QUIP_Y           rectMenuQuipY
#define RECTMENU_BORDER_NORMAL    rectMenuBorderNormal
#define RECTMENU_BORDER_ALT       rectMenuBorderAlt
#define RECTMENU_BOX_COLOR_0      rectMenuBoxColor0
#define RECTMENU_BOX_COLOR_1      rectMenuBoxColor1
#define RECTMENU_BOX_COLOR_2      rectMenuBoxColor2
#define RECTMENU_HIGHLIGHT_GREEN  rectMenuHighlightGreen
#define RECTMENU_ANY_TAP          rectMenuAnyTap
#define RECTMENU_ANY_HOLD         rectMenuAnyHold
#define RECTMENU_ACTIVE_SUBMENU   rectMenuActiveSub
#define RECTMENU_FRAMES_REMAINING rectMenuFramesRemaining
#define RECTMENU_ADV_RNG          rectMenuAdvRng
#define RECTMENU_DRAW_GT4         rectMenuDrawGT4

#include "../../retail_bindings.h"

extern char rectMenuTimeBuffer[32] asm("sdata_static+49912");
extern u32 rectMenuPlayerTap[4] asm("sdata_static+55844");
extern s32 rectMenuPlayerHold[4] asm("sdata_static+55800");
extern s16 rectMenuFontHeight[FONT_NUM] asm("data+6600");
extern s16 rectMenuQuipParams[8] asm("data+20820");
extern s16 rectMenuQuipY[4] asm("data+20828");
extern u32 rectMenuBorderNormal asm("sdata_static+1228");
extern u32 rectMenuBorderAlt asm("sdata_static+1232");
extern Color rectMenuBoxColor0 asm("sdata_static+1268");
extern Color rectMenuBoxColor1 asm("sdata_static+1272");
extern Color rectMenuBoxColor2 asm("sdata_static+1276");
extern struct RngDeadCoedState rectMenuAdvRng asm("sdata_static+1788");
extern Color rectMenuHighlightGreen asm("sdata_static+2524");

// NOTE(aalhendi): The resident callers use these small symbols through $gp;
// their addresses are supplied by the matching target, without new storage.
extern s16 rectMenuFramesRemaining;
extern s32 rectMenuAnyTap;
extern s32 rectMenuAnyHold;
extern struct RectMenu *rectMenuActiveSub;

// NOTE(aalhendi): This wrapper zero-extends the byte transparency and sign-extends
// the short scale into caller-wide arguments; DecalHUD consumes their low bits.
extern void rectMenuDrawGT4(struct Icon *icon, s32 posX, s32 posY, struct PrimMem *primMem, u32 *ot, Color color0, Color color1, Color color2, Color color3,
                            s32 transparency, s32 scale) asm("DecalHUD_DrawPolyGT4");
