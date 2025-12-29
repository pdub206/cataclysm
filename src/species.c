/**
* @file species.c
* Race/species related configuration, skill bonuses, modifiers, and stat limitations.
* 
* This set of code was not originally part of the circlemud distribution.
*/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "species.h"

struct species_skill_bonus {
  int skill;
  int start;
};

/* Keep species ordering in sync with the SPECIES_* defines. */
const char *species_types[NUM_SPECIES] = {
  "Human",
  "City Elf",
  "Desert Elf",
  "Half-Elf",
  "Dwarf",
  "Mul",
  "Half-Giant",
  "Mantis",
  "Gith",
  "Aarakocra",
  "Dray",
  "Kenku",
  "Jozhal",
  "Pterran",
  "Tarek",
  "Aprig",
  "Carru",
  "Crodlu",
  "Erdlu",
  "Inix",
  "Jhakar",
  "Kank",
  "Mekillot",
  "Worm",
  "Renk",
  "Rat",
  "Undead",
  "Dragon"
};

/* PC creation options (1-based ordering in menus). */
const int pc_species_list[] = {
  SPECIES_HUMAN,
  SPECIES_CITY_ELF,
  SPECIES_DESERT_ELF,
  SPECIES_HALF_ELF,
  SPECIES_DWARF,
  SPECIES_MUL,
  SPECIES_HALF_GIANT,
  SPECIES_MANTIS,
  SPECIES_GITH,
  -1
};

static const struct species_skill_bonus species_skill_none[] = {
  { -1, 0 }
};

/* Per-species skill bonuses.
 * Usage:
 * 1) Define a bonus list for a species (see commented example below).
 * 2) In species_skill_bonuses[], point that species at the new list.
 * 3) Keep the { -1, 0 } terminator as the final entry.
 *
 * Example (commented out):
 *
 * static const struct species_skill_bonus species_skill_human[] = {
 *   { SKILL_PERCEPTION, 5 },
 *   { -1, 0 }
 * };
 */
static const struct species_skill_bonus *species_skill_bonuses[NUM_SPECIES] = {
  /* species_skill_human, */ /* Human (example if you enable the list above) */
  species_skill_none, /* Human */
  species_skill_none, /* City Elf */
  species_skill_none, /* Desert Elf */
  species_skill_none, /* Half-Elf */
  species_skill_none, /* Dwarf */
  species_skill_none, /* Mul */
  species_skill_none, /* Half-Giant */
  species_skill_none, /* Mantis */
  species_skill_none, /* Gith */
  species_skill_none, /* Aarakocra */
  species_skill_none, /* Dray */
  species_skill_none, /* Kenku */
  species_skill_none, /* Jozhal */
  species_skill_none, /* Pterran */
  species_skill_none, /* Tarek */
  species_skill_none, /* Aprig */
  species_skill_none, /* Carru */
  species_skill_none, /* Crodlu */
  species_skill_none, /* Erdlu */
  species_skill_none, /* Inix */
  species_skill_none, /* Jhakar */
  species_skill_none, /* Kank */
  species_skill_none, /* Mekillot */
  species_skill_none, /* Worm */
  species_skill_none, /* Renk */
  species_skill_none, /* Rat */
  species_skill_none, /* Undead */
  species_skill_none  /* Dragon */
};

/* Ability minimums by species (STR, DEX, CON, INT, WIS, CHA). Zero = no min. */
static const int species_ability_mins[NUM_SPECIES][NUM_ABILITIES] = {
  { 0, 0, 0, 0, 0, 0 }, /* Human */
  { 0, 0, 0, 0, 0, 0 }, /* City Elf */
  { 0, 0, 0, 0, 0, 0 }, /* Desert Elf */
  { 0, 0, 0, 0, 0, 0 }, /* Half-Elf */
  { 0, 0, 0, 0, 0, 0 }, /* Dwarf */
  { 0, 0, 0, 0, 0, 0 }, /* Mul */
  { 0, 0, 0, 0, 0, 0 }, /* Half-Giant */
  { 0, 0, 0, 0, 0, 0 }, /* Mantis */
  { 0, 0, 0, 0, 0, 0 }, /* Gith */
  { 0, 0, 0, 0, 0, 0 }, /* Aarakocra */
  { 0, 0, 0, 0, 0, 0 }, /* Dray */
  { 0, 0, 0, 0, 0, 0 }, /* Kenku */
  { 0, 0, 0, 0, 0, 0 }, /* Jozhal */
  { 0, 0, 0, 0, 0, 0 }, /* Pterran */
  { 0, 0, 0, 0, 0, 0 }, /* Tarek */
  { 0, 0, 0, 0, 0, 0 }, /* Aprig */
  { 0, 0, 0, 0, 0, 0 }, /* Carru */
  { 0, 0, 0, 0, 0, 0 }, /* Crodlu */
  { 0, 0, 0, 0, 0, 0 }, /* Erdlu */
  { 0, 0, 0, 0, 0, 0 }, /* Inix */
  { 0, 0, 0, 0, 0, 0 }, /* Jhakar */
  { 0, 0, 0, 0, 0, 0 }, /* Kank */
  { 0, 0, 0, 0, 0, 0 }, /* Mekillot */
  { 0, 0, 0, 0, 0, 0 }, /* Worm */
  { 0, 0, 0, 0, 0, 0 }, /* Renk */
  { 0, 0, 0, 0, 0, 0 }, /* Rat */
  { 0, 0, 0, 0, 0, 0 }, /* Undead */
  { 0, 0, 0, 0, 0, 0 }  /* Dragon */
};

/* Ability modifiers by species (STR, DEX, CON, INT, WIS, CHA). */
static const int species_ability_mods[NUM_SPECIES][NUM_ABILITIES] = {
  { 0, 0, 0, 0, 0, 0 }, /* Human */
  { 0, 0, 0, 0, 0, 0 }, /* City Elf */
  { 0, 0, 0, 0, 0, 0 }, /* Desert Elf */
  { 0, 0, 0, 0, 0, 0 }, /* Half-Elf */
  { 0, 0, 0, 0, 0, 0 }, /* Dwarf */
  { 0, 0, 0, 0, 0, 0 }, /* Mul */
  { 0, 0, 0, 0, 0, 0 }, /* Half-Giant */
  { 0, 0, 0, 0, 0, 0 }, /* Mantis */
  { 0, 0, 0, 0, 0, 0 }, /* Gith */
  { 0, 0, 0, 0, 0, 0 }, /* Aarakocra */
  { 0, 0, 0, 0, 0, 0 }, /* Dray */
  { 0, 0, 0, 0, 0, 0 }, /* Kenku */
  { 0, 0, 0, 0, 0, 0 }, /* Jozhal */
  { 0, 0, 0, 0, 0, 0 }, /* Pterran */
  { 0, 0, 0, 0, 0, 0 }, /* Tarek */
  { 0, 0, 0, 0, 0, 0 }, /* Aprig */
  { 0, 0, 0, 0, 0, 0 }, /* Carru */
  { 0, 0, 0, 0, 0, 0 }, /* Crodlu */
  { 0, 0, 0, 0, 0, 0 }, /* Erdlu */
  { 0, 0, 0, 0, 0, 0 }, /* Inix */
  { 0, 0, 0, 0, 0, 0 }, /* Jhakar */
  { 0, 0, 0, 0, 0, 0 }, /* Kank */
  { 0, 0, 0, 0, 0, 0 }, /* Mekillot */
  { 0, 0, 0, 0, 0, 0 }, /* Worm */
  { 0, 0, 0, 0, 0, 0 }, /* Renk */
  { 0, 0, 0, 0, 0, 0 }, /* Rat */
  { 0, 0, 0, 0, 0, 0 }, /* Undead */
  { 0, 0, 0, 0, 0, 0 }  /* Dragon */
};

/* Ability caps by species (STR, DEX, CON, INT, WIS, CHA). Zero = no cap. */
static const int species_ability_caps[NUM_SPECIES][NUM_ABILITIES] = {
  { 16, 0, 0, 0, 0, 0 }, /* Human */
  { 14, 0, 0, 0, 0, 0 }, /* City Elf */
  { 14, 0, 0, 0, 0, 0 }, /* Desert Elf */
  { 14, 0, 0, 0, 0, 0 }, /* Half-Elf */
  { 18, 0, 0, 0, 0, 0 }, /* Dwarf */
  { 20, 0, 0, 0, 0, 0 }, /* Mul */
  { 24, 0, 0, 0, 0, 0 }, /* Half-Giant */
  { 0, 0, 0, 0, 0, 0 },  /* Mantis */
  { 0, 0, 0, 0, 0, 0 },  /* Gith */
  { 0, 0, 0, 0, 0, 0 },  /* Aarakocra */
  { 0, 0, 0, 0, 0, 0 },  /* Dray */
  { 0, 0, 0, 0, 0, 0 },  /* Kenku */
  { 0, 0, 0, 0, 0, 0 },  /* Jozhal */
  { 0, 0, 0, 0, 0, 0 },  /* Pterran */
  { 0, 0, 0, 0, 0, 0 },  /* Tarek */
  { 0, 0, 0, 0, 0, 0 },  /* Aprig */
  { 0, 0, 0, 0, 0, 0 },  /* Carru */
  { 0, 0, 0, 0, 0, 0 },  /* Crodlu */
  { 0, 0, 0, 0, 0, 0 },  /* Erdlu */
  { 0, 0, 0, 0, 0, 0 },  /* Inix */
  { 0, 0, 0, 0, 0, 0 },  /* Jhakar */
  { 0, 0, 0, 0, 0, 0 },  /* Kank */
  { 0, 0, 0, 0, 0, 0 },  /* Mekillot */
  { 0, 0, 0, 0, 0, 0 },  /* Worm */
  { 0, 0, 0, 0, 0, 0 },  /* Renk */
  { 0, 0, 0, 0, 0, 0 },  /* Rat */
  { 0, 0, 0, 0, 0, 0 },  /* Undead */
  { 0, 0, 0, 0, 0, 0 }   /* Dragon */
};

const char *get_species_name(int species)
{
  if (species >= 0 && species < NUM_SPECIES)
    return species_types[species];
  return "Unassigned";
}

int pc_species_count(void)
{
  int count = 0;
  while (pc_species_list[count] != -1)
    count++;
  return count;
}

bool species_is_pc_selectable(int species)
{
  int i = 0;

  while (pc_species_list[i] != -1) {
    if (pc_species_list[i] == species)
      return TRUE;
    i++;
  }
  return FALSE;
}

int species_from_pc_choice(int choice)
{
  int count = pc_species_count();

  if (choice < 1 || choice > count)
    return SPECIES_UNDEFINED;
  return pc_species_list[choice - 1];
}

int species_ability_mod(int species, int ability)
{
  if (species < 0 || species >= NUM_SPECIES)
    return 0;
  if (ability < 0 || ability >= NUM_ABILITIES)
    return 0;
  return species_ability_mods[species][ability];
}

int species_ability_min(int species, int ability)
{
  if (species < 0 || species >= NUM_SPECIES)
    return 0;
  if (ability < 0 || ability >= NUM_ABILITIES)
    return 0;
  return species_ability_mins[species][ability];
}

int species_ability_cap(int species, int ability)
{
  if (species < 0 || species >= NUM_SPECIES)
    return 0;
  if (ability < 0 || ability >= NUM_ABILITIES)
    return 0;
  return species_ability_caps[species][ability];
}

static void apply_species_ranges(struct char_data *ch)
{
  int species;
  int cap, min;

  if (!ch)
    return;

  species = GET_SPECIES(ch);
  if (species < 0 || species >= NUM_SPECIES)
    return;

  min = species_ability_min(species, ABIL_STR);
  if (min > 0 && ch->real_abils.str < min)
    ch->real_abils.str = min;
  cap = species_ability_cap(species, ABIL_STR);
  if (cap > 0 && ch->real_abils.str > cap)
    ch->real_abils.str = cap;

  min = species_ability_min(species, ABIL_DEX);
  if (min > 0 && ch->real_abils.dex < min)
    ch->real_abils.dex = min;
  cap = species_ability_cap(species, ABIL_DEX);
  if (cap > 0 && ch->real_abils.dex > cap)
    ch->real_abils.dex = cap;

  min = species_ability_min(species, ABIL_CON);
  if (min > 0 && ch->real_abils.con < min)
    ch->real_abils.con = min;
  cap = species_ability_cap(species, ABIL_CON);
  if (cap > 0 && ch->real_abils.con > cap)
    ch->real_abils.con = cap;

  min = species_ability_min(species, ABIL_INT);
  if (min > 0 && ch->real_abils.intel < min)
    ch->real_abils.intel = min;
  cap = species_ability_cap(species, ABIL_INT);
  if (cap > 0 && ch->real_abils.intel > cap)
    ch->real_abils.intel = cap;

  min = species_ability_min(species, ABIL_WIS);
  if (min > 0 && ch->real_abils.wis < min)
    ch->real_abils.wis = min;
  cap = species_ability_cap(species, ABIL_WIS);
  if (cap > 0 && ch->real_abils.wis > cap)
    ch->real_abils.wis = cap;

  min = species_ability_min(species, ABIL_CHA);
  if (min > 0 && ch->real_abils.cha < min)
    ch->real_abils.cha = min;
  cap = species_ability_cap(species, ABIL_CHA);
  if (cap > 0 && ch->real_abils.cha > cap)
    ch->real_abils.cha = cap;
}

void apply_species_bonuses(struct char_data *ch)
{
  int species;

  if (!ch)
    return;

  species = GET_SPECIES(ch);
  if (species < 0 || species >= NUM_SPECIES)
    return;

  apply_species_ranges(ch);

  ch->real_abils.str += species_ability_mod(species, ABIL_STR);
  ch->real_abils.dex += species_ability_mod(species, ABIL_DEX);
  ch->real_abils.con += species_ability_mod(species, ABIL_CON);
  ch->real_abils.intel += species_ability_mod(species, ABIL_INT);
  ch->real_abils.wis += species_ability_mod(species, ABIL_WIS);
  ch->real_abils.cha += species_ability_mod(species, ABIL_CHA);

  apply_species_ranges(ch);
  ch->aff_abils = ch->real_abils;
}

void grant_species_skills(struct char_data *ch)
{
  int species;
  const struct species_skill_bonus *bonus;

  if (!ch)
    return;

  species = GET_SPECIES(ch);
  if (species < 0 || species >= NUM_SPECIES)
    return;

  for (bonus = species_skill_bonuses[species]; bonus->skill != -1; bonus++) {
    if (bonus->skill >= 0 && bonus->skill <= MAX_SKILLS) {
      if (GET_SKILL(ch, bonus->skill) < bonus->start)
        SET_SKILL(ch, bonus->skill, bonus->start);
    }
  }
}

static void remove_species_skills(struct char_data *ch, int species)
{
  const struct species_skill_bonus *bonus;

  if (!ch)
    return;
  if (species < 0 || species >= NUM_SPECIES)
    return;

  for (bonus = species_skill_bonuses[species]; bonus->skill != -1; bonus++) {
    if (bonus->skill >= 0 && bonus->skill <= MAX_SKILLS) {
      if (GET_SKILL(ch, bonus->skill) <= bonus->start)
        SET_SKILL(ch, bonus->skill, 0);
    }
  }
}

static void remove_species_bonuses(struct char_data *ch, int species)
{
  if (!ch)
    return;
  if (species < 0 || species >= NUM_SPECIES)
    return;

  ch->real_abils.str -= species_ability_mod(species, ABIL_STR);
  ch->real_abils.dex -= species_ability_mod(species, ABIL_DEX);
  ch->real_abils.con -= species_ability_mod(species, ABIL_CON);
  ch->real_abils.intel -= species_ability_mod(species, ABIL_INT);
  ch->real_abils.wis -= species_ability_mod(species, ABIL_WIS);
  ch->real_abils.cha -= species_ability_mod(species, ABIL_CHA);
}

int parse_species(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return SPECIES_UNDEFINED;

  for (i = 0; i < NUM_SPECIES; i++) {
    if (!str_cmp(arg, species_types[i]))
      return i;
  }

  return SPECIES_UNDEFINED;
}

void update_species(struct char_data *ch, int new_species)
{
  int old_species;

  if (!ch)
    return;

  old_species = GET_SPECIES(ch);
  if (old_species == new_species)
    return;

  if (old_species >= 0 && old_species < NUM_SPECIES) {
    remove_species_skills(ch, old_species);
    remove_species_bonuses(ch, old_species);
  }

  GET_SPECIES(ch) = new_species;

  if (new_species >= 0 && new_species < NUM_SPECIES) {
    apply_species_bonuses(ch);
    grant_species_skills(ch);
  } else {
    ch->aff_abils = ch->real_abils;
  }
}
