#ifndef OVR_227_H
#define OVR_227_H

enum OverlayRDATA_227_Counts
{
	OVR227_BUCKET_COUNT = 11,
	OVR227_SCRATCH_INIT_WORD_COUNT = 24,
	OVR227_SETUP_COPY0_WORD_COUNT = 15,
	OVR227_SETUP_COPY1_WORD_COUNT = 3,
	OVR227_CLIP_RECORD_JUMP_WORD_COUNT = 24,
};

enum OverlayRDATA_227_Addresses
{
	OVR227_RDATA_START = 0x800ab48c,
	OVR227_RDATA_BUCKET_SETUP_BASE = 0x800ab544,
	OVR227_RDATA_STOP = 0x800ab9f0,
};

struct OverlayRDATA_227_BucketSetupCopy
{
	u32 loopCounter;
	u32 sourceAddress;
	u32 scratchOffset;
};

struct OverlayRDATA_227_BucketSetupRecord
{
	struct OverlayRDATA_227_BucketSetupCopy copies[2];
	u32 padding;
	u32 copy0[OVR227_SETUP_COPY0_WORD_COUNT];
	u32 copy1[OVR227_SETUP_COPY1_WORD_COUNT];
};

struct OverlayRDATA_227
{
	// 0x800ab48c
	u32 bucketSetupAddresses[OVR227_BUCKET_COUNT];

	// 0x800ab4b8
	u32 bucketHandlerAddresses[OVR227_BUCKET_COUNT];

	// 0x800ab4e4
	u32 scratchInitTable[OVR227_SCRATCH_INIT_WORD_COUNT];

	// 0x800ab544
	struct OverlayRDATA_227_BucketSetupRecord bucketSetups[OVR227_BUCKET_COUNT];

	// 0x800ab990
	u32 clipRecordJumpTable[OVR227_CLIP_RECORD_JUMP_WORD_COUNT];
};

extern struct OverlayRDATA_227 R227;

#endif
