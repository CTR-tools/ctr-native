#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
struct Data data;
static struct GameTracker s_tracker;
static char *s_suffixStrings[] = {"th", "o", "&"};
static struct UiElement2D s_hud[UI_HUD_SLOT_COUNT * 4];
static s16 s_suffixX;
static s16 s_suffixY;
static int s_aspectFactor = 1000;

#include <game/UI/UI_Convert.c>
#include <game/UI/UI_PosSuffix.c>

s32 DecalFont_GetLineWidth(char *str, s16 fontType)
{
	(void)fontType;
	return (str[1] == '\0' ? 9 : 18) * s_aspectFactor / 1000;
}

int Widescreen_ScaleWidth(int width)
{
	return width * s_aspectFactor / 1000;
}

void DecalFont_DrawLine(char *str, s16 posX, s16 posY, s16 fontType, s16 flags)
{
	(void)str;
	(void)fontType;
	(void)flags;
	s_suffixX = posX;
	s_suffixY = posY;
}

int main(void)
{
	sdata->gGT = &s_tracker;
	sdata->lngStrings = s_suffixStrings;
	data.stringIndexSuffix[0] = 0;
	data.stringIndexSuffix[1] = 1;
	data.stringIndexSuffix[2] = 2;
	data.hudStructPtr[2] = s_hud;
	data.hudStructPtr[3] = s_hud;
	s_hud[UI_HUD_SLOT_BIG1].x = 10;
	s_hud[UI_HUD_SLOT_RANK].x = 38;
	s_hud[2 * UI_HUD_SLOT_COUNT + UI_HUD_SLOT_BIG1].x = 10;
	s_hud[2 * UI_HUD_SLOT_COUNT + UI_HUD_SLOT_RANK].x = 38;

	CTR_TEST_SECTION("position indicator margins mirror in split-screen modes");

	// 4P layouts: left RANK x is 38, right RANK x is 462, both at y 77.
	const s16 leftX = 38;
	const s16 rightX = 462;
	const s16 y = 77;
	const int depth = UI_POS_SUFFIX_BIG_NUM_Z_BASE;

	for (int players = 1; players <= 4; players++)
	{
		s_tracker.numPlyrCurrGame = players;
		for (int id = 0; id < players; id++)
		{
			struct Driver driver = {0};
			struct Instance bigNum = {0};
			driver.driverID = id;
			driver.driverRank = 0;
			driver.instBigNum = &bigNum;
			bigNum.matrix.t[0] = 0x7fff;

			s16 posX = players >= 3 && (id & 1) != 0 ? rightX : leftX;
			UI_DrawPosSuffix(posX, y, &driver, 0);

			CTR_TEST_CHECK(bigNum.matrix.t[2] == depth, "the rank number keeps its depth");
			CTR_TEST_CHECK_EQ(s_suffixY, y, "the suffix keeps its vertical position");
			if (players >= 3 && (id & 1) != 0)
			{
				CTR_TEST_CHECK_EQ(s_suffixX + 18, SCREEN_WIDTH - 10,
				                  "the two-letter suffix ends at the mirrored left margin");
				CTR_TEST_CHECK_EQ(bigNum.matrix.t[0], UI_ConvertX_2(s_suffixX - UI_POS_SUFFIX_3P4P_BIG_NUM_X_OFFSET, depth),
				                  "right-side rank numbers keep the suffix on their right");
				// A two-letter suffix ("st", "nd", "rd", "th") is about 18
				// layout units wide and must not be clipped by the viewport.
				CTR_TEST_CHECK(s_suffixX + 18 < 512, "a two-letter suffix stays inside the right viewport");
			}
			else
			{
				CTR_TEST_CHECK_EQ(s_suffixX, posX, "1P/2P and left-side suffixes keep their layout X");
				if (players < 3)
				{
					CTR_TEST_CHECK_EQ(bigNum.matrix.t[0], 0x7fff, "1P/2P do not move the rank number");
				}
				else
				{
					CTR_TEST_CHECK_EQ(bigNum.matrix.t[0], UI_ConvertX_2(leftX - UI_POS_SUFFIX_3P4P_BIG_NUM_X_OFFSET, depth),
					                  "left-side rank numbers keep the inward offset");
				}
			}
		}
	}

	CTR_TEST_SECTION("suffix width and battle rank determine the right margin");
	struct Driver driver = {0};
	driver.driverID = 1;
	s_tracker.numPlyrCurrGame = 4;
	s_tracker.gameMode1 = BATTLE_MODE;
	s_tracker.battleSetup.finishedRankOfEachTeam[0] = 1;
	UI_DrawPosSuffix(rightX, y, &driver, 0);
	CTR_TEST_CHECK_EQ(s_suffixX + 9, SCREEN_WIDTH - 10, "a one-character battle suffix ends at the same mirrored margin");
	CTR_TEST_CHECK_EQ(UI_PosSuffixOffset(rightX, &driver), s_suffixX - rightX, "the 2D number receives the same shift as the suffix");

	CTR_TEST_SECTION("number spacing follows widescreen compression");
	s_aspectFactor = 500;
	s_tracker.gameMode1 = 0;
	struct Instance bigNum = {0};
	struct Driver leftDriver = {0};
	struct Instance leftBigNum = {0};
	s_tracker.drivers[0] = &leftDriver;
	driver.instBigNum = &bigNum;
	for (int id = 0; id < 2; id++)
	{
		driver.driverID = id;
		s16 suffixX = id == 0 ? leftX : rightX;
		s16 numberX = id == 0 ? 10 : 434;
		UI_DrawPosSuffix(suffixX, y, &driver, 0);
		CTR_TEST_CHECK_EQ(bigNum.matrix.t[0], UI_ConvertX_2(s_suffixX - 8, depth),
		                  "the 3D number keeps a compressed distance from the suffix");
		CTR_TEST_CHECK_EQ(UI_PosSuffixNumberX(suffixX, numberX, &driver), s_suffixX - 14,
		                  "the 2D number keeps a compressed distance from the suffix");
		if (id == 1)
		{
			CTR_TEST_CHECK_EQ(s_suffixX + 9, SCREEN_WIDTH - 24, "the compressed suffix keeps its mirrored margin");
		}
	}
	leftDriver.instBigNum = &leftBigNum;
	driver.driverID = 1;
	UI_DrawPosSuffix(rightX, y, &driver, 0);
	CTR_TEST_CHECK_EQ(s_suffixX + 9, SCREEN_WIDTH - 21,
	                  "the right margin accounts for the left 3D number's scaled overhang");

	CTR_TEST_SECTION("the right-hand number and suffix move together when the suffix width changes");
	s_aspectFactor = 1000;
	s_tracker.gameMode1 = BATTLE_MODE;
	s_tracker.battleSetup.finishedRankOfEachTeam[0] = 0;
	UI_DrawPosSuffix(rightX, y, &driver, 0);
	int twoLetterSuffixX = s_suffixX;
	int twoLetterModelX = bigNum.matrix.t[0];
	int twoLetterIconX = UI_PosSuffixNumberX(rightX, 434, &driver);
	s_tracker.battleSetup.finishedRankOfEachTeam[0] = 1;
	UI_DrawPosSuffix(rightX, y, &driver, 0);
	CTR_TEST_CHECK_EQ(s_suffixX - twoLetterSuffixX, 9, "the shorter suffix moves right by the difference in width");
	CTR_TEST_CHECK_EQ(bigNum.matrix.t[0] - twoLetterModelX, 9, "the 3D number moves with the suffix");
	CTR_TEST_CHECK_EQ(UI_PosSuffixNumberX(rightX, 434, &driver) - twoLetterIconX, 9, "the 2D number moves with the suffix");
	CTR_TEST_CHECK_EQ(s_suffixX + 9, twoLetterSuffixX + 18, "the right edge of the block stays anchored");
	for (int factor = 500; factor <= 1000; factor += 500)
	{
		s_aspectFactor = factor;
		s_tracker.battleSetup.finishedRankOfEachTeam[0] = 2;
		UI_DrawPosSuffix(rightX, y, &driver, 0);
		int edge = SCREEN_WIDTH - (leftX + Widescreen_ScaleWidth(10 - leftX) - Widescreen_ScaleWidth(UI_POS_SUFFIX_3P4P_MODEL_LEFT_OVERHANG));
		CTR_TEST_CHECK_EQ(s_suffixX + Widescreen_ScaleWidth(UI_POS_SUFFIX_ORDINAL_VISIBLE_WIDTH), edge,
		                  "the ordinal indicator uses its scaled visible width at the mirrored margin");
	}

	return CtrTest_Summary();
}
