#ifndef CTR_TEST_CHAOS_JUMP_FIXTURE_H
#define CTR_TEST_CHAOS_JUMP_FIXTURE_H

static u32 PreparedJump(struct Driver *driver, u32 tapped, u32 held, int rainCloudEffect, u32 actions)
{
	struct GamepadBuffer pad = {.buttonsTapped = tapped, .buttonsHeldCurrFrame = held};
	driver->actionsFlagSet = actions;
	Chaos_ApplyInputRules(driver, &pad);
	actions = VehPhysProc_JumpInput(driver, pad.buttonsTapped, pad.buttonsHeldCurrFrame, rainCloudEffect, actions);
	return Chaos_ApplyDriver(driver, actions);
}

#endif
