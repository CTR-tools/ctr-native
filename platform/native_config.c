#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

NativeConfig g_config = {1};

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
        printf("[Config] Raw line: '%s'\n", p);


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

		if (strcmp(section, "General") != 0)
			continue;

		char *eq = strchr(p, '=');
		if (!eq)
			continue;

		*eq = '\0';
		char *key = trimWhitespace(p);
		char *value = trimWhitespace(eq + 1);

        printf("[Config] Key='%s' Value='%s'\n", key, value);

		if (strcmp(key, "skip_intro") == 0) {
			g_config.skipIntro = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            printf("[Config] skip_intro = %d\n", g_config.skipIntro);
        }

	}

	fclose(f);

}