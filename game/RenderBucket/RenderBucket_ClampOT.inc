// NOTE(aalhendi): Clamp scaled byte offsets once for context and retail writers.
	normalOtAddress >>= 17;
	normalOtAddress <<= 2;
	work2 = (s32)normalOtAddress - (s32)word;
	if (work2 < 0)
		normalOtAddress = word + otBase;
	else
	{
		work2 = (s32)normalOtAddress - (s32)normalWork0;
		normalOtAddress += otBase;
		if (work2 > 0)
			normalOtAddress = normalWork0 + otBase;
	}
