#ifndef CTR_COMPILER_H
#define CTR_COMPILER_H

#if defined(CTR_NATIVE)
#include <stdlib.h>
#endif

// NOTE(aalhendi): The MSVC C runtime exposes non-standard min/max macros in C mode. They collide with the project's typed helpers even when NOMINMAX is set.
#if defined(_MSC_VER)
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

#if (defined(__GNUC__) && ((__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 96)))) || defined(__clang__)
#define CTR_MAY_ALIAS __attribute__((may_alias))
#else
#define CTR_MAY_ALIAS
#endif

#if defined(__GNUC__) || defined(__clang__)
#define CTR_PRINTF_FORMAT(fmtArg, firstVararg) __attribute__((format(printf, fmtArg, firstVararg)))
#else
#define CTR_PRINTF_FORMAT(fmtArg, firstVararg)
#endif

#if !defined(CTR_NATIVE)
// NOTE(aalhendi): ASPSX takes the full 20-bit BREAK code, not GNU as's split fields.
#define CTR_TRAP() __asm__ volatile("break 0x400")
#elif defined(__GNUC__) || defined(__clang__)
#define CTR_TRAP() __builtin_trap()
#else
#define CTR_TRAP() abort()
#endif

// NOTE(aalhendi): These constraints isolate GCC 2.8.1 register allocation and
// scheduling requirements from shared game logic. Native expansions preserve
// only their C semantics.
#if defined(CTR_NATIVE)
#define CTR_PSX_MATCH_SECTION(sectionName)
// NOTE(aalhendi): These opt-in contracts describe custom MIPS call frames.
// Native keeps the host ABI; the caller still owns the scratch save protocol.
#define CTR_PSX_UNSAFE_MANUAL_AT_FRAME
#define CTR_PSX_UNSAFE_MANUAL_AT_FULL_FRAME
#define CTR_PSX_ADJACENT_TAIL(targetName)
#define CTR_PSX_PRESERVES_AT24
#define CTR_PSX_REGCALL_S012(readMask)
#if defined(__GNUC__) || defined(__clang__)
#define CTR_PSX_REGISTER(registerName) __attribute__((unused))
#else
#define CTR_PSX_REGISTER(registerName)
#endif
#define CTR_PSX_BIND_VALUE_CLOBBER(value, registerName) ((void)(value))
#define CTR_PSX_DEPEND_VALUE(value, dependency)         ((void)(value), (void)(dependency))
#define CTR_PSX_ORDER_VALUES(value, dependency)         ((void)(value), (void)(dependency))
#define CTR_PSX_DEPEND_MEMORY(pointer, dependency)      ((void)(pointer), (void)(dependency))
#define CTR_PSX_CLOBBER(registerName)                   ((void)0)
#define CTR_PSX_KEEP_VALUE(value)                       ((void)(value))
#define CTR_PSX_KEEP_VALUE_RELAXED(value)               ((void)(value))
#define CTR_PSX_OBSERVE_VALUE(value)                    ((void)(value))
#define CTR_PSX_OBSERVE_MEMORY(value)                   ((void)sizeof(value))
#define CTR_PSX_ZERO_VALUE(value)                       ((value) = 0)
#define CTR_PSX_AND_MASK_NONZERO(result, mask, value)   ((result) = ((mask) & (value)), (result) != 0)
#define CTR_PSX_ZERO_VALUE_AFTER(value, first, second)  ((void)(first), (void)(second), (value) = 0)
#define CTR_PSX_MEMORY_BARRIER()                        ((void)0)
#define CTR_PSX_FORGET_VALUE(value)                     ((void)(value))
#define CTR_PSX_RELOAD(value)                           ((void)(value))
#define CTR_PSX_CAPTURE_REGISTER(value, nativeValue)    ((value) = (nativeValue))
#define CTR_PSX_COPY_VALUE(result, value)               ((result) = (value))
#define CTR_PSX_WITH_RETAIL_PAGE(result, address, offset, nativeValue, expression) ((result) = (nativeValue), (expression))
#define CTR_PSX_PREPARE_QUEUE_ARGS(bigfileOut, typeOut, indexOut, bigfileIn, queueType, indexIn) \
	do                                                                                          \
	{                                                                                           \
		(bigfileOut) = (bigfileIn);                                                               \
		(typeOut) = (queueType);                                                                  \
		(indexOut) = (indexIn);                                                                   \
	} while (0)
#define CTR_PSX_LOAD_SYMBOL_PAGE(page, symbolExpression) \
	do                                                   \
	{                                                    \
		(page) = 0;                                      \
	} while (0)
#define CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(page, symbolExpression, dependency) \
	do                                                                     \
	{                                                                      \
		(page) = 0;                                                        \
		(void)(dependency);                                                \
	} while (0)
#define CTR_PSX_LOAD_WORD_FROM_PAGE(value, page, symbolExpression, nativeValue) \
	do                                                                          \
	{                                                                           \
		(void)sizeof(page);                                                     \
		(value) = (nativeValue);                                                \
	} while (0)
#define CTR_PSX_LOAD_WORD_FROM_PAGE_AFTER(value, page, symbolExpression, nativeValue, dependency) \
	do                                                                                            \
	{                                                                                             \
		(void)sizeof(page);                                                                       \
		(void)(dependency);                                                                       \
		(value) = (nativeValue);                                                                  \
	} while (0)
#define CTR_PSX_ADD_POINTER_OFFSET(result, base, offset)                         ((result) = (void *)((char *)(base) + (offset)))
#define CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(result, base, offset)            ((result) = (void *)((char *)(base) + (offset)))
#define CTR_PSX_ADD_POINTER_IMMEDIATE(result, base, byteOffset, nativeValue)     ((result) = (nativeValue))
// NOTE(aalhendi): PSX keeps this add in C so GCC may place it in a branch delay slot.
// Native uses the real host pointer rather than the retail page and low offset.
#define CTR_PSX_ADD_PAGE_OFFSET(result, page, offset, nativeValue) \
	do                                                             \
	{                                                              \
		(void)(page);                                              \
		(result) = (nativeValue);                                  \
	} while (0)
#define CTR_PSX_ADD_U32(result, lhs, rhs)                                        ((result) = (u32)(lhs) + (u32)(rhs))
#define CTR_PSX_NEGATE_IN_PLACE(value)                                           ((value) = CTR_MipsNegLo(value))
#define CTR_PSX_NEGATE(result, value)                                            ((result) = CTR_MipsNegLo(value))
#define CTR_PSX_LOAD_IMMEDIATE(result, value)                                    ((result) = (value))
#define CTR_PSX_LOAD_STACK_WORD(result, byteOffset, nativeValue)                 ((result) = (nativeValue))
// NOTE(aalhendi): The dependency places a retail stack-argument load after
// its page load; native still reads the ordinary C parameter.
#define CTR_PSX_LOAD_STACK_WORD_AFTER(result, byteOffset, nativeValue, dependency) \
	do                                                                             \
	{                                                                              \
		(result) = (nativeValue);                                                  \
		(void)(dependency);                                                        \
	} while (0)
#define CTR_PSX_LOAD_WORD(result, nativeValue)                                   ((result) = (nativeValue))
#define CTR_PSX_LOAD_WORD_AFTER(result, nativeValue, dependency)                 ((result) = (nativeValue))
#define CTR_PSX_LOAD_WORD_DELAYED(result, nativeValue)                           ((result) = (nativeValue))
#define CTR_PSX_LOAD_WORD_VOLATILE(result, nativeValue)                          ((result) = (nativeValue))
#define CTR_PSX_LOAD_SIGNED_BYTE(result, base, byteOffset, nativeValue)          ((result) = (s8)(nativeValue))
#define CTR_PSX_LOAD_SIGNED_HALF(result, base, byteOffset, nativeValue)          ((result) = (s16)(nativeValue))
#define CTR_PSX_LOAD_SIGNED_HALF_VOLATILE(result, base, byteOffset, nativeValue) ((result) = (s16)(nativeValue))
#define CTR_PSX_LOAD_SIGNED_HALF_AFTER(result, base, byteOffset, nativeValue, dependency) \
	do                                                                                    \
	{                                                                                     \
		(result) = (s16)(nativeValue);                                                    \
		(void)(dependency);                                                               \
	} while (0)
#define CTR_PSX_LOAD_UNSIGNED_BYTE(result, base, byteOffset, nativeValue) ((result) = (u8)(nativeValue))
#define CTR_PSX_LOAD_GP_UNSIGNED_BYTE_AFTER(result, gpOffset, nativeValue, dependency) \
	do                                                                                 \
	{                                                                                  \
		(result) = (u8)(nativeValue);                                                  \
		(void)(dependency);                                                            \
	} while (0)
#define CTR_PSX_LOAD_UNSIGNED_HALF(result, base, byteOffset, nativeValue) ((result) = (u16)(nativeValue))
#define CTR_PSX_LOAD_UNSIGNED_HALF_AFTER(result, base, byteOffset, nativeValue, dependency) \
	do                                                                                   \
	{                                                                                    \
		(result) = (u16)(nativeValue);                                                    \
		(void)(dependency);                                                              \
	} while (0)
#define CTR_PSX_SHIFT_LEFT_IN_PLACE(value, shift)                         ((value) = CTR_MipsSll((value), (shift)))
#define CTR_PSX_SHIFT_RIGHT_ARITHMETIC(result, value, shift)              ((result) = CTR_MipsSra((value), (shift)))
#define CTR_PSX_SHIFT_RIGHT_ARITHMETIC_IN_PLACE(value, shift)             ((value) = CTR_MipsSra((value), (shift)))
#define CTR_PSX_STORE_HALF(lvalue, value)                                 ((lvalue) = (s16)(value))
// NOTE(aalhendi): A far sData halfword uses a rounded page and signed low
// offset on PSX; native writes the same field through its ordinary lvalue.
#define CTR_PSX_STORE_ABSOLUTE_HALF(lvalue, address, value)               ((lvalue) = (s16)(value))
#define CTR_PSX_BIND_ABSOLUTE_PAGE_PTR(result, lvalue, address)           ((result) = &(lvalue))
#define CTR_PSX_SUBTRACT(result, lhs, rhs)                                ((result) = CTR_MipsSubLo((lhs), (rhs)))
#define CTR_PSX_STORE_COP2_WORD(address, cop2Register)                    CTR_WriteU32LE((address), (u32)MFC2_S(cop2Register))
#define CTR_PSX_STORE_COP2_VEC3(address, xRegister, yRegister, zRegister) \
	do                                                                    \
	{                                                                     \
		(address)[0] = (s32)MFC2_S(xRegister);                            \
		(address)[1] = (s32)MFC2_S(yRegister);                            \
		(address)[2] = (s32)MFC2_S(zRegister);                            \
	} while (0)
#define CTR_PSX_STORE_COP2_HALF_DEAD_SCRATCH(base, byteOffset, cop2Register, scratchRegister) \
	do                                                                                        \
	{                                                                                         \
		*(s16 *)((u8 *)(base) + (byteOffset)) = (s16)MFC2_S(cop2Register);                    \
	} while (0)
#define CTR_PSX_GTE_PIPELINE_DELAY()                          ((void)0)
#define CTR_PSX_GTE_READ_DELAY()                              ((void)0)
#define CTR_PSX_PAGE_LVALUE(type, page, offset, nativeLvalue) (nativeLvalue)
#define CTR_PSX_ADD_SYMBOL_LOW(result, page, symbolExpression, nativeValue) \
	do                                                                      \
	{                                                                       \
		(void)sizeof(page);                                                 \
		(result) = (nativeValue);                                           \
	} while (0)
#define CTR_PSX_ADD_SYMBOL_LOW_DISTINCT(result, page, symbolExpression, nativeValue, identity) \
	CTR_PSX_ADD_SYMBOL_LOW(result, page, symbolExpression, nativeValue)
#define CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(value, symbolExpression, nativeValue) \
	do                                                                        \
	{                                                                         \
		(value) = (nativeValue);                                              \
	} while (0)
#else
#define CTR_PSX_UNSAFE_MANUAL_AT_FRAME                                          __attribute__((mips_unsafe_manual_at_frame))
#define CTR_PSX_UNSAFE_MANUAL_AT_FULL_FRAME                                     __attribute__((mips_unsafe_manual_at_full_frame))
#define CTR_PSX_ADJACENT_TAIL(targetName)                                       __attribute__((mips_adjacent_tail(targetName)))
#define CTR_PSX_PRESERVES_AT24                                                  __attribute__((mips_preserves_at24))
#define CTR_PSX_REGCALL_S012(readMask)                                          __attribute__((mips_regcall_s012(readMask)))

#define CTR_NegateS32(value) __builtin_mips_neg_si(value)
#define CTR_PSX_MATCH_SECTION(sectionName)                                      __attribute__((section(sectionName)))
#define CTR_PSX_REGISTER(registerName)                                          __asm__(registerName)
#define CTR_PSX_BIND_VALUE_CLOBBER(value, registerName)                         __asm__("" : "+r"(value) : : registerName)
#define CTR_PSX_DEPEND_VALUE(value, dependency)                                 __asm__("" : "+r"(value) : "r"(dependency))
#define CTR_PSX_ORDER_VALUES(value, dependency)                                 __asm__ volatile("" : : "r"(value), "r"(dependency))
#define CTR_PSX_DEPEND_MEMORY(pointer, dependency)                              __asm__ volatile("" : "+m"(*(pointer)) : "r"(dependency))
#define CTR_PSX_CLOBBER(registerName)                                           __asm__ volatile("" : : : registerName)
#define CTR_PSX_KEEP_VALUE(value)                                               __asm__ volatile("" : "+r"(value))
#define CTR_PSX_KEEP_VALUE_RELAXED(value)                                       __asm__("" : "+r"(value))
#define CTR_PSX_OBSERVE_VALUE(value)                                            __asm__ volatile("" : : "r"(value))
#define CTR_PSX_OBSERVE_MEMORY(value)                                           __asm__("" : : "m"(value))
#define CTR_PSX_ZERO_VALUE(value)                                               __asm__("move %0,$0" : "=r"(value))
#define CTR_PSX_AND_MASK_NONZERO(result, mask, value)                            ({ (result) = (mask); __asm__ volatile("and %0,%1,%0" : "+r"(result) : "r"(value)); (result) != 0; })
#define CTR_PSX_ZERO_VALUE_AFTER(value, first, second)                          __asm__("move %0,$0" : "=r"(value) : "r"(first), "r"(second))
#define CTR_PSX_MEMORY_BARRIER()                                                __asm__ volatile("" : : : "memory")
#define CTR_PSX_FORGET_VALUE(value)                                             __asm__ volatile("" : "=r"(value) : "0"(value))
#define CTR_PSX_RELOAD(value)                                                   __asm__("" : "+m"(value))
#define CTR_PSX_CAPTURE_REGISTER(value, nativeValue)                            __asm__("" : "=r"(value))
#define CTR_PSX_COPY_VALUE(result, value)                                       __asm__("move %0,%1" : "=r"(result) : "r"(value))
#define CTR_PSX_WITH_RETAIL_PAGE(result, address, offset, nativeValue, expression) \
	({                                                                           \
		(result) = (void *)(address);                                              \
		CTR_PSX_KEEP_VALUE_RELAXED(result);                                       \
		CTR_PSX_ADD_PAGE_OFFSET(result, result, offset, nativeValue);           \
		(expression);                                                             \
	})
#define CTR_PSX_PREPARE_QUEUE_ARGS(bigfileOut, typeOut, indexOut, bigfileIn, queueType, indexIn) \
	__asm__ volatile("move %0,%3\n\tli %1,%4\n\tmove %2,%5" : "=&r"(bigfileOut), "=&r"(typeOut), "=&r"(indexOut) : "r"(bigfileIn), "I"(queueType), "r"(indexIn))
#define CTR_PSX_LOAD_SYMBOL_PAGE(page, symbolExpression)                        __asm__("lui %0,%%hi(" symbolExpression ")" : "=r"(page))
#define CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(page, symbolExpression, dependency)      __asm__("lui %0,%%hi(" symbolExpression ")" : "=r"(page) : "r"(dependency))
#define CTR_PSX_LOAD_WORD_FROM_PAGE(value, page, symbolExpression, nativeValue) __asm__("lw %0,%%lo(" symbolExpression ")(%1)" : "=r"(value) : "r"(page))
#define CTR_PSX_LOAD_WORD_FROM_PAGE_AFTER(value, page, symbolExpression, nativeValue, dependency) \
	__asm__("lw %0,%%lo(" symbolExpression ")(%1)" : "=r"(value) : "r"(page), "r"(dependency))
#define CTR_PSX_ADD_POINTER_OFFSET(result, base, offset)                     __asm__("addu %0,%1,%2" : "=r"(result) : "r"(base), "r"(offset))
#define CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(result, base, offset)        __asm__("addu %0,%2,%1" : "=r"(result) : "r"(base), "r"(offset))
#define CTR_PSX_ADD_POINTER_IMMEDIATE(result, base, byteOffset, nativeValue) __asm__("addiu %0,%1,%2" : "=r"(result) : "r"(base), "I"(byteOffset))
#define CTR_PSX_ADD_PAGE_OFFSET(result, page, offset, nativeValue)                 ((result) = (void *)((u32)(page) + (offset)))
#define CTR_PSX_ADD_U32(result, lhs, rhs)                                    __asm__("addu %0,%1,%2" : "=r"(result) : "r"(lhs), "r"(rhs))
#define CTR_PSX_NEGATE_IN_PLACE(value)                                       __asm__("negu %0,%0" : "+r"(value))
#define CTR_PSX_NEGATE(result, value)                                        __asm__("subu %0,$0,%1" : "=r"(result) : "r"(value))
#define CTR_PSX_LOAD_IMMEDIATE(result, value)                                __asm__("li %0,%1" : "=r"(result) : "I"(value))
#define CTR_PSX_LOAD_STACK_WORD(result, byteOffset, nativeValue)             __asm__ volatile("lw %0," #byteOffset "($sp)" : "=r"(result) : : "memory")
#define CTR_PSX_LOAD_STACK_WORD_AFTER(result, byteOffset, nativeValue, dependency) __asm__("lw %0," #byteOffset "($sp)" : "=r"(result) : "r"(dependency))
#define CTR_PSX_LOAD_WORD(result, nativeValue)                               __asm__("lw %0,%1" : "=r"(result) : "m"(nativeValue))
#define CTR_PSX_LOAD_WORD_AFTER(result, nativeValue, dependency)                   __asm__("lw %0,%1" : "=r"(result) : "m"(nativeValue), "r"(dependency))
#define CTR_PSX_LOAD_WORD_DELAYED(result, nativeValue)                       __asm__ volatile("lw %0,%1\n\tnop" : "=r"(result) : "m"(nativeValue))
#define CTR_PSX_LOAD_WORD_VOLATILE(result, nativeValue)                      __asm__ volatile("lw %0,%1" : "=r"(result) : "m"(nativeValue))
#define CTR_PSX_LOAD_SIGNED_BYTE(result, base, byteOffset, nativeValue) __asm__("lb %0,%2(%1)" : "=r"(result) : "r"(base), "I"(byteOffset), "m"(nativeValue))
#define CTR_PSX_LOAD_SIGNED_HALF(result, base, byteOffset, nativeValue) __asm__("lh %0,%2(%1)" : "=r"(result) : "r"(base), "I"(byteOffset), "m"(nativeValue))
#define CTR_PSX_LOAD_SIGNED_HALF_VOLATILE(result, base, byteOffset, nativeValue) \
	__asm__ volatile("lh %0,%2(%1)" : "=r"(result) : "r"(base), "I"(byteOffset) : "memory")
#define CTR_PSX_LOAD_SIGNED_HALF_AFTER(result, base, byteOffset, nativeValue, dependency) \
	__asm__("lh %0,%2(%1)" : "=r"(result) : "r"(base), "I"(byteOffset), "r"(dependency), "m"(nativeValue))
#define CTR_PSX_LOAD_UNSIGNED_BYTE(result, base, byteOffset, nativeValue) __asm__("lbu %0,%2(%1)" : "=r"(result) : "r"(base), "I"(byteOffset), "m"(nativeValue))
// NOTE(aalhendi): This scheduling seam is used before any write to the loaded byte.
#define CTR_PSX_LOAD_GP_UNSIGNED_BYTE_AFTER(result, gpOffset, nativeValue, dependency) __asm__("lbu %0,%2($gp)" : "=r"(result) : "r"(dependency), "I"(gpOffset))
#define CTR_PSX_LOAD_UNSIGNED_HALF(result, base, byteOffset, nativeValue) __asm__("lhu %0,%2(%1)" : "=r"(result) : "r"(base), "I"(byteOffset), "m"(nativeValue))
#define CTR_PSX_LOAD_UNSIGNED_HALF_AFTER(result, base, byteOffset, nativeValue, dependency) \
	__asm__("lhu %0,%2(%1)" : "=r"(result) : "r"(base), "I"(byteOffset), "r"(dependency), "m"(nativeValue))
#define CTR_PSX_SHIFT_LEFT_IN_PLACE(value, shift)                         __asm__("sll %0,%0,%1" : "+r"(value) : "I"(shift))
#define CTR_PSX_SHIFT_RIGHT_ARITHMETIC(result, value, shift)              __asm__("sra %0,%1,%2" : "=r"(result) : "r"(value), "I"(shift))
#define CTR_PSX_SHIFT_RIGHT_ARITHMETIC_IN_PLACE(value, shift)             __asm__("sra %0,%0,%1" : "+r"(value) : "I"(shift))
#define CTR_PSX_STORE_HALF(lvalue, value)                                 __asm__ volatile("sh %1,%0" : "=m"(lvalue) : "r"(value))
#define CTR_PSX_STORE_ABSOLUTE_HALF(lvalue, address, value)                         \
	do                                                                              \
	{                                                                               \
		u32 page;                                                                   \
		__asm__("lui %0,%1" : "=r"(page) : "i"((((u32)(address)) + 0x8000) >> 16)); \
		*(s16 *)(page + (s16)(address)) = (s16)(value);                             \
	} while (0)
#define CTR_PSX_BIND_ABSOLUTE_PAGE_PTR(result, lvalue, address)                     \
	do                                                                              \
	{                                                                               \
		u32 page;                                                                   \
		__asm__("lui %0,%1" : "=r"(page) : "i"((((u32)(address)) + 0x8000) >> 16)); \
		(result) = (void *)(page + (s16)(address));                                 \
	} while (0)
#define CTR_PSX_SUBTRACT(result, lhs, rhs)             __asm__("subu %0,%1,%2" : "=r"(result) : "r"(lhs), "r"(rhs))
#define CTR_PSX_STORE_COP2_WORD(address, cop2Register) __asm__ volatile("swc2 $" #cop2Register ",0(%0)" : : "r"(address) : "memory")
#define CTR_PSX_STORE_COP2_VEC3(address, xRegister, yRegister, zRegister) \
	__asm__ volatile("swc2 $" #xRegister ",0(%0)\n\t"                     \
	                 "swc2 $" #yRegister ",4(%0)\n\t"                     \
	                 "swc2 $" #zRegister ",8(%0)"                         \
	                 :                                                    \
	                 : "r"(address)                                       \
	                 : "memory")
// NOTE(aalhendi): The scratch register is consumed entirely within this
// sequence and must hold no C value across the call. Keeping it out of the
// clobber list lets GCC 2.8 reuse the same dead register before the GTE read.
#define CTR_PSX_STORE_COP2_HALF_DEAD_SCRATCH(base, byteOffset, cop2Register, scratchRegister) \
	__asm__ volatile("mfc2 " scratchRegister ",$" #cop2Register "\n\tnop"                     \
	                 "\n\tsh " scratchRegister ",%1(%0)"                                      \
	                 :                                                                        \
	                 : "r"(base), "I"(byteOffset)                                             \
	                 : "memory")
#define CTR_PSX_GTE_PIPELINE_DELAY()                                        __asm__ volatile("nop\n\tnop")
#define CTR_PSX_GTE_READ_DELAY()                                            __asm__ volatile("nop")
#define CTR_PSX_PAGE_LVALUE(type, page, offset, nativeLvalue)               (*(type *)((u32)(page) + (s32)(offset)))
#define CTR_PSX_ADD_SYMBOL_LOW(result, page, symbolExpression, nativeValue) __asm__("addiu %0,%1,%%lo(" symbolExpression ")" : "=r"(result) : "r"(page))
#define CTR_PSX_ADD_SYMBOL_LOW_DISTINCT(result, page, symbolExpression, nativeValue, identity) \
	__asm__("addiu %0,%1,%%lo(" symbolExpression ") # " identity : "=r"(result) : "r"(page))
#define CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(value, symbolExpression, nativeValue) __asm__("addiu %0,%0,%%lo(" symbolExpression ")" : "+r"(value))
#endif

#endif
