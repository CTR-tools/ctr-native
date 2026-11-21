#include <common.h>

enum
{
	UI_CHAOS_NOTICE_SLIDE_MS = 320,
	UI_CHAOS_NOTICE_MULTIPLAYER_Y = 100,
	UI_CHAOS_BANNER_ROW_HEIGHT = 20,
	UI_CHAOS_COMPACT_BANNER_ROW_HEIGHT = 9,
	UI_CHAOS_LIST_RIGHT = SCREEN_WIDTH - 12,
	UI_CHAOS_LIST_TOP = 64,
	UI_CHAOS_LIST_MULTIPLAYER_TOP = 32,
	UI_CHAOS_MAX_DOCKED = CHAOS_MAX_ACTIVATIONS,
	UI_CHAOS_LIST_ROW_HEIGHT = 6,
	UI_CHAOS_LIST_SCALE = FP(0.5),
	UI_CHAOS_ROW_GAP = 2,
	UI_CHAOS_SCREEN_MARGIN = 8,
	UI_CHAOS_FINAL_LAP_HEIGHT = 17,
	UI_CHAOS_MAX_ENTRIES = CHAOS_MAX_ACTIVATIONS + CHAOS_MAX_INSTANT_NOTICES,
	UI_CHAOS_MAX_PERSONAL_ENTRIES = UI_CHAOS_MAX_ENTRIES * CHAOS_MAX_HUMAN_PLAYERS,
};

static int UI_Chaos_CollectEntries(const struct ChaosPlayerRuntime *pool, const struct ChaosActivation **entries)
{
	int count = 0;
	for (int i = 0; i < pool->numActive; i++)
	{
		entries[count++] = &pool->active[i];
	}
	for (int i = 0; i < pool->numInstantNotices; i++)
	{
		entries[count++] = &pool->instantNotices[i];
	}
	// Older entries first, regardless of which lifecycle owns their storage.
	for (int i = 1; i < count; i++)
	{
		const struct ChaosActivation *entry = entries[i];
		int index = i;
		while (index > 0 && entries[index - 1]->elapsedMS < entry->elapsedMS)
		{
			entries[index] = entries[index - 1];
			index--;
		}
		entries[index] = entry;
	}
	return count;
}

struct UI_Chaos_ReservedRow
{
	int top;
	int bottom;
	bool bannersOnly;
};

static int UI_Chaos_CollectReservedRows(struct GameTracker *gGT, struct UI_Chaos_ReservedRow *rows)
{
	int count = 0;
	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		if (sdata->finalLapTextTimer[i] > 0)
		{
			const struct PushBuffer *pb = &gGT->pushBuffer[i];
			const int lapY = pb->rect.y + pb->rect.h / 4;
			rows[count++] = (struct UI_Chaos_ReservedRow){lapY, lapY + UI_CHAOS_FINAL_LAP_HEIGHT, true};
		}
		const struct ChaosRestart *restart = &sdata->chaosRuntime.restart[i];
		RECT lights;
		if (restart->active && DotLights_GetBounds(gGT, i, restart->timer, &lights))
		{
			rows[count++] = (struct UI_Chaos_ReservedRow){lights.y, lights.y + lights.h, false};
		}
	}
	for (int i = 1; i < count; i++)
	{
		struct UI_Chaos_ReservedRow row = rows[i];
		int j = i;
		while (j > 0 && rows[j - 1].top > row.top)
		{
			rows[j] = rows[j - 1];
			j--;
		}
		rows[j] = row;
	}
	return count;
}

static int UI_Chaos_FreeRowY(const struct UI_Chaos_ReservedRow *rows, int count, int y, int height, bool down, bool banner)
{
	for (int step = 0; step < count; step++)
	{
		const struct UI_Chaos_ReservedRow *row = &rows[down ? step : count - 1 - step];
		if (row->bannersOnly && !banner)
		{
			continue;
		}
		if (y < row->bottom && y + height > row->top)
		{
			y = down ? row->bottom + UI_CHAOS_ROW_GAP : row->top - height - UI_CHAOS_ROW_GAP;
		}
	}
	return y;
}

static s16 UI_Chaos_LayoutNotices(struct GameTracker *gGT, const struct ChaosActivation *const *entries,
                                int count, const struct UI_Chaos_ReservedRow *reserved, int numReserved,
                                int *rowY, int dockTop, int dockBottom)
{
	const bool stackDown = gGT->numPlyrCurrGame == 1;
	const int soloHeight = gGT->pushBuffer[0].rect.h > 0 ? gGT->pushBuffer[0].rect.h : SCREEN_HEIGHT;
	int y = gGT->numPlyrCurrGame > 1 ? UI_CHAOS_NOTICE_MULTIPLAYER_Y : soloHeight / 4;
	for (int i = count - 1; i >= 0; i--)
	{
		const struct ChaosActivation *entry = entries[i];
		if (ChaosEffect_Def(entry->effect) == NULL ||
		    entry->noticeRemainingMS <= (ChaosEffect_IsInstant(entry->effect) ? 0 : CHAOS_ACTIVATION_DOCK_MS))
		{
			continue;
		}
		y = UI_Chaos_FreeRowY(reserved, numReserved, y, UI_CHAOS_BANNER_ROW_HEIGHT, stackDown, true);
		if (y < UI_CHAOS_SCREEN_MARGIN || y + UI_CHAOS_BANNER_ROW_HEIGHT > SCREEN_HEIGHT - UI_CHAOS_SCREEN_MARGIN)
		{
			// A crowded announcement stack uses compact rows below the docked list.
			y = dockBottom > dockTop ? dockBottom + UI_CHAOS_ROW_GAP : UI_CHAOS_SCREEN_MARGIN;
			for (int j = 0; j < count; j++)
			{
				if (entries[j]->noticeRemainingMS >
				    (ChaosEffect_IsInstant(entries[j]->effect) ? 0 : CHAOS_ACTIVATION_DOCK_MS))
				{
					y = UI_Chaos_FreeRowY(reserved, numReserved, y, UI_CHAOS_COMPACT_BANNER_ROW_HEIGHT, true, true);
					rowY[j] = y;
					y += UI_CHAOS_COMPACT_BANNER_ROW_HEIGHT;
				}
			}
			return FONT_SMALL;
		}
		rowY[i] = y;
		y += stackDown ? UI_CHAOS_BANNER_ROW_HEIGHT : -UI_CHAOS_BANNER_ROW_HEIGHT;
	}
	return FONT_BIG;
}

static void UI_Chaos_DrawNotice(struct GameTracker *gGT, const struct ChaosActivation *entry,
                               int bannerY, s16 bannerFont)
{
	const char *effectLabel = LANG_GetString(LNG_EXPANDED_CHAOS_EFFECT_BASE + entry->effect);
	const bool instant = ChaosEffect_IsInstant(entry->effect);
	const int elapsedMS = CHAOS_ACTIVATION_NOTICE_MS + (instant ? 0 : CHAOS_ACTIVATION_DOCK_MS) - entry->noticeRemainingMS;
	const int centerX = SCREEN_WIDTH / 2;
	int startX = centerX;
	int endX = centerX;
	int slideMS = 0;
	int slideDurationMS = UI_CHAOS_NOTICE_SLIDE_MS;
	s16 font = bannerFont;
	s32 scale = FP(1.0);
	u32 *ot = gGT->backBuffer->otMem.uiOT;
	u16 flags = JUSTIFY_CENTER | ORANGE;
	if (elapsedMS < UI_CHAOS_NOTICE_SLIDE_MS)
	{
		startX = SCREEN_WIDTH + DecalFont_GetLineWidth((char *)effectLabel, font) / 2;
		slideMS = elapsedMS;
	}
	else if (!instant && entry->noticeRemainingMS <= CHAOS_ACTIVATION_DOCK_MS)
	{
		font = FONT_SMALL;
		scale = UI_CHAOS_LIST_SCALE;
		ot += UI_HUD_ORDERING_SLOT_BEHIND_HUD;
		flags = JUSTIFY_RIGHT | ORANGE;
		startX = centerX + DecalFont_GetLineWidth((char *)effectLabel, font) * scale / FP(1.0) / 2;
		endX = UI_CHAOS_LIST_RIGHT;
		slideMS = CHAOS_ACTIVATION_DOCK_MS - entry->noticeRemainingMS;
		slideDurationMS = CHAOS_ACTIVATION_DOCK_MS;
	}
	SVec2 pos;
	UI_Lerp2D_Linear(CTR_VECTOR_DATA(&pos), (s16)startX, (s16)bannerY, (s16)endX, (s16)bannerY,
	                 slideMS, (s16)slideDurationMS);
	DecalFont_DrawLineScaledOT((char *)effectLabel, pos.x, pos.y, font, (s16)flags, scale,
	                           ot);
}

static int UI_Chaos_CollectPersonalEntries(int player, int players, const struct ChaosActivation **entries)
{
	int count = 0;
	// Put the viewport's own effects first, then relevant effects from other pools.
	for (int offset = 0; offset < players; offset++)
	{
		const int owner = (player + offset) % players;
		const struct ChaosActivation *poolEntries[UI_CHAOS_MAX_ENTRIES];
		const int numEntries = UI_Chaos_CollectEntries(&sdata->chaosRuntime.players[owner], poolEntries);
		for (int i = numEntries - 1; i >= 0; i--)
		{
			const struct ChaosActivation *entry = poolEntries[i];
			const ChaosEffectDef *def = ChaosEffect_Def(entry->effect);
			if (def == NULL)
			{
				continue;
			}
			const bool global = def->scope == CHAOS_SCOPE_BOTS || entry->effect == CHAOS_EFFECT_ZA_WARUDO;
			const bool partner = entry->effect == CHAOS_EFFECT_BODY_SWAP &&
			    sdata->chaosRuntime.bodySwapPartner[owner] == player + 1;
			if ((Chaos_IsPlayerEnabled(player) && (owner == player || partner)) || global)
			{
				entries[count++] = entry;
			}
		}
	}
	return count;
}

static bool UI_Chaos_LayoutPersonalRows(const RECT *viewport, int top, int height, int count,
                                      const struct UI_Chaos_ReservedRow *reserved, int numReserved, int *rowY)
{
	int y = viewport->y + top;
	for (int i = 0; i < count; i++)
	{
		y = UI_Chaos_FreeRowY(reserved, numReserved, y, height, true, false);
		if (y + height > viewport->y + viewport->h - UI_CHAOS_SCREEN_MARGIN)
		{
			return false;
		}
		rowY[i] = y;
		y += height;
	}
	return true;
}

static void UI_Chaos_DrawPersonal(struct GameTracker *gGT)
{
	struct UI_Chaos_ReservedRow reserved[CHAOS_MAX_HUMAN_PLAYERS * 2];
	const int numReserved = UI_Chaos_CollectReservedRows(gGT, reserved);
	for (int player = 0; player < gGT->numPlyrCurrGame && player < CHAOS_MAX_HUMAN_PLAYERS; player++)
	{
		const struct Driver *driver = gGT->drivers[player];
		const RECT *viewport = &gGT->pushBuffer[player].rect;
		if (driver == NULL || (driver->actionsFlagSet & ACTION_RACE_FINISHED) != 0 || viewport->w <= 0 || viewport->h <= 0)
		{
			continue;
		}
		const struct ChaosActivation *entries[UI_CHAOS_MAX_PERSONAL_ENTRIES];
		const int count = UI_Chaos_CollectPersonalEntries(player, gGT->numPlyrCurrGame, entries);
		int rowY[UI_CHAOS_MAX_PERSONAL_ENTRIES];
		int height = UI_CHAOS_LIST_ROW_HEIGHT;
		if (!UI_Chaos_LayoutPersonalRows(viewport, UI_CHAOS_LIST_MULTIPLAYER_TOP, height, count, reserved, numReserved, rowY))
		{
			// Compact the stack only when restart lights and a full pool require it.
			while (!UI_Chaos_LayoutPersonalRows(viewport, UI_CHAOS_SCREEN_MARGIN, height, count, reserved, numReserved, rowY))
			{
				if (--height == 0)
				{
					break;
				}
			}
		}
		if (height == 0)
		{
			continue;
		}
		for (int i = 0; i < count; i++)
		{
			const struct ChaosActivation *entry = entries[i];
			const char *name = LANG_GetString(LNG_EXPANDED_CHAOS_EFFECT_BASE + entry->effect);
			s32 scale = UI_CHAOS_LIST_SCALE * height / UI_CHAOS_LIST_ROW_HEIGHT;
			const int width = DecalFont_GetLineWidth((char *)name, FONT_SMALL);
			const int available = viewport->w - UI_CHAOS_SCREEN_MARGIN * 2;
			if (width > 0 && width * scale / FP(1.0) > available)
			{
				scale = available * FP(1.0) / width;
			}
			DecalFont_DrawLineScaledOT((char *)name, viewport->x + viewport->w - UI_CHAOS_SCREEN_MARGIN,
			                           rowY[i], FONT_SMALL, JUSTIFY_RIGHT | ORANGE, scale,
			                           &gGT->backBuffer->otMem.uiOT[UI_HUD_ORDERING_SLOT_BEHIND_HUD]);
		}
	}
}

void UI_Chaos_DrawActivation(struct GameTracker *gGT)
{
	if (!Chaos_IsRunning(gGT) || (gGT->hudFlags & HUD_FLAG_RACE_HUD) == 0 || gGT->numPlyrCurrGame < 1 ||
	    MainGameEnd_AllHumansFinished(gGT))
	{
		return;
	}
	const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[0];
	const struct ChaosActivation *entries[UI_CHAOS_MAX_ENTRIES];
	Chaos_DrawWrongWayWarnings(gGT);
	if (Chaos_UsesPersonalPools())
	{
		UI_Chaos_DrawPersonal(gGT);
		return;
	}
	const int count = UI_Chaos_CollectEntries(pool, entries);
	// Runtime activation and notice queues already contain unique effect IDs.
	int rowY[UI_CHAOS_MAX_ENTRIES];
	int docked[UI_CHAOS_MAX_DOCKED];
	int numDocked = 0;
	const int dockTop = gGT->numPlyrCurrGame > 1 ? UI_CHAOS_LIST_MULTIPLAYER_TOP : UI_CHAOS_LIST_TOP;
	int dockBottom = dockTop;
	struct UI_Chaos_ReservedRow reserved[CHAOS_MAX_HUMAN_PLAYERS * 2];
	const int numReserved = UI_Chaos_CollectReservedRows(gGT, reserved);
	for (int i = count - 1; i >= 0 && numDocked < UI_CHAOS_MAX_DOCKED; i--)
	{
		const struct ChaosActivation *entry = entries[i];
		if (ChaosEffect_Def(entry->effect) != NULL && !ChaosEffect_IsInstant(entry->effect) &&
		    entry->noticeRemainingMS <= CHAOS_ACTIVATION_DOCK_MS)
		{
			dockBottom = UI_Chaos_FreeRowY(reserved, numReserved, dockBottom, UI_CHAOS_LIST_ROW_HEIGHT, true, false);
			rowY[i] = dockBottom;
			docked[numDocked++] = i;
			dockBottom += UI_CHAOS_LIST_ROW_HEIGHT;
		}
	}
	const s16 bannerFont = UI_Chaos_LayoutNotices(gGT, entries, count, reserved, numReserved, rowY, dockTop, dockBottom);
	for (int row = 0; row < numDocked; row++)
	{
		const struct ChaosActivation *entry = entries[docked[row]];
		if (entry->noticeRemainingMS > 0)
		{
			UI_Chaos_DrawNotice(gGT, entry, rowY[docked[row]], FONT_SMALL);
		}
		else
		{
			DecalFont_DrawLineScaledOT((char *)LANG_GetString(LNG_EXPANDED_CHAOS_EFFECT_BASE + entry->effect), UI_CHAOS_LIST_RIGHT, (s16)rowY[docked[row]],
			                           FONT_SMALL, JUSTIFY_RIGHT | ORANGE, UI_CHAOS_LIST_SCALE,
			                           &gGT->backBuffer->otMem.uiOT[UI_HUD_ORDERING_SLOT_BEHIND_HUD]);
		}
	}
	for (int i = 0; i < count; i++)
	{
		const struct ChaosActivation *entry = entries[i];
		if (ChaosEffect_Def(entry->effect) != NULL && entry->noticeRemainingMS >
		    (ChaosEffect_IsInstant(entry->effect) ? 0 : CHAOS_ACTIVATION_DOCK_MS))
		{
			UI_Chaos_DrawNotice(gGT, entry, rowY[i], bannerFont);
		}
	}
}
