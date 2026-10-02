#include <common.h>

int MainInit_StringToLevID(char *str)
{
	s32 levelID;
	for (levelID = 0; levelID < 0x41; levelID++)
	{
		if (strncmp(GAME_LEVEL_METADATA[levelID].name_Debug, str, strlen(GAME_LEVEL_METADATA[levelID].name_Debug)) == 0)
		{
			return levelID;
		}
	}

	return 0;
}

void MainInit_VRAMClear(void)
{
	DRAWENV drawEnv;

	TILE commands;

	SetDefDrawEnv(&drawEnv, 0, 0, 0x400, 0x200);
	drawEnv.dfe = '\x01';
	PutDrawEnv(&drawEnv);

#ifdef CTR_NATIVE
	// NOTE(aalhendi): The native tag bridge reads the whole packed word.
	commands.tag = 0x3ffffff;
#endif
	commands.code = 2;
	commands.r0 = 0;
	commands.g0 = 0;
	commands.b0 = 0;
	commands.w = 0x3ff;
	commands.h = 0x1ff;
	commands.x0 = 0;
	commands.y0 = 0;
#ifndef CTR_NATIVE
	setaddr(&commands, 0xffffff);
	setlen(&commands, 3);
#endif
	DrawOTag(&commands);

	commands.w = 0x3ff;
	commands.h = 1;
	commands.x0 = 0;
	commands.y0 = 0x1ff;
	DrawOTag(&commands);
}

// Native wraps the VRAM page moves in a platform frame for presentation.
void MainInit_VRAMDisplay(void)
{
	RECT r;
	DR_MOVE move;

	s16 x[2] = {0, 0x100};
	s16 y[2] = {0, 0x128};
	s16 i;
	s16 j;

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native SetDrawMove reads the packed tag before termination.
	move.tag = 0;
#endif

	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			r.x = x[i] + 0x200;
			r.y = 0x10c;
			r.w = 0x100;
			r.h = 0xd8;

			SetDrawMove(&move, &r, x[i], y[j]);

			move.tag |= 0xffffff;

			DrawOTag(&move);
			DrawSync(0);
		}
	}

#ifdef CTR_NATIVE
	Platform_PresentVRAMDisplay();
#endif
}
