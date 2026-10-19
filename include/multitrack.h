#ifndef MULTITRACK_H
#define MULTITRACK_H

/*
 * MultiTrack (native): every custom track of the CTR-MultiTrack ROM.
 * Track data is appended to BIGFILE.BIG by scripts/pack_tracks.py; each
 * track owns 8 entries laid out like a stock level slot. The track table
 * (MTHeader + MTEntry[count]) is assets/track_table.bin, the same bytes the
 * PS1 build compiles onto the end of overlay 230. Selecting a custom track
 * makes LOAD_GetBigfileIndex redirect its host slot to those entries.
 * Port of psx-modding-toolchain/games/CTR/mods/MultiTrack.
 */

#define MT_MAGIC 0x4B52544D // 'MTRK'

#define MT_NAME_LEN   28
#define MT_AUTHOR_LEN 16

enum MTFlags
{
	MT_FLAG_BATTLE = 1,
	MT_FLAG_NO_AI = 2, // lev has no nav paths: arcade bots would have nowhere to drive
};

struct MTEntry
{
	u8 slot;    // host levelID
	u8 flags;   // MTFlags
	u16 bigBase; // first of this track's 8 bigfile entries
	char name[MT_NAME_LEN];
	char author[MT_AUTHOR_LEN];
};

struct MTHeader
{
	int magic;
	int count;
	struct MTEntry entries[0];
};

struct MTState
{
	int magic;         // MT_MAGIC once the menu has initialised this struct
	int redirectLevel; // host levelID to redirect, -1 = none
	int redirectBase;  // bigfile index replacing levelID*8
	short cursor[2];   // list cursor for race / battle lists
};

extern struct MTState gMTState;
#define MT_STATE (&gMTState)

// track table, NULL-safe: count 0 when assets/track_table.bin is missing
struct MTHeader *MT_Table(void);

// custom track currently loaded (redirect active for gGT->levelID), or NULL
struct MTEntry *MT_ActiveEntry(void);

// CTR_BOOT_LEVEL / CTR_BOOT_TRACK env: boot straight into a level (debug, screenshots)
int MT_BootLevel(int defaultLevel);

#endif
