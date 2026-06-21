#include <common.h>

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b38e4-0x800b3978.
void RB_Blade_ThTick(struct Thread *t)
{
	struct Blade *bladeObj;
	struct Instance *bladeInst;
	SVec3 rot;

	bladeObj = (struct Blade *)t->object;
	bladeInst = t->inst;

	rot.x = bladeInst->instDef->rot.x;
	rot.y = bladeInst->instDef->rot.y + 0x400;
	rot.z = bladeObj->angle;

	bladeObj->angle += 0x100;

	// converted to TEST in rebuildPS1
	ConvertRotToMatrix(&bladeInst->matrix, &rot);

	bladeInst->scale.x = 0x1000;
	bladeInst->scale.y = 0x1000;
	bladeInst->scale.z = 0x1000;

	ThTick_FastRET(t);
}

static char s_blade[] = "blade";

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b3978-0x800b39dc.
void RB_Blade_LInB(struct Instance *inst)
{
	// Four "blades" from two blimps, Hot Air Skyway
	// One "blade" is a group of three fins,

	struct Blade *bladeObj;

	struct Thread *t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Blade), NONE, SMALL, STATIC),

	    RB_Blade_ThTick, // behavior
	    s_blade,         // debug name
	    0                // thread relative
	);

	if (t == 0)
		return;
	inst->thread = t;
	t->inst = inst;

	bladeObj = ((struct Blade *)t->object);
	bladeObj->angle = 0;
}
