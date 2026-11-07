#ifndef NATIVE_LANGUAGE_DISC_H
#define NATIVE_LANGUAGE_DISC_H

#include <macros.h>

enum NativeLanguageSlot
{
	NATIVE_LANGUAGE_ENGLISH = 1,
	NATIVE_LANGUAGE_FRENCH = 3,
	NATIVE_LANGUAGE_GERMAN,
	NATIVE_LANGUAGE_ITALIAN,
	NATIVE_LANGUAGE_SPANISH,
	NATIVE_LANGUAGE_DUTCH,
};

static inline const char *NativeLanguageDisc_VoiceCode(int slot)
{
	switch (slot)
	{
	case NATIVE_LANGUAGE_FRENCH: return "FRN";
	case NATIVE_LANGUAGE_GERMAN: return "GRM";
	case NATIVE_LANGUAGE_ITALIAN: return "ITL";
	case NATIVE_LANGUAGE_SPANISH: return "SPN";
	case NATIVE_LANGUAGE_DUTCH: return "DCH";
	default: return "ENG";
	}
}

// The caller owns *bytesOut and frees it with free(). Only European retail
// language slots are accepted; the base image remains the English source.
int NativeLanguageDisc_ReadText(int slot, u8 **bytesOut, u32 *sizeOut);
const char *NativeLanguageDisc_TextString(const u8 *bytes, u32 size, int index);
const char *NativeLanguageDisc_SelfName(int slot);
int NativeLanguageDisc_HasText(int slot);
int NativeLanguageDisc_HasVoice(int slot);
int NativeLanguageDisc_PalIndex(int usIndex);
int NativeLanguageDisc_PalXaIndex(int category, int usIndex);

struct NativeLanguageVoiceSample
{
	u8 *data;
	u32 size;
	u16 pitch;
	u16 duration;
	u8 flags;
	u8 volume;
};

int NativeLanguageDisc_ReadVoiceSample(int language, int soundID, struct NativeLanguageVoiceSample *sample);

#endif
