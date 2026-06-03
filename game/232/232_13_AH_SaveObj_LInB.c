#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af7f0-0x800af9f8.
void AH_SaveObj_LInB(struct Instance *savInst)
{
	s16 rot[3];

	struct GameTracker *gGT = sdata->gGT;
	struct SpawnType2 *spawn;
	struct Thread *t;
	struct Instance *inst;
	struct SaveObj *save;

	// if this Instance's thread is not valid
	if (savInst->thread == NULL)
	{
		t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct SaveObj), NONE, SMALL, STATIC),

		                         AH_SaveObj_ThTick, R232.s_saveobj, 0);

		savInst->thread = t;

		// if the thread was built properly
		if (t != NULL)
		{
			save = t->object;

			t->inst = savInst;

			t->funcThDestroy = AH_SaveObj_ThDestroy;

			// initialize object
			save->flags = 0;

			save->scanlineFrame = 0;

			// make invisible
			savInst->flags |= 0x80;

			spawn = gGT->level1->ptrSpawnType2_PosRot;

			if (spawn == NULL)
			{
				save->inst = NULL;
			}
			else
			{
				inst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_SCAN], R232.s_scan, 0);
				save->inst = inst;

				// NOTE(aalhendi): Native low-RAM audit candidate only. Retail
				// uses this scan instance allocation before any null fallback;
				// keep unpatched until a valid hub/save repro proves failure.
				memcpy(&inst->matrix, &savInst->matrix, sizeof(inst->matrix));

				rot[0] = spawn->posCoords[3];
				rot[1] = spawn->posCoords[4];
				rot[2] = spawn->posCoords[5];

				ConvertRotToMatrix(&inst->matrix, rot);

				inst->matrix.t[0] = spawn->posCoords[0];
				inst->matrix.t[1] = spawn->posCoords[1];
				inst->matrix.t[2] = spawn->posCoords[2];

				inst->unk50 = 0xf8;
			}
		}
	}
	return;
}
