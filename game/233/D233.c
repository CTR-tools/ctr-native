#include <common.h>

static void OVR233_ResetGarage(void);
static void OVR233_ResetCreditsBSS(void);

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct OverlayRDATA_233) == 0xbd90);
CTR_STATIC_ASSERT_LAYOUT(sizeof(void *) == 4);

struct OverlayDATA_233 D233;

#define OVR233_GARAGE_INITIALIZER                          \
	{                                                      \
	    .menuGarage =                                      \
	        {                                              \
	            .stringIndexTitle = 0xFFFF,                \
	            .posX_curr = 0x100,                        \
	            .posY_curr = 0x6c,                         \
	            .unk1 = 0,                                 \
	            .state = 0x823,                            \
	            .rows = 0,                                 \
	            .funcPtr = CS_Garage_MenuProc,             \
	            .drawStyle = 0,                            \
	        },                                             \
	    .numFramesMax_GarageMove = 0x1d,                   \
	    .padding1 = 0x14,                                  \
	    .numFramesMax_Zoom = 0x14,                         \
	    .fovMin = 0x12c,                                   \
	    .fovMax = 0x190,                                   \
	    .unusedArr_garageChars = {0, 1, 2, 3, 4, 5, 6, 7}, \
	    .unusedArr_lngIndex = {0x248, 0x249, 0x24a, 0},    \
	    .barStat =                                         \
	        {                                              \
	            0x37,                                      \
	            0x37,                                      \
	            0x37,                                      \
	            0x30,                                      \
	            0x50,                                      \
	            0x20,                                      \
	            0x50,                                      \
	            0x20,                                      \
	            0xA,                                       \
	            0x1c,                                      \
	            0x30,                                      \
	            0x50,                                      \
	        },                                             \
	    .unusedArr_Colors = {0xff, 0xff00, 0xff00ff},      \
	    .barColors =                                       \
	        {                                              \
	            0xc80000,                                  \
	            0xA8700,                                   \
	            0xb428,                                    \
	            0xb4b4,                                    \
	            0x64dc,                                    \
	            0x28dc,                                    \
	            0xeb,                                      \
	        },                                             \
	}

static const struct OVR233_Garage s_gGarageInitialState = OVR233_GARAGE_INITIALIZER;

struct OVR233_Garage gGarage = OVR233_GARAGE_INITIALIZER;

// NOTE(aalhendi): Retail overlay data at 0x800b9488-0x800b9498.
#define OVR233_CREDITS_BSS_INITIALIZER          \
	{                                           \
	    .creditGhost_Pos = {0x64, 0x3c, 0x12c}, \
	    .creditText_PosX = 0x14,                \
	}

static const struct Ovr233_Credits_BSS s_creditsBSSInitialState = OVR233_CREDITS_BSS_INITIALIZER;

struct Ovr233_Credits_BSS creditsBSS = OVR233_CREDITS_BSS_INITIALIZER;

void OVR233_RebuildInitMatrixTable(void)
{
	D233.cs_initMatrixTable[0].data = &D233.cs_initMatrixData[0];
	D233.cs_initMatrixTable[0].count = 41;
	D233.cs_initMatrixTable[1].data = &D233.cs_initMatrixData[41];
	D233.cs_initMatrixTable[1].count = 45;
	D233.cs_initMatrixTable[2].data = &D233.cs_initMatrixData[86];
	D233.cs_initMatrixTable[2].count = 49;
	D233.cs_initMatrixTable[3].data = &D233.cs_initMatrixData[135];
	D233.cs_initMatrixTable[3].count = 55;
}

static void OVR233_ResetD233(void)
{
	D233.VertSplitLine = R233.VertSplitLine;
	D233.boolLoadNextSwap = R233.boolLoadNextSwap;
	D233.boolStartToSkip = R233.boolStartToSkip;
	D233.bossCutsceneIndex = R233.bossCutsceneIndex;
	D233.CutsceneManipulatesAudio = R233.CutsceneManipulatesAudio;
	D233.cs_initMatrixBool = R233.cs_initMatrixBool;
	D233.isCutsceneOver = R233.isCutsceneOver;
	D233.PodiumInitUnk2 = R233.PodiumInitUnk2;
	D233.FXVolumeBackup = R233.FXVolumeBackup;
	D233.MusicVolumeBackup = R233.MusicVolumeBackup;
	D233.VoiceVolumeBackup = R233.VoiceVolumeBackup;
	D233.volumeunknown = R233.volumeunknown;
	D233.PodiumInitUnk3 = R233.PodiumInitUnk3;
	D233.cutsceneState = R233.cutsceneState;
	D233.ptrModelBossHead = R233.ptrModelBossHead;
	D233.ptrModelBossBody = R233.ptrModelBossBody;
	memcpy(D233.cs_initMatrixData, R233.cs_initMatrixData, sizeof(D233.cs_initMatrixData));
	OVR233_RebuildInitMatrixTable();
}

#ifdef CTR_NATIVE
struct Ovr233RetailPointerRange
{
	uintptr_t retailStart;
	uintptr_t retailEnd;
	char *nativeStart;
};

#define OVR233_RETAIL_BASE 0x800ab9f0u
// clang-format off
#define OVR233_OPCODE_RANGE(FIELD, OFFSET, SIZE)                       \
	{                                                                  \
	    OVR233_RETAIL_BASE + (OFFSET),                                 \
	    OVR233_RETAIL_BASE + (OFFSET) + (SIZE),                        \
	    (char *)&R233.FIELD[0],                                        \
	}
// clang-format on

char *CS_OVR233_TranslateRetailOpcodePointer(char *opCodeAt)
{
	uintptr_t ptr = (uintptr_t)opCodeAt;
	static const struct Ovr233RetailPointerRange ranges[] = {
	    OVR233_OPCODE_RANGE(bossOpcodeData, OVR233_BOSS_OPCODE_DATA, 0x2b4),
	    OVR233_OPCODE_RANGE(script_tawnaNormal, OVR233_SCRIPT_TAWNA_NORMAL, 0x28),
	    OVR233_OPCODE_RANGE(script_tawnaCredits, OVR233_SCRIPT_TAWNA_CREDITS, 0x164c),
	    OVR233_OPCODE_RANGE(script_default, OVR233_SCRIPT_DEFAULT, 0x18),
	    OVR233_OPCODE_RANGE(script_dingofire, OVR233_SCRIPT_DINGOFIRE, 0x38),
	    OVR233_OPCODE_RANGE(danceOtherOpcodeData, OVR233_DANCE_OTHER_OPCODE_DATA, 0x12d4),
	    OVR233_OPCODE_RANGE(introModelOpcodeData, OVR233_INTRO_MODEL_OPCODE_DATA, 0x3b0),
	    OVR233_OPCODE_RANGE(introCutsceneOpcodeData, OVR233_INTRO_CUTSCENE_OPCODE_DATA, 0x11c),
	    OVR233_OPCODE_RANGE(introEndingOpcodeData, OVR233_INTRO_ENDING_OPCODE_DATA, 0x50),
	    OVR233_OPCODE_RANGE(creditsCutsceneOpcodeData, OVR233_CREDITS_CUTSCENE_OPCODE_DATA, 0x1dc),
	    OVR233_OPCODE_RANGE(creditsOpcodeData, OVR233_CREDITS_OPCODE_DATA, 0x40),
	    OVR233_OPCODE_RANGE(boxAndAdvCharSelectOpcodeData, OVR233_BOX_ADV_CHAR_SELECT_OPCODE_DATA, 0x10c4),
	};

	for (int i = 0; i < (int)(sizeof(ranges) / sizeof(ranges[0])); i++)
	{
		const struct Ovr233RetailPointerRange *range = &ranges[i];
		if ((ptr >= range->retailStart) && (ptr < range->retailEnd))
			return range->nativeStart + (ptr - range->retailStart);
	}

	return opCodeAt;
}

#undef OVR233_OPCODE_RANGE
#undef OVR233_RETAIL_BASE
#endif

void OVR233_ResetRuntimeState(void)
{
	OVR233_ResetD233();
	OVR233_ResetGarage();
	OVR233_ResetCreditsBSS();
}

void OVR233_InitData(void)
{
	OVR233_ResetRuntimeState();
}

void CS_OVR233_ResetRuntimeState(void)
{
	OVR233_ResetRuntimeState();
}

void CS_OVR233_InitData(void)
{
	OVR233_InitData();
}

static void OVR233_ResetGarage(void)
{
	gGarage = s_gGarageInitialState;
}

static void OVR233_ResetCreditsBSS(void)
{
	creditsBSS = s_creditsBSSInitialState;
}

#undef OVR233_CREDITS_BSS_INITIALIZER
#undef OVR233_GARAGE_INITIALIZER
