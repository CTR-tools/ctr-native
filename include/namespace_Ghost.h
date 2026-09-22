#ifndef CTR_NATIVE_NAMESPACE_GHOST_H
#define CTR_NATIVE_NAMESPACE_GHOST_H

// GhostTape is 0x268 large
// GhostRecBuf is 0x3e00

enum
{
	GHOST_OP_POSITION = 0x80,
	GHOST_OP_ANIMATION,
	GHOST_OP_BOOST,
	GHOST_OP_INSTANCE,
	GHOST_OP_IDLE,
};
typedef u8 GhostOpcode;

enum GhostPacketSize
{
	GHOST_SIZE_POSITION = 11,
	GHOST_SIZE_ANIMATION = 3,
	GHOST_SIZE_BOOST = 6,
	GHOST_SIZE_INSTANCE = 2,
	GHOST_SIZE_IDLE = 1,
	GHOST_SIZE_VELOCITY = 5,
};

enum GhostTapeRecordConstant
{
	GHOST_RECORD_BUFFER_SIZE = 0x3e00,
	GHOST_TAPE_VERSION_RETAIL = -4,
	GHOST_RECORD_POSITION_SHIFT = 3,
	GHOST_RECORD_ROTATION_SHIFT = 4,
	GHOST_RECORD_INTERVAL_MASK_8 = 7,
	GHOST_RECORD_INTERVAL_MASK_32 = 0x1f,
	GHOST_RECORD_VELOCITY_MAX = 0x80,
	GHOST_RECORD_VELOCITY_MIN_EXCLUSIVE = -0x7c,
	GHOST_RECORD_TIME_DELTA_MAX_EXCLUSIVE = 0xff01,
	GHOST_RECORD_INSTANCE_SPLIT_FLAG = 0x2000,
	GHOST_RECORD_INSTANCE_SPLIT_SHIFT = 0xd,
	GHOST_RECORD_BUFFER_END_GUARD = 0x40,
	GHOST_RECORD_OVERFLOW_TEXT_FRAMES = CTR_SECONDS_TO_FRAMES(6),
	GHOST_RECORD_BOOST_COOLDOWN_FRAMES = 0x1e,
};

CTR_STATIC_ASSERT(sizeof(GhostOpcode) == 0x1);

#define GHOST_IS_OPCODE(b) ((u8)((b) + GHOST_OP_POSITION) < (GHOST_OP_IDLE - GHOST_OP_POSITION + 1))
#define Ghost_ReadBE16(p)  ((u16)((p)[0] << 8 | (p)[1]))

struct GhostPacket
{
	SVec3 pos;
	SVec3 rot;

	u8 *bufferPacket;

	// 0x10 -- size of packet
};

CTR_STATIC_ASSERT(sizeof(struct GhostPacket) == 0x10);

struct GhostTape
{
	// 0x0
	struct GhostHeader *gh;
	void *ptrStart; // gh->0x28
	void *ptrEnd;   // gh->0x28 + gh->size
	void *ptrCurr;

	// 0x10
	s32 unk10;

	// 0x14
	s32 timeElapsedInRace;

	// 0x18
	s32 timeInPacket32_backup;

	// 0x1c
	s32 unk1C;
	s32 unk20;

	// 0x24
	// Retail copies these snapshots after the first replay tick.
	SVec3 unk1;
	SVec3 unk2;
	SVec3 unk3;
	SVec3 unk4;

	// 0x3C
	s32 timeInPacket01;

	// 0x40
	s32 timeInPacket32;

	// 0x44
	s32 timeBetweenPackets;

	// 0x48
	s32 numPacketsInArray;

	// 0x4C
	s16 packetID;
	u16 padPacketID;

	// 0x50
	struct GhostPacket packets[0x21];

	// 0x260
	u32 constDEADC0ED;

	// 0x264
	struct GhostHeader *gh_again; // Selected header retained by Init1.

	// 0x268 bytes large
};


struct GhostHeader
{
	// 0x0
	s16 version;
	u16 size;

	// 0x4
	s16 levelID;

	// 0x6
	s16 characterID;

	// 0x8
	s32 speedApprox; // Restored when replay hands the driver back to bot control.
	s32 ySpeed;

	// 0x10
	s32 timeElapsedInRace;

	// 0x14
	// Reserved bytes; recording leaves their existing contents unchanged.
	char emptyPadding[0x14];

	// 0x28
	// Variable-length packet data follows the header.
};

#define GHOSTHEADER_GETRECORDBUFFER(x) ((char *)((x) + 1))

#ifndef GHOST_RECORDING
#define GHOST_RECORDING      (sdata->GhostRecording)
#define GHOST_PLAYING        (sdata->ptrGhostTapePlaying)
#define GHOST_CAN_SAVE       (sdata->boolCanSaveGhost)
#define GHOST_TOO_BIG        (sdata->boolGhostTooBigToSave)
#define GHOST_OVERFLOW_TIMER (sdata->ghostOverflowTextTimer)
#define GHOST_DRAWING        (sdata->boolGhostsDrawing)
#define GHOST_TAPES          (sdata->ptrGhostTape)
#define GHOST_REPLAY_HUMAN   (sdata->boolReplayHumanGhost)
#define GHOST_NAME           (sdata->s_ghost)
#define GHOST_NAME_STAFF     (sdata->s_ghost1)
#define GHOST_NAME_HUMAN     (sdata->s_ghost0)
#endif

#endif
