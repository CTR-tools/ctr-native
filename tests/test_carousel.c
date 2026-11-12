#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
struct Data data;
int OtherFX_Play(u32 soundID, int flags) { (void)soundID; (void)flags; return 0; }
void DecalFont_DrawLine(char *text, s16 x, s16 y, s16 font, s16 flags)
{ (void)text; (void)x; (void)y; (void)font; (void)flags; }
s32 DecalFont_GetLineWidth(char *text, s16 font) { (void)font; return (s32)strlen(text); }
void CTR_Box_DrawClearBox(const RECT *rect, const Color *color, s32 transparency, u32 *ot, struct PrimMem *memory)
{ (void)rect; (void)color; (void)transparency; (void)ot; (void)memory; }
void RECTMENU_DrawInnerRect(RECT *rect, int style, u32 *ot) { (void)rect; (void)style; (void)ot; }

#define MM_TRIG_APPROX (data.trigApprox)
#define MM_DRAW_LINE_WIDE_FLAGS DecalFont_DrawLine
#define MM_DRAW_CLEAR_BOX CTR_Box_DrawClearBox
#include <game/230/MM_Carousel.c>

struct Fixture { int selected; int selections; int confirmed; unsigned int enabled; };
static bool Enabled(void *context, int index) { return (((struct Fixture *)context)->enabled & (1u << index)) != 0; }
static void Selected(void *context, int index) { struct Fixture *f = context; f->selected = index; f->selections++; }
static void Confirm(void *context, int index) { ((struct Fixture *)context)->confirmed = index; }

int main(void)
{
	struct Fixture fixture = {.confirmed = -1, .enabled = (1u << 0) | (1u << 2)};
	struct MM_CarouselSource source = {.context = &fixture, .count = 3, .enabled = Enabled, .selected = Selected, .confirm = Confirm};
	struct MM_Carousel carousel;
	CTR_TEST_SECTION("Carousel selection and callbacks");
	MM_Carousel_Init(&carousel, &source, 1, 10);
	CTR_TEST_CHECK(carousel.selected == 2 && fixture.selected == 2 && fixture.selections == 1,
	               "initial selection skips disabled entries and notifies the caller");
	CTR_TEST_CHECK(MM_Carousel_RowIndex(&carousel, &source, 3) == 0 && MM_Carousel_RowIndex(&carousel, &source, 5) == 0,
	               "visible rows wrap and omit disabled entries");
	CTR_TEST_CHECK(MM_Carousel_Update(&carousel, &source, BTN_DOWN, 11) && carousel.next == 0 && carousel.selected == 2,
	               "navigation starts an animated move without changing selection immediately");
	MM_Carousel_Update(&carousel, &source, BTN_CROSS_one, 12);
	MM_Carousel_Update(&carousel, &source, BTN_CROSS_one, 13);
	CTR_TEST_CHECK(fixture.confirmed == -1 && fixture.selections == 1, "confirmation is ignored during the animation");
	MM_Carousel_Update(&carousel, &source, 0, 14);
	CTR_TEST_CHECK(carousel.selected == 0 && fixture.selected == 0 && fixture.selections == 2 && carousel.scrollStart == 14,
	               "the new selection and scroll timer are published when the animation ends");
	CTR_TEST_CHECK(MM_Carousel_Update(&carousel, &source, BTN_CROSS_one, 15) && fixture.confirmed == 0,
	               "confirmation invokes the callback for the selected entry");
	fixture.enabled = 0;
	MM_Carousel_Init(&carousel, &source, 0, 16);
	CTR_TEST_CHECK(carousel.selected == -1 && MM_Carousel_RowIndex(&carousel, &source, 4) == -1 &&
	               !MM_Carousel_Update(&carousel, &source, BTN_DOWN, 17), "a list with no enabled entries is safe and inert");
	source.count = 1;
	fixture.enabled = 1;
	MM_Carousel_Init(&carousel, &source, 0, 18);
	CTR_TEST_CHECK(MM_Carousel_RowIndex(&carousel, &source, 0) == 0 &&
	               MM_Carousel_Update(&carousel, &source, BTN_UP, 19) && carousel.changeFrames == 0,
	               "a single enabled entry wraps without starting a pointless animation");
	source.count = 0;
	MM_Carousel_Init(&carousel, &source, 0, 20);
	CTR_TEST_CHECK(carousel.selected == -1 && !MM_Carousel_Update(&carousel, &source, BTN_CROSS_one, 21),
	               "an empty source cannot invoke confirmation");
	return CtrTest_Summary();
}
