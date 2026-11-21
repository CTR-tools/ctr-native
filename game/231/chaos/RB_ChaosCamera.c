#include <common.h>
#include <chaos/chaos_gate.h>
#include <game/Vehicle/VehPhysForce.h>
#include "RB_ChaosRuntime.h"

enum
{
	CHAOS_TOP_DOWN_HEIGHT = 0xa00,
	CHAOS_TOP_DOWN_PITCH = 0x400,
	CHAOS_TOP_DOWN_YAW_OFFSET = 0x800,
	CHAOS_TOP_DOWN_CUTAWAY_HEIGHT = 0x80,
};

static void Chaos_SetTopDownCamera(struct CameraDC *camera, const struct Driver *driver)
{
	struct PushBuffer *pushBuffer = camera->pushBuffer;
	const int x = driver->posCurr.x >> 8;
	const int y = driver->posCurr.y >> 8;
	const int z = driver->posCurr.z >> 8;
	const int height = y + CHAOS_TOP_DOWN_HEIGHT;
	pushBuffer->pos.x = (s16)x;
	pushBuffer->pos.y = (s16)(height > INT16_MAX ? INT16_MAX : height);
	pushBuffer->pos.z = (s16)z;
	pushBuffer->rot.x = CHAOS_TOP_DOWN_PITCH;
	pushBuffer->rot.y = (s16)(driver->angle + CHAOS_TOP_DOWN_YAW_OFFSET);
	pushBuffer->rot.z = 0;
	pushBuffer->distanceToScreen_PREV = pushBuffer->distanceToScreen_CURR;
	struct QuadBlock *ground = driver->underDriver != NULL ? driver->underDriver : driver->currBlockTouching;
	if (ground != NULL)
	{
		camera->ptrQuadBlock = ground;
		camera->quadBlockSearchHit = true;
	}
}

static bool Chaos_TopDownHidesQuadBlock(const struct Driver *driver, const struct QuadBlock *block, const struct LevVertex *vertices)
{
	if (block == driver->underDriver || block == driver->currBlockTouching)
	{
		return false;
	}
	// Same high-detail triangles and normal winding as COLL_FIXED_QUADBLK.
	static const u8 triangles[][3] = {
		{0, 4, 5}, {4, 6, 5}, {6, 4, 1}, {5, 6, 2},
		{8, 6, 7}, {7, 3, 8}, {1, 7, 6}, {2, 6, 8},
	};
	const int numTriangles = block->index[2] == block->index[3] ? 4 : (int)len(triangles);
	bool ceiling = false;
	for (int i = 0; i < numTriangles; i++)
	{
		const SVec3 *a = &vertices[block->index[triangles[i][0]]].pos;
		const SVec3 *b = &vertices[block->index[triangles[i][1]]].pos;
		const SVec3 *c = &vertices[block->index[triangles[i][2]]].pos;
		const s64 normalY = (s64)(c->z - a->z) * (b->x - a->x) - (s64)(c->x - a->x) * (b->z - a->z);
		if (normalY > 0)
		{
			// Keep any road-facing part, including unflagged ramps and mixed blocks.
			return false;
		}
		ceiling |= normalY < 0;
	}
	// Ceiling classification is independent of kart position and block bounds.
	return ceiling;
}

static bool Chaos_TopDownHidesInstance(const struct Driver *driver, const struct PushBuffer *pushBuffer,
                                     const struct Instance *instance, int nearestDepth, bool coversKart)
{
	if (!coversKart || instance->instDef == NULL || instance->thread != NULL)
	{
		return false;
	}
	const int ceiling = (driver->posCurr.y >> 8) + VehPhysForce_ScaleValue(driver, CHAOS_TOP_DOWN_CUTAWAY_HEIGHT);
	return nearestDepth < (pushBuffer->pos.y - ceiling) * 4;
}

static void Chaos_SetTopDownVisibility(struct VisMem *visibility, const struct mesh_info *mesh, int playerIndex)
{
	// Ground-level PVS omits road hidden behind crests and tunnel entrances.
	// Let the overhead camera's frustum select geometry instead.
	memset(visibility->visLeafList[playerIndex], 0xff, ((mesh->numBspNodes + 0x1f) >> 5) * sizeof(int));
	memset(visibility->visFaceList[playerIndex], 0xff, ((mesh->numQuadBlock + 0x1f) >> 5) * sizeof(int));
	// Force retail lists to be decoded again after expiry, even on the same block.
	visibility->visLeafSrc[playerIndex] = NULL;
	visibility->visFaceSrc[playerIndex] = NULL;
}

static const struct Driver *Chaos_TopDownDriver(const struct PushBuffer *pushBuffer)
{
	const struct GameTracker *tracker = sdata->gGT;
	if (pushBuffer == NULL || !sdata->chaosRuntime.raceInitialized || !sdata->chaosSettings.armed ||
	    !ChaosGate_IsRaceEligible(tracker->gameMode1 & ~PAUSE_ALL, tracker->overlayIndex_Threads,
	                             tracker->trafficLightsTimer < 1))
	{
		return NULL;
	}
	for (int i = 0; i < tracker->numPlyrCurrGame && i < (int)len(tracker->cameraDC); i++)
	{
		const struct CameraDC *camera = &tracker->cameraDC[i];
		const struct Driver *driver = camera->driverToFollow;
		if (camera->pushBuffer == pushBuffer && Chaos_HasEffectForDriver(CHAOS_EFFECT_CLASSIC_GTA, driver) &&
		    (driver->actionsFlagSet & ACTION_BOT) == 0)
		{
			return driver;
		}
	}
	return NULL;
}

bool Chaos_ApplyCameraRules(struct CameraDC *camera)
{
	struct PushBuffer *pushBuffer = camera->pushBuffer;
	const struct Driver *driver = Chaos_TopDownDriver(pushBuffer);
	if (driver == NULL || driver != camera->driverToFollow)
	{
		return false;
	}
	Chaos_SetTopDownCamera(camera, driver);
	return true;
}

bool Chaos_HideCameraQuadBlock(const struct PushBuffer *pushBuffer, const struct QuadBlock *block, const struct LevVertex *vertices)
{
	const struct Driver *driver = Chaos_TopDownDriver(pushBuffer);
	if (driver == NULL)
	{
		return false;
	}
	return Chaos_TopDownHidesQuadBlock(driver, block, vertices);
}

void Chaos_ApplyLevelVisibilityRules(const struct PushBuffer *pushBuffer, struct VisMem *visibility,
                                   const struct mesh_info *mesh, int playerIndex)
{
	if (Chaos_TopDownDriver(pushBuffer) != NULL)
	{
		Chaos_SetTopDownVisibility(visibility, mesh, playerIndex);
	}
}

bool Chaos_HideCameraInstance(const struct PushBuffer *pushBuffer, const struct Instance *instance, int nearestDepth, bool coversKart)
{
	const struct Driver *driver = Chaos_TopDownDriver(pushBuffer);
	if (driver == NULL)
	{
		return false;
	}
	return Chaos_TopDownHidesInstance(driver, pushBuffer, instance, nearestDepth, coversKart);
}

