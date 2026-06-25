#include <common.h>

// for oxide intro and ND box
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b087c-0x800b0b38
void CS_Cutscene_Start(void)
{
	struct CsThreadInitData initData = {0};

	struct GameTracker *gGT = sdata->gGT;

	CS_Thread_Init(0, R233.s_introcam, 0, 0, 0);

	if ((gGT->gameMode2 & CREDITS) != 0)
	{
		D233.isCutsceneOver = 0;

		CS_Credits_Init();

		CS_Instance_InitMatrix();
		return;
	}

	if (gGT->levelID != NAUGHTY_DOG_CRATE)
	{
		return;
	}

	CS_Instance_InitMatrix();

	initData.podiumPos.x = 0;
	initData.podiumPos.y = 0;
	initData.podiumPos.z = 0;
	initData.characterPos.x = 0;
	initData.characterPos.y = 0;
	initData.characterPos.z = 0;
	initData.rot.x = 0;
	initData.rot.y = 0;
	initData.rot.z = 0;

	CS_Thread_Init(NDI_BOX_BOX_01, R233.s_box1, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_BOX_02, R233.s_box2, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_BOX_02_BOTTOM, R233.s_box2_bottom, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_BOX_02_FRONT, R233.s_box2_front, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_BOX_02A, R233.s_box2_A, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_BOX_03, R233.s_box3, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_CODE, R233.s_code, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_GLOW, R233.s_glow, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_LID, R233.s_lid, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_LIDB, R233.s_lidb, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_LIDC, R233.s_lidc, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_LIDD, R233.s_lidd, &initData, 0, 0);
	CS_Thread_Init(NDI_BOX_LID2, R233.s_lid2, &initData, 0, 0);
	CS_Thread_Init(NDI_KART0, R233.s_kart0, &initData, 0, 0);
	CS_Thread_Init(NDI_KART1, R233.s_kart1, &initData, 0, 0);
	CS_Thread_Init(NDI_KART2, R233.s_kart2, &initData, 0, 0);
	CS_Thread_Init(NDI_KART3, R233.s_kart3, &initData, 0, 0);
	CS_Thread_Init(NDI_KART6, R233.s_kart6, &initData, 0, 0);
	CS_Thread_Init(NDI_KART7, R233.s_kart7, &initData, 0, 0);
}
