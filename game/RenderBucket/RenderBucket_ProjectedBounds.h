#ifndef RENDER_BUCKET_PROJECTED_BOUNDS_H
#define RENDER_BUCKET_PROJECTED_BOUNDS_H

// NOTE(aalhendi): Both adapters supply side-effect-free operands; sxy and the
// six bounds are mutable lvalues. Differences wrap before their signed tests.
#define RENDER_BUCKET_EXPAND_PROJECTED_BOUNDS(sxy, depth, minX, maxX, minY, maxY, minZ, maxZ) \
	do                                                                                        \
	{                                                                                         \
		register s32 difference CTR_PSX_REGISTER("$8");                                       \
                                                                                              \
		difference = CTR_MipsSra((s32)(sxy), 16);                                             \
		difference = CTR_MipsSubLo(difference, (minY));                                       \
		if (difference < 0)                                                                   \
		{                                                                                     \
			(minY) = CTR_MipsSra((s32)(sxy), 16);                                             \
		}                                                                                     \
		difference = CTR_MipsSra((s32)(sxy), 16);                                             \
		difference = CTR_MipsSubLo(difference, (maxY));                                       \
		if (difference > 0)                                                                   \
		{                                                                                     \
			(maxY) = CTR_MipsSra((s32)(sxy), 16);                                             \
		}                                                                                     \
                                                                                              \
		(sxy) <<= 16;                                                                         \
		difference = CTR_MipsSra((s32)(sxy), 16);                                             \
		difference = CTR_MipsSubLo(difference, (minX));                                       \
		if (difference < 0)                                                                   \
		{                                                                                     \
			(minX) = CTR_MipsSra((s32)(sxy), 16);                                             \
		}                                                                                     \
		difference = CTR_MipsSra((s32)(sxy), 16);                                             \
		difference = CTR_MipsSubLo(difference, (maxX));                                       \
		if (difference > 0)                                                                   \
		{                                                                                     \
			(maxX) = CTR_MipsSra((s32)(sxy), 16);                                             \
		}                                                                                     \
                                                                                              \
		difference = CTR_MipsSubLo((depth), (minZ));                                          \
		if (difference < 0)                                                                   \
		{                                                                                     \
			(minZ) = (depth);                                                                 \
		}                                                                                     \
		difference = CTR_MipsSubLo((depth), (maxZ));                                          \
		if (difference > 0)                                                                   \
		{                                                                                     \
			(maxZ) = (depth);                                                                 \
		}                                                                                     \
	} while (0)

#endif
