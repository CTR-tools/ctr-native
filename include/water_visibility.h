#ifndef CTR_NATIVE_WATER_VISIBILITY_H
#define CTR_NATIVE_WATER_VISIBILITY_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Missing default masks mean the water vertices must remain animated. */
static inline void WaterVisibility_CopyDefault(void *destination, const void *source, size_t bytes)
{
	if (bytes == 0) { return; }
	if (source != NULL) { memcpy(destination, source, bytes); }
	else { memset(destination, 0xff, bytes); }
}

static inline uint32_t WaterVisibility_NextWord(const uint32_t **list)
{
	if (*list == NULL) { return UINT32_MAX; }
	return *(*list)++;
}

#endif
