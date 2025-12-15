/**
* @file constants.c
* Numeric and string contants used by the MUD.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
* @todo Come up with a standard for descriptive arrays. Either all end with
* newlines or all of them don not.
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"	/* alias_data */
#include "constants.h"

/** Current tbaMUD version.
 * @todo cpp_extern isn't needed here (or anywhere) as the extern reserved word
 * works correctly with C compilers (at least in my Experience)
 * Jeremy Osborne 1/28/2008 */
cpp_extern const char *tbamud_version = "tbaMUD 2025";

/* strings corresponding to ordinals/bitvectors in structs.h */
/* (Note: strings for class definitions in class.c instead of here) */

/** Description of cardinal directions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest", /* Diagonals only used if CONFIG_DIAGONAL_DIRS is set */
  "northeast",
  "southeast",
  "southwest",
  "\n"
};

const char *autoexits[] =
{
  "n",
  "e",
  "s",
  "w",
  "u",
  "d",
  "nw",
  "ne",
  "se",
  "sw",
  "\n"
};

/** Room flag descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *room_bits[] = {
  "DARK",
  "DEATH",
  "NO_MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "NO_TRACK",
  "NO_MAGIC",
  "TUNNEL",
  "PRIVATE",
  "GODROOM",
  "HOUSE",
  "HCRSH",
  "ATRIUM",
  "OLC",
  "*",				/* The BFS Mark. */
  "WORLDMAP",
  "QUIT",
  "SAVE",
  "\n"
};

/** Room flag descriptions. (ZONE_x)
 * @pre Must be in the same order as the defines in structs.h.
 * Must end array with a single newline. */
const char *zone_bits[] = {
  "CLOSED",
  "NO_IMMORT",
  "QUEST",
  "GRID",
  "NOBUILD",
  "!ASTRAL",
  "WORLDMAP",
  "\n"
};

/** Exit bits for doors.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *exit_bits[] = {
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "\n"
};

/* Furniture: which body positions are supported on this piece */
const char *furniture_position_bits[] = {
  "STAND",   /* can stand on it */
  "SIT",     /* can sit on it */
  "REST",    /* can rest on it */
  "SLEEP",   /* can sleep on it */
  "\n"
};

/* Furniture flags: behavior & semantics */
const char *furniture_flag_bits[] = {
  "ON",            /* interact 'on' it (table, stage, dais) */
  "IN",            /* interact 'in' it (bed with canopy, tub) */
  "AT",            /* interact 'at' it (altar, bar, desk) */
  "BLOCKS_PASSAGE",/* occupies/blocks movement in room */
  "MOUNT_POINT",   /* anchor for ropes, mounts, etc. */
  "NO_DRAG",       /* cannot be dragged */
  "\n"
};

/** Description of the room sector type.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "Underwater",
  "In Flight",
  "Scrublands",
  "Sands",
  "Rocky Terrain",
  "Roads",
  "Underground",
  "Silt Sea",
  "Ashlands",
  "Tablelands",
  "\n"
};

/** PC and NPC sex.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *genders[] =
{
  "neutral",
  "male",
  "female",
  "\n"
};

/** Character positions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *position_types[] = {
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "\n"
};

/** Player flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *player_bits[] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "DELETED",
  "LOADRM",
  "NO_WIZL",
  "NO_DEL",
  "INVST",
  "CRYO",
  "DEAD",    /* You should never see this flag on a character in game. */
  "IBT_BUG",
  "IBT_IDEA",
  "IBT_TYPO",
  "UNUSED1",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "UNUSED5",
  "\n"
};

/** Mob action flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *action_bits[] = {
  "SPEC",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "AWARE",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR_EVIL",
  "AGGR_GOOD",
  "AGGR_NEUTRAL",
  "MEMORY",
  "HELPER",
  "NO_CHARM",
  "NO_SUMMN",
  "NO_SLEEP",
  "NO_BASH",
  "NO_BLIND",
  "NO_KILL",
  "DEAD",    /* You should never see this. */
  "\n"
};

/** PC Preference flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "NO_SHOUT",
  "NO_TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "NO_HASS",
  "QUEST",
  "SUMN",
  "NO_REP",
  "LIGHT",
  "C1",
  "C2",
  "NO_WIZ",
  "L1",
  "L2",
  "RMFLG",
  "D_AUTO",
  "CLS",
  "BLDWLK",
  "AFK",
  "AUTOLOOT",
  "AUTOGOLD",
  "AUTOSPLIT",
  "AUTOSAC",
  "AUTOASSIST",
  "AUTOMAP",
  "AUTOKEY",
  "AUTODOOR",
  "ZONERESETS",
  "\n"
};

/** Affected bits.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *affected_bits[] =
{
  "\0", /* DO NOT REMOVE!! */
  "BLIND",
  "INVIS",
  "DET-ALIGN",
  "DET-INVIS",
  "DET-MAGIC",
  "SENSE-LIFE",
  "WATWALK",
  "SANCT",
  "GROUP",
  "CURSE",
  "INFRA",
  "POISON",
  "PROT-EVIL",
  "PROT-GOOD",
  "SLEEP",
  "NO_TRACK",
  "FLY",
  "SCUBA",
  "SNEAK",
  "HIDE",
  "SCAN",
  "CHARM",
  "BANDAGED",
  "\n"
};

/** Connection type descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *connected_types[] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Disconnecting",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Text edit",
  "Config edit",
  "Social edit",
  "Trigger edit",
  "Help edit",
  "Quest edit",
  "Preference edit",
  "IBT edit",
  "Message edit",
  "Protocol Detection",
  "\n"
};

/** Describes the position in the equipment listing.
 * @pre Must be in the same order as the defines.
 * Not used in sprinttype() so no \\n. */
const char *wear_where[] = {
  "<used as light>      ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn around neck>   ",
  "<worn on back>       ",
  "<worn on body>       ",
  "<worn on head>       ",
  "<worn on legs>       ",
  "<worn on feet>       ",
  "<worn on hands>      ",
  "<worn on arms>       ",
  "<worn as shield>     ",
  "<worn about body>    ",
  "<worn about waist>   ",
  "<worn around wrist>  ",
  "<worn around wrist>  ",
  "<wielded>            ",
  "<held>               "
};

/* Describes where an item can be worn.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *equipment_types[] = {
  "Used as light",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on back",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "\n"
};

/** Describes the type of object.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FURNITURE",
  "FREE",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "FREE2",
  "CONTAINER",
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "\n"
};

/** Describes the wear flags set on an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BACK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "\n"
};

/** Describes the extra flags applied to an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *extra_bits[] = {
  "GLOW",
  "HUM",
  "NO_RENT",
  "NO_DONATE",
  "NO_INVIS",
  "INVISIBLE",
  "MAGIC",
  "NO_DROP",
  "BLESS",
  "ANTI_GOOD",
  "ANTI_EVIL",
  "ANTI_NEUTRAL",
  "ANTI_SORCEROR",
  "ANTI_CLERIC",
  "ANTI_ROGUE",
  "ANTI_FIGHTER",
  "ANTI_BARBARIAN",
  "ANTI_RANGER",
  "ANTI_BARD",
  "ANTI_DRUID",
  "NO_SELL",
  "QUEST_ITEM",
  "\n"
};

/* Number of extra flags (excluding the sentinel "\n") */
const int NUM_EXTRA_FLAGS = (sizeof(extra_bits) / sizeof(extra_bits[0]) - 1);

/** Describes the apply types.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *apply_types[] = {
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "CHA",
  "CLASS",
  "LEVEL",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "MAXMANA",
  "MAXHIT",
  "MAXMOVE",
  "GOLD",
  "EXP",
  "ARMOR",
  "SAVE_STR",
  "SAVE_DEX",
  "SAVE_CON",
  "SAVE_INT",
  "SAVE_WIS",
  "SAVE_CHA",
  "\n"
};

/** Describes the closure mechanism for a container.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};

/* Number of container flags (excluding the sentinel "\n") */
const int NUM_CONTAINER_FLAGS = (sizeof(container_bits) / sizeof(container_bits[0]) - 1);

/** Describes the liquid description.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "\n"
};

/** Describes a one word alias for each type of liquid.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "\n"
};

/** Define the effect of liquids on hunger, thirst, and drunkenness, in that
 * order. See values.doc for more information.
 * @pre Must be in the same order as the defines. */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13}
};

/** Describes the color of the various drinks.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "\n"
};

/** Used to describe the level of fullness of a drink container. Not used in
 * sprinttype() so no \\n. */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};

/** Define a set of opposite directions from the cardinal directions. */
int rev_dir[] =
{
  SOUTH,
  WEST,
  NORTH,
  EAST,
  DOWN,
  UP,
  SOUTHEAST,
  SOUTHWEST,
  NORTHWEST,
  NORTHEAST
};

/** How much movement is lost moving through a particular sector type. */
int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  6,	/* Mountains  */
  4,	/* Swimming   */
  1,	/* Unswimable */
  5,	/* Underwater */
  1,  /* Flying */
  3,  /* Scrubland */
  3,  /* Sand */
  5,  /* Rocky Hill */
  1,  /* Road */
  2,  /* Underground */
  4,  /* Silt */
  2,  /* Ashland */
  2   /* Tableland */
};

/** The names of the days of the mud week. Not used in sprinttype(). */
const char *weekdays[] = {
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the Day of the Great Gods",
  "the Day of the Sun"
};

/** The names of the mud months. Not used in sprinttype(). */
const char *month_name[] = {
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Frost Giant",
  "Month of the Old Forces",
  "Month of the Grand Struggle",
  "Month of the Spring",
  "Month of Nature",
  "Month of Futility",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Heat",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
  "Month of the Great Evil"
};

/** Names for mob trigger types. */
const char *trig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "Act",
  "Death",
  "Greet",
  "Greet-All",
  "Entry",
  "Receive",
  "Fight",
  "HitPrcnt",
  "Bribe",
  "Load",
  "Memory",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
	"Damage",
  "\n"
};

/** Names for object trigger types. */
const char *otrig_types[] = {
  "Global",
  "Random",
  "Command",
  "UNUSED1",
  "UNUSED2",
  "Timer",
  "Get",
  "Drop",
  "Give",
  "Wear",
  "UNUSED3",
  "Remove",
  "UNUSED4",
  "Load",
  "UNUSED5",
  "Cast",
  "Leave",
  "UNUSED6",
  "Consume",
  "Time",
  "\n"
};

/** Names for world (room) trigger types. */
const char *wtrig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "UNUSED1",
  "Zone Reset",
  "Enter",
  "Drop",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "UNUSED5",
  "UNUSED6",
  "UNUSED7",
  "UNUSED8",
  "Cast",
  "Leave",
  "Door",
  "Login",
  "Time",
  "\n"
};

/** The names of the different channels that history is stored for.
 * @todo Only referenced by do_history at the moment. Should be moved local
 * to that function. */
const char *history_types[] = {
  "all",
  "say",
  "wiznet",
  "tell",
  "shout",
  "\n"
};

/** Flag names for Ideas, Bugs and Typos (defined in ibt.h) */
const char *ibt_bits[] = {
  "Resolved",
  "Important",
  "InProgress",
  "\n"
};

/* 5e system helpers */

/* Armor slot table for ascending AC rules
*  Fields are AC, bulk, magic cap
*/
const struct armor_slot armor_slots[] = {
  { "head",   2, 1, 1 },
  { "body",   3, 3, 3 },
  { "legs",   2, 1, 2 },
  { "arms",   1, 1, 1 },
  { "hands",  1, 1, 1 },
  { "feet",   1, 1, 1 },
  { "right wrist",  1, 1, 1 },
  { "left wrist",  1, 1, 1 },
};

const int NUM_ARMOR_SLOTS = sizeof(armor_slots) / sizeof(armor_slots[0]);

/* Wear-position mapping for armor_slots[] order */
const int ARMOR_WEAR_POSITIONS[] = {
  WEAR_HEAD,      /* "head"  */
  WEAR_BODY,      /* "body"  */
  WEAR_LEGS,      /* "legs"  */
  WEAR_ARMS,      /* "arms"  */
  WEAR_HANDS,     /* "hands" */
  WEAR_FEET,      /* "feet"  */
  WEAR_WRIST_R,   /* "right wrist"  */
  WEAR_WRIST_L    /* "left wrist"  */
};

/* Armor flag names for obj->value[3] */
const char *armor_flag_bits[] = {
  "STEALTHDISADV",  /* ARMF_STEALTH_DISADV */
  "REQ_STR15",      /* ARMF_REQ_STR15 */
  "\n"
};

/* --- End of constants arrays. --- */

/* Various arrays we count so we can check the world files.  These
 * must be at the bottom of the file so they're pre-declared. */
  /** Number of defined room bit descriptions. */
  size_t	room_bits_count = sizeof(room_bits) / sizeof(room_bits[0]) - 1,
  /** Number of defined action bit descriptions. */
	action_bits_count = sizeof(action_bits) / sizeof(action_bits[0]) - 1,
	/** Number of defined affected bit descriptions. */
	affected_bits_count = sizeof(affected_bits) / sizeof(affected_bits[0]) - 1,
	/** Number of defined extra bit descriptions. */
	extra_bits_count = sizeof(extra_bits) / sizeof(extra_bits[0]) - 1,
	/** Number of defined wear bit descriptions. */
	wear_bits_count = sizeof(wear_bits) / sizeof(wear_bits[0]) - 1;
