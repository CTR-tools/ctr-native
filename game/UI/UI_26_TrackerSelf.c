#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004fd34-0x8005045c.
void UI_TrackerSelf(struct Driver *d)
{
	s16 y;
	s16 x;
	s16 timer;
	s16 sVar4;
	s16 sVar5;
	s16 sVar6;
	s16 *ptrAnim;
	u16 driverid;
	s16 screenPosXY[2];
	u32 rgb0, rgb1, rgb2;
	int warpballDist;
	int missileDist;
	int beep_rate;
	u_long *ot;
	POLY_G3 *p;
	SVECTOR pos;
	struct PrimMem *primMem;
	s16 orientation;
	u32 bgColor;
	s16 sVar18;
	int screenPosX;
	s16 screenPosY;
	struct GameTracker *gGT = sdata->gGT;

	// get index of driver in driver array
	driverid = d->driverID;

	timer = data.trackerTimer[driverid];

	if (
	    // timer loop ended
	    (timer == 0) &&

	    // no more missile chasing player
	    (d->thTrackingMe == 0))
	{
		// clear type of object tracking the player
		data.trackerType[driverid] = (s16)(int)NULL;
		return;
	}

	warpballDist = 0;

	// If no missile or warpball is chasing this driver
	if (d->thTrackingMe == 0)
	{
		ptrAnim = &data.trackerAnim2[0];
	}
	// if a missile or warpball is chasing this driver
	else
	{
		ptrAnim = &data.trackerAnim1[0];
	}

	x = ptrAnim[timer * 2 + 0];
	y = ptrAnim[timer * 2 + 1];

	struct Thread *trackerTh = RB_GetThread_ClosestTracker(d);

	if (
	    // timer loop ended
	    (timer == 0) &&

	    // no missiles chasing player
	    (trackerTh == NULL))
	{
		// trackerWeapon hit intended driverTarget
		if ((((struct TrackerWeapon *)d->thTrackingMe->object)->flags & 0x10) != 0)
			goto LAB_8004fe8c;

		// reset timer loop
		sVar18 = 12;
	}

	// missile chasing player,
	// or dead missile + timer loop active
	else
	{
		if ((
		        // if 27th bit of Actions Flag set is on (means ?)
		        ((d->actionsFlagSet & 0x4000000) != 0) &&

		        // tracker chasing driver
		        (trackerTh == d->thTrackingMe)) ||

		    // timer loop active
		    (timer != 0))
			goto LAB_8004fe8c;

		// turn on 27th bit of Actions Flag set (means ?)
		d->actionsFlagSet |= 0x4000000;

		// reset timer loop
		sVar18 = 8;
	}

	// reset timer loop
	data.trackerTimer[driverid] = sVar18;

LAB_8004fe8c:

	// set pointer of the missile or warpball chasing the player
	d->thTrackingMe = trackerTh;

	if (timer != 0)
	{
		data.trackerTimer[driverid]--;
	}

	MATRIX *m = &gGT->pushBuffer[driverid].matrix_ViewProj;
	gte_SetRotMatrix(m);
	gte_SetTransMatrix(m);

	pos.vx = (s16)d->instSelf->matrix.t[0];
	pos.vy = (s16)d->instSelf->matrix.t[1];
	pos.vz = (s16)d->instSelf->matrix.t[2];

	gte_ldv0(&pos);
	gte_rtps();
	gte_stsxy(&screenPosXY);

	// red?
	bgColor = 0xff;

	// if no missile or warpball is chasing this player
	if (d->thTrackingMe == NULL)
	{
		sVar18 = data.trackerDist[driverid];
	}

	// if a missile or warpball is chasing this player
	else
	{
		struct TrackerWeapon *tw = d->thTrackingMe->object;

		// NOTE(aalhendi): Retail always computes tracker distance here.
		missileDist = VehCalc_FastSqrt(tw->distanceToTarget, 0);

		missileDist = missileDist / 0x32;
		sVar18 = (s16)missileDist;
		data.trackerDist[driverid] = sVar18;

		beep_rate = 5;
		if (missileDist > 100)
			beep_rate = 10;
		if (missileDist > 200)
			beep_rate = 30;

		// red
		bgColor = 0xff;

		if ((gGT->timer % beep_rate) == 0)
		{
			if ((gGT->gameMode1 & PAUSE_ALL) == 0)
			{
				// "homing in" sound
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050014-0x8005001c for tracker beep SFX.
				OtherFX_Play(0x56, 1);
			}

			// white
			bgColor = 0xffffff;
		}

		if (d->thTrackingMe->inst->model->id == DYNAMIC_WARPBALL)
		{
			struct CheckpointNode *firstNode = &gGT->level1->ptr_restart_points[0];

			warpballDist = ((tw->ptrNodeCurr->distToFinish - firstNode[d->unknown_lap_related[1]].distToFinish

			                 ) *
			                8);

			// if warpball is further in the lap than the driver,
			// then add a full lap of distance until warpball hits driver
			if (warpballDist < 0)
			{
				// add length of track
				warpballDist += (firstNode[0].distToFinish * 8);
			}

			// type of object following driver is a warpball
			data.trackerType[driverid] = 1;
		}

		// if this is not a warpball
		else
		{
			// type of object is missile, or nothing?
			data.trackerType[driverid] = 0;
		}
	}

	// driver screenspace x and y
	screenPosX = (s16)screenPosXY[0];
	screenPosY = (s16)screenPosXY[1];

	// check distance
	if (warpballDist < 16000)
	{
		primMem = &gGT->backBuffer->primMem;

		sVar6 = sVar18 + (x >> 8);
		sVar5 = (s16)((y * 7) >> 12);

		for (char i = 0; i < 2; i++)
		{
			// if left side or right side
			orientation = 1;
			if (i == 0)
			{
				orientation = -1;
			}

			// if tracking object is warpball
			if (data.trackerType[driverid] == 1)
			{
				// tracker border colors (blue)
				rgb0 = 0x309c6900;

				rgb2 = 0x30ffff00;
			}

			// if tracking object is missile
			else
			{
				// tracker border colors (orange)
				rgb0 = 0x3000699c;

				rgb2 = 0x3000ffff;
			}

			p = primMem->curr;
			if (p > (POLY_G3 *)primMem->endMin100)
				return;
			primMem->curr = p + 1;

			*(int *)&p->r0 = rgb0;
			*(int *)&p->r1 = 0x30ffffff;
			*(int *)&p->r2 = rgb2;

			sVar4 = orientation * sVar6;
			p->x0 = screenPosX + sVar4;
			p->x2 = screenPosX + sVar4;

			sVar4 = orientation * sVar18;
			p->x1 = screenPosX + sVar4;

			p->y0 = screenPosY - (sVar5 + 12);
			p->y1 = screenPosY - 12;
			p->y2 = screenPosY - 12;

			ot = gGT->pushBuffer[driverid].ptrOT;

			*(int *)p = *ot | 0x6000000;
			*ot = CtrGpu_PrimToOTLink24(p);

			// next Prim
			POLY_G3 *pLast = p;
			p = primMem->curr;
			if (p > (POLY_G3 *)primMem->endMin100)
				return;
			primMem->curr = p + 1;

			// if tracking object is warpball
			if (data.trackerType[driverid] == 1)
			{
				rgb1 = 0x305b5b00;

				rgb0 = 0x30322b01;

				rgb2 = 0x30ffbb00;
			}

			// if tracking object is missile
			else
			{
				rgb1 = 0x30005b5b;

				rgb0 = 0x30012b32;

				rgb2 = 0x3000bbff;
			}

			*(int *)&p->r0 = rgb0;
			*(int *)&p->r1 = rgb1;
			*(int *)&p->r2 = rgb2;

			*(int *)&p->x0 = *(int *)&pLast->x0;
			*(int *)&p->x1 = *(int *)&pLast->x1;
			p->x2 = pLast->x2;

			p->y2 = screenPosY + sVar5 - 12;

			*(int *)p = *ot | 0x6000000;
			*ot = CtrGpu_PrimToOTLink24(p);
		}
	}

	UI_TrackerBG(

	    // missile lock-on icon
	    gGT->ptrIcons[0x2d],

	    screenPosX - (x >> 7), screenPosY - ((y * 0xf) >> 0xb),

	    &gGT->backBuffer->primMem, gGT->pushBuffer[driverid].ptrOT, 1, x, y, bgColor);
}
