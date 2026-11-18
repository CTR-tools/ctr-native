#include <common.h>

// Each game-facing suite owns its sdata_static fixture. Pull this archive member
// in only when that suite references the resident pointer.
struct sData *sdata = &sdata_static;
