// NOTE(aalhendi): One signed table/quadrant algorithm serves native callers
// and the retail entry files' distinct register interfaces.
#ifdef CTR_NATIVE
	u32 trigAngle = angle;
	s32 trigSine;
	s32 trigCosine;
#endif
	register u32 trigWork CTR_PSX_REGISTER("$3");

	trigWork = (u32)&data.trigApprox[0];
	trigSine = ANG_MODULO_HALF_PI(trigAngle);
	trigSine <<= 2;
	// NOTE(aalhendi): Forget the symbolic base equivalence to retain ADDU operand order.
	CTR_PSX_KEEP_VALUE(trigWork);
	trigWork += trigSine;
	trigSine = (s32)CTR_ReadU32AlignedLE((const void *)trigWork);
	trigWork = trigAngle & ANG_QUADRANT_BIT;
	if (trigWork != 0)
		goto quadrant;
	trigWork = trigAngle & ANG_SIGN_BIT;
	trigCosine = trigSine >> 16;
	trigSine = (s32)((u32)trigSine << 16);
	trigSine >>= 16;
	if (trigWork != 0)
		goto both_negative;
	goto finished;

quadrant:
	trigWork = trigAngle & ANG_SIGN_BIT;
	trigCosine = (s32)((u32)trigSine << 16);
	trigCosine >>= 16;
	trigSine >>= 16;
	if (trigWork != 0)
		goto sine_negative;
	trigCosine = CTR_NegateS32(trigCosine);
	goto finished;

both_negative:
	trigCosine = CTR_NegateS32(trigCosine);
	trigSine = CTR_NegateS32(trigSine);
	goto finished;

sine_negative:
	trigSine = CTR_NegateS32(trigSine);

finished:
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Sequential writes leave an aliased cosine with the opposite
	// sign from the independent result on the sign-bit path.
	if (sine == cosine && (trigAngle & ANG_SIGN_BIT))
	{
		trigCosine = CTR_NegateS32(trigCosine);
	}
	*sine = trigSine;
	*cosine = trigCosine;
#else
	return;
#endif
