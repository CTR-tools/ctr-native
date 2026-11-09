/* Some custom levels omit the end-of-race camera list altogether. */
static s16 *CAM_EndOfRace_Cameras(const struct Level *level)
{
	if (level == NULL || level->ptrSpawnType1 == NULL || level->ptrSpawnType1->count <= ST1_CAMERA_EOR)
	{
		return NULL;
	}
	void **pointers = ST1_GETPOINTERS(level->ptrSpawnType1);
	return pointers[ST1_CAMERA_EOR];
}

void CAM_EndOfRace_Battle(struct CameraDC *cDC, struct Driver *d)
{
	s32 height = data.Spin360_heightOffset_cameraPos[(s32)sdata->gGT->numPlyrCurrGame];
	cDC->transitionTo.pos.x = 0xffe5;
	cDC->transitionTo.pos.y = height;
	cDC->transitionTo.pos.z = 0xc0;
	cDC->flags |= CAMERA_FLAG_BATTLE_END_OF_RACE;
	cDC->transitionBlend = 0;
	cDC->transitionFrame = 60;
	cDC->transitionFrameCount = 60;

	struct PushBuffer *pb = cDC->pushBuffer;
	s32 dx = CTR_MipsSubLo(pb->pos.x, CTR_MipsSra(d->posCurr.x, 8));
	s32 dz = CTR_MipsSubLo(pb->pos.z, CTR_MipsSra(d->posCurr.z, 8));
	cDC->spin360Angle = ratan2(dx, dz);
}

void CAM_EndOfRace(struct CameraDC *cDC, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	if ((gGT->gameMode1 & BATTLE_MODE) == 0 && gGT->numPlyrCurrGame < 3 &&
	    CAM_EndOfRace_Cameras(gGT->level1) != NULL)
	{
		cDC->flags |= CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED;
	}
	else
	{
		CAM_EndOfRace_Battle(cDC, d);
	}
}
