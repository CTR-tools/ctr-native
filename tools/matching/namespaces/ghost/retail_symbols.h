#define GHOST_RECORDING      ghostRecording
#define GHOST_PLAYING        ghostPlaying
#define GHOST_CAN_SAVE       ghostCanSave
#define GHOST_TOO_BIG        ghostTooBig
#define GHOST_OVERFLOW_TIMER ghostOverflowTimer
#define GHOST_DRAWING        ghostDrawing
#define GHOST_TAPES          ghostTapes
#define GHOST_REPLAY_HUMAN   ghostReplayHuman
#define GHOST_NAME           ghostName
#define GHOST_NAME_STAFF     ghostNameStaff
#define GHOST_NAME_HUMAN     ghostNameHuman

#include "../../retail_bindings.h"

extern struct GhostRecording ghostRecording asm("sdata_static+11400");
extern struct GhostHeader *ghostPlaying;
extern b16 ghostCanSave;
extern b16 ghostTooBig;
extern s16 ghostOverflowTimer;
extern b16 ghostDrawing;
extern struct GhostTape *ghostTapes[2] asm("sdata_static+2016");
extern b16 ghostReplayHuman asm("sdata_static+2540");
extern char ghostName[8] asm("sdata_static+212");
extern char ghostNameStaff[] asm("sdata_static+220");
extern char ghostNameHuman[] asm("sdata_static+228");
