#ifndef CTR_NATIVE_ADVENTURE_DIFFICULTY_SAVE_H
#define CTR_NATIVE_ADVENTURE_DIFFICULTY_SAVE_H

#include <namespace_Memcard.h>
#include <difficulty.h>

static inline enum DifficultyLevel AdvDifficultySave_Decode(u8 saved)
{
	if (saved >= DIFFICULTY_COUNT)
	{
		return DIFFICULTY_NORMAL;
	}
	return (enum DifficultyLevel)saved;
}

static inline u8 AdvDifficultySave_Encode(enum DifficultyLevel difficulty)
{
	return (unsigned int)difficulty < DIFFICULTY_COUNT ? (u8)difficulty : DIFFICULTY_NORMAL;
}

#endif
