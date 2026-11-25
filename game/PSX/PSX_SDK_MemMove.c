#include <ctr_compiler.h>
#include <macros.h>

void *memmove(void *dst, const void *src, s32 count)
{
	u8 *d = (u8 *)dst;
	const u8 *s = (const u8 *)src;
	// NOTE(aalhendi): Retail PsyQ memmove returns the post-loop d pointer and
	// routes the backward-copy branch through the shared jr $ra epilogue.
	void *ret CTR_PSX_LOCAL_REGISTER_HINT("$2");

	if (d >= s)
	{
		while (count-- > 0)
		{
			d[count] = s[count];
		}
		ret = d;
		goto done;
	}
	while (count-- > 0)
	{
		*d++ = *s++;
	}
	ret = d;
done:
	CTR_PSX_OBSERVE_VALUE(ret);
	return ret;
}
