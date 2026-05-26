#include <common.h>

// used for character icons in races
// get position of icon based on a circular motion to move the driver up or down in the ranks
// param1 = pointer to (x,y) position
// param2 = drawn position
// param3 = absolute position
// param4 = frame counter
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004eaa8-0x8004ec18.
void UI_Lerp2D_Angular(s16 *ptrPos, s16 drawnPosition, s16 absolutePosition, s16 frameCounter)
{
	int angle;
	int drawnPositionInt;
	int absolutePositionInt;

	// Moving up moves icon to the right
	// Moving down moves icon to the left
	// 0x1b is a constant for base radius

	// drawn position
	drawnPositionInt = (int)drawnPosition;

	// absolute position
	absolutePositionInt = (int)absolutePosition;

	angle = MATH_Sin(((int)frameCounter << 0xb) / 5);

	// if driver "just" passed another driver
	if (absolutePositionInt < drawnPositionInt)
	{
		ptrPos[0] = (s16)(angle * 0x14 >> 0xc) + 0x14;
	}

	// if driver "was" passed by another driver
	else
	{
		ptrPos[0] = 0x14 - (s16)(angle * 0x14 >> 0xc);
	}

	// absolutePositionInt - drawnPositionInt is either -1 or +1
	// 0x1b is vertical size of the icon
	ptrPos[1] =

	    // Y value where all icons start
	    0x39 +

	    // start Y before transition
	    (drawnPosition * 0x1b) +

	    // transition per frame
	    (
	        // distance to travel
	        ((((absolutePositionInt - drawnPositionInt) * 0x1b)

	          // move more each frame
	          * (int)frameCounter) *
	         4)

	        // divide distance down
	        / (5 * 4));

	return;
}
