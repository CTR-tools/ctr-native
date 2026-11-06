/* Bounded inspection of NTSC-U TrackROM loaders, not a MIPS interpreter.
 * Layout references: CTR-ModSDK symbols/gcc-syms926.txt and PS1_TrackROM.
 * In particular, configurable loaders get their level ID from overlay 230. */
enum
{
	PATCH_MUSIC_EXE_HEADER = 2048,
	PATCH_MUSIC_BANK_TABLE = 0x7414c,
	PATCH_MUSIC_SONG_TABLE = 0x7416c,
	PATCH_MUSIC_LOADER_START = 0x23a00 + PATCH_MUSIC_EXE_HEADER,
	PATCH_MUSIC_LOADER_END = 0x24000 + PATCH_MUSIC_EXE_HEADER,
	PATCH_MUSIC_LEVEL_VALUE = -2,
};

struct PatchMusicBytes
{
	unsigned char *data;
	size_t size;
};

static int64_t PatchMusic_Register(const struct PatchMusicBytes *code, size_t before, unsigned int reg, int depth)
{
	if (reg == 0) { return 0; }
	if (depth > 5) { return -1; }
	size_t start = before > 128 ? before - 128 : 0;
	for (size_t at = before; at >= start + 4;)
	{
		at -= 4;
		uint32_t w = PatchDisc_Read32(code->data + at);
		unsigned int op = w >> 26, rs = w >> 21 & 31, rt = w >> 16 & 31, rd = w >> 11 & 31;
		int imm = (int16_t)w;
		if (op == 3)
		{
			/* Calls preserve s0-s7, but not the argument/temporary registers. */
			if (reg < 16 || reg > 23) { return -1; }
			continue;
		}
		if (op == 2 || (op >= 4 && op <= 7) || (op == 0 && (w & 63) == 8)) { return -1; }
		if (op == 0)
		{
			if (rd != reg) { continue; }
			if (((w & 63) == 33 || (w & 63) == 37) && rt == 0)
			{
				return PatchMusic_Register(code, at, rs, depth + 1);
			}
			return -1;
		}
		if (rt != reg || !((op >= 8 && op <= 15) || (op >= 32 && op <= 38))) { continue; }
		if (op == 15) { return (int64_t)(w & 65535) << 16; }
		if (op == 35 && rs == 16 && imm == 0x1a10) { return PATCH_MUSIC_LEVEL_VALUE; }
		if (op == 9 || op == 13)
		{
			int64_t base = PatchMusic_Register(code, at, rs, depth + 1);
			if (base < 0) { return -1; }
			return op == 9 ? base + imm : base | (w & 65535);
		}
		return -1;
	}
	return -1;
}

static int PatchMusic_ConfigLevel(const struct PatchMusicBytes *menu, uint32_t address)
{
	int level = -1;
	for (size_t at = 0; at + 4 <= menu->size; at += 4)
	{
		uint32_t w = PatchDisc_Read32(menu->data + at);
		if (w >> 26 != 40) { continue; } /* sb value, offset(base) */
		int64_t base = PatchMusic_Register(menu, at, w >> 21 & 31, 0);
		if (base < 0 || base + (int16_t)w != address) { continue; }
		int64_t value = PatchMusic_Register(menu, at, w >> 16 & 31, 0);
		if (value < 0 || value >= 18 || (level >= 0 && level != value)) { return -1; }
		level = (int)value;
	}
	return level;
}

static int PatchMusic_ComparedLevel(const struct PatchMusicBytes *exe, const struct PatchMusicBytes *menu,
                                  size_t branch, unsigned int reg)
{
	int64_t value = PatchMusic_Register(exe, branch, reg, 0);
	if (value >= 0 && value < 18) { return (int)value; }
	/* A byte load is deliberately not resolved from the EXE's initial value:
	 * the menu initializes this variable before loading the chosen circuit. */
	for (size_t at = branch; at >= 4 && branch - at < 32;)
	{
		at -= 4;
		uint32_t w = PatchDisc_Read32(exe->data + at);
		if ((w >> 16 & 31) != reg) { continue; }
		if (w >> 26 == 36)
		{
			int64_t base = PatchMusic_Register(exe, at, w >> 21 & 31, 0);
			if (base < 0 || base + (int16_t)w < 0 || base + (int16_t)w > UINT32_MAX) { return -1; }
			return PatchMusic_ConfigLevel(menu, (uint32_t)(base + (int16_t)w));
		}
		if ((w >> 26 >= 8 && w >> 26 <= 15) || (w >> 26 >= 32 && w >> 26 <= 38)) { break; }
	}
	return -1;
}

static int PatchMusic_ExeValid(const struct PatchMusicBytes *exe)
{
	return exe->size > PATCH_MUSIC_SONG_TABLE + 18 && memcmp(exe->data, "PS-X EXE", 8) == 0 &&
	       PatchDisc_Read32(exe->data + 24) == UINT32_C(0x80010000) &&
	       PatchDisc_Read32(exe->data + 28) <= exe->size - PATCH_MUSIC_EXE_HEADER;
}

static int PatchMusic_Level(const struct PatchMusicBytes *original, const struct PatchMusicBytes *exe,
                            const struct PatchMusicBytes *menu, int slot)
{
	if (!PatchMusic_ExeValid(original) || !PatchMusic_ExeValid(exe)) { return -1; }
	/* These are the two retail table lookup paths in Music_AsyncParseBanks.
	 * A patched audio routine requires a new recognizer, not guessed offsets. */
	if (memcmp(original->data + 0x1deec + 2048, exe->data + 0x1deec + 2048, 32) != 0 ||
	    memcmp(original->data + 0x1e26c + 2048, exe->data + 0x1e26c + 2048, 32) != 0)
	{
		return -1;
	}
	if (memcmp(original->data + PATCH_MUSIC_LOADER_START, exe->data + PATCH_MUSIC_LOADER_START,
	           PATCH_MUSIC_LOADER_END - PATCH_MUSIC_LOADER_START) == 0)
	{
		return slot;
	}
	int level = -1, matches = 0;
	for (size_t at = PATCH_MUSIC_LOADER_START; at + 64 < PATCH_MUSIC_LOADER_END; at += 4)
	{
		if (PatchDisc_Read32(exe->data + at) != (UINT32_C(0x24060000) | (unsigned int)slot * 8)) { continue; }
		/* addiu a2,zero,VRM; addiu a1,zero,3; jal LOAD_AppendQueue */
		if (PatchDisc_Read32(exe->data + at + 4) != UINT32_C(0x24050003) ||
		    PatchDisc_Read32(exe->data + at + 8) != UINT32_C(0x0c00cb4c)) { continue; }
		size_t lev = at + 4;
		for (; lev < at + 48; lev += 4)
		{
			if (PatchDisc_Read32(exe->data + lev) == (UINT32_C(0x24060000) | ((unsigned int)slot * 8 + 1)) &&
			    PatchDisc_Read32(exe->data + lev + 4) == UINT32_C(0x24050002) &&
			    PatchDisc_Read32(exe->data + lev + 8) == UINT32_C(0x0c00cb4c)) { break; }
		}
		if (lev == at + 48) { continue; }
		for (size_t branch = at - 4; branch + 64 >= at; branch -= 4)
		{
			uint32_t w = PatchDisc_Read32(exe->data + branch);
			if (w >> 26 != 5) { continue; }
			int target = (int)branch + 4 + (int16_t)w * 4;
			if (target <= (int)lev + 12 || target > (int)at + 256) { continue; }
			unsigned int lhs = w >> 21 & 31, rhs = w >> 16 & 31;
			if (PatchMusic_Register(exe, branch, lhs, 0) != PATCH_MUSIC_LEVEL_VALUE) { continue; }
			int candidate = PatchMusic_ComparedLevel(exe, menu, branch, rhs);
			if (candidate < 0) { return -1; }
			level = candidate;
			matches++;
			break;
		}
	}
	return matches == 1 ? level : -1;
}

/* TrackROM may index a table in ALL.BIN using the selected physical slot.
 * Match the five instructions that form that lookup, not a whole function:
 * lbu index; lui table; addiu table; addu table,index; lbu level. */
static int PatchMusic_AllBinLevel(const struct PatchMusicBytes *exe, const struct PatchMusicBytes *all, int slot)
{
	enum { TRACK_COUNT = 18, MIPS_LBU = 36, MIPS_LUI = 15, MIPS_ADDIU = 9, MIPS_ADDU = 33 };
	const uint32_t allBase = UINT32_C(0x807fe000);
	if (slot < 0 || slot >= TRACK_COUNT || !PatchMusic_ExeValid(exe) || all->data == NULL)
	{
		return -1;
	}
	int level = -1;
	for (size_t at = PATCH_MUSIC_EXE_HEADER; at + 20 <= exe->size; at += 4)
	{
		uint32_t index = PatchDisc_Read32(exe->data + at);
		uint32_t upper = PatchDisc_Read32(exe->data + at + 4);
		uint32_t lower = PatchDisc_Read32(exe->data + at + 8);
		uint32_t add = PatchDisc_Read32(exe->data + at + 12);
		uint32_t load = PatchDisc_Read32(exe->data + at + 16);
		unsigned int indexReg = index >> 16 & 31, tableReg = upper >> 16 & 31;
		if (index >> 26 != MIPS_LBU || upper >> 26 != MIPS_LUI || lower >> 26 != MIPS_ADDIU ||
		    (lower >> 21 & 31) != tableReg || (lower >> 16 & 31) != tableReg ||
		    add >> 26 != 0 || (add & 63) != MIPS_ADDU || (add >> 21 & 31) != indexReg ||
		    (add >> 16 & 31) != tableReg || (add >> 11 & 31) != indexReg ||
		    load >> 26 != MIPS_LBU || (load >> 21 & 31) != indexReg || (int16_t)load != 0)
		{
			continue;
		}
		uint32_t address = ((upper & 0xffff) << 16) + (int16_t)lower;
		if (address < allBase || (uint64_t)(address - allBase) + TRACK_COUNT > all->size) { continue; }
		int candidate = all->data[address - allBase + slot];
		if (candidate >= TRACK_COUNT || (level >= 0 && level != candidate)) { return -1; }
		level = candidate;
	}
	return level;
}
