#include <platform/native_custom_cup_storage.h>
#include <platform/native_assets.h>
#include <SDL3/SDL.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { NATIVE_CUP_PATH_SIZE = 1024, NATIVE_CUP_LINE_SIZE = 8192, NATIVE_CUP_ID_ATTEMPTS = 16 };

static bool NativeCup_Directory(char *directory, size_t size)
{
	const char *base = NativeAssets_GetBaseDir();
	if (base == NULL || base[0] == '\0') { return false; }
	int length = snprintf(directory, size, "%s/custom-cups", base);
	return length >= 0 && (size_t)length < size;
}

static bool NativeCup_Path(char *path, size_t size, const char *id)
{
	char directory[NATIVE_CUP_PATH_SIZE];
	if (id == NULL || strlen(id) != NATIVE_CUSTOM_CUP_ID_SIZE - 1 || !NativeCup_Directory(directory, sizeof(directory)))
	{ return false; }
	for (size_t i = 0; id[i] != '\0'; i++)
	{
		if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f'))) { return false; }
	}
	int length = snprintf(path, size, "%s/%s.ini", directory, id);
	return length >= 0 && (size_t)length < size;
}

static int NativeCup_Hex(char c)
{
	if (c >= '0' && c <= '9') { return c - '0'; }
	if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
	if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
	return -1;
}

/* Escape delimiters and controls so every track occupies exactly one line. */
static bool NativeCup_Decode(char *value)
{
	char *out = value;
	while (*value != '\0')
	{
		if (*value == '%')
		{
			if (value[1] == '\0' || value[2] == '\0') { return false; }
			int high = NativeCup_Hex(value[1]), low = NativeCup_Hex(value[2]);
			if (high < 0 || low < 0 || (high == 0 && low == 0)) { return false; }
			*out++ = (char)(high * 16 + low);
			value += 3;
		}
		else { *out++ = *value++; }
	}
	*out = '\0';
	return true;
}

static bool NativeCup_WriteValue(FILE *file, const char *value)
{
	for (const unsigned char *p = (const unsigned char *)value; *p != 0; p++)
	{
		if (*p == '%' || *p == '|' || *p < 0x20 || *p == 0x7f)
		{
			if (fprintf(file, "%%%02X", *p) < 0) { return false; }
		}
		else if (fputc(*p, file) == EOF) { return false; }
	}
	return true;
}

static bool NativeCup_ReadTrack(struct CustomCup *cup, char *value)
{
	char *separator = strchr(value, '|');
	int laps = CUSTOM_CUP_DEFAULT_LAPS;
	if (separator != NULL)
	{
		*separator++ = '\0';
		char *end;
		errno = 0;
		long parsed = strtol(separator, &end, 10);
		if (errno != 0 || *separator == '\0' || *end != '\0' || parsed < 1 || parsed > INT_MAX) { return false; }
		laps = (int)parsed;
	}
	return NativeCup_Decode(value) && CustomCup_InsertTrack(cup, cup->trackCount, value, laps);
}

static enum NativeCustomCupStorageResult NativeCup_Read(FILE *file, struct CustomCup *cup)
{
	char line[NATIVE_CUP_LINE_SIZE];
	bool section = false, name = false;
	for (;;)
	{
		size_t length = 0;
		int c;
		while ((c = fgetc(file)) != EOF && c != '\n')
		{
			if (c == 0 || length == sizeof(line) - 1) { return NATIVE_CUSTOM_CUP_INVALID; }
			line[length++] = (char)c;
		}
		if (c == EOF && length == 0) { break; }
		if (length > 0 && line[length - 1] == '\r') { length--; }
		line[length] = '\0';
		if (line[0] == '\0' || line[0] == ';' || line[0] == '#') { continue; }
		if (!section && strcmp(line, "[cup]") == 0) { section = true; }
		else if (section && !name && strncmp(line, "name=", 5) == 0)
		{
			char *value = line + 5;
			if (!NativeCup_Decode(value) || strlen(value) >= sizeof(cup->name)) { return NATIVE_CUSTOM_CUP_INVALID; }
			strcpy(cup->name, value);
			name = true;
		}
		else if (section && strncmp(line, "track=", 6) == 0)
		{
			if (!NativeCup_ReadTrack(cup, line + 6)) { return NATIVE_CUSTOM_CUP_INVALID; }
		}
		else { return NATIVE_CUSTOM_CUP_INVALID; }
	}
	if (ferror(file)) { return NATIVE_CUSTOM_CUP_IO_ERROR; }
	return section && name && CustomCup_IsValid(cup) ? NATIVE_CUSTOM_CUP_OK : NATIVE_CUSTOM_CUP_INVALID;
}

enum NativeCustomCupStorageResult NativeCustomCup_Load(const char *id, struct CustomCup *cup)
{
	char path[NATIVE_CUP_PATH_SIZE];
	if (cup == NULL || !NativeCup_Path(path, sizeof(path), id)) { return NATIVE_CUSTOM_CUP_INVALID; }
	FILE *file = fopen(path, "rb");
	if (file == NULL) { return errno == ENOENT ? NATIVE_CUSTOM_CUP_NOT_FOUND : NATIVE_CUSTOM_CUP_IO_ERROR; }
	struct CustomCup loaded = {0};
	enum NativeCustomCupStorageResult result = NativeCup_Read(file, &loaded);
	if (fclose(file) != 0 && result == NATIVE_CUSTOM_CUP_OK) { result = NATIVE_CUSTOM_CUP_IO_ERROR; }
	if (result == NATIVE_CUSTOM_CUP_OK)
	{
		CustomCup_Clear(cup);
		*cup = loaded;
	}
	else { CustomCup_Clear(&loaded); }
	return result;
}

enum NativeCustomCupStorageResult NativeCustomCup_Save(char id[NATIVE_CUSTOM_CUP_ID_SIZE], const struct CustomCup *cup)
{
	char directory[NATIVE_CUP_PATH_SIZE], path[NATIVE_CUP_PATH_SIZE], temporary[NATIVE_CUP_PATH_SIZE];
	char target[NATIVE_CUSTOM_CUP_ID_SIZE];
	if (id == NULL || !CustomCup_IsValid(cup)) { return NATIVE_CUSTOM_CUP_INVALID; }
	if (!NativeCup_Directory(directory, sizeof(directory)) || !SDL_CreateDirectory(directory)) { return NATIVE_CUSTOM_CUP_IO_ERROR; }
	if (id[0] != '\0')
	{
		struct CustomCup existing = {0};
		enum NativeCustomCupStorageResult result = NativeCustomCup_Load(id, &existing);
		CustomCup_Clear(&existing);
		if (result != NATIVE_CUSTOM_CUP_OK) { return result; }
		memcpy(target, id, sizeof(target));
	}
	else
	{
		bool found = false;
		for (int i = 0; i < NATIVE_CUP_ID_ATTEMPTS; i++)
		{
			snprintf(target, sizeof(target), "%08x%08x%08x%08x", SDL_rand_bits(), SDL_rand_bits(), SDL_rand_bits(), SDL_rand_bits());
			if (!NativeCup_Path(path, sizeof(path), target)) { return NATIVE_CUSTOM_CUP_IO_ERROR; }
			SDL_PathInfo info;
			if (!SDL_GetPathInfo(path, &info)) { found = true; break; }
		}
		if (!found) { return NATIVE_CUSTOM_CUP_IO_ERROR; }
	}
	if (!NativeCup_Path(path, sizeof(path), target)) { return NATIVE_CUSTOM_CUP_INVALID; }
	int n = snprintf(temporary, sizeof(temporary), "%s.%llu.tmp", path, (unsigned long long)SDL_GetTicksNS());
	if (n < 0 || (size_t)n >= sizeof(temporary)) { return NATIVE_CUSTOM_CUP_IO_ERROR; }
	SDL_PathInfo temporaryInfo;
	if (SDL_GetPathInfo(temporary, &temporaryInfo)) { return NATIVE_CUSTOM_CUP_IO_ERROR; }
	FILE *file = fopen(temporary, "wb");
	if (file == NULL) { return NATIVE_CUSTOM_CUP_IO_ERROR; }
	bool ok = fputs("[cup]\nname=", file) >= 0 && NativeCup_WriteValue(file, cup->name) && fputc('\n', file) != EOF;
	for (int i = 0; ok && i < cup->trackCount; i++)
	{
		if (strlen(cup->tracks[i]) > (NATIVE_CUP_LINE_SIZE - 32) / 3) { ok = false; break; }
		ok = fputs("track=", file) >= 0 && NativeCup_WriteValue(file, cup->tracks[i]) && fprintf(file, "|%d\n", cup->laps[i]) >= 0;
	}
	if (fclose(file) != 0) { ok = false; }
	if (ok) { ok = SDL_RenamePath(temporary, path); }
	if (!ok) { SDL_RemovePath(temporary); return NATIVE_CUSTOM_CUP_IO_ERROR; }
	memcpy(id, target, sizeof(target));
	return NATIVE_CUSTOM_CUP_OK;
}
