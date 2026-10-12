#ifndef RACE_FIELD_H
#define RACE_FIELD_H

// How many racers a race fields, and how many of them are bots.
//
// Three places have to agree on this:
// MainInit_Drivers, which spawns one driver per racer counted here; LOAD's
// random-bot selector, which fills exactly those AI slots; and the Adventure
// hub's warp pad, which lays out a starting grid this long before the race has
// loaded.
//
// Every function here is pure: the mode, the cup and the option values arrive as
// arguments, because the hub has to ask about the round it is about to load while
// its own flags still describe the hub.
//

// Racers on a starting grid, players included.
enum
{
	RACE_FIELD_FULL_GRID_RACERS = 8,
	// The shorter grid retail gives a pair: eight racers need a roster of their
	// own, and a pair on the retail roster has none.
	RACE_FIELD_PAIR_GRID_RACERS = 6,
	// The boss of a boss race, and the only bot it fields.
	RACE_FIELD_BOSS_RACERS = 1,
};

// Modes MainInit_Drivers never spawns a bot for: nothing races against a clock,
// against crystals, or behind a starting grid at all.
#define RACE_FIELD_NO_BOT_MODES \
	(GAME_CUTSCENE | CRYSTAL_CHALLENGE | RELIC_RACE | ADVENTURE_ARENA | TIME_TRIAL | MAIN_MENU | BATTLE_MODE)

// Retail writes this mask as the literal 0x2c122020.
CTR_STATIC_ASSERT(RACE_FIELD_NO_BOT_MODES == 0x2c122020);

// The eight-racer grid is the full retail roster.
CTR_STATIC_ASSERT(RACE_FIELD_FULL_GRID_RACERS == LOAD_CHARACTER_ID_COUNT);

// The Purple Gem Cup always fields its four bosses, whoever is driving.
static inline b32 RaceField_IsPurpleGemCupRace(int gameMode1, s32 cupID)
{
	return (gameMode1 & ADVENTURE_CUP) != 0 && cupID == CUP_ID_PURPLE_GEM;
}

// Modes that pick their own AI lineup write the racers past the players
// themselves, so the extended multiplayer option must leave those slots alone.
static inline b32 RaceField_HasManagedAIRoster(int gameMode1, s32 cupID)
{
	if ((gameMode1 & RACE_FIELD_NO_BOT_MODES) != 0)
	{
		return true;
	}

	// A boss race fields its own boss in a slot of its own, so the extended
	// option does not get to add racers behind it either.
	if (IS_BOSS_RACE(gameMode1))
	{
		return true;
	}

	return RaceField_IsPurpleGemCupRace(gameMode1, cupID);
}

// The extended multiplayer option fills every grid slot with a racer, in Arcade
// and in Adventure alike. Whoever writes character IDs and whoever counts AI
// slots have to agree on this, or the extra slots spawn drivers on IDs nobody
// assigned.
static inline b32 RaceField_UsesExtendedGrid(int gameMode1, s32 cupID, b32 extendedMultiplayer)
{
	if (!extendedMultiplayer)
	{
		return false;
	}

	if ((gameMode1 & (ARCADE_MODE | ADVENTURE_MODE)) == 0)
	{
		return false;
	}

	return !RaceField_HasManagedAIRoster(gameMode1, cupID);
}

// Racers on the grid for this race, players included. A race with no grid of its
// own, or one too full already for another racer, fields only its players.
static inline int RaceField_RacerCount(int gameMode1, s32 cupID, int playerCount, b32 extendedMultiplayer)
{
	b32 extendedGrid = RaceField_UsesExtendedGrid(gameMode1, cupID, extendedMultiplayer);

	if ((gameMode1 & RACE_FIELD_NO_BOT_MODES) != 0)
	{
		return playerCount;
	}

	if ((gameMode1 & (ARCADE_MODE | ADVENTURE_MODE)) == 0)
	{
		return playerCount;
	}

	// Retail caps the field at six racers, so a third or fourth player only
	// fits once the extended multiplayer option has filled the grid out.
	if (playerCount >= 3 && !extendedGrid)
	{
		return playerCount;
	}

	if (IS_BOSS_RACE(gameMode1))
	{
		return playerCount + RACE_FIELD_BOSS_RACERS;
	}

	if (RaceField_IsPurpleGemCupRace(gameMode1, cupID))
	{
		return playerCount + LOAD_2P_AI_SET_RACER_COUNT;
	}

	if ((playerCount == 1) || extendedGrid)
	{
		return RACE_FIELD_FULL_GRID_RACERS;
	}

	return RACE_FIELD_PAIR_GRID_RACERS;
}

// Bots on the grid: the racers past the players.
static inline int RaceField_BotCount(int gameMode1, s32 cupID, int playerCount, b32 extendedMultiplayer)
{
	return RaceField_RacerCount(gameMode1, cupID, playerCount, extendedMultiplayer) - playerCount;
}

#endif