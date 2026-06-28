#include <macros.h>
#include <platform/native_assets.h>
#include <platform/native_renderer.h>
#include <platform/native_str.h>
#include <psx/libgpu.h>

#include <stdio.h>
#include <string.h>

#define NATIVE_STR_EXTRACTED_SECTOR_SIZE    0x800
#define NATIVE_STR_CD_SECTOR_SIZE           0x920
#define NATIVE_STR_CD_SUBHEADER_SIZE        0x8
#define NATIVE_STR_SECTOR_HEADER            0x20
#define NATIVE_STR_EXTRACTED_SECTOR_PAYLOAD (NATIVE_STR_EXTRACTED_SECTOR_SIZE - NATIVE_STR_SECTOR_HEADER)
#define NATIVE_STR_CD_SECTOR_PAYLOAD        NATIVE_STR_EXTRACTED_SECTOR_PAYLOAD
#define NATIVE_STR_MAX_RECORD_SIZE          NATIVE_STR_CD_SECTOR_SIZE
#define NATIVE_STR_MAX_FRAME_SECTORS        10
#define NATIVE_STR_MAX_FRAME_BYTES          (NATIVE_STR_MAX_FRAME_SECTORS * NATIVE_STR_CD_SECTOR_PAYLOAD)
#define NATIVE_STR_PATH_MAX                 1024
#define NATIVE_STR_MAX_WIDTH                512
#define NATIVE_STR_MAX_HEIGHT               240
#define NATIVE_STR_ID                       0x80010160u
#define NATIVE_STR_BS_ID                    0x3800u
#define NATIVE_STR_END_OF_BLOCK             0xfe00u
#define NATIVE_STR_IDCT_SHIFT               14
#define NATIVE_STR_IDCT_SCALE               (1 << NATIVE_STR_IDCT_SHIFT)
#define NATIVE_STR_SCRAPBOOK_PATH           "TEST.STR"
#define NATIVE_STR_SCRAPBOOK_FRAME_COUNT    0x1148

enum NativeSTRFormat
{
	NATIVE_STR_FORMAT_EXTRACTED,
	NATIVE_STR_FORMAT_CD_STREAM,
};

struct NativeSTRState
{
	FILE *file;
	s32 active;
	s32 format;
	s32 loop;
	s32 bigfileIndex;
	s32 frameIndex;
	s32 frameLimit;
	s32 frameSize;
	s32 width;
	s32 height;
	u8 frameData[NATIVE_STR_MAX_FRAME_BYTES];
	u16 rgb555[NATIVE_STR_MAX_WIDTH * NATIVE_STR_MAX_HEIGHT];
};

struct NativeSTRSectorHeader
{
	u32 id;
	u16 chunkIndex;
	u16 chunkCount;
	u32 frameIndex;
	u32 frameSize;
	u16 width;
	u16 height;
};

struct NativeSTRBitReader
{
	const u8 *data;
	s32 size;
	s32 bitOffset;
};

struct NativeSTRAcGroup
{
	u16 prefix;
	u8 prefixBits;
	u8 indexBits;
	u8 valueCount;
	u16 values[16];
};

global_variable struct NativeSTRState s_str;

// NOTE(aalhendi): MDEC tables and BS v1/v2/v3 Huffman groups are transcribed
// from psx-spx's documented PS1 MDEC/STR format.
global_variable const u8 s_mdecZagzig[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
};

// NOTE(aalhendi): PSX MDEC default quantization table: MPEG-1 intra matrix
// with the first value changed from 8 to 2.
global_variable const u8 s_mdecQuantTable[64] = {
    2,  16, 19, 22, 26, 27, 29, 34, 16, 16, 22, 24, 27, 29, 34, 37, 19, 22, 26, 27, 29, 34, 34, 38, 22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48, 26, 27, 29, 32, 35, 40, 48, 58, 26, 27, 29, 34, 38, 46, 56, 69, 27, 29, 35, 38, 46, 56, 69, 83,
};

// NOTE(aalhendi): Q14 fixed-point form of the standard 8x8 IDCT cosine
// basis. Native decodes track-preview STR frames on CPU; retail sends this
// work to PS1 MDEC hardware.
// clang-format off
global_variable const s16 s_idctBasis[8][8] = {
    { 11585,  16069,  15137,  13623,  11585,   9102,   6270,   3196},
    { 11585,  13623,   6270,  -3196, -11585, -16069, -15137,  -9102},
    { 11585,   9102,  -6270, -16069, -11585,   3196,  15137,  13623},
    { 11585,   3196, -15137,  -9102,  11585,  13623,  -6270, -16069},
    { 11585,  -3196, -15137,   9102,  11585, -13623,  -6270,  16069},
    { 11585,  -9102,  -6270,  16069, -11585,  -3196,  15137, -13623},
    { 11585, -13623,   6270,   3196, -11585,  16069, -15137,   9102},
    { 11585, -16069,  15137, -13623,  11585,  -9102,   6270,  -3196},
};
// clang-format on

// NOTE(aalhendi): BS v1/v2/v3 AC Huffman groups, expressed from the documented
// PS1 MDEC bit patterns. CTR track previews use BS v2.
global_variable const struct NativeSTRAcGroup s_acGroups[] = {
    {0x2, 2, 0, 1, {NATIVE_STR_END_OF_BLOCK}},
    {0x3, 2, 0, 1, {0x0001}},
    {0x3, 3, 0, 1, {0x0401}},
    {0x2, 3, 1, 2, {0x0002, 0x0801}},
    {0x3, 4, 1, 2, {0x1001, 0x0c01}},
    {0x5, 5, 0, 1, {0x0003}},
    {0x4, 5, 3, 8, {0x3401, 0x0006, 0x3001, 0x2c01, 0x0c02, 0x0403, 0x0005, 0x2801}},
    {0x1, 4, 2, 4, {0x1c01, 0x1801, 0x0402, 0x1401}},
    {0x1, 5, 2, 4, {0x0802, 0x2401, 0x0004, 0x2001}},
    {0x1, 7, 3, 8, {0x4001, 0x1402, 0x0007, 0x0803, 0x0404, 0x3c01, 0x3801, 0x1002}},
    {0x1, 8, 4, 16, {0x000b, 0x2002, 0x1003, 0x000a, 0x0804, 0x1c02, 0x5401, 0x5001, 0x0009, 0x4c01, 0x4801, 0x0405, 0x0c03, 0x0008, 0x1802, 0x4401}},
    {0x1, 9, 4, 16, {0x2802, 0x2402, 0x1403, 0x0c04, 0x0805, 0x0407, 0x0406, 0x000f, 0x000e, 0x000d, 0x000c, 0x6801, 0x6401, 0x6001, 0x5c01, 0x5801}},
    {0x1, 10, 4, 16, {0x001f, 0x001e, 0x001d, 0x001c, 0x001b, 0x001a, 0x0019, 0x0018, 0x0017, 0x0016, 0x0015, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010}},
    {0x1, 11, 4, 16, {0x0028, 0x0027, 0x0026, 0x0025, 0x0024, 0x0023, 0x0022, 0x0021, 0x0020, 0x040e, 0x040d, 0x040c, 0x040b, 0x040a, 0x0409, 0x0408}},
    {0x1, 12, 4, 16, {0x0412, 0x0411, 0x0410, 0x040f, 0x1803, 0x4002, 0x3c02, 0x3802, 0x3402, 0x3002, 0x2c02, 0x7c01, 0x7801, 0x7401, 0x7001, 0x6c01}},
};

internal u16 NativeSTR_ReadLE16(const u8 *p)
{
	return (u16)(p[0] | (p[1] << 8));
}

internal u32 NativeSTR_ReadLE32(const u8 *p)
{
	return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

internal s32 NativeSTR_Sign10(u32 value)
{
	value &= 0x3ff;
	return (value & 0x200) ? (s32)value - 0x400 : (s32)value;
}

internal s32 NativeSTR_Clamp8(s32 value)
{
	if (value < 0)
	{
		return 0;
	}
	if (value > 255)
	{
		return 255;
	}
	return value;
}

internal u16 NativeSTR_NegateMdecCode(u16 code)
{
	return (u16)((code & 0xfc00) | ((u32)-NativeSTR_Sign10(code) & 0x3ff));
}

internal u32 NativeSTR_ReadBits(struct NativeSTRBitReader *br, s32 count)
{
	u32 value = 0;
	s32 i;

	for (i = 0; i < count; i++)
	{
		s32 byteOffset = (br->bitOffset >> 4) * 2;
		s32 bit = 15 - (br->bitOffset & 0xf);
		u32 word = 0;

		if (byteOffset + 1 < br->size)
		{
			word = NativeSTR_ReadLE16(&br->data[byteOffset]);
		}

		value = (value << 1) | ((word >> bit) & 1);
		br->bitOffset++;
	}

	return value;
}

internal u32 NativeSTR_PeekBits(struct NativeSTRBitReader *br, s32 count)
{
	s32 oldBitOffset = br->bitOffset;
	u32 value = NativeSTR_ReadBits(br, count);

	br->bitOffset = oldBitOffset;
	return value;
}

internal s32 NativeSTR_ReadAcCode(struct NativeSTRBitReader *br, u16 *outCode)
{
	u32 i;

	if (NativeSTR_PeekBits(br, 6) == 0x1)
	{
		NativeSTR_ReadBits(br, 6);
		*outCode = (u16)NativeSTR_ReadBits(br, 16);
		return 1;
	}

	if (NativeSTR_PeekBits(br, 12) == 0)
	{
		NativeSTR_ReadBits(br, 12);
		*outCode = NATIVE_STR_END_OF_BLOCK;
		return 1;
	}

	for (i = 0; i < sizeof(s_acGroups) / sizeof(s_acGroups[0]); i++)
	{
		const struct NativeSTRAcGroup *group = &s_acGroups[i];

		if (NativeSTR_PeekBits(br, group->prefixBits) == group->prefix)
		{
			u16 code;
			u32 index = 0;
			u32 sign = 0;

			NativeSTR_ReadBits(br, group->prefixBits);
			if (group->indexBits != 0)
			{
				index = NativeSTR_ReadBits(br, group->indexBits);
			}

			if (group->values[0] != NATIVE_STR_END_OF_BLOCK)
			{
				sign = NativeSTR_ReadBits(br, 1);
			}

			if (index >= group->valueCount)
			{
				return 0;
			}

			code = group->values[index];
			*outCode = (sign != 0) ? NativeSTR_NegateMdecCode(code) : code;
			return 1;
		}
	}

	return 0;
}

internal void NativeSTR_IDCT(const s32 *coefficients, s32 *out)
{
	s32 x;
	s32 y;

	for (y = 0; y < 8; y++)
	{
		for (x = 0; x < 8; x++)
		{
			s64 sum = 0;
			s32 u;
			s32 v;

			for (v = 0; v < 8; v++)
			{
				for (u = 0; u < 8; u++)
				{
					sum += (s64)s_idctBasis[x][u] * s_idctBasis[y][v] * coefficients[v * 8 + u];
				}
			}

			if (sum >= 0)
			{
				sum += (s64)NATIVE_STR_IDCT_SCALE * NATIVE_STR_IDCT_SCALE * 2;
			}
			else
			{
				sum -= (s64)NATIVE_STR_IDCT_SCALE * NATIVE_STR_IDCT_SCALE * 2;
			}

			out[y * 8 + x] = (s32)(sum / ((s64)NATIVE_STR_IDCT_SCALE * NATIVE_STR_IDCT_SCALE * 4));
		}
	}
}

internal s32 NativeSTR_DecodeBlock(struct NativeSTRBitReader *br, s32 quant, s32 *out)
{
	s32 coefficients[64];
	s32 k = 0;
	s32 target;
	u16 code;

	memset(coefficients, 0, sizeof(coefficients));

	code = (u16)NativeSTR_ReadBits(br, 10);
	target = s_mdecZagzig[0];
	coefficients[target] = NativeSTR_Sign10(code) * s_mdecQuantTable[target];

	for (;;)
	{
		if (NativeSTR_ReadAcCode(br, &code) == 0)
		{
			return 0;
		}

		if (code == NATIVE_STR_END_OF_BLOCK)
		{
			break;
		}

		k += ((code >> 10) & 0x3f) + 1;
		if (k > 63)
		{
			break;
		}

		target = s_mdecZagzig[k];
		coefficients[target] = (NativeSTR_Sign10(code) * s_mdecQuantTable[target] * quant + 4) / 8;
	}

	NativeSTR_IDCT(coefficients, out);
	return 1;
}

internal u16 NativeSTR_YCbCrToRGB555(s32 y, s32 cb, s32 cr)
{
	s32 r = NativeSTR_Clamp8(y + ((91881 * cr + 32768) >> 16) + 128);
	s32 g = NativeSTR_Clamp8(y - ((22554 * cb + 46802 * cr + 32768) >> 16) + 128);
	s32 b = NativeSTR_Clamp8(y + ((116130 * cb + 32768) >> 16) + 128);

	return (u16)((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10));
}

internal s32 NativeSTR_DecodeMacroblock(struct NativeSTRBitReader *br, s32 quant, s32 baseX, s32 baseY)
{
	s32 blocks[6][64];
	s32 i;
	s32 x;
	s32 y;

	for (i = 0; i < 6; i++)
	{
		if (NativeSTR_DecodeBlock(br, quant, blocks[i]) == 0)
		{
			return 0;
		}
	}

	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 16; x++)
		{
			s32 dstX = baseX + x;
			s32 dstY = baseY + y;
			s32 yBlockIndex;
			s32 luma;
			s32 cb;
			s32 cr;

			if ((dstX >= s_str.width) || (dstY >= s_str.height))
			{
				continue;
			}

			if (y < 8)
			{
				yBlockIndex = (x < 8) ? 2 : 3;
			}
			else
			{
				yBlockIndex = (x < 8) ? 4 : 5;
			}

			luma = blocks[yBlockIndex][(y & 7) * 8 + (x & 7)];
			cr = blocks[0][(y >> 1) * 8 + (x >> 1)];
			cb = blocks[1][(y >> 1) * 8 + (x >> 1)];
			s_str.rgb555[dstY * s_str.width + dstX] = NativeSTR_YCbCrToRGB555(luma, cb, cr);
		}
	}

	return 1;
}

internal s32 NativeSTR_DecodeFrame(void)
{
	u16 mdecSize;
	u16 bsId;
	u16 quant;
	u16 version;
	struct NativeSTRBitReader br;
	s32 codedWidth;
	s32 codedHeight;
	s32 baseX;
	s32 baseY;

	mdecSize = NativeSTR_ReadLE16(&s_str.frameData[0]);
	bsId = NativeSTR_ReadLE16(&s_str.frameData[2]);
	quant = NativeSTR_ReadLE16(&s_str.frameData[4]);
	version = NativeSTR_ReadLE16(&s_str.frameData[6]);

	(void)mdecSize;

	if ((bsId != NATIVE_STR_BS_ID) || (quant > 0x3f) || ((version != 1) && (version != 2)))
	{
		return 0;
	}

	br.data = s_str.frameData;
	br.size = s_str.frameSize;
	br.bitOffset = 8 * 8;

	codedWidth = (s_str.width + 15) & ~15;
	codedHeight = (s_str.height + 15) & ~15;

	for (baseX = 0; baseX < codedWidth; baseX += 16)
	{
		for (baseY = 0; baseY < codedHeight; baseY += 16)
		{
			if (NativeSTR_DecodeMacroblock(&br, quant, baseX, baseY) == 0)
			{
				return 0;
			}
		}
	}

	return 1;
}

internal s32 NativeSTR_ParseSectorHeader(const u8 *sector, s32 headerOffset, struct NativeSTRSectorHeader *header)
{
	const u8 *src = &sector[headerOffset];

	header->id = NativeSTR_ReadLE32(&src[0]);
	header->chunkIndex = NativeSTR_ReadLE16(&src[4]);
	header->chunkCount = NativeSTR_ReadLE16(&src[6]);
	header->frameIndex = NativeSTR_ReadLE32(&src[8]);
	header->frameSize = NativeSTR_ReadLE32(&src[12]);
	header->width = NativeSTR_ReadLE16(&src[16]);
	header->height = NativeSTR_ReadLE16(&src[18]);

	return (header->id == NATIVE_STR_ID) && (header->chunkCount > 0) && (header->chunkCount <= NATIVE_STR_MAX_FRAME_SECTORS) &&
	       (header->frameSize <= NATIVE_STR_MAX_FRAME_BYTES) && (header->width > 0) && (header->width <= NATIVE_STR_MAX_WIDTH) && (header->height > 0) &&
	       (header->height <= NATIVE_STR_MAX_HEIGHT);
}

internal void NativeSTR_CopySectorPayload(const u8 *sector, s32 headerOffset, const struct NativeSTRSectorHeader *header, s32 payloadSize, s32 *copied)
{
	s32 remaining = (s32)header->frameSize - *copied;
	s32 copyBytes = (remaining < payloadSize) ? remaining : payloadSize;

	if (copyBytes <= 0)
	{
		return;
	}

	memcpy(&s_str.frameData[*copied], &sector[headerOffset + NATIVE_STR_SECTOR_HEADER], (size_t)copyBytes);
	*copied += copyBytes;
}

internal s32 NativeSTR_ReadNextFrameFromFile(void)
{
	u8 sector[NATIVE_STR_EXTRACTED_SECTOR_SIZE];
	struct NativeSTRSectorHeader firstHeader;
	s32 copied = 0;
	s32 chunk;

	if (fread(sector, 1, sizeof(sector), s_str.file) != sizeof(sector))
	{
		return 0;
	}

	if ((NativeSTR_ParseSectorHeader(sector, 0, &firstHeader) == 0) || (firstHeader.chunkIndex != 0))
	{
		return 0;
	}

	s_str.width = firstHeader.width;
	s_str.height = firstHeader.height;
	s_str.frameSize = (s32)firstHeader.frameSize;

	for (chunk = 0; chunk < firstHeader.chunkCount; chunk++)
	{
		struct NativeSTRSectorHeader header;
		if (chunk != 0)
		{
			if (fread(sector, 1, sizeof(sector), s_str.file) != sizeof(sector))
			{
				return 0;
			}
		}

		if ((NativeSTR_ParseSectorHeader(sector, 0, &header) == 0) || (header.chunkIndex != chunk) || (header.frameIndex != firstHeader.frameIndex) ||
		    (header.frameSize != firstHeader.frameSize))
		{
			return 0;
		}

		NativeSTR_CopySectorPayload(sector, 0, &header, NATIVE_STR_EXTRACTED_SECTOR_PAYLOAD, &copied);
	}

	return copied == (s32)firstHeader.frameSize;
}

internal s32 NativeSTR_ReadNextCdRecord(u8 *sector, struct NativeSTRSectorHeader *header)
{
	while (fread(sector, 1, NATIVE_STR_CD_SECTOR_SIZE, s_str.file) == NATIVE_STR_CD_SECTOR_SIZE)
	{
		// NOTE(aalhendi): TEST.STR is the raw CD/XA scrapbook stream. Each
		// 0x920-byte record has an XA subheader before the STR chunk header,
		// unlike extracted track-preview STR files. The video chunk still only
		// carries the logical 0x800-byte STR sector, so payload copies stay at
		// 0x7e0 after that header.
		if (NativeSTR_ParseSectorHeader(sector, NATIVE_STR_CD_SUBHEADER_SIZE, header) != 0)
		{
			return 1;
		}
	}

	return 0;
}

internal s32 NativeSTR_ReadNextFrameFromCdStream(void)
{
	u8 sector[NATIVE_STR_MAX_RECORD_SIZE];
	struct NativeSTRSectorHeader firstHeader;
	s32 copied = 0;
	s32 expectedChunk;

	do
	{
		if (NativeSTR_ReadNextCdRecord(sector, &firstHeader) == 0)
		{
			return 0;
		}
	} while (firstHeader.chunkIndex != 0);

	s_str.width = firstHeader.width;
	s_str.height = firstHeader.height;
	s_str.frameSize = (s32)firstHeader.frameSize;
	NativeSTR_CopySectorPayload(sector, NATIVE_STR_CD_SUBHEADER_SIZE, &firstHeader, NATIVE_STR_CD_SECTOR_PAYLOAD, &copied);

	for (expectedChunk = 1; expectedChunk < firstHeader.chunkCount; expectedChunk++)
	{
		struct NativeSTRSectorHeader header;

		for (;;)
		{
			if (NativeSTR_ReadNextCdRecord(sector, &header) == 0)
			{
				return 0;
			}

			if ((header.frameIndex == firstHeader.frameIndex) && (header.frameSize == firstHeader.frameSize) && (header.chunkIndex == expectedChunk))
			{
				NativeSTR_CopySectorPayload(sector, NATIVE_STR_CD_SUBHEADER_SIZE, &header, NATIVE_STR_CD_SECTOR_PAYLOAD, &copied);
				break;
			}

			if ((header.chunkIndex == 0) && (header.frameIndex != firstHeader.frameIndex))
			{
				return 0;
			}
		}
	}

	return copied == (s32)firstHeader.frameSize;
}

internal s32 NativeSTR_ReadNextFrame(void)
{
	s32 tries;
	s32 maxTries = (s_str.loop != 0) ? 2 : 1;

	for (tries = 0; tries < maxTries; tries++)
	{
		if ((s_str.frameLimit > 0) && (s_str.frameIndex >= s_str.frameLimit))
		{
			if (s_str.loop == 0)
			{
				return 0;
			}

			rewind(s_str.file);
			s_str.frameIndex = 0;
		}

		if (((s_str.format == NATIVE_STR_FORMAT_CD_STREAM) ? NativeSTR_ReadNextFrameFromCdStream() : NativeSTR_ReadNextFrameFromFile()) != 0)
		{
			s_str.frameIndex++;
			return 1;
		}

		if (s_str.loop == 0)
		{
			return 0;
		}

		rewind(s_str.file);
		s_str.frameIndex = 0;
	}

	return 0;
}

internal s32 NativeSTR_ResolveBigfilePath(s32 bigfileIndex, char *dst, s32 dstCount)
{
	FILE *file;
	char line[256];
	s32 index = 0;

	if ((dst == NULL) || (dstCount <= 0))
	{
		return 0;
	}

	file = NativeAssets_Open("bigfile.txt", "r");
	if (file == NULL)
	{
		return 0;
	}

	while (fgets(line, sizeof(line), file) != NULL)
	{
		if (index == bigfileIndex)
		{
			NativeStr8 lineText = NativeStr8_FromCString(line);
			char relativePath[256];
			int written;

			while ((lineText.len != 0) && ((lineText.ptr[lineText.len - 1u] == '\r') || (lineText.ptr[lineText.len - 1u] == '\n')))
			{
				lineText.len--;
			}

			written = snprintf(relativePath, sizeof(relativePath), "bigfile/%.*s", (int)lineText.len, (const char *)lineText.ptr);
			if ((written <= 0) || ((size_t)written >= sizeof(relativePath)) || !NativeAssets_ResolvePath(relativePath, dst, (size_t)dstCount))
			{
				fclose(file);
				return 0;
			}

			fclose(file);
			return 1;
		}

		index++;
	}

	fclose(file);
	return 0;
}

s32 NativeSTR_StartTrackPreview(s32 bigfileIndex, s32 frameCount)
{
	char path[NATIVE_STR_PATH_MAX];

	if ((s_str.active != 0) && (s_str.bigfileIndex == bigfileIndex))
	{
		return 1;
	}

	NativeSTR_Stop();

	if (NativeSTR_ResolveBigfilePath(bigfileIndex, path, sizeof(path)) == 0)
	{
		return 0;
	}

	s_str.file = fopen(path, "rb");
	if (s_str.file == NULL)
	{
		return 0;
	}

	s_str.active = 1;
	s_str.format = NATIVE_STR_FORMAT_EXTRACTED;
	s_str.loop = 1;
	s_str.bigfileIndex = bigfileIndex;
	s_str.frameIndex = 0;
	s_str.frameLimit = frameCount;
	return 1;
}

s32 NativeSTR_StartScrapbook(void)
{
	if ((s_str.active != 0) && (s_str.format == NATIVE_STR_FORMAT_CD_STREAM))
	{
		return 1;
	}

	NativeSTR_Stop();

	s_str.file = NativeAssets_Open(NATIVE_STR_SCRAPBOOK_PATH, "rb");
	if (s_str.file == NULL)
	{
		return 0;
	}

	s_str.active = 1;
	s_str.format = NATIVE_STR_FORMAT_CD_STREAM;
	s_str.loop = 0;
	s_str.bigfileIndex = -1;
	s_str.frameIndex = 0;
	s_str.frameLimit = NATIVE_STR_SCRAPBOOK_FRAME_COUNT;
	return 1;
}

void NativeSTR_Stop(void)
{
	if (s_str.file != NULL)
	{
		fclose(s_str.file);
		s_str.file = NULL;
	}

	s_str.active = 0;
	s_str.format = NATIVE_STR_FORMAT_EXTRACTED;
	s_str.loop = 0;
	s_str.bigfileIndex = -1;
	s_str.frameIndex = 0;
	s_str.frameLimit = 0;
	s_str.width = 0;
	s_str.height = 0;
	s_str.frameSize = 0;
}

s32 NativeSTR_UploadNextFrame(s32 dstX, s32 dstY)
{
	RECT16 rect;

	if ((s_str.active == 0) || (s_str.file == NULL))
	{
		return 0;
	}

	if ((NativeSTR_ReadNextFrame() == 0) || (NativeSTR_DecodeFrame() == 0))
	{
		return 0;
	}

	rect.x = dstX;
	rect.y = dstY;
	rect.w = s_str.width;
	rect.h = s_str.height;
	LoadImage(&rect, (uint32_t *)s_str.rgb555);
	// NOTE(aalhendi): Track-preview STR draws these uploaded pixels as same-pass
	// textured primitives. Retail LoadImage is GPU-visible immediately; refresh
	// the host VRAM texture at that boundary.
	NativeRenderer_UpdateVRAM();
	return 1;
}
