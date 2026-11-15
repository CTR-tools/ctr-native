#include <common.h>

void INSTANCE_Birth(struct Instance *inst, struct Model *model, const char *name, struct Thread *th, u32 flags)
{
	s32 i;
	struct GameTracker *gGT;
	char *dst = inst->name;
	char *last = &inst->name[sizeof(inst->name) - 1];

	// NOTE(aalhendi): Retail copies a fixed 15-byte field. Native also accepts
	// NULL and short C strings, so stop at their terminator and zero-pad.
	while (dst < last && name != NULL && *name != '\0')
	{
		*dst++ = *name++;
	}
	while (dst <= last)
		*dst++ = '\0';

	inst->depthBiasNormal = 0xfe;
	inst->depthBiasSecondary = 0xc;
	inst->animIndex = 0;
	inst->specLightX = 1;

	gGT = GAME_TRACKER;
	inst->model = model;

	inst->scale.x = 0x1000;
	inst->scale.y = 0x1000;
	inst->scale.z = 0x1000;

	inst->flags = flags;
	inst->alphaScale = 0;
	inst->colorRGBA = 0;
	inst->instDef = 0;

	inst->animFrame = 0;
	inst->vertSplit = 0;
	inst->reflectionRGBA = 0x7f7f7f;

	inst->thread = th;
	inst->compressedNormalAndDriverIndex = 0;

	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		inst->idpp[i].mh = 0;
		inst->idpp[i].pushBuffer = &gGT->pushBuffer[i];
		inst->idpp[i].instFlags = 0;
	}
}


struct Instance *INSTANCE_Birth3D(struct Model *model, const char *name, struct Thread *th)
{
	struct Instance *inst = (struct Instance *)JitPool_Add(&GAME_TRACKER->JitPools.instance);

	if (inst != 0)
	{
		INSTANCE_Birth(inst, model, name, th, DRAW_COLLISION_MASK);
	}

	return inst;
}


struct Instance *INSTANCE_Birth2D(struct Model *model, const char *name, struct Thread *th)
{
	struct GameTracker *gGT;
	struct Instance *inst;
	s32 i;

	inst = (struct Instance *)JitPool_Add(&GAME_TRACKER->JitPools.instance);

	if (inst != NULL)
	{
		INSTANCE_Birth(inst, model, name, th, 0x40f);
	}
	else
	{
		// NOTE(aalhendi): Retail assumes capacity; native cannot write through
		// the null instance when the shared pool is exhausted.
		return NULL;
	}

	gGT = GAME_TRACKER;
	inst->idpp[0].pushBuffer = &gGT->pushBuffer_UI;

	i = 1;
	if (i < gGT->numPlyrCurrGame)
	{
		// NOTE(aalhendi): Keep retail's separate pre-loop and loop pointer lifetimes.
		struct GameTracker *loopTracker = gGT;

		do
		{
			inst->idpp[i].pushBuffer = 0;
			i++;
		} while (i < loopTracker->numPlyrCurrGame);
	}

	return inst;
}

static void INSTANCE_RollbackThreadBirth(struct Thread *t, struct Thread *relativeTh)
{
	struct GameTracker *gGT = GAME_TRACKER;

	if (relativeTh == NULL)
	{
		gGT->threadBuckets[t->flags & 0xff].thread = t->siblingThread;
	}
	else if ((t->flags & SELF_SIBLING) != 0)
	{
		relativeTh->siblingThread = t->siblingThread;
	}
	else if ((t->flags & CHILD_BETWEEN) != 0)
	{
		relativeTh->childThread = t->childThread;
	}
	else
	{
		relativeTh->childThread = t->siblingThread;
	}

	PROC_DestroyObject(t->object, t->flags);
	LIST_AddFront(&gGT->JitPools.thread.free, (struct Item *)t);
}

struct Instance *INSTANCE_BirthWithThread(s32 modelID, const char *name, s32 poolType, s32 bucket, void *funcThTick, s32 objSize, struct Thread *parent)
{
	struct Model *lookupModel;
	struct Model *model;
	struct Thread *t;
	struct Instance *inst;
	u32 remainder;
	register u32 sizeFlags CTR_PSX_REGISTER("$2");

	lookupModel = GAME_TRACKER->modelPtr[modelID];

	if (lookupModel == NULL)
	{
		return NULL;
	}

	// NOTE(aalhendi): Keep the validated lookup separate from the model retained
	// across allocation, preserving retail's register lifetime at the alignment branch.
	CTR_PSX_OBSERVE_VALUE(lookupModel);
	model = lookupModel;

	// Round payloads such as TalkingMask up to a word, then pack the size field.
	remainder = objSize & 3;
	if (remainder != 0)
	{
		remainder -= 4;
		sizeFlags = ((u32)objSize - remainder) << 16;
	}
	else
	{
		sizeFlags = (u32)objSize << 16;
	}

	// NOTE(aalhendi): Retail combines the pool bits in v0 before the bucket.
	sizeFlags = poolType | sizeFlags;
	CTR_PSX_OBSERVE_VALUE(sizeFlags);
	t = PROC_BirthWithObject(sizeFlags | bucket, funcThTick, name, parent);

	// NOTE(aalhendi): Retail assumes the thread and instance pools have capacity.
	// Native returns failure instead of writing through PS1 low memory.
	if (t == NULL)
	{
		return NULL;
	}

	t->modelIndex = modelID;
	inst = INSTANCE_Birth3D(model, name, t);

	if (inst == NULL)
	{
		INSTANCE_RollbackThreadBirth(t, parent);
		return NULL;
	}

	t->inst = inst;

	return inst;
}


struct Instance *INSTANCE_BirthWithThread_Stack(const struct InstanceBirthParams *params)
{
	return INSTANCE_BirthWithThread(params->modelID, params->name, params->poolType, params->bucket, params->funcThTick, params->objSize, params->parent);
}


void INSTANCE_Death(struct Instance *inst)
{
	JitPool_Remove(&GAME_TRACKER->JitPools.instance, (struct Item *)inst);
}


void INSTANCE_LevInitAll(struct InstDef *definitions, s32 count)
{
	struct InstDef *levInstDef = definitions;
	struct Instance *inst;
	struct MetaDataMODEL *meta;
	s32 i;

	for (i = 0; i < count; i++, levInstDef++)
	{
		struct GameTracker *gGT;
		s32 j;

		levInstDef->ptrInstance = (struct Instance *)LIST_RemoveFront(&GAME_TRACKER->JitPools.instance.free);
		inst = levInstDef->ptrInstance;

		if (inst == NULL)
		{
			return;
		}

		// NOTE(aalhendi): Retail copies the 44-byte serialized prefix past the
		// pool links. Its final two words are replaced by instDef and matrix.
		memcpy((u8 *)inst + offsetof(struct Instance, name), levInstDef, offsetof(struct InstDef, ptrInstance));

		inst->depthBiasNormal = levInstDef->unk24 - 2;
		inst->depthBiasSecondary = levInstDef->unk24 + 12;

		inst->reflectionRGBA = 0x7f7f7f;

		inst->animIndex = 0;
		inst->animFrame = 0;

		inst->instDef = levInstDef;

		inst->vertSplit = 0;
		inst->specLightX = 1;
		inst->compressedNormalAndDriverIndex = 0;

		ConvertRotToMatrix(&inst->matrix, &levInstDef->rot);

		// NOTE(aalhendi): Keep the tracker load between position stores for
		// GCC 2.8.1's retail load-delay scheduling.
		inst->matrix.t[0] = levInstDef->pos.x;
		gGT = GAME_TRACKER;
		inst->matrix.t[1] = levInstDef->pos.y;
		inst->matrix.t[2] = levInstDef->pos.z;

		inst->thread = NULL;

		for (j = 0; j < gGT->numPlyrCurrGame; j++)
		{
			inst->idpp[j].mh = 0;
			inst->idpp[j].pushBuffer = &gGT->pushBuffer[j];
		}

		// The podium suppresses LEV callbacks; models without one stay static.
		if ((GAME_TRACKER->gameMode2 & NO_LEV_INSTANCE) == 0)
		{
			meta = COLL_LevModelMeta(levInstDef->model->id);
			if (meta != NULL && meta->LInB != NULL)
			{
				meta->LInB(inst);
			}
		}

		// Time trials hide pickups and time crates; relic races count time crates.
		// NOTE(aalhendi): Separate predicates preserve retail's signed loads
		// and branch sharing; the callback above may also replace the model.
		if ((GAME_TRACKER->gameMode1 & TIME_TRIAL) != 0 &&
		    ((u32)((u16)levInstDef->model->id - PU_FRUIT_CRATE) < 2 || levInstDef->model->id == PU_WUMPA_FRUIT ||
		     levInstDef->model->id == STATIC_TIME_CRATE_01 || levInstDef->model->id == STATIC_TIME_CRATE_02 || levInstDef->model->id == STATIC_TIME_CRATE_03))
		{
			inst->flags &= ~DRAW_COLLISION_MASK;
		}
		else if ((GAME_TRACKER->gameMode1 & RELIC_RACE) != 0)
		{
			if (levInstDef->model->id == STATIC_TIME_CRATE_01 || levInstDef->model->id == STATIC_TIME_CRATE_02 || levInstDef->model->id == STATIC_TIME_CRATE_03)
			{
				GAME_TRACKER->timeCratesInLEV++;
			}
			else if ((u16)(levInstDef->model->id - PU_FRUIT_CRATE) < 2 || levInstDef->model->id == PU_WUMPA_FRUIT)
			{
				inst->flags &= ~DRAW_COLLISION_MASK;
			}
		}
		else if (levInstDef->model->id == STATIC_TIME_CRATE_01 || levInstDef->model->id == STATIC_TIME_CRATE_02 ||
		         levInstDef->model->id == STATIC_TIME_CRATE_03)
		{
			inst->flags &= ~DRAW_COLLISION_MASK;
		}

		if ((GAME_TRACKER->gameMode1 & CRYSTAL_CHALLENGE) != 0)
		{
			if (levInstDef->model->id == STATIC_CRYSTAL)
			{
				GAME_TRACKER->numCrystalsInLEV++;
			}
			else if (levInstDef->model->id == PU_FRUIT_CRATE)
			{
				inst->flags &= ~DRAW_COLLISION_MASK;
			}
		}

		else
		{
			// Crystals, TNT and nitro placements belong to crystal challenges.
			if (levInstDef->model->id == STATIC_CRYSTAL || levInstDef->model->id == PU_EXPLOSIVE_CRATE || levInstDef->model->id == STATIC_CRATE_TNT)
			{
				inst->flags &= ~DRAW_COLLISION_MASK;
			}
		}

		if ((GAME_TRACKER->gameMode2 & TOKEN_RACE) == 0)
		{
			// C-T-R letters are only active in token races.
			if ((u32)((u16)levInstDef->model->id - STATIC_C) < 3)
			{
				inst->flags &= ~DRAW_COLLISION_MASK;
			}
		}
	}
}


void INSTANCE_LevDelayedLInBs(struct InstDef *instDef, s32 numInstances)
{
	s32 i;

	for (i = 0; i < numInstances; i++)
	{
		struct MetaDataMODEL *meta = COLL_LevModelMeta(instDef->model->id);

		if ((meta != NULL) && (meta->LInB != NULL))
		{
			meta->LInB(instDef->ptrInstance);
		}

		instDef++;
	}
}


s32 INSTANCE_GetNumAnimFrames(struct Instance *pInstance, s32 animIndex)
{
	struct Model *pModel;
	struct ModelHeader *pHeader;
	struct ModelAnim **animations;
	struct ModelAnim *pAnim;

	// Animations belong to the model's first LOD header.
	pModel = pInstance->model;
	if (pModel == NULL || pModel->numHeaders <= 0 || (pHeader = pModel->headers) == NULL || animIndex >= (s32)pHeader->numAnimations ||
	    (animations = pHeader->ptrAnimations) == NULL)
		return 0;

	pAnim = animations[animIndex];
	// The high bit selects interpolation, not an additional frame.
	return pAnim != NULL ? pAnim->numFrames & 0x7fff : 0;
}
