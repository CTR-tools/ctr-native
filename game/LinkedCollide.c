#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800314e0-0x800315ac. Retail ignores _objTh.
struct Instance *LinkedCollide_Radius(struct Instance *objInst, struct Thread *_objTh, struct Thread *thBucket, u32 hitRadius)
{
	int diff_z, diff_y, diff_x;
	u32 diff_dist;
	struct Instance *thInst;

	// if thread valid, loop through every thread in the linked list until there are no more threads
	while (thBucket != 0)
	{
		thInst = thBucket->inst;

		// get difference in X, Y, and Z, from both instances
		diff_x = thInst->matrix.t[0] - objInst->matrix.t[0];
		diff_y = thInst->matrix.t[1] - objInst->matrix.t[1];
		diff_z = thInst->matrix.t[2] - objInst->matrix.t[2];

		diff_dist = diff_x * diff_x + diff_z * diff_z;

		// Minecart
		if (objInst->model->id == DYNAMIC_MINE_CART)
		{
			// Cylinder collision
			if ((diff_dist < hitRadius) && (-0x20 < diff_y))
			{
				return thInst;
			}
		}

		// Spherical collision for everything else
		else if (diff_dist + diff_y * diff_y < hitRadius)
		{
			return thInst;
		}

		// next thread in the list (thread bucket)
		thBucket = thBucket->siblingThread;
	}
	// no collision
	return 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800315ac-0x80031608.
struct Instance *LinkedCollide_Hitbox_Desc(struct HitboxDesc *objBoxDesc)
{
	return LinkedCollide_Hitbox(objBoxDesc->inst, objBoxDesc->thread, objBoxDesc->bucket, objBoxDesc->bbox);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031608-0x80031734. Retail ignores _objTh.
struct Instance *LinkedCollide_Hitbox(struct Instance *objInst, struct Thread *_objTh, struct Thread *thBucket, struct BoundingBox bbox)
{
	struct Instance *thInst;
	int diff_y;
	MATRIX thInstMatrix;
	SVECTOR thInstPos;
	VECTOR outVec;
	s32 flags[2];

	(void)_objTh;

	// Loop over thBucket Linked List
	for (; thBucket != 0; thBucket = thBucket->siblingThread)
	{
		thInst = thBucket->inst;

		thInstPos.vx = thInst->matrix.t[0];
		thInstPos.vy = thInst->matrix.t[1];
		thInstPos.vz = thInst->matrix.t[2];

		diff_y = thInst->matrix.t[1] - objInst->matrix.t[1];

		MATH_HitboxMatrix(&thInstMatrix, &objInst->matrix);

		SetRotMatrix(&thInstMatrix);
		SetTransMatrix(&thInstMatrix);

		RotTrans(&thInstPos, &outVec, flags);

		if ((bbox.min.x < outVec.vx) && (outVec.vx < bbox.max.x) && (bbox.min.z < outVec.vz) && (outVec.vz < bbox.max.z) && (bbox.min.y <= diff_y) &&
		    (diff_y < bbox.max.y))
		{
			return thInst; // collision thread instance
		}
	}
	return 0;
}
