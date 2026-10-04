#include <common.h>

// NOTE(aalhendi): MIPS owns these initialized fragments, not the intervening
// compiler tables. Derive its storage widths from the shared rData view.

#define UI_VS_WAIT_RDATA_INITIALIZER {{{0x55, 0x35}, {0xaa, 0x35}, {0x55, 0x43}, {0xaa, 0x43}}, "p%d:%2.02d"}

CTR_STATIC_ASSERT(offsetof(struct rData, uiVsWait) == 0x16ec);
CTR_STATIC_ASSERT(offsetof(struct rData, uiVsQuipDispatchStorage) == 0x1718);
CTR_STATIC_ASSERT(offsetof(struct rData, s_oxide) == 0x1740);

struct RdataModelStrings
{
#define RDATA_MODEL_STRING(field, text) char field[sizeof(((struct rData *)0)->field)];
#include "zGlobal_RDATA_models.inc"
#undef RDATA_MODEL_STRING
};

struct RdataLoadingStrings
{
#define RDATA_LOADING_STRING(field, text) char field[sizeof(((struct rData *)0)->field)];
#include "zGlobal_RDATA_loading.inc"
#undef RDATA_LOADING_STRING
};

struct RdataLevelResourceStrings
{
#define RDATA_RESOURCE_STRING(field, text) char field[sizeof(((struct rData *)0)->field)];
#include "zGlobal_RDATA_resources.inc"
#undef RDATA_RESOURCE_STRING
};

struct RdataVehicleStrings
{
#define RDATA_VEHICLE_STRING(field, text) char field[sizeof(((struct rData *)0)->field)];
#include "zGlobal_RDATA_vehicle.inc"
#undef RDATA_VEHICLE_STRING
};

// NOTE(aalhendi): Each misc fragment uses the maintained field widths. The
// list also supplies the current native seed policy without a second value list.
#define RDATA_MISC_BEGIN(cluster, firstField, sectionName) \
	struct RdataMisc##cluster                              \
	{
#define RDATA_MISC_STRING(cluster, field, text, nativeSeed) char field[sizeof(((struct rData *)0)->field)];
#define RDATA_MISC_END(cluster, nextField) \
	}                                      \
	;
#include "zGlobal_RDATA_misc.inc"
#undef RDATA_MISC_BEGIN
#undef RDATA_MISC_STRING
#undef RDATA_MISC_END

#ifdef CTR_NATIVE
#define RDATA_MISC_BEGIN(cluster, firstField, sectionName)              \
	enum                                                                \
	{                                                                   \
		RdataMisc##cluster##Offset = offsetof(struct rData, firstField) \
	};
#define RDATA_MISC_STRING(cluster, field, text, nativeSeed)                                                                      \
	CTR_STATIC_ASSERT(offsetof(struct RdataMisc##cluster, field) == offsetof(struct rData, field) - RdataMisc##cluster##Offset); \
	CTR_STATIC_ASSERT(sizeof(text) <= sizeof(((struct rData *)0)->field));
#define RDATA_MISC_END(cluster, nextField) \
	CTR_STATIC_ASSERT(sizeof(struct RdataMisc##cluster) == offsetof(struct rData, nextField) - RdataMisc##cluster##Offset);
#include "zGlobal_RDATA_misc.inc"
#undef RDATA_MISC_BEGIN
#undef RDATA_MISC_STRING
#undef RDATA_MISC_END

#define RDATA_MODEL_STRING(field, text)                                                                                                        \
	CTR_STATIC_ASSERT(offsetof(struct RdataModelStrings, field) == offsetof(struct rData, field) - offsetof(struct rData, s_STATIC_GNORMALZ)); \
	CTR_STATIC_ASSERT(sizeof(text) <= sizeof(((struct rData *)0)->field));
#include "zGlobal_RDATA_models.inc"
#undef RDATA_MODEL_STRING
CTR_STATIC_ASSERT(sizeof(struct RdataModelStrings) == offsetof(struct rData, rdataPauseData) - offsetof(struct rData, s_STATIC_GNORMALZ));

#define RDATA_LOADING_STRING(field, text)                                                                                                 \
	CTR_STATIC_ASSERT(offsetof(struct RdataLoadingStrings, field) == offsetof(struct rData, field) - offsetof(struct rData, s_screen18)); \
	CTR_STATIC_ASSERT(sizeof(text) <= sizeof(((struct rData *)0)->field));
#include "zGlobal_RDATA_loading.inc"
#undef RDATA_LOADING_STRING
CTR_STATIC_ASSERT(sizeof(struct RdataLoadingStrings) == offsetof(struct rData, jumpPointers5) - offsetof(struct rData, s_screen18));

#define RDATA_RESOURCE_STRING(field, text)                                                                                                       \
	CTR_STATIC_ASSERT(offsetof(struct RdataLevelResourceStrings, field) == offsetof(struct rData, field) - offsetof(struct rData, s_HUB_ALLOC)); \
	CTR_STATIC_ASSERT(sizeof(text) <= sizeof(((struct rData *)0)->field));
#include "zGlobal_RDATA_resources.inc"
#undef RDATA_RESOURCE_STRING
CTR_STATIC_ASSERT(sizeof(struct RdataLevelResourceStrings) == offsetof(struct rData, LOAD_TenStages_jumpPointers4) - offsetof(struct rData, s_HUB_ALLOC));

#define RDATA_VEHICLE_STRING(field, text)                                                                                              \
	CTR_STATIC_ASSERT(offsetof(struct RdataVehicleStrings, field) == offsetof(struct rData, field) - offsetof(struct rData, s_oxide)); \
	CTR_STATIC_ASSERT(sizeof(text) <= sizeof(((struct rData *)0)->field));
#include "zGlobal_RDATA_vehicle.inc"
#undef RDATA_VEHICLE_STRING
CTR_STATIC_ASSERT(sizeof(struct RdataVehicleStrings) == sizeof(struct rData) - offsetof(struct rData, s_oxide));

// NOTE(aalhendi): Native MainInit reuses part of rdata as RenderBucket scratch.
// Keep this aggregate writable; model-name contents there are not persistent.
struct rData rdata = {
    .uiVsWait = UI_VS_WAIT_RDATA_INITIALIZER,

#define RDATA_MODEL_STRING(field, text) .field = text,
#include "zGlobal_RDATA_models.inc"
#undef RDATA_MODEL_STRING

#define RDATA_MISC_BEGIN(cluster, firstField, sectionName)
#define RDATA_MISC_NATIVE_0(field, text)
#define RDATA_MISC_NATIVE_1(field, text)                    .field = text,
#define RDATA_MISC_STRING(cluster, field, text, nativeSeed) CTR_JOIN(RDATA_MISC_NATIVE_, nativeSeed)(field, text)
#define RDATA_MISC_END(cluster, nextField)
#include "zGlobal_RDATA_misc.inc"
#undef RDATA_MISC_BEGIN
#undef RDATA_MISC_NATIVE_0
#undef RDATA_MISC_NATIVE_1
#undef RDATA_MISC_STRING
#undef RDATA_MISC_END

#define RDATA_VEHICLE_STRING(field, text) .field = text,
#include "zGlobal_RDATA_vehicle.inc"
#undef RDATA_VEHICLE_STRING

#define RDATA_LOADING_STRING(field, text) .field = text,
#include "zGlobal_RDATA_loading.inc"
#undef RDATA_LOADING_STRING

#define RDATA_RESOURCE_STRING(field, text) .field = text,
#include "zGlobal_RDATA_resources.inc"
#undef RDATA_RESOURCE_STRING

};
#else
struct UI_VsWaitRdata rdataUiVsWait __attribute__((section(".rdata.ui.vs_wait"))) = UI_VS_WAIT_RDATA_INITIALIZER;

#define RDATA_MISC_BEGIN(cluster, firstField, sectionName)  struct RdataMisc##cluster rdataMisc##cluster __attribute__((section(sectionName))) = {
#define RDATA_MISC_STRING(cluster, field, text, nativeSeed) .field = text,
#define RDATA_MISC_END(cluster, nextField) \
	}                                      \
	;
#include "zGlobal_RDATA_misc.inc"
#undef RDATA_MISC_BEGIN
#undef RDATA_MISC_STRING
#undef RDATA_MISC_END

struct RdataModelStrings rdataModelStrings __attribute__((section(".rdata.model_names"))) = {
#define RDATA_MODEL_STRING(field, text) .field = text,
#include "zGlobal_RDATA_models.inc"
#undef RDATA_MODEL_STRING
};

struct RdataLoadingStrings rdataLoadingStrings __attribute__((section(".rdata.loading_names"))) = {
#define RDATA_LOADING_STRING(field, text) .field = text,
#include "zGlobal_RDATA_loading.inc"
#undef RDATA_LOADING_STRING
};

struct RdataVehicleStrings rdataVehicleStrings __attribute__((section(".rdata.vehicle_names"))) = {
#define RDATA_VEHICLE_STRING(field, text) .field = text,
#include "zGlobal_RDATA_vehicle.inc"
#undef RDATA_VEHICLE_STRING
};

struct RdataLevelResourceStrings rdataLevelResourceStrings __attribute__((section(".rdata.level_resource_names"))) = {
#define RDATA_RESOURCE_STRING(field, text) .field = text,
#include "zGlobal_RDATA_resources.inc"
#undef RDATA_RESOURCE_STRING
};
#endif

#undef UI_VS_WAIT_RDATA_INITIALIZER
