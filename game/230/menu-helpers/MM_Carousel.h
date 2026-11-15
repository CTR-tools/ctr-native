#ifndef CTR_MM_CAROUSEL_H
#define CTR_MM_CAROUSEL_H

/* The source and its context remain owned by the caller. Labels may be
   borrowed or written to the supplied buffer; font-encoded labels bypass UTF-8
   conversion. Selection fires on initialization and after an animated move;
   confirmation fires on a settled row. Decorations are drawn per visible row. */
struct MM_CarouselSource
{
	void *context;
	int count;
	bool (*enabled)(void *context, int index);
	const char *(*label)(void *context, int index, char *buffer, size_t capacity);
	bool (*labelIsFontEncoded)(void *context, int index);
	void (*decorate)(void *context, int index, const RECT16 *row, bool selected, u32 scrollFrame);
	void (*selected)(void *context, int index);
	void (*confirm)(void *context, int index);
};

struct MM_Carousel
{
	int selected;
	int next;
	int changeFrames;
	int direction;
	u32 scrollStart;
};

void MM_Carousel_Init(struct MM_Carousel *carousel, const struct MM_CarouselSource *source, int selected, u32 frame);
bool MM_Carousel_Update(struct MM_Carousel *carousel, const struct MM_CarouselSource *source, u32 buttons, u32 frame);
int MM_Carousel_RowIndex(const struct MM_Carousel *carousel, const struct MM_CarouselSource *source, int row);
void MM_Carousel_Draw(const struct MM_Carousel *carousel, const struct MM_CarouselSource *source,
                      struct GameTracker *tracker, int x, int y, u32 frame);

#endif
