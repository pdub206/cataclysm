/**************************************************************************
*  File: class.c                                           Part of tbaMUD *
*  Usage: Source file for class-specific code.                            *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/* This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class, you
 * should go through this entire file from beginning to end and add the
 * appropriate new special cases for your new class. */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "class.h"

/* Names first */
const char *class_abbrevs[] = {
  "Mu",
  "Cl",
  "Ro",
  "Wa",
  "Ba",
  "Ra",
  "Br",
  "Dr",
  "\n"
};

const char *pc_class_types[] = {
  "Sorceror",
  "Cleric",
  "Rogue",
  "Fighter",
  "Barbarian",
  "Ranger",
  "Bard",
  "Druid",
  "\n"
};

/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"\r\n"
"Select a class:\r\n"
"  [\t(C\t)]leric\r\n"
"  R[\t(o\t)]gue\r\n"
"  [\t(F\t)]ighter\r\n"
"  [\t(S\t)]orceror\r\n"
"  [\t(B\t)]arbarian\r\n"
"  [\t(R\t)]anger\r\n"
"  B[\t(a\t)]rd\r\n"
"  [\t(D\t)]ruid\r\n";

/* The code to interpret a class letter -- used in interpreter.c when a new
 * character is selecting a class and by 'set class' in act.wizard.c. */
int parse_class(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 's': return CLASS_SORCEROR;
  case 'c': return CLASS_CLERIC;
  case 'f': return CLASS_FIGHTER;
  case 'o': return CLASS_ROGUE;
  case 'b': return CLASS_BARBARIAN;
  case 'r': return CLASS_RANGER;
  case 'a': return CLASS_BARD;
  case 'd': return CLASS_DRUID;
  default:  return CLASS_UNDEFINED;
  }
}

/* bitvectors (i.e., powers of two) for each class, mainly for use in do_who
 * and do_users.  Add new classes at the end so that all classes use sequential
 * powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, etc.) up to
 * the limit of your bitvector_t, typically 0-31. */
bitvector_t find_class_bitvector(const char *arg)
{
  size_t rpos, ret = 0;

  for (rpos = 0; rpos < strlen(arg); rpos++)
    ret |= (1 << parse_class(arg[rpos]));

  return (ret);
}

/* These are definitions which control the guildmasters for each class.
 * The  first field (top line) controls the highest percentage skill level a
 * character of the class is allowed to attain in any skill.  (After this
 * level, attempts to practice will say "You are already learned in this area."
 *
 * The second line controls the maximum percent gain in learnedness a character
 * is allowed per practice -- in other words, if the random die throw comes out
 * higher than this number, the gain will only be this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a character
 * is allowed per practice -- in other words, if the random die throw comes
 * out below this number, the gain will be set up to this number.
 *
 * The fourth line simply sets whether the character knows 'spells' or 'skills'.
 * This does not affect anything except the message given to the character when
 * trying to practice (i.e. "You know of the following spells" vs. "You know of
 * the following skills" */

#define SPELL	0
#define SKILL	1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] = {
  /* SOR	CLE	THE	FIG BAR RAN BARD DRU */
  { 95,		95,	85,	80, 75, 85, 85, 95	},	/* learned level */
  { 100,	100,	12,	12, 11, 12, 13, 90	},	/* max per practice */
  { 25,		25,	0,	0, 0, 0, 25, 25	},	/* min per practice */
  { SPELL,	SPELL,	SKILL,	SKILL, SKILL, SKILL, SKILL, SKILL	},	/* prac name */
};

/* The appropriate rooms for each guildmaster/guildguard; controls which types
 * of people the various guildguards let through.  i.e., the first line shows
 * that from room 3017, only SORCERORS are allowed to go south. Don't forget
 * to visit spec_assign.c if you create any new mobiles that should be a guild
 * master or guard so they can act appropriately. If you "recycle" the
 * existing mobs that are used in other guilds for your new guild, then you
 * don't have to change that file, only here. Guildguards are now implemented
 * via triggers. This code remains as an example. */
/* TO-DO: Is this necessary anymore now that there are no official guild rooms? */
struct guild_info_type guild_info[] = {

/* Midgaard */
 { CLASS_SORCEROR,      3017,    SOUTH   },
 { CLASS_CLERIC,        3004,    NORTH   },
 { CLASS_ROGUE,         3027,    EAST   },
 { CLASS_FIGHTER,       3021,    EAST   },
 { CLASS_BARBARIAN,     3021,    EAST   },
 { CLASS_RANGER,        3021,    EAST   },
 { CLASS_BARD,          3021,    EAST   },
 { CLASS_DRUID,         3021,    EAST   },

/* Brass Dragon */
  { -999 /* all */ ,	5065,	WEST	},

/* this must go last -- add new guards above! */
  { -1, NOWHERE, -1}
};

/* 5e system saving throws per class*/
bool has_save_proficiency(int class_num, int ability) {
  switch (class_num) {
    case CLASS_SORCEROR:   return (ability == ABIL_CON || ability == ABIL_CHA);
    case CLASS_CLERIC:     return (ability == ABIL_WIS || ability == ABIL_CHA);
    case CLASS_ROGUE:      return (ability == ABIL_DEX || ability == ABIL_INT);
    case CLASS_FIGHTER:    return (ability == ABIL_STR || ability == ABIL_CON);
    case CLASS_BARBARIAN:  return (ability == ABIL_STR || ability == ABIL_CON);
    case CLASS_RANGER:     return (ability == ABIL_STR || ability == ABIL_DEX);
    case CLASS_BARD:       return (ability == ABIL_DEX || ability == ABIL_CHA);
    case CLASS_DRUID:      return (ability == ABIL_INT || ability == ABIL_WIS);
    default: return FALSE;
  }
}

/* Roll the 6 stats for a character... each stat is made of the sum of the best
 * 3 out of 4 rolls of a 6-sided die.  Each class then decides which priority
 * will be given for the best to worst stats. */
void roll_real_abils(struct char_data *ch)
{
  int i, j, k, temp;
  ubyte table[6];
  ubyte rolls[4];

  for (i = 0; i < 6; i++)
    table[i] = 0;

  for (i = 0; i < 6; i++) {

    for (j = 0; j < 4; j++)
      rolls[j] = rand_number(1, 6);

    temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
      MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

    for (k = 0; k < 6; k++)
      if (table[k] < temp) {
	temp ^= table[k];
	table[k] ^= temp;
	temp ^= table[k];
      }
  }

  switch (GET_CLASS(ch)) {
  case CLASS_SORCEROR:
    ch->real_abils.cha = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.str = table[5];
    break;
  case CLASS_CLERIC:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.dex = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_ROGUE:
    ch->real_abils.dex = table[0];
    ch->real_abils.str = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_FIGHTER:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
  case CLASS_BARBARIAN:
    ch->real_abils.str = table[0];
    ch->real_abils.con = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_RANGER:
    ch->real_abils.dex = table[0];
    ch->real_abils.con = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_BARD:
    ch->real_abils.cha = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.str = table[5];
    break;
  case CLASS_DRUID:
    ch->real_abils.wis = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.str = table[4];
    ch->real_abils.cha = table[5];
    break;
  }
  ch->aff_abils = ch->real_abils;
}

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;

  set_title(ch, NULL);
  roll_real_abils(ch);

  GET_MAX_HIT(ch)  = 90;
  GET_MAX_MANA(ch) = 100;
  GET_MAX_MOVE(ch) = 90;

  switch (GET_CLASS(ch)) {

  case CLASS_SORCEROR:
    SET_SKILL(ch, SPELL_MAGIC_MISSILE, 5);
    SET_SKILL(ch, SPELL_DETECT_INVIS, 5);
    SET_SKILL(ch, SPELL_DETECT_MAGIC, 5);
    SET_SKILL(ch, SPELL_CHILL_TOUCH, 5);
    SET_SKILL(ch, SPELL_INFRAVISION, 5);
    SET_SKILL(ch, SPELL_INVISIBLE, 5);
    SET_SKILL(ch, SPELL_ARMOR, 5);
    SET_SKILL(ch, SPELL_BURNING_HANDS, 5);
    SET_SKILL(ch, SPELL_LOCATE_OBJECT, 5);
    SET_SKILL(ch, SPELL_STRENGTH, 5);
    SET_SKILL(ch, SPELL_SHOCKING_GRASP, 5);
    SET_SKILL(ch, SPELL_SLEEP, 5);
    SET_SKILL(ch, SPELL_LIGHTNING_BOLT, 5);
    SET_SKILL(ch, SPELL_BLINDNESS, 5);
    SET_SKILL(ch, SPELL_DETECT_POISON, 5);
    SET_SKILL(ch, SPELL_COLOR_SPRAY, 5);
    SET_SKILL(ch, SPELL_ENERGY_DRAIN, 5);
    SET_SKILL(ch, SPELL_CURSE, 5);
    SET_SKILL(ch, SPELL_POISON, 5);
    SET_SKILL(ch, SPELL_FIREBALL, 5);
    SET_SKILL(ch, SPELL_CHARM, 5);
    SET_SKILL(ch, SPELL_IDENTIFY, 5);
    SET_SKILL(ch, SPELL_FLY, 5);
    SET_SKILL(ch, SPELL_ENCHANT_WEAPON, 5);
    SET_SKILL(ch, SPELL_CLONE, 5);
    SET_SKILL(ch, SKILL_UNARMED, 5);
    break;

  case CLASS_CLERIC:
    SET_SKILL(ch, SPELL_CURE_LIGHT, 5);
    SET_SKILL(ch, SPELL_ARMOR, 5);
    SET_SKILL(ch, SPELL_CREATE_FOOD, 5);
    SET_SKILL(ch, SPELL_CREATE_WATER, 5);
    SET_SKILL(ch, SPELL_DETECT_POISON, 5);
    SET_SKILL(ch, SPELL_DETECT_ALIGN, 5);
    SET_SKILL(ch, SPELL_CURE_BLIND, 5);
    SET_SKILL(ch, SPELL_BLESS, 5);
    SET_SKILL(ch, SPELL_DETECT_INVIS, 5);
    SET_SKILL(ch, SPELL_BLINDNESS, 5);
    SET_SKILL(ch, SPELL_INFRAVISION, 5);
    SET_SKILL(ch, SPELL_PROT_FROM_EVIL, 5);
    SET_SKILL(ch, SPELL_POISON, 5);
    SET_SKILL(ch, SPELL_GROUP_ARMOR, 5);
    SET_SKILL(ch, SPELL_CURE_CRITIC, 5);
    SET_SKILL(ch, SPELL_SUMMON, 5);
    SET_SKILL(ch, SPELL_REMOVE_POISON, 5);
    SET_SKILL(ch, SPELL_IDENTIFY, 5);
    SET_SKILL(ch, SPELL_WORD_OF_RECALL, 5);
    SET_SKILL(ch, SPELL_DARKNESS, 5);
    SET_SKILL(ch, SPELL_EARTHQUAKE, 5);
    SET_SKILL(ch, SPELL_DISPEL_EVIL, 5);
    SET_SKILL(ch, SPELL_DISPEL_GOOD, 5);
    SET_SKILL(ch, SPELL_SANCTUARY, 5);
    SET_SKILL(ch, SPELL_CALL_LIGHTNING, 5);
    SET_SKILL(ch, SPELL_HEAL, 5);
    SET_SKILL(ch, SPELL_CONTROL_WEATHER, 5);
    SET_SKILL(ch, SPELL_SENSE_LIFE, 5);
    SET_SKILL(ch, SPELL_HARM, 5);
    SET_SKILL(ch, SPELL_GROUP_HEAL, 5);
    SET_SKILL(ch, SPELL_REMOVE_CURSE, 5);
    SET_SKILL(ch, SKILL_SHIELD_USE, 5);
    break;

  case CLASS_ROGUE:
    SET_SKILL(ch, SKILL_SNEAK, 5);
    SET_SKILL(ch, SKILL_HIDE, 5);
    SET_SKILL(ch, SKILL_TRACK, 5);
    SET_SKILL(ch, SKILL_STEAL, 5);
    SET_SKILL(ch, SKILL_BACKSTAB, 5);
    SET_SKILL(ch, SKILL_PICK_LOCK, 5);
    SET_SKILL(ch, SKILL_UNARMED, 5);
    SET_SKILL(ch, SKILL_SHIELD_USE, 5);
    SET_SKILL(ch, SKILL_PIERCING_WEAPONS, 5);
    SET_SKILL(ch, SKILL_PERCEPTION, 5);
    break;

  case CLASS_FIGHTER:
    SET_SKILL(ch, SKILL_KICK, 5);
    SET_SKILL(ch, SKILL_RESCUE, 5);
    SET_SKILL(ch, SKILL_BANDAGE, 5);
    SET_SKILL(ch, SKILL_BASH, 5);
    SET_SKILL(ch, SKILL_UNARMED, 5);
    SET_SKILL(ch, SKILL_SLASHING_WEAPONS, 5);
    SET_SKILL(ch, SKILL_PIERCING_WEAPONS, 5);
    SET_SKILL(ch, SKILL_BLUDGEONING_WEAPONS, 5);
    SET_SKILL(ch, SKILL_SHIELD_USE, 5);
    SET_SKILL(ch, SKILL_PERCEPTION, 5);
    break;

  case CLASS_BARBARIAN:
    SET_SKILL(ch, SKILL_KICK, 5);
    SET_SKILL(ch, SKILL_RESCUE, 5);
    SET_SKILL(ch, SKILL_BANDAGE, 5);
    SET_SKILL(ch, SKILL_WHIRLWIND, 5);
    SET_SKILL(ch, SKILL_UNARMED, 5);
    SET_SKILL(ch, SKILL_PIERCING_WEAPONS, 5);
    SET_SKILL(ch, SKILL_BLUDGEONING_WEAPONS, 5);
    SET_SKILL(ch, SKILL_PERCEPTION, 5);
    break;

  case CLASS_RANGER:
    SET_SKILL(ch, SKILL_SNEAK, 5);
    SET_SKILL(ch, SKILL_HIDE, 5);
    SET_SKILL(ch, SKILL_BANDAGE, 5);
    SET_SKILL(ch, SKILL_TRACK, 5);
    SET_SKILL(ch, SKILL_BASH, 5);
    SET_SKILL(ch, SKILL_UNARMED, 5);
    SET_SKILL(ch, SKILL_SLASHING_WEAPONS, 5);
    SET_SKILL(ch, SKILL_PIERCING_WEAPONS, 5);
    SET_SKILL(ch, SKILL_SHIELD_USE, 5);
    SET_SKILL(ch, SKILL_PERCEPTION, 5);
    break;

  case CLASS_BARD:
    SET_SKILL(ch, SPELL_ARMOR, 5);
    SET_SKILL(ch, SPELL_IDENTIFY, 5);
    SET_SKILL(ch, SKILL_BANDAGE, 5);
    SET_SKILL(ch, SKILL_TRACK, 5);
    SET_SKILL(ch, SKILL_PICK_LOCK, 5);
    SET_SKILL(ch, SKILL_UNARMED, 5);
    SET_SKILL(ch, SKILL_PIERCING_WEAPONS, 5);
    SET_SKILL(ch, SKILL_SHIELD_USE, 5);
    SET_SKILL(ch, SKILL_PERCEPTION, 5);
    break;

  case CLASS_DRUID:
    SET_SKILL(ch, SPELL_DETECT_INVIS, 5);
    SET_SKILL(ch, SPELL_DETECT_MAGIC, 5);
    SET_SKILL(ch, SPELL_LOCATE_OBJECT, 5);
    SET_SKILL(ch, SKILL_BANDAGE, 5);
    SET_SKILL(ch, SKILL_TRACK, 5);
    SET_SKILL(ch, SKILL_UNARMED, 5);
    SET_SKILL(ch, SKILL_PIERCING_WEAPONS, 5);
    SET_SKILL(ch, SKILL_SHIELD_USE, 5);
    SET_SKILL(ch, SKILL_PERCEPTION, 5);
    break;
  }

  advance_level(ch);

  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, HUNGER) = 24;
  GET_COND(ch, DRUNK) = 0;

  if (CONFIG_SITEOK_ALL)
    SET_BIT_AR(PLR_FLAGS(ch), PLR_SITEOK);
}

/* This function controls the change to maxmove, maxmana, and maxhp for each
 * class every time they gain a level. */
void advance_level(struct char_data *ch)
{
  int add_hp, add_mana = 0, add_move = 0, i;

  add_hp = GET_ABILITY_MOD(GET_CON(ch));

  switch (GET_CLASS(ch)) {

  case CLASS_SORCEROR:
    add_hp += rand_number(3, 8);
    add_mana = rand_number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = rand_number(0, 2);
    break;

  case CLASS_CLERIC:
    add_hp += rand_number(5, 10);
    add_mana = rand_number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = rand_number(0, 2);
    break;

  case CLASS_ROGUE:
    add_hp += rand_number(7, 13);
    add_mana = 0;
    add_move = rand_number(1, 3);
    break;

  case CLASS_FIGHTER:
    add_hp += rand_number(10, 15);
    add_mana = 0;
    add_move = rand_number(1, 3);
    break;

  case CLASS_BARBARIAN:
    add_hp += rand_number(12, 18);
    add_mana = 0;
    add_move = rand_number(1, 3);
    break;

  case CLASS_RANGER:
    add_hp += rand_number(10, 15);
    add_mana = 0;
    add_move = rand_number(2, 4);
    break;

  case CLASS_BARD:
    add_hp += rand_number(7, 13);
    add_mana = rand_number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = rand_number(1, 3);
    break;

  case CLASS_DRUID:
    add_hp += rand_number(10, 15);
    add_mana = rand_number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = rand_number(1, 4);
    break;
  }

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }

  snoop_check(ch);
  save_char(ch);
}

/* This simply calculates the backstab multiplier based on a character's level.
 * This used to be an array, but was changed to be a function so that it would
 * be easier to add more levels to your MUD.  This doesn't really create a big
 * performance hit because it's not used very often. */
int backstab_mult(int level)
{
  if (level < LVL_IMMORT)
    return 2;  /* mortal levels */
  else
    return 20;	  /* immortals */
}

/* invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors. */
int invalid_class(struct char_data *ch, struct obj_data *obj)
{
  if (OBJ_FLAGGED(obj, ITEM_ANTI_SORCEROR) && IS_SORCEROR(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_FIGHTER) && IS_FIGHTER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ROGUE) && IS_ROGUE(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BARBARIAN) && IS_BARBARIAN(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_RANGER) && IS_RANGER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BARD) && IS_BARD(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DRUID) && IS_DRUID(ch))
    return TRUE;

  return FALSE;
}

/* SPELLS AND SKILLS.  This area defines which spells are assigned to which
 * classes, and the minimum level the character must be to use the spell or
 * skill. */
void init_spell_levels(void)
{
  /* SORCERORS */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_SORCEROR, 1);
  spell_level(SPELL_DETECT_INVIS, CLASS_SORCEROR, 1);
  spell_level(SPELL_DETECT_MAGIC, CLASS_SORCEROR, 1);
  spell_level(SPELL_CHILL_TOUCH, CLASS_SORCEROR, 1);
  spell_level(SPELL_INFRAVISION, CLASS_SORCEROR, 1);
  spell_level(SPELL_INVISIBLE, CLASS_SORCEROR, 1);
  spell_level(SPELL_ARMOR, CLASS_SORCEROR, 1);
  spell_level(SPELL_BURNING_HANDS, CLASS_SORCEROR, 1);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_SORCEROR, 1);
  spell_level(SPELL_STRENGTH, CLASS_SORCEROR, 1);
  spell_level(SPELL_SHOCKING_GRASP, CLASS_SORCEROR, 1);
  spell_level(SPELL_SLEEP, CLASS_SORCEROR, 1);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_SORCEROR, 1);
  spell_level(SPELL_BLINDNESS, CLASS_SORCEROR, 1);
  spell_level(SPELL_DETECT_POISON, CLASS_SORCEROR, 1);
  spell_level(SPELL_COLOR_SPRAY, CLASS_SORCEROR, 1);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_SORCEROR, 1);
  spell_level(SPELL_CURSE, CLASS_SORCEROR, 1);
  spell_level(SPELL_POISON, CLASS_SORCEROR, 1);
  spell_level(SPELL_FIREBALL, CLASS_SORCEROR, 1);
  spell_level(SPELL_CHARM, CLASS_SORCEROR, 1);
  spell_level(SPELL_IDENTIFY, CLASS_SORCEROR, 1);
  spell_level(SPELL_FLY, CLASS_SORCEROR, 1);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_SORCEROR, 1);
  spell_level(SPELL_CLONE, CLASS_SORCEROR, 1);
  spell_level(SKILL_UNARMED, CLASS_SORCEROR, 1);

  /* CLERICS */
  spell_level(SPELL_CURE_LIGHT, CLASS_CLERIC, 1);
  spell_level(SPELL_ARMOR, CLASS_CLERIC, 1);
  spell_level(SPELL_CREATE_FOOD, CLASS_CLERIC, 1);
  spell_level(SPELL_CREATE_WATER, CLASS_CLERIC, 1);
  spell_level(SPELL_DETECT_POISON, CLASS_CLERIC, 1);
  spell_level(SPELL_DETECT_ALIGN, CLASS_CLERIC, 1);
  spell_level(SPELL_CURE_BLIND, CLASS_CLERIC, 1);
  spell_level(SPELL_BLESS, CLASS_CLERIC, 1);
  spell_level(SPELL_DETECT_INVIS, CLASS_CLERIC, 1);
  spell_level(SPELL_BLINDNESS, CLASS_CLERIC, 1);
  spell_level(SPELL_INFRAVISION, CLASS_CLERIC, 1);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, 1);
  spell_level(SPELL_POISON, CLASS_CLERIC, 1);
  spell_level(SPELL_GROUP_ARMOR, CLASS_CLERIC, 1);
  spell_level(SPELL_CURE_CRITIC, CLASS_CLERIC, 1);
  spell_level(SPELL_SUMMON, CLASS_CLERIC, 1);
  spell_level(SPELL_REMOVE_POISON, CLASS_CLERIC, 1);
  spell_level(SPELL_IDENTIFY, CLASS_CLERIC, 1);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_CLERIC, 1);
  spell_level(SPELL_DARKNESS, CLASS_CLERIC, 1);
  spell_level(SPELL_EARTHQUAKE, CLASS_CLERIC, 1);
  spell_level(SPELL_DISPEL_EVIL, CLASS_CLERIC, 1);
  spell_level(SPELL_DISPEL_GOOD, CLASS_CLERIC, 1);
  spell_level(SPELL_SANCTUARY, CLASS_CLERIC, 1);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_CLERIC, 1);
  spell_level(SPELL_HEAL, CLASS_CLERIC, 1);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_CLERIC, 1);
  spell_level(SPELL_SENSE_LIFE, CLASS_CLERIC, 1);
  spell_level(SPELL_HARM, CLASS_CLERIC, 1);
  spell_level(SPELL_GROUP_HEAL, CLASS_CLERIC, 1);
  spell_level(SPELL_REMOVE_CURSE, CLASS_CLERIC, 1);
  spell_level(SKILL_SHIELD_USE, CLASS_CLERIC, 1);

  /* ROGUES */
  spell_level(SKILL_SNEAK, CLASS_ROGUE, 1);
  spell_level(SKILL_PICK_LOCK, CLASS_ROGUE, 1);
  spell_level(SKILL_BACKSTAB, CLASS_ROGUE, 1);
  spell_level(SKILL_STEAL, CLASS_ROGUE, 1);
  spell_level(SKILL_HIDE, CLASS_ROGUE, 1);
  spell_level(SKILL_TRACK, CLASS_ROGUE, 1);
  spell_level(SKILL_UNARMED, CLASS_ROGUE, 1);
  spell_level(SKILL_PIERCING_WEAPONS, CLASS_ROGUE, 1);
  spell_level(SKILL_SHIELD_USE, CLASS_ROGUE, 1);
  spell_level(SKILL_PERCEPTION, CLASS_ROGUE, 1);

  /* FIGHTERS */
  spell_level(SKILL_KICK, CLASS_FIGHTER, 1);
  spell_level(SKILL_RESCUE, CLASS_FIGHTER, 1);
  spell_level(SKILL_BANDAGE, CLASS_FIGHTER, 1);
  spell_level(SKILL_BASH, CLASS_FIGHTER, 1);
  spell_level(SKILL_UNARMED, CLASS_FIGHTER, 1);
  spell_level(SKILL_SLASHING_WEAPONS, CLASS_FIGHTER, 1);
  spell_level(SKILL_PIERCING_WEAPONS, CLASS_FIGHTER, 1);
  spell_level(SKILL_BLUDGEONING_WEAPONS, CLASS_FIGHTER, 1);
  spell_level(SKILL_SHIELD_USE, CLASS_FIGHTER, 1);
  spell_level(SKILL_PERCEPTION, CLASS_FIGHTER, 1);

  /* BARBARIANS */
  spell_level(SKILL_KICK, CLASS_BARBARIAN, 1);
  spell_level(SKILL_RESCUE, CLASS_BARBARIAN, 1);
  spell_level(SKILL_BANDAGE, CLASS_BARBARIAN, 1);
  spell_level(SKILL_WHIRLWIND, CLASS_BARBARIAN, 1);
  spell_level(SKILL_UNARMED, CLASS_BARBARIAN, 1);
  spell_level(SKILL_PIERCING_WEAPONS, CLASS_BARBARIAN, 1);
  spell_level(SKILL_BLUDGEONING_WEAPONS, CLASS_BARBARIAN, 1);
  spell_level(SKILL_PERCEPTION, CLASS_BARBARIAN, 1);

  /* RANGERS */
  spell_level(SKILL_SNEAK, CLASS_RANGER, 1);
  spell_level(SKILL_HIDE, CLASS_RANGER, 1);
  spell_level(SKILL_BANDAGE, CLASS_RANGER, 1);
  spell_level(SKILL_TRACK, CLASS_RANGER, 1);
  spell_level(SKILL_BASH, CLASS_RANGER, 1);
  spell_level(SKILL_UNARMED, CLASS_RANGER, 1);
  spell_level(SKILL_SLASHING_WEAPONS, CLASS_RANGER, 1);
  spell_level(SKILL_PIERCING_WEAPONS, CLASS_RANGER, 1);
  spell_level(SKILL_SHIELD_USE, CLASS_RANGER, 1);
  spell_level(SKILL_PERCEPTION, CLASS_RANGER, 1);

  /* BARDS */
  spell_level(SPELL_ARMOR, CLASS_BARD, 1);
  spell_level(SPELL_IDENTIFY, CLASS_BARD, 1);
  spell_level(SKILL_BANDAGE, CLASS_BARD, 1);
  spell_level(SKILL_TRACK, CLASS_BARD, 1);
  spell_level(SKILL_PICK_LOCK, CLASS_BARD, 1);
  spell_level(SKILL_UNARMED, CLASS_BARD, 1);
  spell_level(SKILL_PIERCING_WEAPONS, CLASS_BARD, 1);
  spell_level(SKILL_SHIELD_USE, CLASS_BARD, 1);
  spell_level(SKILL_PERCEPTION, CLASS_BARD, 1);

  /* DRUIDS */
  spell_level(SPELL_DETECT_INVIS, CLASS_DRUID, 1);
  spell_level(SPELL_DETECT_MAGIC, CLASS_DRUID, 1);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_DRUID, 1);
  spell_level(SKILL_BANDAGE, CLASS_DRUID, 1);
  spell_level(SKILL_TRACK, CLASS_DRUID, 1);
  spell_level(SKILL_UNARMED, CLASS_DRUID, 1);
  spell_level(SKILL_PIERCING_WEAPONS, CLASS_DRUID, 1);
  spell_level(SKILL_SHIELD_USE, CLASS_DRUID, 1);
  spell_level(SKILL_PERCEPTION, CLASS_DRUID, 1);
}

/* This is the exp given to implementors -- it must always be greater than the
 * exp required for immortality, plus at least 20,000 or so. */
#define EXP_MAX  10000000

/* Function to return the exp required for each class/level */
int level_exp(int chclass, int level)
{
  if (level > LVL_IMPL || level < 0) {
    log("SYSERR: Requesting exp for invalid level %d!", level);
    return 0;
  }

  /* Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist. */
   if (level > LVL_IMMORT) {
     return EXP_MAX - ((LVL_IMPL - level) * 1000);
   }

  /* Exp required for normal mortals is below */
  switch (chclass) {

    case CLASS_SORCEROR:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case LVL_IMMORT: return 9999999;
    }
    break;

    case CLASS_CLERIC:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case LVL_IMMORT: return 9999999;
    }
    break;

    case CLASS_ROGUE:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case LVL_IMMORT: return 9999999;
    }
    break;

    case CLASS_FIGHTER:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case LVL_IMMORT: return 9999999;
    }
    break;

    case CLASS_BARBARIAN:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case LVL_IMMORT: return 9999999;
    }
    break;

    case CLASS_RANGER:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case LVL_IMMORT: return 9999999;
    }
    break;

    case CLASS_BARD:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case LVL_IMMORT: return 9999999;
    }
    break;

    case CLASS_DRUID:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case LVL_IMMORT: return 9999999;
    }
    break;
  }

  /* This statement should never be reached if the exp tables in this function
   * are set up properly.  If you see exp of 123456 then the tables above are
   * incomplete. */
  log("SYSERR: XP tables not set up correctly in class.c!");
  return 123456;
}

/* Default titles of male characters. */
const char *title_male(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Man";
  if (level == LVL_IMPL)
    return "the Implementor";

  switch (chclass) {

    case CLASS_SORCEROR:
    switch (level) {
      case  1: return "the Apprentice of Magic";
      case LVL_IMMORT: return "the Immortal Warlock";
      case LVL_GOD: return "the Avatar of Magic";
      case LVL_GRGOD: return "the God of Magic";
      default: return "the Sorceror";
    }

    case CLASS_CLERIC:
    switch (level) {
      case  1: return "the Believer";
      case LVL_IMMORT: return "the Immortal Cardinal";
      case LVL_GOD: return "the Inquisitor";
      case LVL_GRGOD: return "the God of Good and Evil";
      default: return "the Cleric";
    }

    case CLASS_ROGUE:
    switch (level) {
      case  1: return "the Pilferer";
      case LVL_IMMORT: return "the Immortal Assassin";
      case LVL_GOD: return "the Demi God of Thieves";
      case LVL_GRGOD: return "the God of Thieves and Tradesmen";
      default: return "the Rogue";
    }

    case CLASS_FIGHTER:
    switch(level) {
      case  1: return "the Swordpupil";
      case LVL_IMMORT: return "the Immortal Warlord";
      case LVL_GOD: return "the Extirpator";
      case LVL_GRGOD: return "the God of War";
      default: return "the Fighter";
    }

    case CLASS_BARBARIAN:
    switch(level) {
      case  1: return "the Savage";
      case LVL_IMMORT: return "the Immortal Conquerer";
      case LVL_GOD: return "the Primal";
      case LVL_GRGOD: return "the God of Blood and Thunder";
      default: return "the Barbarian";
    }

    case CLASS_RANGER:
    switch(level) {
      case  1: return "the Pathfinder";
      case LVL_IMMORT: return "the Immortal Trapmaster";
      case LVL_GOD: return "the Wanderer";
      case LVL_GRGOD: return "the God of the Hunt";
      default: return "the Ranger";
    }

    case CLASS_BARD:
    switch(level) {
      case  1: return "the Singer";
      case LVL_IMMORT: return "the Immortal Songwriter";
      case LVL_GOD: return "the Musician";
      case LVL_GRGOD: return "the God of Ballads";
      default: return "the Bard";
    }

    case CLASS_DRUID:
    switch(level) {
      case  1: return "the Communer";
      case LVL_IMMORT: return "the Immortal Augur";
      case LVL_GOD: return "the Naturophile";
      case LVL_GRGOD: return "the God of Nature";
      default: return "the Druid";
    }
  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}

/* Default titles of female characters. */
const char *title_female(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Woman";
  if (level == LVL_IMPL)
    return "the Implementress";

  switch (chclass) {

    case CLASS_SORCEROR:
    switch (level) {
      case  1: return "the Apprentice of Magic";
      case LVL_IMMORT: return "the Immortal Enchantress";
      case LVL_GOD: return "the Empress of Magic";
      case LVL_GRGOD: return "the Goddess of Magic";
      default: return "the Witch";
    }

    case CLASS_CLERIC:
    switch (level) {
      case  1: return "the Believer";
      case LVL_IMMORT: return "the Immortal Priestess";
      case LVL_GOD: return "the Inquisitress";
      case LVL_GRGOD: return "the Goddess of Good and Evil";
      default: return "the Cleric";
    }

    case CLASS_ROGUE:
    switch (level) {
      case  1: return "the Pilferess";
      case LVL_IMMORT: return "the Immortal Assassin";
      case LVL_GOD: return "the Demi Goddess of Thieves";
      case LVL_GRGOD: return "the Goddess of Thieves and Tradesmen";
      default: return "the Rogue";
    }

    case CLASS_FIGHTER:
    switch(level) {
      case  1: return "the Swordpupil";
      case LVL_IMMORT: return "the Immortal Lady of War";
      case LVL_GOD: return "the Queen of Destruction";
      case LVL_GRGOD: return "the Goddess of War";
      default: return "the Fighter";
    }

    case CLASS_BARBARIAN:
    switch(level) {
      case  1: return "the Savage";
      case LVL_IMMORT: return "the Immortal Conquerer";
      case LVL_GOD: return "the Primal";
      case LVL_GRGOD: return "the Goddess of Blood and Thunder";
      default: return "the Barbarian";
    }

    case CLASS_RANGER:
    switch(level) {
      case  1: return "the Pathfinder";
      case LVL_IMMORT: return "the Immortal Trapmaster";
      case LVL_GOD: return "the Wanderer";
      case LVL_GRGOD: return "the Goddess of the Hunt";
      default: return "the Ranger";
    }

    case CLASS_BARD:
    switch(level) {
      case  1: return "the Singer";
      case LVL_IMMORT: return "the Immortal Songwriter";
      case LVL_GOD: return "the Musician";
      case LVL_GRGOD: return "the Goddess of Ballads";
      default: return "the Bard";
    }

    case CLASS_DRUID:
    switch(level) {
      case  1: return "the Communer";
      case LVL_IMMORT: return "the Immortal Augur";
      case LVL_GOD: return "the Naturophile";
      case LVL_GRGOD: return "the Goddess of Nature";
      default: return "the Druid";
    }
  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}

