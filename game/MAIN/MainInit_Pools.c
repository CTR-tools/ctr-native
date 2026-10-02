#include <common.h>

void MainInit_JitPoolsNew(struct GameTracker *gGT)
{
	u32 gameMode = gGT->gameMode1;
	u32 poolScale;
	u32 renderBucketSize;
	s32 numParticle;
	struct Item *stack;
	s32 i;

	if ((gameMode & ADVENTURE_ARENA) == 0)
	{
		poolScale = 0x1000;
		if ((gameMode & MAIN_MENU) != 0)
		{
			poolScale = 0x400;
		}
	}
	else
	{
		poolScale = 0x800;
	}

	renderBucketSize = 0x800;
	if ((gameMode & ADVENTURE_ARENA) == 0)
	{
		if ((gameMode & MAIN_MENU) != 0)
		{
			renderBucketSize = 0x400;
			if (gGT->levelID == ADVENTURE_GARAGE)
			{
				renderBucketSize = 0x800;
			}
		}
		else
		{
			renderBucketSize = 0x1000;
		}
	}

	MEMPACK_PushState();
	JitPool_Init(&gGT->JitPools.thread, (renderBucketSize * 3) >> 7, sizeof(struct Thread), GAME_RDATA_NAME(s_ThreadPool));
	// NOTE(aalhendi): Keep retail's four-player record size minus its unused views.
	JitPool_Init(&gGT->JitPools.instance, renderBucketSize >> 5,
	             sizeof(struct Instance) + 4 * sizeof(struct InstDrawPerPlayer) - sizeof(struct InstDrawPerPlayer) * (4 - GAME_TRACKER->numPlyrCurrGame),
	             GAME_RDATA_NAME(s_InstancePool));
	JitPool_Init(&gGT->JitPools.smallStack, (poolScale * 0x19) >> 10, THREAD_SMALL_STACK_SIZE, GAME_RDATA_NAME(s_SmallStackPool));
	JitPool_Init(&gGT->JitPools.mediumStack, poolScale >> 7, THREAD_MEDIUM_STACK_SIZE, GAME_RDATA_NAME(s_MediumStackPool));
	if ((gGT->gameMode1 & MAIN_MENU) != 0)
	{
		JitPool_Init(&gGT->JitPools.largeStack, 4, THREAD_LARGE_STACK_SIZE, GAME_RDATA_NAME(s_LargeStackPool));
	}
	else
	{
		JitPool_Init(&gGT->JitPools.largeStack, poolScale >> 9, THREAD_LARGE_STACK_SIZE, GAME_RDATA_NAME(s_LargeStackPool));
	}
	numParticle = poolScale >> 5;
	JitPool_Init(&gGT->JitPools.particle, numParticle, sizeof(struct Particle), GAME_RDATA_NAME(s_ParticlePool));
	JitPool_Init(&gGT->JitPools.oscillator, numParticle, 0x18, GAME_RDATA_NAME(s_OscillatorPool));
	JitPool_Init(&gGT->JitPools.rain, poolScale >> 9, sizeof(struct RainLocal), GAME_RDATA_NAME(s_RainPool));

#ifndef CTR_NATIVE
	GAME_TRACKER->ptrRenderBucketInstance = MEMPACK_AllocMem(renderBucketSize, GAME_RDATA_NAME(s_RENDER_BUCKET_INSTANCE));
#else
	// NOTE(aalhendi): Native retains its writable RDATA bucket scratch outside MEMPACK.
	gGT->ptrRenderBucketInstance = (u8 *)&rdata + offsetof(struct rData, s_STATIC_GNORMALZ) + 148;
#endif

	// NOTE(aalhendi): Free stack payloads start with a self-pointer after the list header.
	stack = gGT->JitPools.smallStack.free.first;
	while (stack != NULL)
	{
		u8 *payload = (u8 *)stack + sizeof(*stack);
		CTR_WriteU32AlignedLE(payload, (u32)payload);
		stack = stack->next;
	}
	stack = gGT->JitPools.mediumStack.free.first;
	while (stack != NULL)
	{
		u8 *payload = (u8 *)stack + sizeof(*stack);
		CTR_WriteU32AlignedLE(payload, (u32)payload);
		stack = stack->next;
	}
	stack = gGT->JitPools.largeStack.free.first;
	while (stack != NULL)
	{
		u8 *payload = (u8 *)stack + sizeof(*stack);
		CTR_WriteU32AlignedLE(payload, (u32)payload);
		stack = stack->next;
	}
	for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
	{
		GAME_CLIP_BUFFERS[i] = MEMPACK_AllocMem(MainDB_GetClipSize(GAME_TRACKER->levelID, GAME_TRACKER->numPlyrCurrGame) << 2, GAME_RDATA_NAME(s_Clip_Buffer));
	}
	// NOTE(aalhendi): Retain the selected render scale through the final clip allocation.
	CTR_PSX_OBSERVE_VALUE(renderBucketSize);
}
