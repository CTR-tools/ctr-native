#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800251ac-0x80025208.
void GAMEPAD_Init(struct GamepadSystem *gGamepads)
{
	int i;
	struct GamepadBuffer *pad;

	PadInitMtap((u8 *)&gGamepads->slotBuffer[0], (u8 *)&gGamepads->slotBuffer[1]);
	PadStartCom();

	for (i = 0; i < 8; i++)
	{
		pad = &gGamepads->gamepad[i];

		// no analog sticks detected
		pad->gamepadType = 0;
		pad->unk44 = 0;
	}

	gGamepads->gamepadsConnectedByFlag = 0xffffffff;

	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80025208-0x800252a0.
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


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800252a0-0x80025410.
void GAMEPAD_ProcessState(struct GamepadBuffer *pad, int padState, s16 id)
{
	char uVar1;
	int iVar2;
	int iVar3;

	switch (padState)
	{
	case 1:
		if (pad->gamepadType != 0)
		{
			pad->gamepadType = 1;
		}
		break;
	case 2:
		pad->motorPower[0] = 0;
		pad->motorPower[1] = 0;
		break;
	case 6:
		if (pad->gamepadType == 0)
		{
			iVar2 = PadSetMainMode(id, 1, 0);
			if (iVar2 != 0)
			{
				pad->gamepadType = 1;
			}
		}
		else if (pad->gamepadType == 1)
		{
			// get number of motors on pad
			iVar2 = PadInfoAct(id, 0xffffffff, 0);
			if (iVar2 > 2)
			{
				iVar2 = 2;
			}

			// set to zero by default
			*(s16 *)&pad->motorPower[0] = 0;

			// loop through motors
			for (iVar3 = 0; iVar3 < iVar2; iVar3++)
			{
				uVar1 = PadInfoAct(id, iVar3, 4);
				pad->motorPower[iVar3] = uVar1;
			}

			PadSetAct(id, &pad->motorSubmit[0], sizeof(pad->motorSubmit));

			if (PadSetActAlign(id, &sdata->unkPadSetActAlign[0]) != 0)
			{
				pad->gamepadType = 2;
			}
		}
		break;
	}
	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80025410-0x800255b4.
void GAMEPAD_PollVsync(struct GamepadSystem *gGamepads)
{
	u32 uVar2;
	u32 uVar4;
	struct GamepadBuffer *pad;
	int port;
	int numPorts;
	int maxPadsPerPort;

	// 2 players, no multitap
	numPorts = 2;
	maxPadsPerPort = 1;

	// If there is a multitap present
	if ((gGamepads->slotBuffer[0].plugged == PLUGGED) && (gGamepads->slotBuffer[0].controllerData == (PAD_ID_MULTITAP << 4)))
	{
		// 4 players, with multitap
		numPorts = 1;
		maxPadsPerPort = 4;
	}


	pad = &gGamepads->gamepad[0];

	// loop through all gamepad ports
	// that gameplay cares about. Either
	// 1 or 2, main ports on console
	for (port = 0; port < numPorts; port++)
	{
		// loop through all gamepads that can connect
		// to this gamepad port. 1 for no mtap, 4 for mtap
		for (char i = 0; i < maxPadsPerPort; i++)
		{
			b32 unpluggedPort = ((
			                         // multitap here, and unplugged
			                         (gGamepads->slotBuffer[port].controllerData == (PAD_ID_MULTITAP << 4)) &&
			                         (gGamepads->slotBuffer[port].controllers[i].plugged != PLUGGED)) ||

			                     // controller unplugged
			                     (gGamepads->slotBuffer[port].plugged != PLUGGED));


			if (unpluggedPort)
			{
				// no analog sticks found
				pad->gamepadType = 0;
			}

			else
			{
				uVar4 = (port << 4) | i;

				// according to libref
				// 0 - PadStateDisCon
				// 1 - PadStateFindPad
				// and many more...
				uVar2 = PadGetState(uVar4);

				GAMEPAD_ProcessState(pad, uVar2, uVar4);
			}

			// increment gamepad counter
			pad++;
		}
	}

	// if there are less than 8 gamepads connected,
	// write to buffers of all Unplugged gamepads
	while (pad < &gGamepads->gamepad[8])
	{
		pad->gamepadType = 0;
		pad++;
	}
}


// NOTE(aalhendi): PSX path ASM-verified NTSC-U 926 0x800255b4-0x80025718.
int GAMEPAD_GetNumConnected(struct GamepadSystem *gGamepads)
{
	int padIndex;
	int bitwiseConnected;

	int numSlots;
	int numPortsPerSlot;

	struct MultitapPacket *ptrControllerPacket;
	struct GamepadBuffer *padCurr;

	// 2 players, no multitap
	numSlots = 2;
	numPortsPerSlot = 1;

	if (
	    // multitap detected
	    (gGamepads->slotBuffer[0].plugged == PLUGGED) && (gGamepads->slotBuffer[0].controllerData == (PAD_ID_MULTITAP << 4)))
	{
		// 4 players, with multitap
		numSlots = 1;
		numPortsPerSlot = 4;
	}

	padIndex = 0;
	bitwiseConnected = 0;
	gGamepads->numGamepadsConnected = 0;
	padCurr = &gGamepads->gamepad[0];

	// TODO: Rename to match PollVsync
	// should be ports and padsPerPort

	for (int Slot = 0; Slot < numSlots; Slot++)
	{
		for (int Port = 0; Port < numPortsPerSlot; Port++)
		{
			ptrControllerPacket = &gGamepads->slotBuffer[Slot];
			if (ptrControllerPacket->plugged == PLUGGED)
			{
				// if multitap plugged in
				if (ptrControllerPacket->controllerData == (PAD_ID_MULTITAP << 4))
				{
					ptrControllerPacket = (struct MultitapPacket *)&ptrControllerPacket->controllers[Port];
				}

				if (ptrControllerPacket->plugged == PLUGGED)
				{
					bitwiseConnected |= 1 << (Slot * 4 + Port);
					gGamepads->numGamepadsConnected = padIndex + 1;

					padCurr->ptrControllerPacket = (struct ControllerPacket *)ptrControllerPacket;
					padCurr->gamepadID = Slot * 0x10 + Port;
				}
			}

			padIndex++;
			padCurr++;
		}
	}

	while (padCurr < &gGamepads->gamepad[8])
	{
		// pad is now unplugged
		padCurr->ptrControllerPacket = 0;
		padCurr++;
	}

	// this name is way too long
	int *ptrToSet = &gGamepads->gamepadsConnectedByFlag;
	int oldVal = *ptrToSet;
	*ptrToSet = bitwiseConnected;

	if (oldVal == -1)
	{
		return 0;
	}
	if (oldVal == bitwiseConnected)
	{
		return 0;
	}

	// return change
	return (u32)((bitwiseConnected ^ oldVal) & oldVal) != 0;
}

// determine which buttons are held this frame,
// store a backup of "currFrame" into "lastFrame"
// param1 is pointer to gamepadSystem

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80025718-0x80025854.
int GAMEPAD_ProcessHold(struct GamepadSystem *gGamepads)
{
	char j;
	char *btnMapPtr;
	u32 *puVar2;
	u16 uVar4;
	u32 uVar5;
	u32 heldAny = 0;

	struct GamepadBuffer *pad;
	struct ControllerPacket *ptrControllerPacket;


	// loop through all 8 gamepadBuffers
	for (pad = &gGamepads->gamepad[0]; pad < &gGamepads->gamepad[8]; pad++)
	{
		pad->buttonsHeldPrevFrame = pad->buttonsHeldCurrFrame;

		ptrControllerPacket = pad->ptrControllerPacket;

		// if pointer is invalid
		if (ptrControllerPacket == NULL)
		{
			// erase buttons held this frame and prev
			pad->buttonsHeldPrevFrame = 0;
			pad->buttonsHeldCurrFrame = 0;
		}

		// must be zero to confirm connection
		else if (ptrControllerPacket->plugged == PLUGGED)
		{
			// endian flip
			uVar4 = (ptrControllerPacket->controllerInput1 << 8) | ptrControllerPacket->controllerInput2;

			uVar4 = uVar4 ^ 0xffff;
			uVar5 = 0;

			// If this is madcatz racing wheel
			if (ptrControllerPacket->controllerData == ((PAD_ID_NEGCON << 4) | 3))
			{
				if (0x40 < ptrControllerPacket->neGcon.btn_1)
				{
					uVar4 |= 0x40;
				}
				if (0x40 < ptrControllerPacket->neGcon.btn_2)
				{
					uVar4 |= 0x80;
				}
				if (0x40 < ptrControllerPacket->neGcon.trg_l)
				{
					uVar4 |= 4;
				}
			}

			// If this is not madcatz racing wheel
			else
			{
				// If this is ANAJ
				// could be different from NPC-105
				if (ptrControllerPacket->controllerData == ((PAD_ID_ANALOG_STICK << 4) | 3))
				{
					uVar4 = uVar4 << 0x10;
				}
			}

			// gamepadMapBtn to map RawInput enum
			// to Buttons enum, to support different
			// types of controllers
			for (btnMapPtr = &data.gamepadMapBtn[0].input[0]; *(int *)&btnMapPtr[0] != 0; btnMapPtr += 8)
			{
				if ((uVar4 & *(int *)&btnMapPtr[0]) != 0)
				{
					uVar5 |= *(int *)&btnMapPtr[4];
				}
			}

			// record buttons held this frame
			pad->buttonsHeldCurrFrame = uVar5;
			heldAny |= uVar5;

			// if nothing was held
			if (uVar5 == 0)
			{
				if (pad->framesSinceLastInput < 65000)
				{
					pad->framesSinceLastInput++;
				}
			}

			// if buttons were pressed
			else
			{
				// reset number of frames since last input
				pad->framesSinceLastInput = 0;
			}
		}
	}

	return heldAny;
}


static int GAMEPAD_ProcessSticks_IsAnalogLike(u8 controllerData)
{
	return controllerData == ((PAD_ID_ANALOG_STICK << 4) | 3) || controllerData == ((PAD_ID_ANALOG << 4) | 3) || controllerData == ((PAD_ID_NEGCON << 4) | 3) ||
	       controllerData == ((PAD_ID_JOGCON << 4) | 3);
}

static s16 GAMEPAD_ProcessSticks_StepTowardZero(s16 value)
{
	int step = value;

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

static s16 GAMEPAD_ProcessSticks_StepTowardMax(s16 value)
{
	int step = value;

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

static s16 GAMEPAD_ProcessSticks_StepTowardCenter(s16 value)
{
	int step = value;

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

static s16 GAMEPAD_ProcessSticks_ResolveAxis(s16 axis, s16 rawAxis, int held, int negativeButton, int positiveButton, int useRaw)
{
	if ((held & negativeButton) != 0)
	{
		return GAMEPAD_ProcessSticks_StepTowardZero(axis);
	}

	if ((held & positiveButton) != 0)
	{
		return GAMEPAD_ProcessSticks_StepTowardMax(axis);
	}

	if (useRaw)
	{
		return rawAxis;
	}

	return GAMEPAD_ProcessSticks_StepTowardCenter(axis);
}

static void GAMEPAD_ProcessSticks_ResetRaw(struct GamepadBuffer *pad)
{
	pad->stickLX_dontUse1 = 0x80;
	pad->stickLY_dontUse1 = 0x80;
	pad->stickRX = 0x80;
	pad->stickRY = 0x80;
}

static void GAMEPAD_ProcessSticks_ResetRawAndResolved(struct GamepadBuffer *pad)
{
	GAMEPAD_ProcessSticks_ResetRaw(pad);
	pad->stickLX = 0x80;
	pad->stickLY = 0x80;
}

static void GAMEPAD_ProcessSticks_CheckIdleAxis(struct GamepadBuffer *pad, s16 axis)
{
	int delta = axis - 0x80;

	if (delta < 0)
	{
		delta = -delta;
	}

	if (delta >= 0x31)
	{
		pad->framesSinceLastInput = 0;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80025854-0x80025d10.
void GAMEPAD_ProcessSticks(struct GamepadSystem *gGamepads)
{
	u8 controllerData;
	int iVar4;
	int iVar7;
	s16 sVar8;

	struct GamepadBuffer *pad;
	struct ControllerPacket *packet;
	struct RacingWheelData *rwd = &data.rwd[0];
	int i;

	for (pad = &gGamepads->gamepad[0], i = 0; i < 8; pad++, i++, rwd = (struct RacingWheelData *)((char *)rwd + sizeof(struct RacingWheelData)))
	{
		packet = pad->ptrControllerPacket;
		pad->rwd = NULL;

		if (packet == NULL)
		{
			GAMEPAD_ProcessSticks_ResetRawAndResolved(pad);
			continue;
		}

		if (packet->plugged == PLUGGED)
		{
			controllerData = packet->controllerData;

			if ((controllerData == ((PAD_ID_ANALOG_STICK << 4) | 3)) || (controllerData == ((PAD_ID_ANALOG << 4) | 3)))
			{
				pad->stickLX_dontUse1 = packet->analog.leftX;

				if (packet->analog.leftY == 0xff && pad->unk_1 != 0xff)
				{
					pad->stickLY_dontUse1 = pad->unk_1;
				}
				else
				{
					pad->stickLY_dontUse1 = packet->analog.leftY;
				}

				pad->unk_1 = packet->analog.leftY;
				pad->stickRX = packet->analog.rightX;
				pad->stickRY = packet->analog.rightY;
			}

			else if (controllerData == ((PAD_ID_NEGCON << 4) | 3))
			{
				if (i < 4)
				{
					pad->rwd = rwd;
				}

				pad->stickLX_dontUse1 = packet->neGcon.twist;
				pad->stickLY_dontUse1 = 0x80;
				pad->stickRX = 0x80;
				pad->stickRY = 0x80;
			}

			else if (controllerData == ((PAD_ID_JOGCON << 4) | 3))
			{
				if (i < 4)
				{
					pad->rwd = rwd;
				}

				sVar8 = packet->jogcon.jog_rot;
				iVar4 = (int)sVar8;

				if (iVar4 < 0)
				{
					iVar7 = ((-10 - iVar4) - rwd->deadZone) * 8;
					if (iVar7 < 0)
					{
						iVar7 = 0;
					}
					if (iVar7 > 0xff)
					{
						iVar7 = 0xff;
					}

					sVar8 += 0x80;
					if (iVar4 < -0x80)
					{
						sVar8 = -0x80;
						sVar8 += 0x80;
					}
				}
				else
				{
					iVar7 = ((iVar4 - 10) - rwd->deadZone) * 8;
					if (iVar7 < 0)
					{
						iVar7 = 0;
					}
					if (iVar7 > 0xff)
					{
						iVar7 = 0xff;
					}

					sVar8 += 0x80;
					if (0x7f < iVar4)
					{
						sVar8 = 0x7f;
						sVar8 += 0x80;
					}
				}
				pad->unk43 = (char)iVar7;
				pad->stickLX_dontUse1 = sVar8;
				pad->stickLY_dontUse1 = 0x80;
				pad->stickRX = 0x80;
				pad->stickRY = 0x80;
			}

			else
			{
				GAMEPAD_ProcessSticks_ResetRaw(pad);
			}
		}

		GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->stickLX_dontUse1);
		GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->stickLY_dontUse1);
		GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->stickRX);
		GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->stickRY);

		controllerData = packet->controllerData;
		iVar4 = GAMEPAD_ProcessSticks_IsAnalogLike(controllerData);
		iVar7 = pad->buttonsHeldCurrFrame;

		pad->stickLX = GAMEPAD_ProcessSticks_ResolveAxis(pad->stickLX, pad->stickLX_dontUse1, iVar7, BTN_LEFT, BTN_RIGHT, iVar4);
		pad->stickLY = GAMEPAD_ProcessSticks_ResolveAxis(pad->stickLY, pad->stickLY_dontUse1, iVar7, BTN_UP, BTN_DOWN, iVar4);
	}
}


// Writes all gamepad variables
// for Tap and Release, based on Hold,
// also maps joysticks onto buttons

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80025d10-0x80025e18.
int GAMEPAD_ProcessTapRelease(struct GamepadSystem *gGamepads)
{
	u32 heldAny = 0;
	int numConnected = gGamepads->numGamepadsConnected;

	if (numConnected <= 0)
	{
		return 0;
	}

	char cVar1;
	cVar1 = sdata->unkPadSetActAlign[6];

	struct GamepadBuffer *pad;
	struct ControllerPacket *ptrControllerPacket;


	for (pad = &gGamepads->gamepad[0]; pad < &gGamepads->gamepad[numConnected]; pad++)
	{
		ptrControllerPacket = pad->ptrControllerPacket;

		// if pointer is invalid
		if (ptrControllerPacket == NULL)
		{
			// erase tap and release
			pad->buttonsTapped = 0;
			pad->buttonsReleased = 0;
		}
		else
		{
			if (cVar1 != 0)
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

			// tapped
			pad->buttonsTapped = ~pad->buttonsHeldPrevFrame & pad->buttonsHeldCurrFrame;

			// released
			pad->buttonsReleased = pad->buttonsHeldPrevFrame & ~pad->buttonsHeldCurrFrame;
		}
	}

	return heldAny;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80025e18-0x800262d0.
void GAMEPAD_ProcessMotors(struct GamepadSystem *gGS)
{
	int totalPower = 0;
	struct GameTracker *gGT = sdata->gGT;

	for (int i = 0; i < gGS->numGamepadsConnected; i++)
	{
		struct GamepadBuffer *pad = &gGS->gamepad[i];
		struct ControllerPacket *packet = pad->ptrControllerPacket;

		if ((packet != 0) && (gGT->boolDemoMode == 0) && ((gGT->gameMode1 & 0xf) == 0) && (RaceFlag_IsTransitioning() == 0))
		{
			if (packet->controllerData == ((PAD_ID_JOGCON << 4) | 3))
			{
				u8 bVar1 = 0x40;

				if (pad->unk44 == 0)
				{
					if (pad->unk46 == 0)
					{
						if ((pad->unk43 < pad->unk42) || (bVar1 = pad->unk43 >> 4, pad->unk48 != 0))
						{
							char unk42 = pad->unk42;
							bVar1 = unk42 >> 4;

							if ((((gGT->timer & bVar1) & 0xf) != 0) && (bVar1 = (unk42 - 0x10) >> 4, (unk42 - 0x10) < 0))
							{
								bVar1 = 0;
							}
						}

						bVar1 = bVar1 | 0x30;
						pad->motorDesired[0] = bVar1;
					}

					else
					{
						pad->motorDesired[0] = pad->unk45;

						pad->unk46 -= gGT->elapsedTimeMS;
						if (pad->unk46 < 1)
						{
							pad->unk46 = 0;
							pad->unk45 = 0;
						}
					}
				}

				else
				{
					pad->motorDesired[0] = bVar1;
				}

				// === Naughty Dog Bug ===
				// original code subtracts elapsed time,
				// but then does "if != 0, set = 0"
				pad->unk48 = 0;

				pad->motorDesired[1] = 0;
			}

			else
			{
				// Frequency Control (on/off)
				pad->motorDesired[0] = 0;

				if (pad->shockFrameFreq != 0)
				{
					if ((gGT->timer & pad->shockValFreq) == 0)
					{
						pad->motorDesired[0] = 0xff;
					}
				}

				// Strength Control (percentage)
				pad->motorDesired[1] = 0;

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
					}
				}
			}

			if (pad->shockFrameFreq != 0)
			{
				pad->shockFrameFreq--;
			}
			if (pad->shockFrameForce1 != 0)
			{
				pad->shockFrameForce1--;
			}
			if (pad->shockFrameForce2 != 0)
			{
				pad->shockFrameForce2--;
			}
		}

		else
		{
			if ((packet == 0) || (packet->controllerData != ((PAD_ID_JOGCON << 4) | 3)) || (pad->unk44 == 0))
			{
				pad->motorDesired[0] = 0;
			}

			else
			{
				pad->motorDesired[0] = 0x40;
			}

			pad->motorDesired[1] = 0;

			pad->shockFrameFreq = 0;
			pad->shockFrameForce1 = 0;
			pad->shockFrameForce2 = 0;

			pad->unk45 = 0;
			pad->unk46 = 0;
		}

		if (pad->unk44 != 0)
		{
			pad->unk44--;
		}

		// Calculate Total Power
		// 1 standard DualShock uses 30 units of power
		if (pad->motorDesired[0] != 0)
		{
			totalPower += pad->motorPower[0];
		}
		if (pad->motorDesired[1] != 0)
		{
			totalPower += pad->motorPower[1];
		}
	}

	// PlayStation can not exceed 60 units
	// of electrical power, in port 1 or 2
	if (totalPower > 60)
	{
		int numPads = gGS->numGamepadsConnected;
		int skipIndex = gGT->timer % numPads;

		for (int i = skipIndex; i < skipIndex + numPads && totalPower > 60; i++)
		{
			struct GamepadBuffer *pad = &gGS->gamepad[i % numPads];

			if (pad->motorDesired[1] != 0)
			{
				pad->motorDesired[1] = 0;
				totalPower -= pad->motorPower[1];
			}
		}

		for (int i = skipIndex; i < skipIndex + numPads && totalPower > 60; i++)
		{
			struct GamepadBuffer *pad = &gGS->gamepad[i % numPads];

			if (pad->motorDesired[0] != 0)
			{
				pad->motorDesired[0] = 0;
				totalPower -= pad->motorPower[0];
			}
		}
	}

	for (int i = 0; i < gGS->numGamepadsConnected; i++)
	{
		struct GamepadBuffer *pad = &gGS->gamepad[i];

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
volatile int gCtrDebugPadHeld = 0;
volatile int gCtrDebugPadTap = 0;
#endif

/// @brief Main gamepad processing function. Polls every connected gamepad and generates global state flags.
/// @param gGamepads - gamepad input system
// NOTE(aalhendi): PSX path ASM-verified NTSC-U 926 0x800262d0-0x800263a0.
int GAMEPAD_ProcessAnyoneVars(struct GamepadSystem *gGamepads)
{
	int heldAny;
	struct GamepadBuffer *pad;

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
	for (int i = 0; i < gGamepads->numGamepadsConnected; i++)
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


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800263a0-0x800263fc
void GAMEPAD_JogCon1(struct Driver *d, char val, u16 timeMS)
{
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
	{
		return;
	}

	struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[d->driverID];

	if ((gb->unk45 & 0xf) > (val & 0xf))
	{
		return;
	}

	gb->unk45 = val;
	gb->unk46 = timeMS;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800263fc-0x80026440.
void GAMEPAD_JogCon2(struct Driver *d, char val, s16 timeMS)
{
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
	{
		return;
	}

	struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[d->driverID];

	gb->unk42 = val;
	gb->unk48 = timeMS;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80026440-0x800264c0.
void GAMEPAD_ShockFreq(struct Driver *d, int frame, int val)
{
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
	{
		return;
	}

	// 0 for enabled,
	// 1 for disabled
	if ((sdata->gGT->gameMode1 & (0x100 << d->driverID)) != 0)
	{
		return;
	}

	struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[d->driverID];

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


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800264c0-0x80026540.
void GAMEPAD_ShockForce1(struct Driver *d, int frame, int val)
{
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
	{
		return;
	}

	// 0 for enabled,
	// 1 for disabled
	if ((sdata->gGT->gameMode1 & (0x100 << d->driverID)) != 0)
	{
		return;
	}

	struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[d->driverID];

	if (gb->framesSinceLastInput >= 0x385)
	{
		return;
	}

	if (gb->shockValForce1 >= val)
	{
		return;
	}

	gb->shockFrameForce1 = frame;
	gb->shockValForce1 = val;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80026540-0x800265c0.
void GAMEPAD_ShockForce2(struct Driver *d, int frame, int val)
{
	if ((d->actionsFlagSet & ACTION_BOT) != 0)
	{
		return;
	}

	// 0 for enabled,
	// 1 for disabled
	if ((sdata->gGT->gameMode1 & (0x100 << d->driverID)) != 0)
	{
		return;
	}

	struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[d->driverID];

	if (gb->framesSinceLastInput >= 0x385)
	{
		return;
	}

	if (gb->shockValForce2 >= val)
	{
		return;
	}

	gb->shockFrameForce2 = frame;
	gb->shockValForce2 = val;
}
