#include <shared/ctr_test.h>
#include <platform/graphics/native_text_vertices.c>

int main(void)
{
	float left = 0.0f;
	float right = 0.0f;
	int primitives[2];
	CTR_TEST_SECTION("text vertices follow primitive lifetime");
	CTR_TEST_CHECK(!NativeTextVertices_Get(&primitives[0], &left, &right), "an unregistered primitive has no precise vertices");
	CTR_TEST_CHECK(NativeTextVertices_Set(&primitives[0], 1.25f, 5.75f), "a primitive can register precise edges");
	CTR_TEST_CHECK(NativeTextVertices_Get(&primitives[0], &left, &right) && left == 1.25f && right == 5.75f,
	               "the registered edges are returned exactly");
	CTR_TEST_CHECK(!NativeTextVertices_Get(&primitives[1], &left, &right), "another primitive does not inherit those edges");
	CTR_TEST_CHECK(NativeTextVertices_Set(&primitives[0], 2.5f, 6.5f), "a primitive can update its edges");
	CTR_TEST_CHECK(NativeTextVertices_Get(&primitives[0], &left, &right) && left == 2.5f && right == 6.5f,
	               "the updated edges replace the old values");
	NativeTextVertices_EndFrame();
	CTR_TEST_CHECK(!NativeTextVertices_Get(&primitives[0], &left, &right), "edges expire after the frame is consumed");

	CTR_TEST_SECTION("hash collisions and capacity");
	for (uintptr_t i = 0; i < NATIVE_TEXT_VERTEX_SLOT_COUNT; i++)
	{
		const void *key = (const void *)(4 * (1 + i * NATIVE_TEXT_VERTEX_SLOT_COUNT));
		CTR_TEST_CHECK(NativeTextVertices_Set(key, (float)i, (float)i + 1), "colliding primitive addresses remain distinct");
	}
	const void *first = (const void *)4;
	const void *last = (const void *)(4 * (1 + (NATIVE_TEXT_VERTEX_SLOT_COUNT - 1) * NATIVE_TEXT_VERTEX_SLOT_COUNT));
	CTR_TEST_CHECK(NativeTextVertices_Get(first, &left, &right) && left == 0.0f, "the first colliding entry remains accessible");
	CTR_TEST_CHECK(NativeTextVertices_Get(last, &left, &right) && left == NATIVE_TEXT_VERTEX_SLOT_COUNT - 1,
	               "the last colliding entry remains accessible");
	CTR_TEST_CHECK(!NativeTextVertices_Set(&primitives[0], 0, 1), "a full table reports failure for the integer fallback");
	NativeTextVertices_EndFrame();
	CTR_TEST_CHECK(NativeTextVertices_Set(&primitives[0], 3, 4), "slots are reusable in the next frame");
	return CtrTest_Summary();
}
