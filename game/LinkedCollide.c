#include <common.h>


struct Instance *LinkedCollide_Radius(struct Instance *objInst, struct Thread *_objTh, struct Thread *thBucket, u32 hitRadius)
{
	u32 diff_z, diff_x;
	s32 diff_y;
	u32 diff_dist;
	struct Instance *thInst;

	(void)_objTh;

	// NOTE(aalhendi): Retail compares squared distances modulo 32 bits.
	// Unsigned deltas keep subtraction, products, and sums defined on native.

	// Find the first collision in the thread bucket.
	while (thBucket != 0)
	{
		thInst = thBucket->inst;

		// Acquire the horizontal deltas before the signed cylinder height.
		diff_x = (u32)thInst->matrix.t[0] - (u32)objInst->matrix.t[0];
		diff_z = (u32)thInst->matrix.t[2] - (u32)objInst->matrix.t[2];
		diff_y = (s32)((u32)thInst->matrix.t[1] - (u32)objInst->matrix.t[1]);


		// NOTE(aalhendi): Keep each metric branch-local and the Z square
		// separate; old GCC otherwise hoists it or reuses the delta's register.

		// Minecart
		if (objInst->model->id == DYNAMIC_MINE_CART)
		{
			u32 zSquared;

			diff_x *= diff_x;
			zSquared = diff_z * diff_z;
			diff_dist = diff_x + zSquared;

			// Cylinder collision; hitRadius is already a squared threshold.
			if ((diff_dist < hitRadius) && (-0x20 < diff_y))
			{
				return thInst;
			}
		}

		// Spherical collision for everything else
		else
		{
			u32 zSquared;

			diff_x *= diff_x;
			zSquared = diff_z * diff_z;
			diff_dist = diff_x + zSquared;
			if (diff_dist + (u32)diff_y * (u32)diff_y < hitRadius)
			{
				return thInst;
			}
		}

		// next thread in the list (thread bucket)
		thBucket = thBucket->siblingThread;
	}
	// No collision.
	return 0;
}


struct Instance *LinkedCollide_Hitbox_Desc(struct HitboxDesc *objBoxDesc)
{
	return LinkedCollide_Hitbox(objBoxDesc->inst, objBoxDesc->thread, objBoxDesc->bucket, objBoxDesc->bbox);
}


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

		// NOTE(aalhendi): Height uses full instance translations, not the
		// narrowed SDK vector used for the horizontal transform below.
		diff_y = thInst->matrix.t[1] - objInst->matrix.t[1];

		thInstPos.vx = thInst->matrix.t[0];
		thInstPos.vy = thInst->matrix.t[1];
		thInstPos.vz = thInst->matrix.t[2];

		MATH_HitboxMatrix(&thInstMatrix, &objInst->matrix);

		SetRotMatrix(&thInstMatrix);
		SetTransMatrix(&thInstMatrix);

		RotTrans(&thInstPos, &outVec, flags);

		if ((outVec.vx < bbox.max.x) && (bbox.min.x < outVec.vx) && (outVec.vz < bbox.max.z) && (bbox.min.z < outVec.vz) && (bbox.min.y <= diff_y) &&
		    (diff_y < bbox.max.y))
		{
			return thInst; // collision thread instance
		}
	}
	return 0;
}
