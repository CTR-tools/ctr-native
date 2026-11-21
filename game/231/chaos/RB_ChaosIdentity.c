#include <common.h>
#include <game/Vehicle/VehPhysForce.h>
#include "RB_ChaosEffects.h"
#include "RB_ChaosIdentity.h"

enum
{
	CHAOS_TRANSFORM_DELAY_MS = 100,
	CHAOS_TRANSFORM_SMOKE_COUNT = 8,
	CHAOS_TRANSFORM_SMOKE_ICON_GROUP = 1,
	CHAOS_TRANSFORM_SMOKE_SCALE = 0x2000,
	CHAOS_TRANSFORM_SMOKE_GROWTH = 0x200,
};

void Chaos_PreloadCharacterModels(struct BigHeader *bigfile)
{
	for (int character = 0; character < GAME_CHARACTER_COUNT; character++)
	{
		struct Model *model = VehBirth_GetModelByName(GAME_CHARACTER_METADATA[character].name_Debug);
		if (model != NULL)
		{
			sdata->chaosCharacterModels[character] = model;
			continue;
		}
		u32 size;
		u8 *file = LOAD_DramFile(bigfile, BI_RACERMODELHI + character, NULL, &size, -1);
		if (file != NULL)
		{
			sdata->chaosCharacterModels[character] = (struct Model *)(file + LOAD_MODEL_FILE_HEADER_BYTES);
		}
	}
}

void Chaos_SetCharacterModel(struct Driver *driver, int character, struct Model *model)
{
	struct Instance *instance = driver->instSelf;
	GAME_CHARACTER_IDS[driver->driverID] = character;
	instance->model = model;
	memcpy(instance->name, model->name, sizeof(instance->name));
	int frames = VehFrameInst_GetNumAnimFrames(instance, instance->animIndex);
	if (frames == 0)
	{
		instance->animIndex = 0;
		frames = VehFrameInst_GetNumAnimFrames(instance, 0);
	}
	if (instance->animFrame >= frames)
	{
		instance->animFrame = VehFrameInst_GetStartFrame(instance->animIndex, frames);
	}
	driver->matrixArray = BAKED_GTE_MATRIX_NONE;
	driver->matrixIndex = 0;
	VehBirth_SetWheelSizeForCharacter(driver, character);
}

static void Chaos_SpawnTransformationSmoke(struct Driver *driver)
{
	for (int i = 0; i < CHAOS_TRANSFORM_SMOKE_COUNT; i++)
	{
		struct Particle *particle = Particle_Init(0, GAME_TRACKER->iconGroup[CHAOS_TRANSFORM_SMOKE_ICON_GROUP], data.emSet_BurnSmoke);
		if (particle == NULL)
		{
			break;
		}
		particle->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal = CHAOS_TRANSFORM_SMOKE_SCALE;
		particle->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].velocity = CHAOS_TRANSFORM_SMOKE_GROWTH;
		VehPhysForce_ScaleParticle(driver, particle);
		particle->owner.driverInst = driver->instSelf;
		particle->otIndexOffset = driver->instSelf->depthBiasNormal;
	}
}

static void Chaos_TickTransformation(struct ChaosIdentity *identity, struct Driver *driver, s32 elapsedMS)
{
	identity->transformRemainingMS -= elapsedMS;
	if (identity->transformRemainingMS <= 0)
	{
		identity->transformRemainingMS = 0;
		Chaos_SetCharacterModel(driver, identity->pendingCharacter, sdata->chaosCharacterModels[identity->pendingCharacter]);
	}
}

void Chaos_UpdateTransformations(struct GameTracker *gGT)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	for (int i = 0; i < (int)len(runtime->identity); i++)
	{
		struct ChaosIdentity *identity = &runtime->identity[i];
		struct Driver *driver = gGT->drivers[i];
		if (identity->transformRemainingMS > 0 && driver != NULL)
		{
			if ((driver->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
			{
				identity->transformRemainingMS = 0;
			}
			else
			{
				Chaos_TickTransformation(identity, driver, gGT->elapsedTimeMS);
			}
		}
	}
}

void Chaos_ChangeIdentity(struct Driver *driver)
{
	bool present[GAME_CHARACTER_COUNT] = {0};
	for (int i = 0; i < (int)len(GAME_TRACKER->drivers); i++)
	{
		if (GAME_TRACKER->drivers[i] != NULL)
		{
			present[GAME_CHARACTER_IDS[i]] = true;
		}
	}
	for (int i = 0; i < (int)len(sdata->chaosRuntime.identity); i++)
	{
		const struct ChaosIdentity *identity = &sdata->chaosRuntime.identity[i];
		if (identity->transformRemainingMS > 0)
		{
			present[identity->pendingCharacter] = true;
		}
	}
	int choices[GAME_CHARACTER_COUNT];
	int count = 0;
	for (int character = 0; character < GAME_CHARACTER_COUNT; character++)
	{
		if (!present[character] && sdata->chaosCharacterModels[character] != NULL)
		{
			choices[count++] = character;
		}
	}
	if (count == 0)
	{
		return;
	}
	const int player = driver->driverID;
	const int character = choices[ChaosEffect_NextRandom(&sdata->chaosRng) % count];
	struct ChaosIdentity *identity = &sdata->chaosRuntime.identity[player];
	if (!identity->changed)
	{
		identity->originalCharacter = GAME_CHARACTER_IDS[player];
		identity->originalModel = driver->instSelf->model;
		identity->changed = true;
	}
	Chaos_SetDrivingClass(driver, GAME_CHARACTER_METADATA[character].engineID);
	identity->pendingCharacter = character;
	identity->transformRemainingMS = CHAOS_TRANSFORM_DELAY_MS;
	Chaos_SpawnTransformationSmoke(driver);
}

