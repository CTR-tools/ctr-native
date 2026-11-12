#ifndef CTR_EXTENDED_LAP_TIMES_H
#define CTR_EXTENDED_LAP_TIMES_H

/* The existing per-player timing slots are part of resident state and therefore
   follow race resets and checkpoints. For long races only slots 0 and 1 are
   used, for the best completed lap and its zero-based index. */
enum { EXTENDED_LAP_HISTORY_CAPACITY = 7 };

static inline int *ExtendedLapTimes_PlayerSlots(int driverID)
{
	switch (driverID)
	{
	case 0: return sdata->LapTimes.p1_Min1s;
	case 1: return sdata->LapTimes.p2_Min1s;
	case 2: return sdata->LapTimes.p3_Min1s;
	case 3: return sdata->LapTimes.p4_Min1s;
	default: return NULL;
	}
}

static inline void ExtendedLapTimes_RecordBest(int driverID, int lapIndex, int lapTime)
{
	int *slots = ExtendedLapTimes_PlayerSlots(driverID);
	if (slots != NULL && (lapIndex == 0 || lapTime < slots[0]))
	{
		slots[0] = lapTime;
		slots[1] = lapIndex;
	}
}

#endif
