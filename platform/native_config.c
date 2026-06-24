#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

NativeConfig g_config = {1, 1};

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

	}

	fclose(f);

}