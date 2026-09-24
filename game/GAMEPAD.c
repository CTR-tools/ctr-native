#include <common.h>

#ifndef GAMEPAD_ACT_ALIGN
#define GAMEPAD_ACT_ALIGN  sdata->padActuatorAlignment
#define GAMEPAD_BUTTON_MAP data.gamepadMapBtn
#define GAMEPAD_WHEEL_DATA data.rwd
#endif

void GAMEPAD_Init(struct GamepadSystem *gGamepads)
{
	s32 i;
	struct GamepadBuffer *pad;

	PadInitMtap((u8 *)&gGamepads->slotBuffer[0], (u8 *)&gGamepads->slotBuffer[1]);
	PadStartCom();

	for (i = 0; i < 8; i++)
	{
		pad = &gGamepads->gamepad[i];

		// no analog sticks detected
		pad->gamepadType = 0;
		pad->jogCenteringFrames = 0;
	}

	gGamepads->gamepadsConnectedByFlag = 0xffffffff;

	return;
}


void GAMEPAD_SetMainMode(void)
{
	PadSetMainMode(0, 0, 0);
	PadSetMainMode(1, 0, 0);
	PadSetMainMode(2, 0, 0);
	PadSetMainMode(3, 0, 0);
	PadSetMainMode(0x10, 0, 0);
	PadSetMainMode(0x11, 0, 0);
	PadSetMainMode(0x12, 0, 0);
	PadSetMainMode(0x13, 0, 0);
}


void GAMEPAD_ProcessState(struct GamepadBuffer *pad, s32 padState, s32 id)
{
	s32 numMotors;
	s32 motor;

	switch (padState)
	{
	case 2:
	{
		pad->motorPower[0] = 0;
		pad->motorPower[1] = 0;
		break;
	}
	case 1:
	{
		if (pad->gamepadType != 0)
		{
			pad->gamepadType = 1;
		}
		break;
	}
	case 6:
	{
		switch (pad->gamepadType)
		{
		case 0:
		{
			if (PadSetMainMode(id, 1, 0) != 0)
			{
				pad->gamepadType = 1;
			}
			break;
		}
		case 1:
		{
			// get number of motors on pad
			numMotors = PadInfoAct(id, 0xffffffff, 0);
			if (numMotors > 2)
			{
				numMotors = 2;
			}

			// loop through motors
			for (motor = 0; motor < numMotors; motor++)
			{
				pad->motorPower[motor] = (u8)PadInfoAct(id, motor, 4);
			}
			for (; numMotors < 2; ++numMotors)
			{
				pad->motorPower[numMotors] = 0;
			}

			PadSetAct(id, &pad->motorSubmit[0], sizeof(pad->motorSubmit));

			if (PadSetActAlign(id, &GAMEPAD_ACT_ALIGN[0]) != 0)
			{
				pad->gamepadType = 2;
			}
		}
		}
		break;
	}
	}
	return;
}


void GAMEPAD_PollVsync(struct GamepadSystem *gGamepads)
{
	u32 padState;
	u32 padID;
	struct GamepadBuffer *pad;
	s32 port;
	s32 numPorts;
	s32 maxPadsPerPort;
	s32 tap;
	s32 padIndex = 0;
	struct ControllerPacket *packet;

	// 2 players, no multitap
	numPorts = 2;
	maxPadsPerPort = 1;

	// If there is a multitap present
	packet = &gGamepads->slotBuffer[0].controller;
	if (packet->plugged == PLUGGED)
	{
		if (packet->controllerData == (PAD_ID_MULTITAP << 4))
		{
			numPorts = 1;
			maxPadsPerPort = 4;
		}
	}


	// loop through all gamepad ports
	// that gameplay cares about. Either
	// 1 or 2, main ports on console
	port = 0;
	if (numPorts != 0)
	{
		do
		{
			// loop through all gamepads that can connect
			// to this gamepad port. 1 for no mtap, 4 for mtap
			tap = 0;
			if (maxPadsPerPort != 0)
			{
				do
				{
					pad = &gGamepads->gamepad[padIndex];
					packet = &gGamepads->slotBuffer[port].controller;
					if (packet->controllerData == (PAD_ID_MULTITAP << 4))
					{
						if (packet->plugged != PLUGGED)
						{
							goto unplugged;
						}
						packet = &gGamepads->slotBuffer[port].multitap.controllers[tap];
					}
					if (packet->plugged != PLUGGED)
					{
					unplugged:
						// no analog sticks found
						pad->gamepadType = 0;
					}

					else
					{
						padID = (port << 4) | tap;

						// according to libref
						// 0 - PadStateDisCon
						// 1 - PadStateFindPad
						// and many more...
						padState = PadGetState(padID);

						GAMEPAD_ProcessState(pad, padState, padID);
					}

					// increment gamepad counter

					padIndex += 1;
					tap++;
				} while (tap < maxPadsPerPort);
			}
			port++;
		} while (port < numPorts);
	}

	// if there are less than 8 gamepads connected,
	// NOTE(aalhendi): End the per-port pointer lifetimes before the unused-pad
	// sweep. These non-emitting constraints preserve retail register allocation.
	CTR_PSX_CLOBBER("s1");
	CTR_PSX_CLOBBER("s3");
	while (padIndex < 8)
	{
		pad = &gGamepads->gamepad[padIndex];
		pad->gamepadType = 0;

		padIndex++;
	}
}


s32 GAMEPAD_GetNumConnected(struct GamepadSystem *gGamepads)
{
	s32 bitwiseConnected = 0;
	s32 padIndex = 0;

	s32 numPorts;
	s32 maxPadsPerPort;

	union GamepadSlot *slotPacket;
	struct ControllerPacket *packet;
	struct GamepadBuffer *pad;
	s32 port;
	s32 tap;
	u32 *connectedFlags;
	u32 previousFlags;

	// 2 players, no multitap
	numPorts = 2;
	maxPadsPerPort = 1;

	gGamepads->numGamepadsConnected = 0;
	packet = &gGamepads->slotBuffer[0].controller;
	if (packet->plugged == PLUGGED)
	{
		if (packet->controllerData == (PAD_ID_MULTITAP << 4))
		{
			numPorts = 1;
			maxPadsPerPort = 4;
		}
	}


	port = 0;
	if (numPorts != 0)
	{
		do
		{
			tap = 0;
			if (maxPadsPerPort != 0)
			{
				do
				{
					pad = &gGamepads->gamepad[padIndex];
					slotPacket = &gGamepads->slotBuffer[port];
					// NOTE(aalhendi): Native can switch between single-pad and multitap
					// layouts on hotplug. An unplugged slot must not retain the old view.
#ifdef CTR_NATIVE
					pad->ptrControllerPacket = NULL;
#endif
					packet = &slotPacket->controller;
					if (slotPacket->multitap.controllerData == (PAD_ID_MULTITAP << 4))
					{
						if (slotPacket->multitap.plugged != PLUGGED)
						{
							goto nextPad;
						}
						packet = &slotPacket->multitap.controllers[tap];
					}

					if (packet->plugged == PLUGGED)
					{
						bitwiseConnected |= 1 << (port * 4 + tap);
						gGamepads->numGamepadsConnected = padIndex + 1;

						pad->ptrControllerPacket = packet;
						pad->gamepadID = port * 0x10 + tap;
					}

				nextPad:

					padIndex += 1;
					tap++;
				} while (tap < maxPadsPerPort);
			}
			port++;
		} while (port < numPorts);
	}
	// NOTE(aalhendi): Keep the per-port iterator separate from the unused-pad sweep.
	CTR_PSX_CLOBBER("a3");
	while (padIndex < 8)
	{
		// pad is now unplugged
		pad = &gGamepads->gamepad[padIndex];
		pad->ptrControllerPacket = 0;

		padIndex++;
	}

	connectedFlags = &gGamepads->gamepadsConnectedByFlag;
	previousFlags = *connectedFlags;

	if (previousFlags != (u32)-1)
	{
		if ((u32)bitwiseConnected == previousFlags)
		{
			return 0;
		}
		*connectedFlags = bitwiseConnected;
		return (u32)((bitwiseConnected ^ previousFlags) & previousFlags) != 0;
	}

	*connectedFlags = bitwiseConnected;
	return 0;
}

// determine which buttons are held this frame,
// store a backup of "currFrame" into "lastFrame"
// param1 is pointer to gamepadSystem

s32 GAMEPAD_ProcessHold(struct GamepadSystem *gGamepads)
{
	const struct GamepadButtonMap *buttonMap;
	u32 buttonMapRawInput;
	u32 rawInput;
	u32 mappedButtons;
	u32 heldAny = 0;
	s32 i;

	struct GamepadBuffer *pad;
	struct ControllerPacket *ptrControllerPacket;


	// loop through all 8 gamepadBuffers
	for (i = 0; i < 8; i++)
	{
		pad = &gGamepads->gamepad[i];
		pad->buttonsHeldPrevFrame = pad->buttonsHeldCurrFrame;

		ptrControllerPacket = pad->ptrControllerPacket;

		if (ptrControllerPacket != NULL)
		{
			if (ptrControllerPacket->plugged != PLUGGED)
			{
				continue;
			}
			// endian flip
			rawInput = ((ptrControllerPacket->input.high << 8) | ptrControllerPacket->input.low) ^ 0xffff;
			mappedButtons = 0;

			// Normalize controller-specific button wiring before applying the map.
			switch (ptrControllerPacket->controllerData)
			{
			case ((PAD_ID_ANALOG_STICK << 4) | 3):
			{
				rawInput <<= 16;
				break;
			}
			case ((PAD_ID_NEGCON << 4) | 3):
			{
				if (0x40 < ptrControllerPacket->payload.neGcon.btn_1)
				{
					rawInput |= 0x40;
				}
				if (0x40 < ptrControllerPacket->payload.neGcon.btn_2)
				{
					rawInput |= 0x80;
				}
				if (0x40 < ptrControllerPacket->payload.neGcon.trg_l)
				{
					rawInput |= 4;
				}
			}
			}

			// gamepadMapBtn maps RawInput to Buttons to support different controller types.
			for (buttonMap = &GAMEPAD_BUTTON_MAP[0]; (buttonMapRawInput = buttonMap->rawInput) != 0; buttonMap++)
			{
				if ((rawInput & buttonMapRawInput) != 0)
				{
					mappedButtons |= buttonMap->buttons;
				}
			}

			// record buttons held this frame
			pad->buttonsHeldCurrFrame = mappedButtons;

			// Saturate the idle counter while no buttons are held.
			if (mappedButtons != 0)
			{
				pad->framesSinceLastInput = 0;
			}
			else
			{
				if (pad->framesSinceLastInput < 65000)
				{
					pad->framesSinceLastInput++;
				}
			}

			heldAny |= mappedButtons;
		}
		else
		{
			pad->buttonsHeldCurrFrame = 0;
			pad->buttonsHeldPrevFrame = 0;
		}
	}

	return heldAny;
}


static inline s32 GAMEPAD_ProcessSticks_IsAnalogLike(u8 controllerData)
{
	switch (controllerData)
	{
	case ((PAD_ID_ANALOG_STICK << 4) | 3):
	case ((PAD_ID_ANALOG << 4) | 3):
	case ((PAD_ID_NEGCON << 4) | 3):
	case ((PAD_ID_JOGCON << 4) | 3):
	{
		return 1;
	}
	default:
	{
		return 0;
	}
	}
}

static inline s16 GAMEPAD_ProcessSticks_StepTowardZero(s16 value)
{
	s32 step = value;

	if (step > 0)
	{
		step -= 0xff;
		if (step < 0)
		{
			step = 0;
		}
	}
	else
	{
		step += 0xff;
		if (step > 0)
		{
			step = 0;
		}
	}

	return step;
}

static inline s16 GAMEPAD_ProcessSticks_StepTowardMax(s16 value)
{
	s32 step = value;

	if (step >= 0x100)
	{
		step -= 0xff;
		if (step < 0xff)
		{
			step = 0xff;
		}
	}
	else
	{
		step += 0xff;
		if (step >= 0x100)
		{
			step = 0xff;
		}
	}

	return step;
}

static inline s16 GAMEPAD_ProcessSticks_StepTowardCenter(s16 value)
{
	s32 step = value;

	if (step >= 0x81)
	{
		step -= 0xff;
		if (step < 0x80)
		{
			step = 0x80;
		}
	}
	else
	{
		step += 0xff;
		if (step >= 0x81)
		{
			step = 0x80;
		}
	}

	return step;
}

static inline void GAMEPAD_ProcessSticks_ResetRaw(struct GamepadBuffer *pad)
{
	pad->inputStickLX = 0x80;
	pad->inputStickLY = 0x80;
	pad->stickRX = 0x80;
	pad->stickRY = 0x80;
}

static inline void GAMEPAD_ProcessSticks_ResetRawAndResolved(struct GamepadBuffer *pad)
{
	GAMEPAD_ProcessSticks_ResetRaw(pad);
	pad->stickLX = 0x80;
	pad->stickLY = 0x80;
}

static inline void GAMEPAD_ProcessSticks_CheckIdleAxis(struct GamepadBuffer *pad, s16 axis)
{
	s32 delta = axis - 0x80;

	if (delta < 0)
	{
		delta = -delta;
	}

	if (delta >= 0x31)
	{
		pad->framesSinceLastInput = 0;
	}
}

void GAMEPAD_ProcessSticks(struct GamepadSystem *gGamepads)
{
	s32 controllerData;
	s32 useRaw;
	s32 wheelForce;
	// NOTE(aalhendi): Preserve retail's wheel arithmetic and hoisted neutral value.
	// Register bindings constrain GCC 2.8.1 only; native keeps ordinary C locals.
	register s32 wheelDelta CTR_PSX_REGISTER("v0");
	s32 inputValue;
	s16 axisValue;

	struct GamepadBuffer *pad;
	struct ControllerPacket *packet;
	struct RacingWheelData *rwd;
	s32 i = 0;
	register s32 center CTR_PSX_REGISTER("a2") = 0x80;
	CTR_PSX_OBSERVE_VALUE(center);

	for (rwd = &GAMEPAD_WHEEL_DATA[0]; i < 8; i++)
	{
		pad = &gGamepads->gamepad[i];
		packet = pad->ptrControllerPacket;
		pad->rwd = NULL;

		if (packet != NULL)
		{
			if (packet->plugged == PLUGGED)
			{
				controllerData = packet->controllerData;

				switch (controllerData)
				{
				case ((PAD_ID_JOGCON << 4) | 3):
				{
					if (i < 4)
					{
						pad->rwd = rwd;
					}

					inputValue = pad->ptrControllerPacket->payload.jogcon.jog_rot;

					if (inputValue < 0)
					{
						wheelDelta = -inputValue - 10;
						wheelForce = (wheelDelta - rwd->range) * 8;
						if (wheelForce < 0)
						{
							wheelForce = 0;
						}
						if (wheelForce > 0xff)
						{
							wheelForce = 0xff;
						}

						if (inputValue < -0x80)
						{
							inputValue = -0x80;
						}
						inputValue += 0x80;
					}
					else
					{
						wheelDelta = inputValue - 10;
						wheelForce = (wheelDelta - rwd->range) * 8;
						if (wheelForce < 0)
						{
							wheelForce = 0;
						}
						if (wheelForce > 0xff)
						{
							wheelForce = 0xff;
						}

						if (0x7f < inputValue)
						{
							inputValue = 0x7f;
						}
						inputValue += 0x80;
					}
					pad->jogWheelLimitForce = (u8)wheelForce;
					pad->inputStickLX = inputValue;
					pad->inputStickLY = 0x80;
					pad->stickRX = 0x80;
					pad->stickRY = 0x80;

					break;
				}
				case ((PAD_ID_NEGCON << 4) | 3):
				{
					if (i < 4)
					{
						pad->rwd = rwd;
					}

					inputValue = pad->ptrControllerPacket->payload.neGcon.twist;
					pad->inputStickLX = inputValue;
					pad->inputStickLY = 0x80;
					pad->stickRX = 0x80;
					pad->stickRY = 0x80;

					break;
				}
				case ((PAD_ID_ANALOG_STICK << 4) | 3):
				case ((PAD_ID_ANALOG << 4) | 3):
				{
					pad->inputStickLX = pad->ptrControllerPacket->payload.analog.leftX;

					inputValue = pad->ptrControllerPacket->payload.analog.leftY;
					// A lone 0xff LY sample reuses the preceding value.
					if (inputValue == 0xff && pad->previousAnalogLeftY != 0xff)
					{
						pad->inputStickLY = pad->previousAnalogLeftY;
					}
					else
					{
						pad->inputStickLY = inputValue;
					}

					pad->previousAnalogLeftY = inputValue;
					pad->stickRX = pad->ptrControllerPacket->payload.analog.rightX;
					pad->stickRY = pad->ptrControllerPacket->payload.analog.rightY;

					break;
				}
				default:
				{
					GAMEPAD_ProcessSticks_ResetRaw(pad);
				}
				}
			}

			GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->inputStickLX);
			GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->inputStickLY);
			GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->stickRX);
			GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->stickRY);

			controllerData = pad->ptrControllerPacket->controllerData;
			useRaw = GAMEPAD_ProcessSticks_IsAnalogLike(controllerData);

			if (pad->buttonsHeldCurrFrame & BTN_LEFT)
			{
				axisValue = GAMEPAD_ProcessSticks_StepTowardZero(pad->stickLX);
			}
			else if (pad->buttonsHeldCurrFrame & BTN_RIGHT)
			{
				axisValue = GAMEPAD_ProcessSticks_StepTowardMax(pad->stickLX);
			}
			else if (useRaw)
			{
				pad->stickLX = pad->inputStickLX;
				goto resolveY;
			}
			else
			{
				axisValue = GAMEPAD_ProcessSticks_StepTowardCenter(pad->stickLX);
			}
			pad->stickLX = axisValue;
		resolveY:;
			if (pad->buttonsHeldCurrFrame & BTN_UP)
			{
				axisValue = GAMEPAD_ProcessSticks_StepTowardZero(pad->stickLY);
			}
			else if (pad->buttonsHeldCurrFrame & BTN_DOWN)
			{
				axisValue = GAMEPAD_ProcessSticks_StepTowardMax(pad->stickLY);
			}
			else if (useRaw)
			{
				pad->stickLY = pad->inputStickLY;
				goto nextPad;
			}
			else
			{
				axisValue = GAMEPAD_ProcessSticks_StepTowardCenter(pad->stickLY);
			}
			pad->stickLY = axisValue;
		nextPad:;
		}
		else
		{
			GAMEPAD_ProcessSticks_ResetRawAndResolved(pad);
		}
		// NOTE(aalhendi): Only four pads can connect. Native must stay within
		// the wheel table while the remaining, empty gamepad buffers are cleared.
#ifdef CTR_NATIVE
		if (i < 3)
#endif
		{
			rwd = (struct RacingWheelData *)((char *)rwd + sizeof(struct RacingWheelData));
		}
	}
}


// Writes all gamepad variables
// for Tap and Release, based on Hold,
// also maps joysticks onto buttons

s32 GAMEPAD_ProcessTapRelease(struct GamepadSystem *gGamepads)
{
	s32 i = 0;
	u32 heldAny = 0;
	s32 numConnected = gGamepads->numGamepadsConnected;
	s32 analogButtonsEnabled;
	struct GamepadBuffer *pad;
	struct ControllerPacket *ptrControllerPacket;

	if (numConnected > 0)
	{
		analogButtonsEnabled = sdata->padActuatorAlignment[6];


		do
		{
			pad = &gGamepads->gamepad[i];
			ptrControllerPacket = pad->ptrControllerPacket;

			if (ptrControllerPacket != NULL)
			{
				if (analogButtonsEnabled != 0)
				{
					if (pad->stickLX < 0x20)
					{
						pad->buttonsHeldCurrFrame |= BTN_LEFT;
					}

					else if (0xe0 < pad->stickLX)
					{
						pad->buttonsHeldCurrFrame |= BTN_RIGHT;
					}

					if (pad->stickLY < 0x20)
					{
						pad->buttonsHeldCurrFrame |= BTN_UP;
					}

					else if (0xe0 < pad->stickLY)
					{
						pad->buttonsHeldCurrFrame |= BTN_DOWN;
					}
				}

				heldAny |= pad->buttonsHeldCurrFrame;
				// NOTE(aalhendi): Retail reloads the held mask before deriving tap/release.
				CTR_PSX_DEPEND_MEMORY(&pad->buttonsHeldCurrFrame, heldAny);

				// tapped
				pad->buttonsTapped = ~pad->buttonsHeldPrevFrame & pad->buttonsHeldCurrFrame;

				// released
				pad->buttonsReleased = pad->buttonsHeldPrevFrame & ~pad->buttonsHeldCurrFrame;
			}
			else
			{
				pad->buttonsTapped = 0;
				pad->buttonsReleased = 0;
			}
			++i;
		} while (i < gGamepads->numGamepadsConnected);
	}

	return heldAny;
}


void GAMEPAD_ProcessMotors(struct GamepadSystem *gGS)
{
	s32 i;
	s32 totalPower;
	s32 remaining;
	s32 strength;
	s32 desired;
	s32 jogPower;
	s32 numPads;
	s32 skipIndex;
	struct GamepadBuffer *pad;

	for (i = 0; i < gGS->numGamepadsConnected; ++i)
	{
		pad = &gGS->gamepad[i];
		if (!(GAME_TRACKER->gameMode1 & PAUSE_ALL) && !GAME_TRACKER->boolDemoMode && pad->ptrControllerPacket != 0 && !RaceFlag_IsTransitioning())
		{
			if (pad->ptrControllerPacket->controllerData == ((PAD_ID_JOGCON << 4) | 3))
			{
				if (pad->jogCenteringFrames != 0)
				{
					pad->motorDesired[0] = 0x40;
				}
				else if ((remaining = pad->jogEffectTimeMS) != 0)
				{
					pad->motorDesired[0] = pad->jogEffectCommand;
					remaining -= GAME_TRACKER->elapsedTimeMS;
					if (remaining <= 0)
					{
						pad->jogEffectTimeMS = 0;
						pad->jogEffectCommand = 0;
					}
					else
					{
						pad->jogEffectTimeMS = remaining;
					}
				}
				else
				{
					if (pad->jogRequestedForce > pad->jogWheelLimitForce || pad->jogForceOverrideTimeMS != 0)
					{
						strength = pad->jogRequestedForce;
						if ((GAME_TRACKER->timer & strength) & 15)
						{
							strength -= 16;
							if (strength < 0)
							{
								strength = 0;
							}
						}
						jogPower = strength >> 4;
					}
					else
					{
						jogPower = pad->jogWheelLimitForce >> 4;
					}
					pad->motorDesired[0] = jogPower | 0x30;
				}
				// NOTE(aalhendi): Retail clears this timer even when time remains.
				if (pad->jogForceOverrideTimeMS != 0)
				{
					remaining = pad->jogForceOverrideTimeMS - GAME_TRACKER->elapsedTimeMS;
					if (remaining != 0)
					{
						remaining = 0;
					}
					pad->jogForceOverrideTimeMS = remaining;
				}
				pad->motorDesired[1] = 0;
			}
			else
			{
				if (pad->shockFrameFreq != 0)
				{
					desired = 0;
					if ((GAME_TRACKER->timer & pad->shockValFreq) == 0)
					{
						desired = -1;
					}
					pad->motorDesired[0] = desired;
				}
				else
				{
					pad->motorDesired[0] = 0;
				}
				if (pad->shockFrameForce1 != 0)
				{
					pad->motorDesired[1] = pad->shockValForce1;
				}
				else
				{
					pad->shockValForce1 = 0;
					if (pad->shockFrameForce2 != 0)
					{
						pad->motorDesired[1] = pad->shockValForce2;
					}
					else
					{
						pad->shockValForce2 = 0;
						pad->motorDesired[1] = 0;
					}
				}
			}
			if (pad->shockFrameFreq != 0)
			{
				--pad->shockFrameFreq;
			}
			if (pad->shockFrameForce1 != 0)
			{
				--pad->shockFrameForce1;
			}
			if (pad->shockFrameForce2 != 0)
			{
				--pad->shockFrameForce2;
			}
		}
		else
		{
			if (pad->ptrControllerPacket != 0 && pad->ptrControllerPacket->controllerData == ((PAD_ID_JOGCON << 4) | 3) && pad->jogCenteringFrames != 0)
			{
				pad->motorDesired[0] = 0x40;
			}
			else
			{
				pad->motorDesired[0] = 0;
			}
			pad->motorDesired[1] = 0;
			pad->shockFrameFreq = 0;
			pad->shockFrameForce1 = 0;
			pad->shockFrameForce2 = 0;
			pad->jogEffectTimeMS = 0;
			pad->jogEffectCommand = 0;
		}
		if (pad->jogCenteringFrames != 0)
		{
			--pad->jogCenteringFrames;
		}
	}

	totalPower = 0;
	for (i = 0; i < gGS->numGamepadsConnected; ++i)
	{
		pad = &gGS->gamepad[i];
		if (pad->motorDesired[0] != 0)
		{
			totalPower += pad->motorPower[0];
		}
		if (pad->motorDesired[1] != 0)
		{
			totalPower += pad->motorPower[1];
		}
	}
	if (totalPower > 60)
	{
		numPads = gGS->numGamepadsConnected;
		skipIndex = (u32)GAME_TRACKER->timer % numPads;
		for (i = skipIndex; i < numPads + skipIndex; ++i)
		{
			if (totalPower <= 60)
			{
				break;
			}
			pad = &gGS->gamepad[i];
			if (i >= numPads)
			{
				pad = &gGS->gamepad[i - numPads];
			}
			if (pad->motorDesired[1] != 0)
			{
				pad->motorDesired[1] = 0;
				totalPower -= pad->motorPower[1];
			}
		}
		for (i = skipIndex; i < numPads + skipIndex; ++i)
		{
			if (totalPower <= 60)
			{
				break;
			}
			pad = &gGS->gamepad[i];
			if (i >= numPads)
			{
				pad = &gGS->gamepad[i - numPads];
			}
			if (pad->motorDesired[0] != 0)
			{
				pad->motorDesired[0] = 0;
				totalPower -= pad->motorPower[0];
			}
		}
	}
	for (i = 0; i < gGS->numGamepadsConnected; ++i)
	{
		pad = &gGS->gamepad[i];
		pad->motorSubmit[0] = pad->motorDesired[0];
		pad->motorSubmit[1] = pad->motorDesired[1];
	}
}


#ifdef CTR_INTERNAL
// NOTE(aalhendi): Internal GDB input shim for runtime probes.
// gCtrDebugPadTap is one-frame input; gCtrDebugPadHeld stays pressed until reset.
// Example route, main menu -> Time Trial:
//   # Select Adventure/Time Trial row from main menu, then confirm each menu.
//   set D230.menuMainMenu.rowSelected=1
//   set gCtrDebugPadTap=0x10
//   continue
//   set gCtrDebugPadTap=0x10
//   continue
//   set gCtrDebugPadTap=0x10
//   continue
volatile s32 gCtrDebugPadHeld = 0;
volatile s32 gCtrDebugPadTap = 0;
#endif

/// @brief Main gamepad processing function. Polls every connected gamepad and generates global state flags.
/// @param gGamepads - gamepad input system
s32 GAMEPAD_ProcessAnyoneVars(struct GamepadSystem *gGamepads)
{
	s32 heldAny;
	s32 i;
	struct GamepadBuffer *pad;

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native hotplug/replay can replace the pad bus layout before
	// input is decoded. Refresh packet views now, but preserve the connection flags
	// so MainFrame can still acknowledge and report disconnects later.
	{
		u32 previousFlags = gGamepads->gamepadsConnectedByFlag;
		GAMEPAD_GetNumConnected(gGamepads);
		gGamepads->gamepadsConnectedByFlag = previousFlags;
		// Tap/release visits only the connected prefix; clear disconnected tail edges.
		for (i = gGamepads->numGamepadsConnected; i < 8; ++i)
		{
			pad = &gGamepads->gamepad[i];
			pad->buttonsTapped = 0;
			pad->buttonsReleased = 0;
		}
	}
#endif

	// process gamepads
	heldAny = GAMEPAD_ProcessHold(gGamepads);
	GAMEPAD_ProcessSticks(gGamepads);
	heldAny |= GAMEPAD_ProcessTapRelease(gGamepads);
	GAMEPAD_ProcessMotors(gGamepads);

	// These are used to see if any button is pressed by anyone
	// during this frame. Reset them all to zero
	gGamepads->anyoneHeldCurr = 0;
	gGamepads->anyoneTapped = 0;
	gGamepads->anyoneReleased = 0;
	gGamepads->anyoneHeldPrev = 0;

	// foreach connected gamepad
	for (i = 0; i < gGamepads->numGamepadsConnected; i++)
	{
		// get gamepad
		pad = &gGamepads->gamepad[i];

		// update global system flag
		gGamepads->anyoneHeldCurr |= pad->buttonsHeldCurrFrame;
		gGamepads->anyoneTapped |= pad->buttonsTapped;
		gGamepads->anyoneReleased |= pad->buttonsReleased;
		gGamepads->anyoneHeldPrev |= pad->buttonsHeldPrevFrame;
	}

#ifdef CTR_INTERNAL
	if (gGamepads->numGamepadsConnected > 0)
	{
		pad = &gGamepads->gamepad[0];

		if (gCtrDebugPadHeld != 0)
		{
			pad->buttonsHeldCurrFrame |= gCtrDebugPadHeld;
			gGamepads->anyoneHeldCurr |= gCtrDebugPadHeld;
		}

		if (gCtrDebugPadTap != 0)
		{
			pad->buttonsTapped |= gCtrDebugPadTap;
			gGamepads->anyoneTapped |= gCtrDebugPadTap;
			gGamepads->anyoneHeldCurr |= gCtrDebugPadTap;
			gCtrDebugPadTap = 0;
		}
	}
#endif

	return heldAny;
}


void GAMEPAD_JogCon1(struct Driver *d, s32 val, u16 timeMS)
{
	struct GamepadBuffer *gb;
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
	{
		return;
	}

	gb = &GAMEPADS->gamepad[d->driverID];

	if ((gb->jogEffectCommand & 0xf) > (val & 0xf))
	{
		return;
	}

	gb->jogEffectCommand = val;
	gb->jogEffectTimeMS = timeMS;
}


void GAMEPAD_JogCon2(struct Driver *d, u8 val, s16 timeMS)
{
	struct GamepadBuffer *gb;
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
	{
		return;
	}

	gb = &GAMEPADS->gamepad[d->driverID];

	gb->jogRequestedForce = val;
	gb->jogForceOverrideTimeMS = timeMS;
}


void GAMEPAD_ShockFreq(struct Driver *d, s32 frame, s32 val)
{
	struct GamepadBuffer *gb;
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
	{
		return;
	}

	// 0 for enabled,
	// 1 for disabled
	if ((GAME_TRACKER->gameMode1 & (P1_VIBRATE << d->driverID)) != 0)
	{
		return;
	}

	gb = &GAMEPADS->gamepad[d->driverID];

	if (gb->framesSinceLastInput >= 0x385)
	{
		return;
	}

	if (gb->shockFrameFreq >= frame)
	{
		return;
	}

	gb->shockFrameFreq = frame;
	gb->shockValFreq = val;
}


void GAMEPAD_ShockForce1(struct Driver *d, s32 frame, s32 val)
{
	struct GamepadBuffer *gb;
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
	{
		return;
	}

	// 0 for enabled,
	// 1 for disabled
	if ((GAME_TRACKER->gameMode1 & (P1_VIBRATE << d->driverID)) != 0)
	{
		return;
	}

	gb = &GAMEPADS->gamepad[d->driverID];

	if (gb->framesSinceLastInput >= 0x385)
	{
		return;
	}

	if (gb->shockValForce1 >= val)
	{
		return;
	}

	gb->shockFrameForce1 = frame;
	gb->shockValForce1 = (u8)val;
}


void GAMEPAD_ShockForce2(struct Driver *d, s32 frame, s32 val)
{
	struct GamepadBuffer *gb;
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
	{
		return;
	}

	// 0 for enabled,
	// 1 for disabled
	if ((GAME_TRACKER->gameMode1 & (P1_VIBRATE << d->driverID)) != 0)
	{
		return;
	}

	gb = &GAMEPADS->gamepad[d->driverID];

	if (gb->framesSinceLastInput >= 0x385)
	{
		return;
	}

	if (gb->shockValForce2 >= val)
	{
		return;
	}

	gb->shockFrameForce2 = frame;
	gb->shockValForce2 = (u8)val;
}
