#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004f9d8-0x8004fd34.
// Draw arrows over the heads of players
void UI_BattleDrawHeadArrows(struct Driver *player)
{
	typedef struct
	{
		u32 tag;
		u32 tpage;
		POLY_G3 g3;
	} G3_SEMITRANS;

	int playerDistance;
	u16 currTeam;
	s16 sVar1;
	s16 sVar3;
	s16 sVar4;
	s16 sVar5;
	int iVar6;
	s16 outXY[2];
	u32 flag;
	u32 color;
	MATRIX *m;
	G3_SEMITRANS *p;
	SVECTOR pos;

	struct GameTracker *gGT = sdata->gGT;

	u8 playerID = player->driverID;

	// pushBuffer ViewProj
	m = &gGT->pushBuffer[playerID].matrix_ViewProj;
	gte_SetRotMatrix(m);
	gte_SetTransMatrix(m);

	u8 numPlyr = gGT->numPlyrCurrGame;

	for (u8 i = 0; i < numPlyr; i++)
	{
		// something related to player structure address
		struct Driver *currDriver = gGT->drivers[i];

		if (
		    // skip yourself, skip invisible, skip finished players
		    (i == playerID) || (currDriver->invisibleTimer != 0) || ((currDriver->actionsFlagSet & ACTION_RACE_FINISHED) != 0))
		{
			continue;
		}

		// If 3 or 4 Players
		sVar1 = 5;

		// If numPlyrCurrGame is less than 3
		if (numPlyr < 3)
		{
			sVar1 = 3;
		}

		struct Instance *currInst = currDriver->instSelf;
		struct Instance *playerInst = player->instSelf;

		// Get X distance and Z distance between two players
		int xDistance = playerInst->matrix.t[0] - currInst->matrix.t[0];
		int zDistance = playerInst->matrix.t[2] - currInst->matrix.t[2];
		int playerDistance = ((xDistance * xDistance) + (zDistance * zDistance));

		// sqrt(0x90000) is 768

		// If currentDriver more than 768 units away from this player,
		// don't draw that driver's arrow
		if (0x90000 > playerDistance)
		{
			continue;
		}

		// load input vector
		pos.vx = currInst->matrix.t[0];
		pos.vy = currInst->matrix.t[1];
		pos.vz = currInst->matrix.t[2];

		CTR_GteLoadSV0(&pos);

		// perspective projection
		gte_rtps();

		// get output
		CTR_GteStoreSXY(&outXY[0]);
		gte_stflg(&flag);

		if ((flag & 0x40000) != 0)
		{
			continue;
		}

		struct PrimMem *primMem = &gGT->backBuffer->primMem;

		p = primMem->cursor;
		if ((int)p > (int)primMem->guardEnd)
		{
			return;
		}

		primMem->cursor = p + 1;

		p->tpage = 0xe1000a20;
		p->g3.tag = 0;

		p->g3.code = 0x32;

		sVar4 = outXY[0];
		sVar5 = outXY[1] + sVar1;
		iVar6 = (0x1000 - (((playerDistance / 6 + (playerDistance >> 0x1f)) >> 0xd) - (playerDistance >> 0x1f)));
		sVar1 = (s16)(iVar6 * 3 >> 10);
		sVar3 = (s16)(iVar6 * 7 >> 12) + 12;

		p->g3.x0 = sVar4 - sVar1;
		p->g3.y0 = sVar5 - sVar3;
		p->g3.x1 = sVar4;
		p->g3.y1 = sVar5 - 12;
		p->g3.x2 = sVar4 + sVar1;
		p->g3.y2 = sVar5 - sVar3;

		// Battle Team of this driver
		currTeam = currDriver->BattleHUD.teamID;

		// color data
		color = *(u32 *)data.ptrColor[PLAYER_BLUE + currTeam];

		// it's all the same color
		*(int *)&p->g3.r0 = (color & 0xffffff) | 0x30000000;
		*(int *)&p->g3.r1 = color | 0x30000000;
		*(int *)&p->g3.r2 = color | 0x30000000;

		uint32_t *ot = gGT->pushBuffer[playerID].ptrOT;

		*(int *)p = CtrGpu_PackOTTag(*ot, 0x8000000);
		*ot = CtrGpu_PrimToOTLink24(p);
	}
}

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
	uint32_t *ot;
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
		{
			goto LAB_8004fe8c;
		}

		// reset timer loop
		sVar18 = 12;
	}

	// missile chasing player,
	// or dead missile + timer loop active
	else
	{
		if ((
		        // if 27th bit of Actions Flag set is on (means ?)
		        ((d->actionsFlagSet & ACTION_TRACKER_TARGETED) != 0) &&

		        // tracker chasing driver
		        (trackerTh == d->thTrackingMe)) ||

		    // timer loop active
		    (timer != 0))
		{
			goto LAB_8004fe8c;
		}

		// turn on 27th bit of Actions Flag set (means ?)
		d->actionsFlagSet |= ACTION_TRACKER_TARGETED;

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

	CTR_GteLoadSV0(&pos);
	gte_rtps();
	CTR_GteStoreSXY(&screenPosXY);

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
		{
			beep_rate = 10;
		}
		if (missileDist > 200)
		{
			beep_rate = 30;
		}

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

			warpballDist = ((tw->ptrNodeCurr->distToFinish - firstNode[d->checkpoint.currentIndex].distToFinish

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

			p = primMem->cursor;
			if (p > (POLY_G3 *)primMem->guardEnd)
			{
				return;
			}
			primMem->cursor = p + 1;

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

			*(int *)p = CtrGpu_PackOTTag(*ot, 0x6000000);
			*ot = CtrGpu_PrimToOTLink24(p);

			// next Prim
			POLY_G3 *pLast = p;
			p = primMem->cursor;
			if (p > (POLY_G3 *)primMem->guardEnd)
			{
				return;
			}
			primMem->cursor = p + 1;

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

			*(int *)p = CtrGpu_PackOTTag(*ot, 0x6000000);
			*ot = CtrGpu_PrimToOTLink24(p);
		}
	}

	UI_TrackerBG(

	    // missile lock-on icon
	    gGT->ptrIcons[0x2d],

	    screenPosX - (x >> 7), screenPosY - ((y * 0xf) >> 0xb),

	    &gGT->backBuffer->primMem, gGT->pushBuffer[driverid].ptrOT, 1, x, y, bgColor);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005045c-0x80050528.
void UI_DrawPosSuffix(s16 posX, s16 posY, struct Driver *d, s16 flags)
{
	int currRank;
	struct GameTracker *gGT = sdata->gGT;

	// If you're not in Battle Mode
	if ((gGT->gameMode1 & BATTLE_MODE) == 0)
	{
		// Get the rank you're in (1st, 2nd, 3rd, etc)
		currRank = d->driverRank;
	}
	else
	{
		// get the rank that the battle team is in
		currRank = gGT->battleSetup.finishedRankOfEachTeam[d->BattleHUD.teamID];
	}

	// Draw the suffix of your current position
	DecalFont_DrawLine(sdata->lngStrings[data.stringIndexSuffix[currRank]], posX, posY, FONT_BIG, flags);

	// setting posZ changes which number draws
	if (d->instBigNum != 0)
	{
		d->instBigNum->matrix.t[2] = (d->driverRank + 0x100);
	}

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050528-0x80050654
void UI_DrawLapCount(s16 posX, int posY, int param_3, struct Driver *d)
{
	s16 type;
	s16 currLap;
	int numLaps;
	int flags;
	char message[24];
	char *str;

	struct GameTracker *gGT;
	int numPlyrCurrGame;

	gGT = sdata->gGT;
	numLaps = gGT->numLaps;
	numPlyrCurrGame = gGT->numPlyrCurrGame;

	currLap = d->lapIndex + 1;

	if (currLap > numLaps)
	{
		currLap = numLaps;
	}

	// 3P or 4P
	type = FONT_SMALL;
	flags = PERIWINKLE;

	// 1P or 2P
	if (numPlyrCurrGame < 3)
	{
		DecalFont_DrawLine(sdata->lngStrings[LNG_LAP], posX, posY, FONT_SMALL, (JUSTIFY_RIGHT | PERIWINKLE));

		sprintf(&message[0], &sdata->s_intDividing[0], currLap, numLaps);
		str = &message[0];
		type = FONT_BIG;
		flags = (JUSTIFY_RIGHT | PERIWINKLE);
	}
	else
	{
		str = &sdata->s_printDividing[0];
		str[0] = currLap + '0';
		str[2] = numLaps + '0';

		type = FONT_SMALL;
		flags = PERIWINKLE;
	}

	// draw string
	DecalFont_DrawLine(str, posX, (posY + 8), type, flags);
}

// Draw how many points or lifes the player has in battle
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050654-0x800507e0.
void UI_DrawBattleScores(int posX, int posY, struct Driver *d)
{
	struct Icon *icon;
	int value;
	char string[32];
	struct GameTracker *gGT = sdata->gGT;

	if ((gGT->gameMode1 & POINT_LIMIT) == 0)
	{
		if ((gGT->gameMode1 & LIFE_LIMIT) == 0)
		{
			return;
		}

		// == Life Limit

		value = d->BattleHUD.numLives;
		icon = gGT->ptrIcons[0x84];
	}

	else
	{
		// == Point Limit ==

		value = gGT->battleSetup.pointsPerTeam[d->BattleHUD.teamID];
		icon = gGT->ptrIcons[0x85];
	}

	// add value to string
	sprintf(string, (char *)&sdata->s_longInt, value);

	DecalFont_DrawLine(string, (s16)(posX + 37), (s16)(posY + 4), FONT_SMALL, data.battleScoreColor[gGT->numPlyrCurrGame - 1][d->driverID]);

	DecalHUD_DrawPolyFT4(icon, posX, posY, &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT, 1, 0x1000);
}
