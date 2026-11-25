#include "PSX_SDK_SpuInternal.h"

u32 SpuSetReverbVoice(s32 on_off, u32 voice_bit)
{
	return _SpuSetAnyVoice(on_off, voice_bit, 204, 205);
}

void _SpuCallback(SpuIRQCallbackProc func)
{
	InterruptCallback(9, (void *)func);
}

void _SpuDataCallback(void (*func)(void))
{
	DMACallback(4, (void *)func);
}
