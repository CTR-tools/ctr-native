#ifndef CTR_NATIVE_TRACK_PATCH_H
#define CTR_NATIVE_TRACK_PATCH_H

/* One local import job at a time. All returned text remains owned by the job. */
int NativeTrackPatch_Start(const char *assets, const char *patch);
int NativeTrackPatch_Poll(void); /* 0 running, 1 success, -1 failure/cancel */
const char *NativeTrackPatch_Message(void);
void NativeTrackPatch_Cancel(void);
void NativeTrackPatch_Shutdown(void);

#endif
