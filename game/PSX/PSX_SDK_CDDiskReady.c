#include <macros.h>
#include <psx/libcd.h>
#include <psx/libetc.h>

int CdDiskReady(int mode)
{
	u8 res[8];
	s32 ok;
	s32 i;

	CdControlB(1, NULL, res);
	if ((res[0] & 0x10) != 0)
	{
		return 16;
	}
	ok = CdControlB(19, NULL, res);
	if (mode == 1)
	{
		if ((res[0] == 2) && (ok != 0))
		{
			return 2;
		}
		return 5;
	}
	for (i = 0; i < 10; i++)
	{
		if ((res[0] & 2) != 0)
		{
			while ((res[0] != 2) || (ok == 0))
			{
				VSync(30);
				ok = CdControlB(19, NULL, res);
				if ((res[0] & 0x10) != 0)
				{
					return 16;
				}
			}
			return 2;
		}
		VSync(30);
		ok = CdControlB(19, NULL, res);
		if ((res[0] & 0x10) != 0)
		{
			return 16;
		}
	}
	return 5;
}
