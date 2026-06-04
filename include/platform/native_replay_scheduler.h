#ifndef PLATFORM_NATIVE_REPLAY_SCHEDULER_H
#define PLATFORM_NATIVE_REPLAY_SCHEDULER_H

#include <macros.h>

#if defined(CTR_INTERNAL)
struct NativeReplaySchedulerFrameInfo
{
	s32 frameTimer;
	s32 frameCounter;
	s32 timer;
	s32 framesInThisLEV;
	s32 elapsedTimeMS;
	s32 msInThisLEV;
	s32 elapsedEventTime;
	s32 mainGameState;
	s32 loadingStage;
	s32 levelID;
	u32 mixRandomNumber;
	u32 audioRNG;
	u32 deadcoed0;
	u32 deadcoed1;
	u32 advRng0;
	u32 advRng1;
};

int NativeReplayScheduler_ConfigureFromArgs(int argc, char **argv);
void NativeReplayScheduler_Shutdown(void);
int NativeReplayScheduler_BeginFrame(const struct NativeReplaySchedulerFrameInfo *info);
int NativeReplayScheduler_ConsumeVSyncPacket(int requestedVBlanks, int *emittedVBlanks);
int NativeReplayScheduler_ConsumeFrameElapsedTimeMS(int *elapsedTimeMS);
int NativeReplayScheduler_EndFrame(const struct NativeReplaySchedulerFrameInfo *info);
void NativeReplayScheduler_RecordVSyncPacket(int emittedVBlanks);
#endif

#endif
