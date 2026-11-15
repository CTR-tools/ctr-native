#include <platform/graphics/native_text_vertices.h>
#include <stdint.h>
#include <string.h>

enum { NATIVE_TEXT_VERTEX_SLOT_COUNT = 8192 };

typedef struct
{
	const void *primitive;
	float left;
	float right;
	u32 generation;
} NativeTextVertexSlot;

static NativeTextVertexSlot s_nativeTextVertexSlots[NATIVE_TEXT_VERTEX_SLOT_COUNT];
static u32 s_nativeTextVertexGeneration = 1;

static NativeTextVertexSlot *NativeTextVertices_Find(const void *primitive, b32 create)
{
	u32 index = ((uintptr_t)primitive >> 2) * 2654435761u % NATIVE_TEXT_VERTEX_SLOT_COUNT;
	for (int i = 0; i < NATIVE_TEXT_VERTEX_SLOT_COUNT; i++)
	{
		NativeTextVertexSlot *slot = &s_nativeTextVertexSlots[(index + i) % NATIVE_TEXT_VERTEX_SLOT_COUNT];
		if (slot->generation != s_nativeTextVertexGeneration)
		{
			return create ? slot : NULL;
		}
		if (slot->primitive == primitive)
		{
			return slot;
		}
	}
	return NULL;
}

b32 NativeTextVertices_Set(const void *primitive, float left, float right)
{
	NativeTextVertexSlot *slot = NativeTextVertices_Find(primitive, true);
	if (slot == NULL)
	{
		return false;
	}
	slot->primitive = primitive;
	slot->left = left;
	slot->right = right;
	slot->generation = s_nativeTextVertexGeneration;
	return true;
}

b32 NativeTextVertices_Get(const void *primitive, float *left, float *right)
{
	NativeTextVertexSlot *slot = NativeTextVertices_Find(primitive, false);
	if (slot == NULL)
	{
		return false;
	}
	*left = slot->left;
	*right = slot->right;
	return true;
}

void NativeTextVertices_EndFrame(void)
{
	if (++s_nativeTextVertexGeneration == 0)
	{
		memset(s_nativeTextVertexSlots, 0, sizeof(s_nativeTextVertexSlots));
		s_nativeTextVertexGeneration = 1;
	}
}
