#ifndef CTR_NATIVE_TRACK_PATCH_H
#define CTR_NATIVE_TRACK_PATCH_H

/* One local import job at a time. All returned text remains owned by the job. */
int NativeTrackPatch_Start(const char *assets, const char *patch);
/* Review jobs keep their validated candidates private until Commit or Discard. */
int NativeTrackPatch_StartReview(const char *assets, const char *patch);
/* Folder imports use the same temporary review and publication transaction. */
const char *NativeTrackPatch_PrepareFolder(const char *assets, const char *source);
const char *NativeTrackPatch_ReviewDirectory(void);
/* identity is the candidate's stable registry identity, not its transient ID. */
const char *NativeTrackPatch_ReviewUpdate(const char *identity, const char *title, const char *author);
int NativeTrackPatch_ReviewSetMusic(const char *identity, const char *source);
const char *NativeTrackPatch_ReviewMusic(const char *identity);
int NativeTrackPatch_ReviewIncluded(const char *identity);
int NativeTrackPatch_ReviewSetIncluded(const char *identity, int included);
const char *NativeTrackPatch_ReviewCommit(void);
int NativeTrackPatch_ReviewCommitStart(void);
/* Transfers a completed background scan to the caller. */
struct NativeTrackRegistry *NativeTrackPatch_TakeRegistry(void);
int NativeTrackPatch_ReviewDiscard(void);
int NativeTrackPatch_ReviewDiscardStart(void);
int NativeTrackPatch_Poll(void); /* 0 running, 1 success, -1 failure/cancel */
const char *NativeTrackPatch_Message(void);
void NativeTrackPatch_Cancel(void);
void NativeTrackPatch_Shutdown(void);

#endif
