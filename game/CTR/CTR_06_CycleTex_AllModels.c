#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021ac0-0x80021b94.
void CTR_CycleTex_AllModels(u32 numModels, struct Model **pModelArray, int timer)
{
	struct Model *pModel;
	struct ModelHeader *pHeader;

	if (pModelArray == NULL)
		return;

	if (numModels == 0)
		return;

	while (true)
	{
		pModel = *pModelArray;
		if (pModel == NULL)
			return;

		// iterate over all model headers
		for (int j = 0; j < pModel->numHeaders; j++)
		{
			pHeader = &pModel->headers[j];

			if ((pHeader->animtex != NULL) && ((pHeader->flags & 2) == 0))
			{
				CTR_CycleTex_Model(pHeader->animtex, timer);
			}
		}

		numModels--;
		if (numModels == 0)
			return;

		pModelArray++;
	}
}
