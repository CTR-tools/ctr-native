#include <common.h>

#ifdef CTR_NATIVE
// NOTE(aalhendi): Retail stores these debug names in EXE RDATA; native does not
// expose the full retail RDATA struct.
static char s_SelectProfileThreadName[] = "LoadSave";
static char s_SelectProfileInstName[] = "loadsave";
#endif

static u32 SelectProfile_LoadSave_Color(int index, u32 flags)
{
	u32 red = (u8)data.MetaDataLoadSave[index].r;
	u32 green = (u8)data.MetaDataLoadSave[index].g;
	u32 blue = (u8)data.MetaDataLoadSave[index].b;

	if ((flags & 0x10) != 0)
	{
		red >>= 1;
		blue >>= 1;
	}

	return (red << 0x14) | (green << 0xc) | (blue << 4);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800485cc-0x800488e0.
void SelectProfile_Init(u16 flags)
{
	struct GameTracker *gGT;
	struct SelectProfileLoadSaveObj *obj;
	struct SelectProfileLoadSaveIcon *icon;
	struct Thread *t;
	int i;

	obj = (struct SelectProfileLoadSaveObj *)sdata->ptrLoadSaveObj;

	if (obj == NULL)
	{
#ifdef CTR_NATIVE
		char *threadName = &s_SelectProfileThreadName[0];
#else
		char *threadName = rdata.s_LoadSave;
#endif

		t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct SelectProfileLoadSaveObj), NONE, SMALL, OTHER), SelectProfile_ThTick, threadName,
		                         NULL);
		// NOTE(aalhendi): Native low-RAM audit candidate only. Retail writes
		// through this allocation result before its later null check; keep
		// unpatched until a valid menu repro proves the allocation can fail.
		obj = (struct SelectProfileLoadSaveObj *)t->object;
		sdata->ptrLoadSaveObj = (int)obj;
		obj->icons = (struct SelectProfileLoadSaveIcon *)&sdata->LoadSaveData[0];
		memset(obj->icons, 0, sizeof(sdata->LoadSaveData));

		if (obj == NULL)
			return;

		obj->thread = t;
	}

	gGT = sdata->gGT;
	icon = obj->icons;

	for (i = 0; i < 12; i++, icon++)
	{
		struct Instance *inst;
		int slot;

		if (icon->inst == NULL)
		{
			struct Model *model = gGT->modelPtr[data.MetaDataLoadSave[i].modelID];
#ifdef CTR_NATIVE
			char *instName = &s_SelectProfileInstName[0];
#else
			char *instName = rdata.s_loadsave;
#endif

			if (model != NULL)
			{
				inst = INSTANCE_Birth3D(model, instName, obj->thread);

				if (inst != NULL)
				{
					struct InstDrawPerPlayer *idpp;
					int player;

					icon->inst = inst;
					slot = i % 3;

					inst->flags |= HIDE_MODEL | SCREENSPACE_INSTANCE;
					if (slot != 1)
					{
						inst->flags |= USE_SPECULAR_LIGHT;
					}

					idpp = INST_GETIDPP(inst);
					idpp[0].pushBuffer = &gGT->pushBuffer_UI;
					for (player = 1; player < gGT->numPlyrCurrGame; player++)
					{
						idpp[player].pushBuffer = NULL;
					}

					inst->colorRGBA = SelectProfile_LoadSave_Color(i, flags);
					inst->scale[0] = data.MetaDataLoadSave[i].scale;
					inst->scale[1] = data.MetaDataLoadSave[i].scale;
					inst->scale[2] = data.MetaDataLoadSave[i].scale;

					icon->rot[0] = 0;
					icon->rot[1] = 0;
					icon->rot[2] = data.spinOffset_LoadSave[slot];

					*(int *)&inst->matrix.m[0][0] = 0x1000;
					*(int *)&inst->matrix.m[0][2] = 0;
					*(int *)&inst->matrix.m[1][1] = 0x1000;
					*(int *)&inst->matrix.m[2][0] = 0;
					inst->matrix.m[2][2] = 0x1000;
				}
			}
		}

		inst = icon->inst;
		if (inst != NULL)
		{
			inst->flags |= HIDE_MODEL;
		}
	}
}
