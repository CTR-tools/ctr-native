// NOTE(aalhendi): nativeSeed records the current initializer policy: 1 emits
// this value into native rdata; 0 leaves its existing startup-zero storage.
// All rows emit in the default retail path. Widths come from struct rData.
RDATA_MISC_BEGIN(Bots, s_asphalt2_thisAppearsTwice, ".rdata.misc.bots")
RDATA_MISC_STRING(Bots, s_asphalt2_thisAppearsTwice, "asphalt2", 1)
RDATA_MISC_STRING(Bots, s_robotcar, "robotcar", 0)
RDATA_MISC_END(Bots, jumpPointers1)

RDATA_MISC_BEGIN(Ghost, s_lensflare, ".rdata.misc.ghost")
RDATA_MISC_STRING(Ghost, s_lensflare, "lensflare", 0)
RDATA_MISC_STRING(Ghost, s_ghost_record_buffer, "ghost record buffer", 0)
RDATA_MISC_STRING(Ghost, s_GhostTape_, "ghost tape", 0)
RDATA_MISC_END(Ghost, jumpPointers2)

RDATA_MISC_BEGIN(Sample, s_LoadSampleBlock, ".rdata.misc.sample")
RDATA_MISC_STRING(Sample, s_LoadSampleBlock, "_LoadSampleBlock", 1)
RDATA_MISC_END(Sample, jumpPointers3)

RDATA_MISC_BEGIN(Language, s_lang_buffer, ".rdata.misc.language")
RDATA_MISC_STRING(Language, s_lang_buffer, "lang buffer", 0)
RDATA_MISC_END(Language, MetaDataHubs)

RDATA_MISC_BEGIN(Menus, s_timeString, ".rdata.misc.menus")
RDATA_MISC_STRING(Menus, s_timeString, "%ld:%ld%ld:%ld%ld", 0)
RDATA_MISC_STRING(Menus, s_LoadSave, "LoadSave", 0)
RDATA_MISC_STRING(Menus, s_loadsave, "loadsave", 0)
RDATA_MISC_END(Menus, jumpPointers6)

RDATA_MISC_BEGIN(Hud, s_crystal1, ".rdata.misc.hud")
RDATA_MISC_STRING(Hud, s_crystal1, "crystal1", 1)
RDATA_MISC_STRING(Hud, s_fruitdisp, "fruitdisp", 1)
RDATA_MISC_STRING(Hud, s_timebox1, "timebox1", 1)
// NOTE(aalhendi): retail is pre-shaped; UI_DrawRaceClock patches digits only.
RDATA_MISC_STRING(Hud, s_timeString_empty, "  :  :  ", 1)
RDATA_MISC_STRING(Hud, s_lapString, "%2.02d/%ld", 1)
RDATA_MISC_END(Hud, uiVsWait)
