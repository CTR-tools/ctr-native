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
};

static int UI_Chaos_CollectEntries(const struct ChaosRuntime *runtime, const struct ChaosActivation **entries)
{
	int count = 0;
	for (int i = 0; i < runtime->numActive; i++)
	{
		entries[count++] = &runtime->active[i];
	}
	for (int i = 0; i < runtime->numInstantNotices; i++)
	{
		entries[count++] = &runtime->instantNotices[i];
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

void UI_Chaos_DrawActivation(struct GameTracker *gGT)
{
	if (!Chaos_IsRunning(gGT) || (gGT->hudFlags & HUD_FLAG_RACE_HUD) == 0 || gGT->numPlyrCurrGame < 1 ||
	    MainGameEnd_AllHumansFinished(gGT))
	{
		return;
	}
	const struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	const struct ChaosActivation *entries[UI_CHAOS_MAX_ENTRIES];
	Chaos_DrawWrongWayWarnings(gGT);
	const int count = UI_Chaos_CollectEntries(runtime, entries);
	// Runtime activation and notice queues already contain unique effect IDs.
	int rowY[UI_CHAOS_MAX_ENTRIES];
	int docked[UI_CHAOS_MAX_DOCKED];
	int numDocked = 0;
	const int dockTop = gGT->numPlyrCurrGame > 1 ? UI_CHAOS_LIST_MULTIPLAYER_TOP : UI_CHAOS_LIST_TOP;
	int dockBottom = dockTop;
	struct UI_Chaos_ReservedRow reserved[ADV_MULTIPLAYER_NUM_PLAYERS_MAX * 2];
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
