/* One algorithm, specialized only for the entry's real visibility lists. */
#ifdef CTR_NATIVE
u32 waterTimer = (u32)timer, waterRemaining = (u32)numWaterVertices;
u32 waterColor, waterFirstOffset, waterSecondOffset, waterBits, waterBitsLeft;
struct WaterVert *waterVertex = waterVert;
const u32 *waterList0;
#if ANIMATE_WATER_LISTS >= 2
const u32 *waterList1;
#endif
#if ANIMATE_WATER_LISTS >= 3
const u32 *waterList2;
#endif
#if ANIMATE_WATER_LISTS >= 4
const u32 *waterList3;
#endif
#else
ANIMATE_SCRATCH_DECLARE();
#endif
u32 firstFrame, pending;
// NOTE(aalhendi): One internal setup-work lane, not an entry/provider ABI.
register u32 frameWork CTR_PSX_REGISTER("$9");
#ifdef CTR_NATIVE
// NOTE(aalhendi): Empty native lists may have null water/texture/visibility data.
if (numWaterVertices == 0)
	return;
#endif
ANIMATE_SCRATCH_SAVE();
waterList0 = (const u32 *)CTR_PSX_INCOMING_POINTER(visOVertList0);
#if ANIMATE_WATER_LISTS >= 2
waterList1 = (const u32 *)CTR_PSX_INCOMING_POINTER(visOVertList1);
#endif
#if ANIMATE_WATER_LISTS >= 3
waterList2 = (const u32 *)CTR_PSX_INCOMING_POINTER(visOVertList2);
#endif
#if ANIMATE_WATER_LISTS >= 4
waterList3 = (const u32 *)CTR_PSX_INCOMING_POINTER(visOVertList3);
#endif
#ifndef CTR_NATIVE
waterVertexAddress -= sizeof(struct WaterVert);
#endif
pending = CTR_PSX_UMOD_HI(firstFrame = waterTimer >> 3, 28u);
#ifdef CTR_NATIVE
waterColor = CTR_ReadU16AlignedLE(waterEnvMap);
#else
waterColor = CTR_ReadU16AlignedLE((const void *)waterColor);
#endif
firstFrame = CTR_PSX_READ_HI(pending);
frameWork = waterTimer & 7u;
frameWork <<= 9;
CTR_GteWriteData(frameWork, 8);
frameWork = firstFrame + 1u;
pending = CTR_PSX_UMOD_HI(frameWork, 28u);
waterBits = *waterList0++;
#if ANIMATE_WATER_LISTS >= 2
{
	u32 list1Bits = *waterList1++;
	waterBits |= list1Bits;
}
#endif
#if ANIMATE_WATER_LISTS >= 3
{
	u32 list2Bits = *waterList2++;
	waterBits |= list2Bits;
}
#endif
#if ANIMATE_WATER_LISTS >= 4
{
	u32 list3Bits = *waterList3++;
	waterBits |= list3Bits;
}
#endif
waterFirstOffset = firstFrame << 1;
frameWork = CTR_PSX_READ_HI(pending);
waterBitsLeft = 32;
waterSecondOffset = frameWork << 1;
goto test_remaining;
decrement_then_call : waterBitsLeft--;
vertex_call :
#ifdef CTR_NATIVE
    AnimateWaterVertex(waterVertex, (u16)waterColor, waterFirstOffset, waterSecondOffset, &waterBits);
waterVertex++;
#else
    waterVertexAddress += sizeof(struct WaterVert);
AnimateWaterVertex();
#endif
test_remaining : if (waterRemaining == 0) goto restore_frame;
waterRemaining--;
if (waterBitsLeft != 0)
	goto decrement_then_call;
waterBits = *waterList0++;
#if ANIMATE_WATER_LISTS >= 2
{
	u32 list1Bits = *waterList1++;
	waterBits |= list1Bits;
}
#endif
#if ANIMATE_WATER_LISTS >= 3
{
	u32 list2Bits = *waterList2++;
	waterBits |= list2Bits;
}
#endif
#if ANIMATE_WATER_LISTS >= 4
{
	u32 list3Bits = *waterList3++;
	waterBits |= list3Bits;
}
#endif
waterBitsLeft = 31;
goto vertex_call;
restore_frame : waterRemaining--;
ANIMATE_SCRATCH_RESTORE();
