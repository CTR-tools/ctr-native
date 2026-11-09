#ifndef CTR_NATIVE_TRACK_IMPORT_VIEW_H
#define CTR_NATIVE_TRACK_IMPORT_VIEW_H

enum { TRACK_REVIEW_TEXT_CHARACTERS = 30 };

enum TrackImportReviewRow
{
	TRACK_REVIEW_TRACK,
	TRACK_REVIEW_INCLUDE,
	TRACK_REVIEW_TITLE,
	TRACK_REVIEW_AUTHOR,
	TRACK_REVIEW_MUSIC_SOURCE,
	TRACK_REVIEW_MUSIC_TRACK,
	TRACK_REVIEW_CONFIRM,
	TRACK_REVIEW_CANCEL,
	TRACK_REVIEW_ROWS,
};

struct TrackImportReviewView
{
	const char *title, *author, *editingValue, *message;
	const char *musicSource, *musicTrack;
	int musicIndex;
	int index, count, row, editing;
	int included, includedCount;
	int installed;
};

void MM_TrackImport_Reset(void);
int MM_TrackImport_GetReview(struct TrackImportReviewView *view);

#endif
