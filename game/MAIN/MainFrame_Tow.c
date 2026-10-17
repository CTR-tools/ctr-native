#include <common.h>

// ---------------------------------------------------------------------------
// Development aid hack. Enabled by the "Tow Player 2" option in the Developer
// Hacks section;
//
// Adventure co-op is won by the pair taking first and second, which is hard to
// reproduce with a single controller because the partner kart just sits on the
// grid. While enabled, select toggles a tow that holds the partner racer right
// behind the lead racer, so one player can produce the podium finish that the
// co-op result is built on.
// ---------------------------------------------------------------------------

enum MainFrameTowConstants
{
	// Driver slots the tow connects: the racer being followed, and the towed one.
	MAINFRAME_TOW_LEAD_SLOT = 0,
	MAINFRAME_TOW_TOWED_SLOT = 1,

	// How far behind the lead kart the towed kart is held, in position units (world
	// unit << FRACTIONAL_BITS_8). Kart-against-kart contact happens inside 0x19
	// world units, so this reads as one kart directly behind the other while keeping
	// the two clear of each other.
	MAINFRAME_TOW_TRAIL_DISTANCE = 0x50 << FRACTIONAL_BITS_8,
};

// Whether the tow is engaged. Select toggles it, so the choice survives across races
// and a round can be repeated without pressing Select again.
static b32 s_towEngaged = 0;

// Parks the towed kart one kart-length behind the lead kart, facing the same way and
// at the same pace. This runs after the vehicle stages, so the position written here
// is the one the cameras, the lap and rank update, and the next frame's physics all
// read back.
static void MainFrame_Tow_ParkBehindLead(struct Driver *lead, struct Driver *towed)
{
	// A heading angle points along (sin, cos) in the XZ plane, so the trailing spot is
	// the lead position walked backwards along that direction. Y comes along too, so
	// the towed kart rides the same piece of track at the same height.
	Vec3 trailPos = {
	    .x = lead->posCurr.x - FP_MULT(MATH_Sin(lead->angle), MAINFRAME_TOW_TRAIL_DISTANCE),
	    .y = lead->posCurr.y,
	    .z = lead->posCurr.z - FP_MULT(MATH_Cos(lead->angle), MAINFRAME_TOW_TRAIL_DISTANCE),
	};

	// Starting the swept segment on the trailing spot as well keeps this frame's
	// collision sweep a normal step instead of a jump across the track.
	towed->posPrev = trailPos;
	towed->posCurr = trailPos;

	// Same heading, so the kart model, the split-screen camera and the lap projection
	// all read the pair as a single line. The movement matrix is copied next to the
	// angle because lap progress measures progress and wrong way against that matrix
	// rather than against the angle.
	towed->angle = lead->angle;
	towed->rotCurr = lead->rotCurr;
	towed->rotPrev = lead->rotPrev;
	towed->matrixMovingDir = lead->matrixMovingDir;

	// Same pace, so the speedometer, the engine note and the swept segment belong to
	// the kart being followed instead of reading as a stalled kart being dragged.
	towed->speed = lead->speed;
	towed->velocity = lead->velocity;
}

void MainFrame_Tow_Update(struct GameTracker *gGT, struct GamepadSystem *gGamepads)
{
	// The option is off, so the aid is off. Drop the toggle as well, so turning the
	// option back on does not silently re-arm a tow the player had engaged earlier.
	if (!g_config.towPlayer2)
	{
		s_towEngaged = 0;
		return;
	}

	if ((gGT->gameMode1 & (GAME_CUTSCENE | END_OF_RACE | PAUSE_ALL)) != 0 ||
	    sdata->ptrActiveMenu != NULL)
	{
		return;
	}

	if ((gGamepads->anyoneTapped & BTN_SELECT) != 0)
	{
		s_towEngaged = !s_towEngaged;
	}

	if (!s_towEngaged)
	{
		return;
	}

	struct Driver *lead = gGT->drivers[MAINFRAME_TOW_LEAD_SLOT];
	struct Driver *towed = gGT->drivers[MAINFRAME_TOW_TOWED_SLOT];

	if (lead == NULL || towed == NULL)
	{
		return;
	}

	MainFrame_Tow_ParkBehindLead(lead, towed);
}
