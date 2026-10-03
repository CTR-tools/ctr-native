#ifndef CTR_BOTS_INIT_DIFFICULTY_RETAIL_SYMBOLS
#define CTR_BOTS_INIT_DIFFICULTY_RETAIL_SYMBOLS
// NOTE(aalhendi): Typed aliases name measured resident storage and define no data.
#define BOT_NAV_HEADERS         ctr_botsNavHeaders
#define BOT_NAV_FRAMES          ctr_botsNavFrames
#define BOT_NAV_FALLBACK_HEADER ctr_botsNavFallback
#define BOT_NAV_NUM_POINTS      ctr_botsNavCount
#define BOT_NAV_LAST_PATH       ctr_botsLastPath
#include "../../retail_bindings.h"
extern struct NavHeader *ctr_botsNavHeaders[3] asm("sdata_static+2944");
extern struct NavFrame *ctr_botsNavFrames[3] asm("sdata_static+2932");
extern struct NavHeader ctr_botsNavFallback asm("sdata_static+2836");
extern u16 ctr_botsNavCount asm("1832($28)");
extern u16 ctr_botsLastPath asm("1796($28)");

#define BOTS_ADV_PROGRESS ctr_botsAdvProgress
extern struct AdvProgress ctr_botsAdvProgress asm("sdata_static+11320");
#define BOTS_KART_ORDER   ctr_botsKartOrder
#define BOTS_DRIVER_PATHS ctr_botsDriverPaths
#define BOTS_ACCEL_ORDER  ctr_botsAccelOrder
#define BOTS_NAV_LIST     ctr_botsNavList
#define BOTS_ADV_RNG      ctr_botsAdvRng
extern char ctr_botsKartOrder[8] asm("sdata_static+1840");
extern char ctr_botsDriverPaths[8] asm("sdata_static+12");
extern u8 ctr_botsAccelOrder[8] asm("sdata_static+1812");
extern struct LinkedList ctr_botsNavList[3] asm("sdata_static+2956");
extern struct RngDeadCoedState ctr_botsAdvRng asm("sdata_static+1788");

#define BOTS_ROBOT_DEBUG_NAME ctr_botsRobotName
extern const char ctr_botsRobotName[] asm("0x8001000c");
#endif
