#include <psx/libcd.h>

extern int CD_ReadyController(int mode, u8 *result);
extern int CD_DataSyncController(int mode);
extern void *DMACallback(int channel, void (*callback)());

int CdReady(int mode, u8 *result)
{
	return CD_ReadyController(mode, result);
}

void *CdDataCallback(void (*callback)())
{
	return DMACallback(3, callback);
}

int CdDataSync(int mode)
{
	return CD_DataSyncController(mode);
}
