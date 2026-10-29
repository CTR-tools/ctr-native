#include <ctr_types.h>

#ifndef CTR_NATIVE
// NOTE(aalhendi): Full and Piece share live retail GPRs, not the C call ABI.
// GCC 2.8.1 requires these bindings before common.h's inline definitions.
register struct Skybox *const skyboxOwner CTR_PSX_REGISTER("$4");
register struct PushBuffer *const pushBufferOwner CTR_PSX_REGISTER("$5");
register struct PrimMem *const primMemOwner CTR_PSX_REGISTER("$6");

// NOTE(aalhendi): Piece preserves these real inputs across all four calls.
// Its original local captures below alias these same hardware registers.
register const struct ShortVertex *skyVertexBaseOwner CTR_PSX_REGISTER("$12");
register u32 *skyOtBaseOwner CTR_PSX_REGISTER("$7");
register u32 skyScreenBoundsOwner CTR_PSX_REGISTER("$15");

register u8 *skyCursor CTR_PSX_REGISTER("$2");
register u8 *skyFace CTR_PSX_REGISTER("$11");
register u32 skyPrimitiveCount CTR_PSX_REGISTER("$14");
register u32 skyS1 CTR_PSX_REGISTER("$17");
register u8 *skyNextOt CTR_PSX_REGISTER("$18");
register u8 *skyPreviousOt CTR_PSX_REGISTER("$25");

register u32 gteSxy2 CTR_PSX_REGISTER("gte_sxy2");
register u32 gteVxy0 CTR_PSX_REGISTER("gte_vxy0");
#endif

#include <common.h>
#include <ctr_gte_transfer.h>
// NOTE(aalhendi): Full saves its incoming RA/S0-S2 in scratch words 0-3;
// Piece preserves AT and the 24-byte frame. Native keeps the host call ABI.
#ifdef CTR_NATIVE
#define DRAW_SKY_MANUAL_AT_DECLARE(name) u32 *name = CTR_SCRATCHPAD_PTR(u32, 0)
#define DRAW_SKY_MANUAL_AT_SAVE(base)    ((void)0)
#define DRAW_SKY_MANUAL_AT_RESTORE(base) ((void)0)
#define DRAW_SKY_MANUAL_AT_RETURN()      return
#else
#define DRAW_SKY_MANUAL_AT_DECLARE(name)    \
	register u32 *name asm("$1");           \
	register u32 drawSkySavedRa asm("$31"); \
	register u32 drawSkySavedS0 asm("$16"); \
	register u32 drawSkySavedS1 asm("$17"); \
	register u32 drawSkySavedS2 asm("$18")

// NOTE(aalhendi): These are incoming hardware values under the unsafe ABI,
// not uninitialized ordinary C data. Restore and return share these operands.
#define DRAW_SKY_MANUAL_AT_SAVE(base)                                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		__asm__ volatile("" : "=r"(drawSkySavedRa), "=r"(drawSkySavedS0), "=r"(drawSkySavedS1), "=r"(drawSkySavedS2)); \
		(base) = CTR_SCRATCHPAD_PTR(u32, 0);                                                                           \
		(base)[0] = drawSkySavedRa;                                                                                    \
		(base)[1] = drawSkySavedS0;                                                                                    \
		(base)[2] = drawSkySavedS1;                                                                                    \
		(base)[3] = drawSkySavedS2;                                                                                    \
	} while (0)

#define DRAW_SKY_MANUAL_AT_RESTORE(base) \
	do                                   \
	{                                    \
		drawSkySavedRa = (base)[0];      \
		drawSkySavedS0 = (base)[1];      \
		drawSkySavedS1 = (base)[2];      \
		drawSkySavedS2 = (base)[3];      \
	} while (0)

// NOTE(aalhendi): Observe the restored ABI tuple through cursor publication,
// then let the compiler emit the ordinary return.
#define DRAW_SKY_MANUAL_AT_RETURN()                                                                                             \
	do                                                                                                                          \
	{                                                                                                                           \
		__asm__ volatile("" : : "r"(drawSkySavedRa), "r"(drawSkySavedS0), "r"(drawSkySavedS1), "r"(drawSkySavedS2) : "memory"); \
		return;                                                                                                                 \
	} while (0)
#endif

#ifdef CTR_NATIVE
#include <gpu.h>

struct DrawSkyContext
{
	const struct ShortVertex *verts;
	u32 *ot;
	u32 *lastFaceOt;
	u32 screenBounds;
	u32 primitiveCount;
};

#define DRAW_SKY_LOAD_DATA_WORD(base, byteOffset, reg)  MTC2(CTR_ReadU32LE((const u8 *)(base) + (byteOffset)), reg)
#define DRAW_SKY_STORE_DATA_WORD(base, byteOffset, reg) CTR_WriteU32AlignedLE((u8 *)(base) + (byteOffset), MFC2(reg))
#define DRAW_SKY_LOAD_VXY0(address)                     MTC2(CTR_ReadU32LE((const void *)(address)), 0)
#define DRAW_SKY_READ_SXY2()                            MFC2(14)
#define DRAW_SKY_RTPT()                                 gte_rtpt_b()
#define DRAW_SKY_PACKET_LINK(dst, cursor)               ((dst) = CtrGpu_PrimToOTLink24(cursor))
// NOTE(aalhendi): Both paths trap at 0x7fffffff; native increments the other bit patterns.
#define DRAW_SKY_COUNT_VISIBLE()              \
	do                                        \
	{                                         \
		if (skyPrimitiveCount == 0x7fffffffu) \
		{                                     \
			CTR_TRAP();                       \
		}                                     \
		skyPrimitiveCount++;                  \
	} while (0)
#else
#define DRAW_SKY_LOAD_DATA_WORD(base, byteOffset, reg)  __asm__ volatile("lwc2 $" CTR_GTE_STRINGIFY(reg) ",%1(%0)" : : "r"(base), "I"(byteOffset) : "memory")
#define DRAW_SKY_STORE_DATA_WORD(base, byteOffset, reg) __asm__ volatile("swc2 $" CTR_GTE_STRINGIFY(reg) ",%1(%0)" : : "r"(base), "I"(byteOffset) : "memory")
#define DRAW_SKY_LOAD_VXY0(address)                     (gteVxy0 = *(const volatile u32 *)(address))
#define DRAW_SKY_READ_SXY2()                            gteSxy2
#define DRAW_SKY_RTPT()                                 __asm__ volatile("cop2 0x280030" : : "b"(gteVxy0) : "gte_sxy2")
#define DRAW_SKY_PACKET_LINK(dst, cursor) \
	do                                    \
	{                                     \
		(dst) = (u32)(cursor) << 8;       \
		(dst) >>= 8;                      \
	} while (0)
#define DRAW_SKY_COUNT_VISIBLE() __asm__ volatile("addi %0,%0,1" : "+r"(skyPrimitiveCount))
#endif

#define DRAW_SKY_EMIT_TRIANGLE()                     \
	do                                               \
	{                                                \
		DRAW_SKY_STORE_DATA_WORD(skyCursor, 4, 20);  \
		DRAW_SKY_STORE_DATA_WORD(skyCursor, 8, 12);  \
		DRAW_SKY_STORE_DATA_WORD(skyCursor, 12, 21); \
		DRAW_SKY_STORE_DATA_WORD(skyCursor, 16, 13); \
		DRAW_SKY_STORE_DATA_WORD(skyCursor, 20, 22); \
		DRAW_SKY_STORE_DATA_WORD(skyCursor, 24, 14); \
		flags = CTR_ReadU32LE(skyPreviousOt);        \
		DRAW_SKY_PACKET_LINK(skyS0, skyCursor);      \
		skyS1 = 0x06000000u;                         \
		flags |= skyS1;                              \
		CTR_WriteU32AlignedLE(skyCursor, flags);     \
		skyCursor += 28;                             \
		CTR_WriteU32AlignedLE(skyPreviousOt, skyS0); \
	} while (0)

#define DRAW_SKY_TEST_VISIBILITY(reject) \
	do                                   \
	{                                    \
		skyS0 = MFC2(12);                \
		flags = CFC2(31);                \
		skyS1 = MFC2(13);                \
		flags <<= 13;                    \
		flags >>= 29;                    \
		if (flags != 0)                  \
		{                                \
			goto reject;                 \
		}                                \
		flags = DRAW_SKY_READ_SXY2();    \
		overlap = skyS0 & skyS1;         \
		overlap &= flags;                \
		skyS0 -= screenBounds;           \
		skyS1 -= screenBounds;           \
		flags -= screenBounds;           \
		skyS0 |= skyS1;                  \
		skyS0 = ~(flags | skyS0);        \
		skyS0 |= overlap;                \
		if ((s32)skyS0 < 0)              \
		{                                \
			skyS0 <<= 16;                \
			goto reject;                 \
		}                                \
		skyS0 <<= 16;                    \
		if ((s32)skyS0 < 0)              \
		{                                \
			goto reject;                 \
		}                                \
	} while (0)

#ifdef CTR_NATIVE
static u32 *DrawSky_Piece(u8 *skyFace, const u8 *countAddress, struct DrawSkyContext *ctx, u32 *prim)
#else
void CTR_PSX_REGCALL_S012(0x0200f884u) CTR_PSX_PRESERVES_AT24 DrawSky_Piece(void)
#endif
{
#ifdef CTR_NATIVE
	u8 *skyCursor = (u8 *)prim;
	u32 skyPrimitiveCount = ctx->primitiveCount;
	u32 skyS1;
	u8 *skyNextOt;
	u8 *skyPreviousOt = (u8 *)ctx->lastFaceOt;
#endif
	register u32 skyRemaining CTR_PSX_REGISTER("$13");
	register u32 skyS0 CTR_PSX_REGISTER("$16");
	register u8 *vertices CTR_PSX_REGISTER("$12");
	register u8 *otBase CTR_PSX_REGISTER("$7");
	register u32 screenBounds CTR_PSX_REGISTER("$15");
	register u32 first CTR_PSX_REGISTER("$3");
	register u32 second CTR_PSX_REGISTER("$8");
	register u32 third CTR_PSX_REGISTER("$9");
	register u32 flags CTR_PSX_REGISTER("$10");
	register u32 overlap CTR_PSX_REGISTER("$24");

	CTR_PSX_CAPTURE_REGISTER(skyRemaining, (u32)countAddress);
	CTR_PSX_CAPTURE_REGISTER(vertices, (u8 *)ctx->verts);
	CTR_PSX_CAPTURE_REGISTER(otBase, (u8 *)ctx->ot);
	CTR_PSX_CAPTURE_REGISTER(screenBounds, ctx->screenBounds);
	skyFace = (u8 *)CTR_ReadU32LE(skyFace + 24);
	skyRemaining = CTR_ReadU16LE((const u8 *)skyRemaining + 8);
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native empty pieces may have a NULL face pointer.
	if (skyRemaining == 0)
	{
		goto piece_return;
	}
#endif
	first = CTR_ReadU16LE(skyFace);
	second = *(const volatile CtrPackedU16 *)(skyFace + 2);
	if (skyRemaining == 0)
	{
		goto piece_return;
	}
	third = CTR_ReadU32LE(skyFace + 4);
	first += (u32)vertices;
	second += (u32)vertices;
	skyNextOt = (u8 *)(u32)CTR_MipsSra((s32)third, 16);
	skyNextOt = (u8 *)((u32)skyNextOt + (u32)otBase);
	third &= 65535u;
	third += (u32)vertices;
	skyRemaining--;
	goto load_vertices;

pipeline_triangle:
	DRAW_SKY_RTPT();
	skyFace += 8;
	first = CTR_ReadU16LE(skyFace);
	second = CTR_ReadU16LE(skyFace + 2);
	third = CTR_ReadU32LE(skyFace + 4);
	first += (u32)vertices;
	second += (u32)vertices;
	skyPreviousOt = skyNextOt;
	skyNextOt = (u8 *)(u32)CTR_MipsSra((s32)third, 16);
	skyNextOt = (u8 *)((u32)skyNextOt + (u32)otBase);
	third &= 65535u;
	third += (u32)vertices;
	DRAW_SKY_TEST_VISIBILITY(load_vertices);
	DRAW_SKY_COUNT_VISIBLE();
	DRAW_SKY_EMIT_TRIANGLE();

load_vertices:
	DRAW_SKY_LOAD_VXY0(first);
	DRAW_SKY_LOAD_DATA_WORD((const u8 *)first, 4, 1);
	DRAW_SKY_LOAD_DATA_WORD((const u8 *)first, 8, 20);
	DRAW_SKY_LOAD_DATA_WORD((const u8 *)second, 0, 2);
	DRAW_SKY_LOAD_DATA_WORD((const u8 *)second, 4, 3);
	DRAW_SKY_LOAD_DATA_WORD((const u8 *)second, 8, 21);
	DRAW_SKY_LOAD_DATA_WORD((const u8 *)third, 0, 4);
	DRAW_SKY_LOAD_DATA_WORD((const u8 *)third, 4, 5);
	DRAW_SKY_LOAD_DATA_WORD((const u8 *)third, 8, 22);
	if (skyRemaining != 0)
	{
		skyRemaining--;
		goto pipeline_triangle;
	}
	DRAW_SKY_RTPT();
	DRAW_SKY_TEST_VISIBILITY(piece_return);
	// NOTE(aalhendi): The final face reuses t9, even across piece boundaries.
	DRAW_SKY_EMIT_TRIANGLE();

piece_return:
#ifdef CTR_NATIVE
	ctx->lastFaceOt = (u32 *)skyPreviousOt;
	ctx->primitiveCount = skyPrimitiveCount;
	return (u32 *)skyCursor;
#else
	return;
#endif
}

#undef DRAW_SKY_TEST_VISIBILITY
#undef DRAW_SKY_EMIT_TRIANGLE
#undef DRAW_SKY_COUNT_VISIBLE
#undef DRAW_SKY_PACKET_LINK
#undef DRAW_SKY_RTPT
#undef DRAW_SKY_READ_SXY2
#undef DRAW_SKY_LOAD_VXY0
#undef DRAW_SKY_STORE_DATA_WORD
#undef DRAW_SKY_LOAD_DATA_WORD

#ifdef CTR_NATIVE
#define DRAW_SKY_FULL_VERTICES ctx.verts
#define DRAW_SKY_FULL_OT       ctx.ot
#define DRAW_SKY_FULL_BOUNDS   ctx.screenBounds
#define DRAW_SKY_FULL_PIECE()  (skyCursor = (u8 *)DrawSky_Piece(skyFace, skyCountAddress, &ctx, (u32 *)skyCursor))
#else
#define DRAW_SKY_FULL_VERTICES skyVertexBaseOwner
#define DRAW_SKY_FULL_OT       skyOtBaseOwner
#define DRAW_SKY_FULL_BOUNDS   skyScreenBoundsOwner
#define DRAW_SKY_FULL_PIECE()  DrawSky_Piece()
#endif

#define DRAW_SKY_FULL_SELECT_PIECE(faceStep, countStep)        \
	do                                                         \
	{                                                          \
		u32 rawFace;                                           \
		u32 rawCount;                                          \
		u32 selectedFace;                                      \
		u32 selectedCount;                                     \
                                                               \
		rawFace = frame[4];                                    \
		rawCount = frame[5];                                   \
		selectedFace = (rawFace + (u32)(faceStep)) & 0x1cu;    \
		selectedCount = (rawCount + (u32)(countStep)) & 0x0eu; \
		skyFace = (u8 *)skyboxOwner + selectedFace;            \
		skyCountAddress = (u8 *)skyboxOwner + selectedCount;   \
		DRAW_SKY_FULL_PIECE();                                 \
	} while (0)

void CTR_PSX_UNSAFE_MANUAL_AT_FRAME DrawSky_Full(void *skybox, struct PushBuffer *pb, struct PrimMem *primMem)
{
#ifdef CTR_NATIVE
	struct Skybox *const skyboxOwner = skybox;
	struct PushBuffer *const pushBufferOwner = pb;
	struct PrimMem *const primMemOwner = primMem;
	u8 *skyCursor;
	u8 *skyFace;
	const u8 *skyCountAddress;
	struct DrawSkyContext ctx;
	u32 *frame;
#else
	register u8 *skyCountAddress CTR_PSX_REGISTER("$13");
	DRAW_SKY_MANUAL_AT_DECLARE(frame);
#endif
	u32 baseFaceOffset;
	u32 rotationWord0;
	u32 rotationWord1;
	u32 rotationWord2;
	u32 rotationWord3;
	u32 rotationWord4;

	DRAW_SKY_MANUAL_AT_SAVE(frame);
	skyCursor = (u8 *)primMemOwner->cursor;
#ifndef CTR_NATIVE
	// NOTE(aalhendi): Retail performs this LW even for a NULL skybox.
	skyPrimitiveCount = (u32) * (const volatile s32 *)&primMemOwner->primitiveCount;
#endif
	if (skyboxOwner != NULL)
	{
#ifdef CTR_NATIVE
		// NOTE(aalhendi): A NULL sky never needs a mapped native scratchpad.
		frame = CTR_SCRATCHPAD_PTR(u32, 0);
#endif
		DRAW_SKY_FULL_BOUNDS = CTR_ReadU32AlignedLE((const u8 *)pushBufferOwner + 0x20);
		DRAW_SKY_FULL_OT = pushBufferOwner->ptrOT;
		DRAW_SKY_FULL_VERTICES = skyboxOwner->ptrVertex;
#ifdef CTR_NATIVE
		// NOTE(aalhendi): Retail carries t6 across pieces but never publishes it.
		ctx.primitiveCount = (u32)primMemOwner->primitiveCount;
#endif
		// NOTE(aalhendi): Stage the rotation words before their hardware writes.
		rotationWord0 = CTR_ReadU32AlignedLE((const u8 *)pushBufferOwner + 0x28);
		rotationWord1 = CTR_ReadU32AlignedLE((const u8 *)pushBufferOwner + 0x2c);
		rotationWord2 = CTR_ReadU32AlignedLE((const u8 *)pushBufferOwner + 0x30);
		rotationWord3 = CTR_ReadU32AlignedLE((const u8 *)pushBufferOwner + 0x34);
		CTR_GteWriteControl(rotationWord0, 0);
		rotationWord4 = CTR_ReadU32AlignedLE((const u8 *)pushBufferOwner + 0x38);
		CTR_GteWriteControl(rotationWord1, 1);
		CTR_GteWriteControl(rotationWord2, 2);
		CTR_GteWriteControl(rotationWord3, 3);
		CTR_GteWriteControl(rotationWord4, 4);
		DRAW_SKY_FULL_OT += 0x3ff;
#ifdef CTR_NATIVE
		// NOTE(aalhendi): Preserve native's leading one-face OT fallback.
		ctx.lastFaceOt = ctx.ot;
#endif
		baseFaceOffset = CTR_ReadU32AlignedLE((const u8 *)pushBufferOwner + 0x08);
		CTR_GteWriteControl(0u, 5);
		CTR_GteWriteControl(0u, 6);
		CTR_GteWriteControl(0u, 7);
		baseFaceOffset = ((baseFaceOffset + 0x500u) >> 7) & 0x1cu;
		skyFace = (u8 *)skyboxOwner + baseFaceOffset;
		frame[4] = baseFaceOffset;
		baseFaceOffset >>= 1;
		frame[5] = baseFaceOffset;
		skyCountAddress = (u8 *)skyboxOwner + baseFaceOffset;
		DRAW_SKY_FULL_PIECE();
		DRAW_SKY_FULL_SELECT_PIECE(4, 2);
		DRAW_SKY_FULL_SELECT_PIECE(-4, -2);
		DRAW_SKY_FULL_SELECT_PIECE(-8, -4);
	}
	DRAW_SKY_MANUAL_AT_RESTORE(frame);
	primMemOwner->cursor = skyCursor;
	DRAW_SKY_MANUAL_AT_RETURN();
}

#undef DRAW_SKY_FULL_SELECT_PIECE
#undef DRAW_SKY_FULL_PIECE
#undef DRAW_SKY_FULL_BOUNDS
#undef DRAW_SKY_FULL_OT
#undef DRAW_SKY_FULL_VERTICES

#undef DRAW_SKY_MANUAL_AT_RETURN
#undef DRAW_SKY_MANUAL_AT_RESTORE
#undef DRAW_SKY_MANUAL_AT_SAVE
#undef DRAW_SKY_MANUAL_AT_DECLARE
