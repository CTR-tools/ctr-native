#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b8dc8-0x800b8f8c
void CS_Credits_ThTick(void)
{
	struct CreditsObj *co = &creditsBSS.creditsObj;
	struct Instance *danceInst = creditsBSS.dancerInst_invisible;

	co->creditDanceInst = danceInst;

	if (danceInst != NULL)
	{
		danceInst->flags |= 0x80;

		danceInst->matrix.t[0] = (int)creditsBSS.creditGhost_Pos[0];
		danceInst->matrix.t[1] = (int)creditsBSS.creditGhost_Pos[1];
		danceInst->matrix.t[2] = (int)creditsBSS.creditGhost_Pos[2];

		struct GameTracker *gGT = sdata->gGT;

		if ((gGT->timer & 3) == 0)
		{
			for (int i = 4; i > 0; i--)
			{
				CS_Credits_AnimateCreditGhost(co->creditGhostInst[i], co->creditGhostInst[i - 1], i);
				co->creditGhostModel[i] = co->creditGhostModel[i - 1];
			}

			CS_Credits_AnimateCreditGhost(co->creditGhostInst[0], co->creditDanceInst, 0);
			co->creditGhostModel[0] = co->creditDanceInst->model;
		}
		else
		{
			CS_Credits_AnimateCreditGhost(co->creditGhostInst[0], co->creditDanceInst, 0);

			for (int i = 1; i < 5; i++)
			{
				struct Instance *ghost = co->creditGhostInst[i];
				ghost->scale[0] += 0x4b;
				ghost->scale[1] += 0x4b;
				ghost->scale[2] += 0x4b;
				ghost->alphaScale += 0x9d;
			}
		}
	}

	if (co->countdown > 0)
	{
		co->countdown--;
	}

	CS_Credits_DrawNames(co);
	CS_Credits_DrawEpilogue(co);
}
