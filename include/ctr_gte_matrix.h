#ifndef CTR_GTE_MATRIX_H
#define CTR_GTE_MATRIX_H

#include <ctr_gte.h>

// NOTE(aalhendi): SDK matrix transfers pair loads through T4-T6. The typed
// read-only operand describes their inputs without a global memory clobber.
static inline void CTR_GteLoadRotMatrix(const MATRIX *matrix)
{
#ifdef CTR_NATIVE
	gte_SetRotMatrix(matrix);
#else
	__asm__ volatile("lw $12,0(%0)\n\t"
	                 "lw $13,4(%0)\n\t"
	                 "ctc2 $12,$0\n\t"
	                 "ctc2 $13,$1\n\t"
	                 "lw $12,8(%0)\n\t"
	                 "lw $13,12(%0)\n\t"
	                 "lw $14,16(%0)\n\t"
	                 "ctc2 $12,$2\n\t"
	                 "ctc2 $13,$3\n\t"
	                 "ctc2 $14,$4"
	                 :
	                 : "r"(matrix), "m"(*matrix)
	                 : "$12", "$13", "$14");
#endif
}

static inline void CTR_GteLoadTransMatrix(const MATRIX *matrix)
{
#ifdef CTR_NATIVE
	gte_SetTransMatrix(matrix);
#else
	__asm__ volatile("lw $12,20(%0)\n\t"
	                 "lw $13,24(%0)\n\t"
	                 "ctc2 $12,$5\n\t"
	                 "lw $14,28(%0)\n\t"
	                 "ctc2 $13,$6\n\t"
	                 "ctc2 $14,$7"
	                 :
	                 : "r"(matrix), "m"(*matrix)
	                 : "$12", "$13", "$14");
#endif
}

#endif
