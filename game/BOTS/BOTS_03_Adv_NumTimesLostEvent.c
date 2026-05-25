#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80012568-0x80012598.
int BOTS_Adv_NumTimesLostEvent(int numLost)
{
	// if you lost more than 10 times
	// the difficulty will not get lower.
	if ((u16)numLost > 10)
	{
		// the array apparently has 12, not sure why it stopped at 11.
		numLost = 10;
	}

	return data.advDifficulty[numLost];
}
