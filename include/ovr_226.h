#ifndef OVR_226_H
#define OVR_226_H

enum OverlayRDATA_226_Counts
{
	OVR226_BUCKET_COUNT = 11,
	OVR226_SCRATCH_INIT_WORD_COUNT = 24,
	OVR226_SETUP_COPY0_WORD_COUNT = 15,
	OVR226_SETUP_COPY1_WORD_COUNT = 3,
	OVR226_CLIP_RECORD_JUMP_WORD_COUNT = 24,
};

enum OverlayRDATA_226_Addresses
{
	OVR226_RDATA_BUCKET_SETUP_BASE = 0x800ab4c4,
};

struct OverlayRDATA_226_BucketSetupCopy
{
	u32 loopCounter;
	u32 sourceAddress;
	u32 scratchOffset;
};

struct OverlayRDATA_226_BucketSetupRecord
{
	struct OverlayRDATA_226_BucketSetupCopy copies[2];
	u32 padding;
	u32 copy0[OVR226_SETUP_COPY0_WORD_COUNT];
	u32 copy1[OVR226_SETUP_COPY1_WORD_COUNT];
};

struct OverlayRDATA_226
{
	// 0x800ab40c
	u32 bucketSetupAddresses[OVR226_BUCKET_COUNT];

	// 0x800ab438
	u32 bucketHandlerAddresses[OVR226_BUCKET_COUNT];

	// 0x800ab464
	u32 scratchInitTable[OVR226_SCRATCH_INIT_WORD_COUNT];

	// 0x800ab4c4
	struct OverlayRDATA_226_BucketSetupRecord bucketSetups[OVR226_BUCKET_COUNT];

	// 0x800ab910
	u32 clipRecordJumpTable[OVR226_CLIP_RECORD_JUMP_WORD_COUNT];
};

extern struct OverlayRDATA_226 R226;

#endif
