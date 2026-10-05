// NOTE(aalhendi): One packet body; owners supply only their two finish paths.
#ifndef CTR_NATIVE
#define NORMAL_STORE_GTE(base, offset, reg) __asm__ volatile("swc2 $" CTR_GTE_STRINGIFY(reg) ",%1(%0)" : : "r"(base), "I"(offset) : "memory")
#else
#define NORMAL_STORE_GTE(base, offset, reg) CTR_WriteU32AlignedLE((u8 *)(base) + (offset), MFC2(reg))
#endif

	work2 = normalScratch->primitiveState.split.alphaScale;
#ifndef CTR_NATIVE
	texture = (const struct TextureLayout *)normalScratch->texturePtr32;
#endif
	MTC2(normalColor0, 20);
	MTC2(normalColor1, 21);
	MTC2(normalColor2, 22);
	MTC2(work2, 8);
	if (work2 != 0)
	{
#ifdef CTR_NATIVE
		// NOTE(aalhendi): Test the logical post-bin OT slot, not host pointer bits.
		normalOtAddress = RenderBucket_OTAddress(otEntry);
#endif
		normalWork0 = normalOtAddress << 7;
		if ((s32)normalWork0 >= 0)
			gte_dpct();
	}

	word = 0x30000000u;
	if (texture != 0)
		goto textured;
	NORMAL_STORE_GTE(normalCursor, 8, 12);
	NORMAL_STORE_GTE(normalCursor, 16, 13);
	NORMAL_STORE_GTE(normalCursor, 24, 14);
	normalWork0 = MFC2(20);
	normalWork1 = 28;
	// NOTE(aalhendi): Keep the command in V1 for the retail OR operand order.
	CTR_PSX_KEEP_VALUE(word);
	word |= normalWork0;
	CTR_WriteU32AlignedLE(normalCursor + 4, word);
	NORMAL_STORE_GTE(normalCursor, 12, 21);
	NORMAL_STORE_GTE(normalCursor, 20, 22);
	RB_PACKET_FINISH_UNTEXTURED();
textured:
	word = CTR_ReadU32LE(&texture->u0);
	normalWork0 = CTR_ReadU32LE(&texture->u1);
	normalWork1 = CTR_ReadU32LE(&texture->u2);
	NORMAL_STORE_GTE(normalCursor, 8, 12);
	NORMAL_STORE_GTE(normalCursor, 20, 13);
	NORMAL_STORE_GTE(normalCursor, 32, 14);
	CTR_WriteU32AlignedLE(normalCursor + 12, word);
	CTR_WriteU32AlignedLE(normalCursor + 24, normalWork0);
	CTR_WriteU32AlignedLE(normalCursor + 36, normalWork1);
	normalWork1 = MFC2(20);
	word = 0x00600000u;
	CTR_PSX_KEEP_VALUE(word);
	normalWork0 = word & normalWork0;
	if (word != normalWork0)
	{
		// NOTE(aalhendi): Observe T0, not old V1; the 0x34 default can then
		// fill the branch slot without invalidating a live input.
		CTR_PSX_OBSERVE_VALUE(normalWork0);
		word = 0x36000000u;
	}
	else
		word = 0x34000000u;
	word |= normalWork1;
	CTR_WriteU32AlignedLE(normalCursor + 4, word);
	NORMAL_STORE_GTE(normalCursor, 16, 21);
	NORMAL_STORE_GTE(normalCursor, 28, 22);
	normalWork1 = 40;
	RB_PACKET_FINISH_TEXTURED();

#undef NORMAL_STORE_GTE
