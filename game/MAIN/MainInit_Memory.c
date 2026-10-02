#include <common.h>

void MainInit_RainBuffer(struct GameTracker *gGT)
{
	{
		int i;
		for (i = 0; i < gGT->numPlyrCurrGame; i++)
		{
			gGT->rainBuffer[i] = gGT->level1->rainBuffer;

			gGT->rainBuffer[i].numParticles_curr /= gGT->numPlyrCurrGame;
			gGT->rainBuffer[i].numParticles_max = (u16)gGT->rainBuffer[i].numParticles_max / gGT->numPlyrCurrGame;
		}
	}
}

void MainInit_PrimMem(struct GameTracker *gGT)
{
	s32 size1P = 95;
	s32 size2P = 120;
	s32 size4P = 150;
	// NOTE(aalhendi): Keep the first database argument live before the size shifts.
	struct PrimMem *firstPrim;

	if (gGT->levelID == ADVENTURE_GARAGE)
	{
		MainDB_PrimMem(&gGT->db[0].primMem, 0x1b800);
		MainDB_PrimMem(&gGT->db[1].primMem, 0x1b800);
		return;
	}

	if ((gGT->gameMode1 & MAIN_MENU) != 0)
	{
		MainDB_PrimMem(&gGT->db[0].primMem, 0x17c00);
		MainDB_PrimMem(&gGT->db[1].primMem, 0x17c00);
		return;
	}

	switch (gGT->numPlyrCurrGame)
	{
	case 0:
		MainDB_PrimMem(&gGT->db[0].primMem, 0x25800);
		MainDB_PrimMem(&gGT->db[1].primMem, 0x25800);
		return;

	case 1:
	{
		struct GameTracker *caseTracker;
		caseTracker = GAME_TRACKER;
		if ((caseTracker->gameMode1 & ADVENTURE_ARENA) != 0)
		{
			MainDB_PrimMem(&gGT->db[0].primMem, 0x1c000);
			MainDB_PrimMem(&gGT->db[1].primMem, 0x1c000);
			return;
		}

		if ((u32)(caseTracker->levelID - INTRO_RACE_TODAY) < 9)
		{
			MainDB_PrimMem(&gGT->db[0].primMem, 0x1e000);
			MainDB_PrimMem(&gGT->db[1].primMem, 0x1e000);
			return;
		}

		if (gGT->levelID < GEM_STONE_VALLEY)
		{
			size1P = GAME_PRIM_MEM_SIZES_1P[gGT->levelID];
		}
		firstPrim = &gGT->db[0].primMem;
		CTR_PSX_DEPEND_VALUE(size1P, firstPrim);
		size4P = size1P << 10;
#ifdef CTR_NATIVE
		// NOTE(aalhendi): Native keeps the zero-sized allocation early return.
		if (size4P == 0)
		{
			return;
		}
#endif
		MainDB_PrimMem(firstPrim, size4P);
		MainDB_PrimMem(&gGT->db[1].primMem, size4P);
		break;
	}

	case 2:
		if (gGT->levelID < GEM_STONE_VALLEY)
		{
			size2P = GAME_PRIM_MEM_SIZES_2P[gGT->levelID];
		}
		firstPrim = &gGT->db[0].primMem;
		CTR_PSX_DEPEND_VALUE(size2P, firstPrim);
		size4P = size2P << 10;
#ifdef CTR_NATIVE
		// NOTE(aalhendi): Native keeps the zero-sized allocation early return.
		if (size4P == 0)
		{
			return;
		}
#endif
		MainDB_PrimMem(firstPrim, size4P);
		MainDB_PrimMem(&gGT->db[1].primMem, size4P);
		break;

	case 3:
	case 4:
		if (gGT->levelID < GEM_STONE_VALLEY)
		{
			size4P = GAME_PRIM_MEM_SIZES_4P[gGT->levelID];
		}
		firstPrim = &gGT->db[0].primMem;
		CTR_PSX_DEPEND_VALUE(size4P, firstPrim);
		size4P <<= 10;
#ifdef CTR_NATIVE
		// NOTE(aalhendi): Native keeps the zero-sized allocation early return.
		if (size4P == 0)
		{
			return;
		}
#endif
		MainDB_PrimMem(firstPrim, size4P);
		MainDB_PrimMem(&gGT->db[1].primMem, size4P);
		break;

	default:
		return;
	}
}

void MainInit_JitPoolsReset(struct GameTracker *gGT)
{
	JitPool_Clear(&gGT->JitPools.thread);
	JitPool_Clear(&gGT->JitPools.instance);
	JitPool_Clear(&gGT->JitPools.smallStack);
	JitPool_Clear(&gGT->JitPools.mediumStack);
	JitPool_Clear(&gGT->JitPools.largeStack);
	JitPool_Clear(&gGT->JitPools.particle);
	JitPool_Clear(&gGT->JitPools.oscillator);
	JitPool_Clear(&gGT->JitPools.rain);
}

void MainInit_OTMem(struct GameTracker *gGT)
{
	u32 gameMode = gGT->gameMode1;

	if ((gameMode & MAIN_MENU) != 0)
	{
		MainDB_OTMem(&gGT->db[0].otMem, 0x1800);
		MainDB_OTMem(&gGT->db[1].otMem, 0x1800);
	}
	else if ((gameMode & ADVENTURE_ARENA) != 0)
	{
		MainDB_OTMem(&gGT->db[0].otMem, 0x2c00);
		MainDB_OTMem(&gGT->db[1].otMem, 0x2c00);
	}
	else if ((gameMode & BATTLE_MODE) != 0)
	{
		MainDB_OTMem(&gGT->db[0].otMem, 0x8000);
		MainDB_OTMem(&gGT->db[1].otMem, 0x8000);
	}
	else if (gGT->numPlyrCurrGame >= 3)
	{
		MainDB_OTMem(&gGT->db[0].otMem, 0x3000);
		MainDB_OTMem(&gGT->db[1].otMem, 0x3000);
	}
	else
	{
		MainDB_OTMem(&gGT->db[0].otMem, 0x2000);
		MainDB_OTMem(&gGT->db[1].otMem, 0x2000);
	}

	// 0x1000 per player, plus 0x18 for linking
	gGT->otSwapchainDB[0] = MEMPACK_AllocMem((gGT->numPlyrCurrGame << 12) | 0x18, GAME_SDATA_NAME(s_ot0));
	gGT->otSwapchainDB[1] = MEMPACK_AllocMem((gGT->numPlyrCurrGame << 12) | 0x18, GAME_SDATA_NAME(s_ot1));
}
