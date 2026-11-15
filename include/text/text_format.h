#ifndef CTR_NATIVE_TEXT_FORMAT_H
#define CTR_NATIVE_TEXT_FORMAT_H

#include <stddef.h>
#include <string.h>

/* Values are already formatted text, never printf format strings. */
struct TextFormatValue
{
	const char *name;
	const char *value;
};

static inline int TextFormat_Apply(char *out, size_t capacity, const char *format,
                                   const struct TextFormatValue *values, size_t count)
{
	if (out == NULL || capacity == 0 || format == NULL) { return 0; }
	size_t used = 0;
	while (*format != '\0')
	{
		const char *text = format;
		size_t length = 1;
		if (*format == '{')
		{
			const char *end = strchr(format + 1, '}');
			if (end == NULL) { out[0] = '\0'; return 0; }
			size_t nameLength = (size_t)(end - format - 1);
			size_t i;
			for (i = 0; i < count; i++)
			{
				if (strlen(values[i].name) == nameLength && memcmp(values[i].name, format + 1, nameLength) == 0) { break; }
			}
			if (i == count) { out[0] = '\0'; return 0; }
			text = values[i].value;
			length = strlen(text);
			format = end + 1;
		}
		else
		{
			format++;
		}
		if (length >= capacity - used) { out[0] = '\0'; return 0; }
		memcpy(out + used, text, length);
		used += length;
	}
	out[used] = '\0';
	return 1;
}

#endif
