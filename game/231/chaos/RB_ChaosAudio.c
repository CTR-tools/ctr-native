#include <common.h>
#include <platform/native_audio.h>
#include "RB_ChaosAudio.h"
#include "RB_ChaosRuntime.h"

enum
{
	CHAOS_ENGINE_AUDIO_CLASS_STRIDE = 4,
	CHAOS_TIME_STOP_CRATE_DELAY_MS = 200,
	CHAOS_TIME_STOP_TICK_PERIOD_MS = 192,
	CHAOS_TIME_STOP_TICK_COUNT = 6,
};

enum ChaosAudioOwner Chaos_SetAudioOwner(enum ChaosAudioOwner owner)
{
	const enum ChaosAudioOwner previous = sdata->chaosRuntime.audioOwner;
	sdata->chaosRuntime.audioOwner = owner;
	return previous;
}

static enum ChaosAudioOwner Chaos_ThreadAudioOwner(const struct Thread *thread)
{
	const int player = Chaos_ThreadPlayer(thread);
	return player >= 0 ? CHAOS_AUDIO_PLAYER_1 + player :
	    Chaos_IsHumanOwnedThread(thread) ? CHAOS_AUDIO_HUMAN : CHAOS_AUDIO_WORLD;
}

enum ChaosAudioOwner Chaos_SetThreadAudio(const struct Thread *thread)
{
	return Chaos_SetAudioOwner(Chaos_ThreadAudioOwner(thread));
}

enum ChaosAudioOwner Chaos_SetSpatialAudio(const struct Thread *thread)
{
	const enum ChaosAudioOwner owner = Chaos_ThreadAudioOwner(thread);
	// Unattributed instances inherit the caller's context (for example, a pickup).
	return Chaos_SetAudioOwner(owner == CHAOS_AUDIO_WORLD ? sdata->chaosRuntime.audioOwner : owner);
}

static bool Chaos_ShouldFreezeAudioOwner(enum ChaosAudioOwner owner)
{
	if (owner == CHAOS_AUDIO_WORLD)
	{
		return true;
	}
	return owner != CHAOS_AUDIO_UI && Chaos_UsesPersonalPools() &&
	    !Chaos_PlayerHasTimeStop((int)owner - CHAOS_AUDIO_PLAYER_1);
}

static enum ChaosAudioOwner Chaos_DriverAudioOwner(const struct Driver *driver)
{
	const bool human = driver != NULL && driver->driverID < GAME_TRACKER->numPlyrCurrGame && (driver->actionsFlagSet & ACTION_BOT) == 0;
	return human ? CHAOS_AUDIO_PLAYER_1 + driver->driverID : CHAOS_AUDIO_WORLD;
}

enum ChaosAudioOwner Chaos_SetDriverAudio(const struct Driver *driver)
{
	return Chaos_SetAudioOwner(Chaos_DriverAudioOwner(driver));
}

static int Chaos_VoicelineIndex(const struct VoicelineItem *voice)
{
	const uintptr_t offset = (uintptr_t)voice - (uintptr_t)sdata->voicelinePool;
	return offset < sizeof(sdata->voicelinePool) && offset % sizeof(*voice) == 0 ? (int)(offset / sizeof(*voice)) : -1;
}

void Chaos_TagVoiceline(const struct VoicelineItem *voice)
{
	const int index = Chaos_VoicelineIndex(voice);
	if (index >= 0)
	{
		sdata->chaosVoicelines[index] = sdata->chaosRuntime.audioOwner;
	}
}

static enum ChaosAudioOwner Chaos_VoicelineAudio(const struct VoicelineItem *voice)
{
	const int index = Chaos_VoicelineIndex(voice);
	return index >= 0 ? sdata->chaosVoicelines[index] : CHAOS_AUDIO_WORLD;
}

bool Chaos_ShouldFreezeVoiceline(const struct VoicelineItem *voice)
{
	return Chaos_IsTimeStopped() && Chaos_ShouldFreezeAudioOwner(Chaos_VoicelineAudio(voice));
}

void Chaos_SetVoicelineXA(const struct VoicelineItem *voice)
{
	sdata->chaosRuntime.xaOwner = Chaos_VoicelineAudio(voice);
}

void Chaos_SetDriverXA(const struct Driver *driver)
{
	sdata->chaosRuntime.xaOwner = Chaos_DriverAudioOwner(driver);
}

void Chaos_TagAudioChannel(struct ChannelStats *channel)
{
	if (channel->channelID < len(sdata->chaosAudioChannels))
	{
		sdata->chaosAudioChannels[channel->channelID] = sdata->chaosRuntime.audioOwner;
	}
}

void Chaos_BackupAudioChannel(const struct ChannelStats *channel, int index)
{
	if (index >= 0 && index < (int)len(sdata->chaosPausedAudioChannels) && channel->channelID < len(sdata->chaosAudioChannels))
	{
		sdata->chaosPausedAudioChannels[index] = sdata->chaosAudioChannels[channel->channelID];
	}
}

void Chaos_RestoreAudioChannel(const struct ChannelStats *channel, int index)
{
	if (index >= 0 && index < (int)len(sdata->chaosPausedAudioChannels) && channel->channelID < len(sdata->chaosAudioChannels))
	{
		sdata->chaosAudioChannels[channel->channelID] = sdata->chaosPausedAudioChannels[index];
	}
}

bool Chaos_IsMenuPreviewSong(const struct Song *song)
{
	return sdata->OptionSlider_BoolPlay && sdata->OptionSlider_Index == HOWL_VOLUME_TYPE_MUSIC &&
	    (song->id == CSEQ_SONG_AKU || song->id == CSEQ_SONG_UKA);
}

bool Chaos_ShouldFreezeAudioChannel(const struct ChannelStats *channel)
{
	if (!Chaos_IsTimeStopped())
	{
		return false;
	}
	const u16 soundID = channel->soundID & 0xffff;
	if (channel->type == HOWL_CHANNEL_TYPE_OTHER_FX &&
	    (soundID == MENU_SFX_MOVE || soundID == MENU_SFX_BACK ||
	     soundID == MENU_SFX_CONFIRM || soundID == MENU_SFX_LOCKED))
	{
		return false;
	}
	if (channel->type == HOWL_CHANNEL_TYPE_MUSIC)
	{
		if ((u32)channel->soundID < len(sdata->songSeq))
		{
			const struct SongSeq *sequence = &sdata->songSeq[channel->soundID];
			if (sequence->songPoolIndex < len(sdata->songPool))
			{
				return !Chaos_IsMenuPreviewSong(&sdata->songPool[sequence->songPoolIndex]);
			}
		}
		return true;
	}
	if (channel->type == HOWL_CHANNEL_TYPE_ENGINE_FX)
	{
		for (int i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
		{
			const struct Driver *driver = GAME_TRACKER->drivers[i];
			if (driver != NULL && (driver->actionsFlagSet & ACTION_BOT) == 0 &&
			    channel->soundID == EngineClass_AudioClass(Chaos_EngineClassForDriver(driver, GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[i]].engineID)) *
			        CHAOS_ENGINE_AUDIO_CLASS_STRIDE + i)
			{
				return Chaos_UsesPersonalPools() && !Chaos_PlayerHasTimeStop(i);
			}
		}
		return true;
	}
	return channel->channelID >= len(sdata->chaosAudioChannels) ||
	    Chaos_ShouldFreezeAudioOwner(sdata->chaosAudioChannels[channel->channelID]);
}

void Chaos_UpdateTimeStopAudio(void)
{
	u32 paused = 0;
	for (const struct ChannelStats *channel = (const struct ChannelStats *)sdata->channelTaken.first;
	     channel != NULL; channel = channel->link.links.next)
	{
		if (Chaos_ShouldFreezeAudioChannel(channel))
		{
			paused |= 1u << channel->channelID;
		}
	}
	NativeAudio_SetPausedVoices(paused, Chaos_IsTimeStopped() && Chaos_ShouldFreezeAudioOwner(sdata->chaosRuntime.xaOwner));
}

void Chaos_PlayTimeEffectSound(int sound)
{
	const enum ChaosAudioOwner previous = Chaos_SetAudioOwner(CHAOS_AUDIO_UI);
	OtherFX_Play(sound, 1);
	Chaos_SetAudioOwner(previous);
}

void Chaos_TickTimeEffectSound(const struct ChaosActivation *entry, s32 elapsedMS)
{
	if (entry->effect != CHAOS_EFFECT_ZA_WARUDO && entry->effect != CHAOS_EFFECT_KING_CRIMSON)
	{
		return;
	}
	if (entry->elapsedMS < CHAOS_TIME_STOP_CRATE_DELAY_MS &&
	    (entry->elapsedMS + elapsedMS >= CHAOS_TIME_STOP_CRATE_DELAY_MS || entry->remainingMS <= elapsedMS))
	{
		Chaos_PlayTimeEffectSound(HOWL_SFX_TIME_CRATE_BREAK);
	}
	if (entry->effect != CHAOS_EFFECT_ZA_WARUDO)
	{
		return;
	}
	s32 cueMS = CHAOS_TIME_STOP_CRATE_DELAY_MS;
	for (int i = 0; i < CHAOS_TIME_STOP_TICK_COUNT; i++)
	{
		if (entry->elapsedMS < cueMS && entry->elapsedMS + elapsedMS >= cueMS && entry->remainingMS > elapsedMS)
		{
			Chaos_PlayTimeEffectSound(HOWL_SFX_TIME_CRATE_TICK);
		}
		cueMS += CHAOS_TIME_STOP_TICK_PERIOD_MS;
	}
}

void Chaos_ChangeEngineAudio(const struct Driver *driver, int previous, int next)
{
	previous = EngineClass_AudioClass(previous);
	next = EngineClass_AudioClass(next);
	EngineAudio_Stop(previous * CHAOS_ENGINE_AUDIO_CLASS_STRIDE + driver->driverID);
	EngineAudio_InitOnce(next * CHAOS_ENGINE_AUDIO_CLASS_STRIDE + driver->driverID, HOWL_SFX_CENTER_NO_DISTORTION);
}
