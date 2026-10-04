OUTPUT_ARCH(mips)

/* Only these sections own bytes; intervening tables and padding stay unowned. */
SECTIONS
{
    .rdata.misc.bots 0x80010000 : { *(.rdata.misc.bots) }
    .rdata.model_names 0x800100cc : { *(.rdata.model_names) }
    .rdata.misc.ghost 0x80011074 : { *(.rdata.misc.ghost) }
    .rdata.misc.sample 0x800110cc : { *(.rdata.misc.sample) }
    .rdata.misc.language 0x80011174 : { *(.rdata.misc.language) }
    .rdata.level_resource_names 0x800111bc : { *(.rdata.level_resource_names) }
    .rdata.loading_names 0x800112e4 : { *(.rdata.loading_names) }
    .rdata.misc.menus 0x80011620 : { *(.rdata.misc.menus) }
    .rdata.misc.hud 0x800116b0 : { *(.rdata.misc.hud) }
    .rdata.ui.vs_wait 0x800116ec : { *(.rdata.ui.vs_wait) }
    .rdata.vehicle_names 0x80011740 : { *(.rdata.vehicle_names) }

    /DISCARD/ :
    {
        *(.reginfo) *(.MIPS.abiflags) *(.pdr)
        *(.gnu.attributes) *(.comment)
    }
}

ASSERT(rdataMiscBots == ADDR(.rdata.misc.bots), "Bots strings symbol misplaced")
ASSERT(SIZEOF(.rdata.misc.bots) == 0x18, "Bots strings initialized size changed")
ASSERT(rdataModelStrings == ADDR(.rdata.model_names), "Model strings symbol misplaced")
ASSERT(SIZEOF(.rdata.model_names) == 0xf98, "Model strings initialized size changed")
ASSERT(rdataMiscGhost == ADDR(.rdata.misc.ghost), "Ghost strings symbol misplaced")
ASSERT(SIZEOF(.rdata.misc.ghost) == 0x2c, "Ghost strings initialized size changed")
ASSERT(rdataMiscSample == ADDR(.rdata.misc.sample), "Sample string symbol misplaced")
ASSERT(SIZEOF(.rdata.misc.sample) == 0x14, "Sample string initialized size changed")
ASSERT(rdataMiscLanguage == ADDR(.rdata.misc.language), "Language string symbol misplaced")
ASSERT(SIZEOF(.rdata.misc.language) == 0xc, "Language string initialized size changed")
ASSERT(rdataLevelResourceStrings == ADDR(.rdata.level_resource_names), "Level resource strings symbol misplaced")
ASSERT(SIZEOF(.rdata.level_resource_names) == 0xe8, "Level resource strings initialized size changed")
ASSERT(rdataLoadingStrings == ADDR(.rdata.loading_names), "Loading strings symbol misplaced")
ASSERT(SIZEOF(.rdata.loading_names) == 0x1a8, "Loading strings initialized size changed")
ASSERT(rdataMiscMenus == ADDR(.rdata.misc.menus), "Menu strings symbol misplaced")
ASSERT(SIZEOF(.rdata.misc.menus) == 0x30, "Menu strings initialized size changed")
ASSERT(rdataMiscHud == ADDR(.rdata.misc.hud), "HUD strings symbol misplaced")
ASSERT(SIZEOF(.rdata.misc.hud) == 0x3c, "HUD strings initialized size changed")
ASSERT(rdataUiVsWait == ADDR(.rdata.ui.vs_wait), "VS wait data symbol misplaced")
ASSERT(SIZEOF(.rdata.ui.vs_wait) == 0x1c, "VS wait initialized size changed")
ASSERT(rdataVehicleStrings == ADDR(.rdata.vehicle_names), "Vehicle strings symbol misplaced")
ASSERT(SIZEOF(.rdata.vehicle_names) == 0x260, "Vehicle strings initialized size changed")
