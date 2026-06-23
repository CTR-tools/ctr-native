struct UiElement2D
{
	s16 x;
	s16 y;
	s16 z;
	s16 scale;
};

struct UiElement3D
{
	// 0x0
	SVec3 rot;
	s16 scale;

	// 0x8
	MATRIX m;

	// 0x28
	SVec3 lightDir;
	s16 _pad_lightDir;

	// 0x30
	s16 vel[4];

	// 0x38 bytes
};

struct QuipStr
{
	s16 lngIndex;
	s16 flags;
	int priority;
};

struct QuipMeta
{
	struct QuipStr *ptrQuipStrCurr;
	struct QuipStr *ptrQuipStrNext;
	s16 conditionType;
	s16 flags;
	int threshold;
	int driverOffset;
	int dataSize;
};

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct UiElement2D) == 0x8);
CTR_STATIC_ASSERT_LAYOUT(OFFSETOF(struct UiElement3D, rot) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(OFFSETOF(struct UiElement3D, scale) == 0x6);
CTR_STATIC_ASSERT_LAYOUT(OFFSETOF(struct UiElement3D, m) == 0x8);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct UiElement3D) == 0x38);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct QuipStr) == 0x8);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct QuipMeta) == 0x18);
