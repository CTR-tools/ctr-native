#ifndef CTR_SDK_STREAM_STATE_H
#define CTR_SDK_STREAM_STATE_H

#include <psx/libcd.h>

typedef void (*StStreamCallback)();

// NOTE(aalhendi): These are independent SDK BSS owners; address names do not imply recovered purpose.
extern StStreamCallback St_streamCallback1;
extern StStreamCallback St_streamCallback2;
extern u32 St_state8009ebf4;
extern u32 St_state8009ebf8;
extern u32 St_state8009ebfc;
extern u32 St_state8009ec00;
extern u32 St_state8009ec04;
extern u32 St_streamMode;
extern u32 St_maskEnd;
extern u32 St_mask;
extern u16 St_state8009ec18;
extern u32 St_state8009ec1c;
extern u32 *St_ringBase;
extern u32 St_state8009ec28;
extern u32 St_state8009ec2c;
extern u32 St_state8009ec30;
extern u32 St_ringCount;
extern u32 St_maskStart;
extern CdlLOC St_backloc;
extern u32 St_backframe;
extern u32 St_state8009ec90;

void init_ring_status(u32 first, u32 count);
void data_ready_callback(void);
void St_cdready_callback(u8 status, u8 *result);

#ifdef CTR_NATIVE
#define ST_STATE_PTR(type, owner, page, offset) (&(owner))
#else
#define ST_STATE_PTR(type, owner, page, offset) ((type *)((page) - (offset)))
#endif

// NOTE(aalhendi): Keep the SDK's distinct signed-low page stores without emitting assembly.
#define ST_STORE(type, owner, offset, value)                     \
	do                                                           \
	{                                                            \
		statePage = 0x800a0000u;                                 \
		CTR_PSX_KEEP_VALUE(statePage);                           \
		*ST_STATE_PTR(type, owner, statePage, offset) = (value); \
	} while (0)

#endif
