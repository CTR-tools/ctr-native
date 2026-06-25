#include <common.h>

void RB_Fruit_GetScreenCoords(struct PushBuffer *pb, struct Instance *inst, s16 *output)
{
	MATRIX *m;
	SVec4 posWorld;

	// load camera matrix
	m = &pb->matrix_ViewProj;
	gte_SetRotMatrix(m);
	gte_SetTransMatrix(m);

	// load input vector, each int casts to s16
	posWorld.x = (s16)inst->matrix.t[0];
	posWorld.y = (s16)inst->matrix.t[1];
	posWorld.z = (s16)inst->matrix.t[2];
	posWorld.w = 0;
	CTR_GteLoadSVec4V0(&posWorld);

	// perspective projection
	gte_rtps();

	// get result
	CTR_GteStoreSXY(&output[0]);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b706c-0x800b70a8.
void RB_Fruit_ThTick(struct Thread *fruitTh)
{
	struct Instance *fruitInst;

	fruitInst = fruitTh->inst;
	fruitInst->thread = NULL;
	fruitTh->flags |= THREAD_FLAG_DEAD;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b70a8-0x800b722c.
int RB_Fruit_ThCollide(struct Thread *fruitTh, struct Thread *driverTh, void *funcThCollide, struct ScratchpadStruct *sps)
{
	struct PushBuffer *pb;
	s16 posScreen[2];
	struct Driver *driver;
	struct Instance *fruitInst;
	struct Fruit *fruitObj;
	int modelID;

	fruitObj = fruitTh->object;
	modelID = sps->Input1.modelID;
	fruitInst = fruitTh->inst;

	// wumpa fruit can be grabbed by players and robotcars
	if ((modelID != DYNAMIC_PLAYER) && (modelID != DYNAMIC_ROBOT_CAR))
	{
		return 0;
	}

	driver = driverTh->object;
	if (modelID == DYNAMIC_PLAYER)
	{
		pb = &sdata->gGT->pushBuffer[driver->driverID];
		RB_Fruit_GetScreenCoords(pb, fruitInst, &posScreen[0]);

		driver->PickupWumpaHUD.startX = pb->rect.x + posScreen[0];
		driver->PickupWumpaHUD.startY = pb->rect.y + posScreen[1] - 0x14;
		driver->PickupWumpaHUD.cooldown = 5;
		driver->PickupWumpaHUD.numCollected++;
	}

	fruitObj->driver = driver;

	*(int *)&fruitInst->scale.x = 0;
	fruitInst->scale.z = 0;
	fruitInst->thread = NULL;

	PlaySound3D(0x43, fruitInst);
	fruitTh->flags |= THREAD_FLAG_DEAD;

	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b722c-0x800b7260.
void RB_Fruit_LInB(struct Instance *inst)
{
	RB_Default_LInB(inst);
	inst->animIndex = 0;
	inst->flags |= ANIM_LOOP;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b7260-0x800b7338.
int RB_Fruit_LInC(struct Instance *fruitInst, struct Thread *driverTh, struct ScratchpadStruct *sps)
{
	typedef int (*FruitCollideFunc)(struct Thread *, struct Thread *, void *, struct ScratchpadStruct *);
	struct Thread *fruitTh;

	fruitTh = fruitInst->thread;
	if (fruitTh == NULL)
	{
		fruitTh = PROC_BirthWithObject(
		    // creation flags
		    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Fruit), NONE, SMALL, STATIC),

		    RB_Fruit_ThTick, // behavior
		    "fruit",         // debug name
		    0                // thread relative
		);

		fruitInst->thread = fruitTh;
		if (fruitTh == NULL)
		{
			return 0;
		}

		fruitTh->inst = fruitInst;
		fruitTh->funcThCollide = (void (*)(struct Thread *))RB_Fruit_ThCollide;
		fruitTh = fruitInst->thread;
	}

	if ((fruitTh == NULL) || (fruitTh->funcThCollide == NULL))
	{
		return 0;
	}

	if (fruitInst->scale.x == 0)
	{
		return 0;
	}

	return ((FruitCollideFunc)fruitTh->funcThCollide)(fruitTh, driverTh, fruitTh->funcThCollide, sps);
}
