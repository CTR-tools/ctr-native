#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800327dc-0x8003282c.
void LOAD_Robots1P(int characterID)
{
	int newCharacterID = 0;

	data.characterIDs[0] = characterID;

	for (int i = 1; i < 8; i++, newCharacterID++)
	{
		if (newCharacterID == characterID)
			newCharacterID++;

		data.characterIDs[i] = newCharacterID;
	}
}
