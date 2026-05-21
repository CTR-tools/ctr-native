#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018ba0-0x80018d20
u8 DECOMP_CAM_Path_Move(int frameIndex, s16 *position, s16 *rotation, s16 *getPath)
{
	s16 frame;
	s16 numPos;
	u16 pathNumNode;
	u16 pathID;
	s16 *ptrCam;
	s16 *move;

	frame = (s16)frameIndex;

	// get number of position on track
	numPos = DECOMP_CAM_Path_GetNumPoints();

	if (frame < 0)
		return 0;
	if (frame >= numPos)
		return 0;

	void **ptrs = ST1_GETPOINTERS(sdata->gGT->level1->ptrSpawnType1);
	ptrCam = ptrs[ST1_CAMERA_PATH];

	pathNumNode = (u16)ptrCam[0];
	pathID = (u16)ptrCam[1];
	move = ptrCam + 2;

	while ((s16)pathNumNode <= frame)
	{
		frame = (s16)(frame - (s16)pathNumNode);
		move = move + (s16)pathNumNode * 6;
		pathNumNode = (u16)move[0];
		pathID = (u16)move[1];
		move = move + 2;
	}

	// advance pointer to pos+rot
	move += (int)frame * 6;

	*getPath = pathID;

	// position of frame
	position[0] = move[0];
	position[1] = move[1];
	position[2] = move[2];

	// rotation of frame
	rotation[0] = ((s16)move[3] >> 4) + 0x800U & 0xfff;
	rotation[1] = (u16)move[4] >> 4;
	rotation[2] = (u16)move[5] >> 4;
	return 1;
}
