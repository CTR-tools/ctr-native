#include "PSX_SDK_StreamState.h"

s32 StGetBackloc(CdlLOC *loc)
{
	s32 result;
	if (St_state8009ec90 == 0)
	{
		CdIntToPos(CdPosToInt(&St_backloc) + 1, loc);
		result = (s32)St_backframe;
	}
	else
	{
		result = -1;
	}
	return result;
}
