#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

NativeConfig g_config = {false, false, 100, false};
int numConfigOptions = 4;
int numConfigSections = 4;

static bool ParseBool(const char *s)
{
    return strcmp(s, "true") == 0 || strcmp(s, "1") == 0;
}

typedef struct {
    const char *section;
    const char *key;
    bool *field;
} ConfigBoolEntry;

static const ConfigBoolEntry s_boolEntries[] = {
    {"General",   "skip_intro", &g_config.skipIntro},
    {"Adventure", "skip_hints", &g_config.skipHints},
	{"Unlocks", "unlock_all_characters", &g_config.unlockAllCharacters}
};

typedef struct {
    const char *section;
    const char *key;
    int *field;
} ConfigIntEntry;

static const ConfigIntEntry s_intEntries[] = {
    {"Vehicle", "speed_stat_multiplier", &g_config.speedMultiplier},
};


static char *trimWhitespace(char *s)
{
	while (isspace((unsigned char)*s))
		s++;
	if (*s == '\0')
		return s;
	char *end = s + strlen(s) - 1;
	while (end > s && isspace((unsigned char)*end))
		end--;
	*(end + 1) = '\0';
	return s;
}

void NativeConfig_Load(void)
{
    printf("[Config] Base:       %s\n", NativeAssets_GetBaseDir());
	FILE *f = fopen("build/config.ini", "r");
	if (!f) {
        printf("[Config] config.ini NOT FOUND (CWD is not base dir)\n");
		return;
    }

    printf("[Config] config.ini opened OK\n");
    printf("[Config] speedMultiplier DEFAULT = %d\n", g_config.speedMultiplier);

	char line[256];
	char section[64] = "";

	while (fgets(line, sizeof(line), f))
	{
		char *p = trimWhitespace(line);

		if (*p == '\0' || *p == ';' || *p == '#')
			continue;

		if (*p == '[')
		{
			char *end = strchr(p + 1, ']');
			if (end)
			{
				*end = '\0';
				strncpy(section, p + 1, sizeof(section) - 1);
				section[sizeof(section) - 1] = '\0';
			}
            printf("[Config] Section: [%s]\n", section);
			continue;
		}

		char *eq = strchr(p, '=');
		if (!eq)
			continue;

		*eq = '\0';
		char *key = trimWhitespace(p);
		char *value = trimWhitespace(eq + 1);

		for (int i = 0; i < (int)(sizeof(s_boolEntries) / sizeof(s_boolEntries[0])); i++) {
			if (strcmp(section, s_boolEntries[i].section) == 0 &&
			    strcmp(key, s_boolEntries[i].key) == 0)
			{
				*s_boolEntries[i].field = ParseBool(value);
				printf("[Config] %s/%s = %d\n", s_boolEntries[i].section, s_boolEntries[i].key, *s_boolEntries[i].field);
				break;
			}
		}

		for (int i = 0; i < (int)(sizeof(s_intEntries) / sizeof(s_intEntries[0])); i++) {
			if (strcmp(section, s_intEntries[i].section) == 0 &&
			    strcmp(key, s_intEntries[i].key) == 0)
			{
				*s_intEntries[i].field = atoi(value);
				printf("[Config] %s/%s = %d\n", s_intEntries[i].section, s_intEntries[i].key, *s_intEntries[i].field);
				break;
			}
		}

	}

	fclose(f);

}

void NativeConfig_Save(void)
{
	FILE *f = fopen("build/config.ini", "w");
	if (!f)
		return;

	fprintf(f, "[General]\n");
	fprintf(f, "skip_intro = %s\n", g_config.skipIntro ? "true" : "false");
	fprintf(f, "\n");
	fprintf(f, "[Adventure]\n");
	fprintf(f, "skip_hints = %s\n", g_config.skipHints ? "true" : "false");
	fprintf(f, "\n");
	fprintf(f, "[Vehicle]\n");
	fprintf(f, "speed_stat_multiplier = %d\n", g_config.speedMultiplier);
	fprintf(f, "[Unlocks]\n");
	fprintf(f, "unlock_all_characters = %s\n", g_config.unlockAllCharacters ? "true" : "false");

	fclose(f);
}