#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b5090-0x800b5210.

int RB_CtrLetter_ThCollide(struct Thread *letterTh, struct Thread *driverTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	s16 posScreen[2];
	struct Driver *driver;
	struct Instance *letterInst;
	struct PushBuffer *pb;

	if (sps->Input1.modelID != DYNAMIC_PLAYER)
		return 0;

	letterInst = letterTh->inst;
	driver = driverTh->object;

	pb = &sdata->gGT->pushBuffer[driver->driverID];
	RB_Fruit_GetScreenCoords(pb, letterInst, &posScreen[0]);

	driver->PickupLetterHUD.startX = pb->rect.x + posScreen[0];
	driver->PickupLetterHUD.startY = pb->rect.y + posScreen[1] - 0x14;
	driver->PickupLetterHUD.cooldown = 10;
	driver->PickupLetterHUD.numCollected++;
	driver->PickupLetterHUD.modelID = letterInst->model->id;

	letterInst->scale.x = 0;
	letterInst->scale.y = 0;
	letterInst->scale.z = 0;
	letterInst->thread = 0;
	letterInst->flags |= HIDE_MODEL;

	OtherFX_Play(100, 1);
	letterTh->flags |= THREAD_FLAG_DEAD;

	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b5210-0x800b52dc.

int RB_CtrLetter_LInC(struct Instance *letterInst, struct Thread *driverTh, struct ScratchpadStruct *sps)
{
	typedef int (*CtrLetterCollideFunc)(struct Thread *, struct Thread *, void *, struct ScratchpadStruct *);
	struct Thread *letterTh;

	letterTh = letterInst->thread;
	if (letterTh == NULL)
	{
		letterTh = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(4, NONE, SMALL, STATIC), RB_CtrLetter_ThTick, "ctr", NULL);

		letterInst->thread = letterTh;
		if (letterTh == NULL)
			return 0;

		letterTh->inst = letterInst;
		letterTh->funcThCollide = (void (*)(struct Thread *))RB_CtrLetter_ThCollide;
		letterTh = letterInst->thread;
	}

	if ((letterTh == NULL) || (letterTh->funcThCollide == NULL))
		return 0;

	if (letterInst->scale.x == 0)
		return 0;

	return ((CtrLetterCollideFunc)letterTh->funcThCollide)(letterTh, driverTh, letterTh->funcThCollide, sps);
}

SVec3 letterLightDir = {0x94F, 0x94F, -0x94F};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b52dc-0x800b5334.

void RB_CtrLetter_ThTick(struct Thread *t)
{
	int sine;
	struct Instance *letterInst;
	struct CtrLetter *letterObj;

	letterInst = t->inst;
	letterObj = t->object;

	// rotate each frame
	letterObj->rot.y += 0x40;
	ConvertRotToMatrix(&letterInst->matrix, &letterObj->rot);

	Vector_SpecLightSpin3D(letterInst, &letterObj->rot.x, &letterLightDir);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b5334-0x800b53e0.

void RB_CtrLetter_LInB(struct Instance *inst)
{
	struct CtrLetter *letterObj;
	struct Thread *t;

	if (inst->thread == NULL)
	{
		t = PROC_BirthWithObject(
		    // creation flags
		    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct CtrLetter), NONE, SMALL, STATIC),

		    RB_CtrLetter_ThTick, // behavior
		    "ctr",               // debug name
		    0                    // thread relative
		);

		inst->thread = t;
		if (t == 0)
			return;

		t->funcThCollide = (void (*)(struct Thread *))RB_CtrLetter_ThCollide;
		t->inst = inst;

		letterObj = ((struct CtrLetter *)t->object);
		letterObj->rot.x = 0;
		letterObj->rot.y = 0;
		letterObj->rot.z = 0;

		inst->scale.x = 0x1800;
		inst->scale.y = 0x1800;
		inst->scale.z = 0x1800;

		inst->colorRGBA = 0xffc8000;

		inst->flags |= (DRAW_TRANSPARENT | USE_SPECULAR_LIGHT);
	}

	RB_Default_LInB(inst);
}
