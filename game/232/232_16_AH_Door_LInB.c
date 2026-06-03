#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b072c-0x800b0b98.
void AH_Door_LInB(struct Instance *inst)
{
	char i;
	int levelID;
	int ratio;
	s16 leftRot[3];
	s16 rightRot[3];

	struct GameTracker *gGT;
	struct Thread *t;
	struct Instance *otherDoorInst;
	struct Model *m;
	struct ModelHeader *headers;
	struct WoodDoor *woodDoor;
	struct Instance **instPtrArr;

	gGT = sdata->gGT;
	levelID = gGT->levelID;

	// If this Instance already has a thread
	if (inst->thread != NULL)
		return;

	t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct WoodDoor), NONE, SMALL, STATIC),
	                         AH_Door_ThTick, // behavior
	                         R232.s_door,    // debug name
	                         0               // thread relative
	);

	inst->thread = t;

	// if the thread failed to build
	if (t == NULL)
		return;

	woodDoor = t->object;

	t->inst = inst;

	t->funcThDestroy = AH_Door_ThDestroy;

	// this instance is always the left-hand door,
	// and every left-hand door has one key hole
	inst->flags |= 0x1000;

	// 4 keys, all next to each other
	instPtrArr = &woodDoor->keyInst[0];
	for (i = 0; i < 4; i++)
	{
		instPtrArr[i] = NULL;
	}

	woodDoor->frameCount_unused = 0;
	woodDoor->camFlags = 0;
	woodDoor->camTimer_unused = 0;
	woodDoor->frameCount_doorOpenAnim = 0;
	woodDoor->keyShrinkFrame = 0;

	woodDoor->doorRot[0] = 0;
	woodDoor->doorRot[1] = 0;
	woodDoor->doorRot[2] = 0;
	woodDoor->doorID = 0;

	for (i = 5; inst->name[i] != '\0'; i++)
	{
		woodDoor->doorID = woodDoor->doorID * 10 + inst->name[i] - '0';
	}

	// Level ID is Glacier Park
	if (levelID == GLACIER_PARK)
	{
		// door with two key holes
		m = gGT->modelPtr[STATIC_DOOR3];
	}

	// Level ID is not Glacier Park
	else if (
	    // Level ID is N Sanity Beach
	    (levelID == N_SANITY_BEACH) &&

	    // doorID == 5
	    (woodDoor->doorID == 5))
	{
		// door with no key holes
		m = gGT->modelPtr[STATIC_DOOR2];
	}

	// if not that door
	else
	{
		// door with one key hole
		m = gGT->modelPtr[STATIC_DOOR];
	}

	// DAT_800abaa4
	// "door"

	// INSTANCE_Birth3D -- ptrModel, name, thread
	otherDoorInst = INSTANCE_Birth3D(m, R232.s_door, t);

	// spawn instance of right-hand door,
	// which is not in LEV file, only built in thread
	woodDoor->otherDoor = otherDoorInst;

	// NOTE(aalhendi): Native low-RAM audit candidate only. Retail uses the
	// generated right-door instance before any null fallback; keep unpatched
	// until a valid hub/door repro proves the allocation can fail.

	// | 0x8000 - reverse culling direction
	otherDoorInst->flags |= 0x9000;

	// copy full matrix (position and rotation)
	// from left-hand door to right-hand door
	otherDoorInst->matrix = inst->matrix;

	// set scaleX to -0x1000
	otherDoorInst->scale[0] = -0x1000;

	ratio = MATH_Cos((int)inst->instDef->rot[1]);

	otherDoorInst->matrix.t[0] += (ratio * 0x600 >> 0xc);

	otherDoorInst->matrix.t[1] = inst->matrix.t[1];

	ratio = MATH_Sin((int)(int)inst->instDef->rot[1]);

	otherDoorInst->matrix.t[2] += (ratio * 0x600 >> 0xc);

	// both doors always face camera
	headers = inst->model->headers;

	headers->flags |= 2;

	headers = otherDoorInst->model->headers;

	headers->flags |= 2;

	if (
	    // Level ID is N Sanity Beach, check door to Gemstone Valley
	    (levelID == N_SANITY_BEACH && woodDoor->doorID == 4 && ((sdata->advProgress.rewards[3] & 0x40) != 0)) ||

	    // Level ID is N Sanity Beach, check door to Glacier Park
	    (levelID == N_SANITY_BEACH && woodDoor->doorID == 5 && ((sdata->advProgress.rewards[3] & 0x10) != 0)) ||

	    // Level ID is Gemstone Valley, check door to Cup room
	    (levelID == GEM_STONE_VALLEY && ((sdata->advProgress.rewards[3] & 0x20) != 0)) ||

	    // Level ID is Lost Ruins, check door to Glacier Park
	    (levelID == THE_LOST_RUINS && ((sdata->advProgress.rewards[3] & 0x80) != 0)) ||

	    // Level ID is Glacier Park, check door to Citadel City
	    (levelID == GLACIER_PARK) && ((sdata->advProgress.rewards[3] & 0x100) != 0))
	{
		// rotation = 90 degrees
		woodDoor->doorRot[1] = 0x400;

		leftRot[0] = woodDoor->doorRot[0];
		leftRot[1] = inst->instDef->rot[1] + woodDoor->doorRot[1];
		leftRot[2] = woodDoor->doorRot[2];

		rightRot[0] = woodDoor->doorRot[0];
		rightRot[1] = inst->instDef->rot[1] - woodDoor->doorRot[1];
		rightRot[2] = woodDoor->doorRot[2];

		// make matrices for both doors rotated open

		// converted to TEST in rebuildPS1
		ConvertRotToMatrix(&inst->matrix, &leftRot[0]);
		ConvertRotToMatrix(&otherDoorInst->matrix, &rightRot[0]);
	}
	return;
}
