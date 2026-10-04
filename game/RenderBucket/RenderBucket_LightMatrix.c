#include <common.h>

static void RenderBucket_GteMulLightMatrixColumns(u32 *m0, u32 *m1, u32 *m2, u32 *m3, u32 *m4)
{
	// NOTE(aalhendi): The retail kernel consumes and returns t3-t7, not five
	// pointers. Snapshot all inputs before the call, then keep ordered stores
	// for the native pointer interface's potentially aliased output words.
#ifdef CTR_NATIVE
	Unknown_8006c558(m0, m1, m2, m3, m4);
#else
	register u32 t3 CTR_PSX_REGISTER("$11") = *m0;
	register u32 t4 CTR_PSX_REGISTER("$12") = *m1;
	register u32 t5 CTR_PSX_REGISTER("$13") = *m2;
	register u32 t6 CTR_PSX_REGISTER("$14") = *m3;
	register u32 t7 CTR_PSX_REGISTER("$15") = *m4;
	__asm__ volatile(".set\tnoreorder\n\tjal Unknown_8006c558\n\tnop\n\t.set\treorder"
	                 : "+r"(t3), "+r"(t4), "+r"(t5), "+r"(t6), "+r"(t7)
	                 :
	                 : "$3", "$8", "$9", "$31", "memory");
	*m0 = t3;
	*m1 = t4;
	*m2 = t5;
	*m3 = t6;
	*m4 = t7;
#endif
}
