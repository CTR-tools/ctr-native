#include <common.h>

// param1 pointer to array of two shorts (x,y)
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004ecd4-0x8004edac.
void UI_Lerp2D_Linear(s16 *ptrPos, s16 startX, s16 startY, s16 endX, s16 endY, int curFrame, s16 endFrame)
{
	int endFrameInt;
	int newPosX;

	// Get end frame
	endFrameInt = (int)endFrame;

	// If interpolation is not done yet
	if (curFrame <= endFrameInt)
	{
		newPosX = curFrame * ((int)endX - (int)startX);

		// newPosY
		curFrame = curFrame * ((int)endY - (int)startY);

		// posX
		*ptrPos = startX + (s16)(newPosX / endFrameInt);

		// posY
		ptrPos[1] = startY + (s16)(curFrame / endFrameInt);
		return;
	}

	// if you already reached the end

	// Set X and Y to EndX and EndY
	*ptrPos = endX;
	ptrPos[1] = endY;
	return;
}
