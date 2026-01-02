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

struct species_base_points {
  int hit;
  int mana;
  int stamina;
};

static const struct species_base_points species_base_points[NUM_SPECIES] = {
  { 100, 90, 100 },  /* Human */
  { 90, 110, 90 },   /* City Elf */
  { 100, 80, 140 },  /* Desert Elf */
  { 95, 90, 95 },    /* Half-Elf */
  { 110, 75, 110 },  /* Dwarf */
  { 110, 80, 110 },  /* Mul */
  { 180, 50, 180 },  /* Half-Giant */
  { 90, 90, 90 },    /* Mantis */
  { 90, 100, 90 },   /* Gith */
  { 90, 90, 90 },    /* Aarakocra */
  { 100, 100, 100 }, /* Dray */
  { 70, 120, 80 },   /* Kenku */
  { 60, 10, 80 },    /* Jozhal */
  { 120, 10, 120 },  /* Pterran */
  { 100, 60, 100 },  /* Tarek */
  { 60, 10, 80 },    /* Aprig */
  { 100, 10, 100 },  /* Carru */
  { 100, 10, 80 },   /* Crodlu */
  { 80, 10, 200 },   /* Erdlu */
  { 120, 10, 400 },  /* Inix */
  { 75, 10, 75 },    /* Jhakar */
  { 100, 10, 300 },  /* Kank */
  { 200, 10, 150 },  /* Mekillot */
  { 100, 10, 150 },  /* Worm */
  { 10, 10, 20 },    /* Renk */
  { 10, 10, 20 },    /* Rat */
  { 110, 110, 110 }, /* Undead */
  { 250, 250, 250 }  /* Dragon */
};

static const struct species_skill_bonus species_skill_none[] = {
  { -1, 0 }
};

/* Skill percent values map to proficiency tiers via GET_PROFICIENCY(). */
static const struct species_skill_bonus species_skill_crodlu[] = {
  { SKILL_PERCEPTION, 45 }, /* +3 proficiency */
  { -1, 0 }
};

static const struct species_skill_bonus species_skill_jhakar[] = {
  { SKILL_PERCEPTION, 45 }, /* +3 proficiency */
  { SKILL_STEALTH, 60 },    /* +4 proficiency */
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
  species_skill_crodlu, /* Crodlu */
  species_skill_none, /* Erdlu */
  species_skill_none, /* Inix */
  species_skill_jhakar, /* Jhakar */
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
  { 14, 14, 14, 14, 14, 14 },  /* Human */
  { 10, 18, 10, 14, 16, 14 },  /* City Elf */
  { 12, 18, 12, 12, 14, 12 },  /* Desert Elf */
  { 13, 16, 13, 13, 15, 10 },  /* Half-Elf */
  { 18, 10, 18, 10, 10, 10 },  /* Dwarf */
  { 20, 14, 18, 10, 10, 8 },   /* Mul */
  { 24, 8, 20, 6, 6, 6 },      /* Half-Giant */
  { 14, 14, 16, 10, 12, 6 },   /* Mantis */
  { 12, 16, 14, 10, 8, 6 },    /* Gith */
  { 13, 16, 14, 13, 13, 13 },  /* Aarakocra */
  { 15, 13, 15, 12, 12, 14 },  /* Dray */
  { 12, 16, 12, 15, 15, 13 },  /* Kenku */
  { 8, 10, 8, 8, 8, 6 },       /* Jozhal */
  { 15, 12, 15, 15, 12, 10 },  /* Pterran */
  { 17, 13, 18, 10, 10, 6 },   /* Tarek */
  { 8, 10, 12, 2, 10, 5 },     /* Aprig */
  { 21, 8, 15, 2, 12, 6 },     /* Carru */
  { 16, 15, 14, 4, 12, 6 },    /* Crodlu */
  { 16, 10, 12, 2, 11, 7 },    /* Erdlu */
  { 22, 12, 18, 2, 10, 7 },    /* Inix */
  { 17, 15, 16, 3, 12, 7 },    /* Jhakar */
  { 18, 10, 14, 2, 10, 4 },    /* Kank */
  { 24, 9, 21, 3, 11, 6 },     /* Mekillot */
  { 22, 8, 19, 2, 2, 6 },      /* Worm */
  { 2, 4, 10, 4, 10, 4 },      /* Renk */
  { 2, 11, 9, 2, 10, 4 },      /* Rat */
  { 18, 16, 18, 16, 16, 16 },  /* Undead */
  { 25, 25, 25, 25, 25, 25 }   /* Dragon */
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

bool get_species_base_points(int species, int *hit, int *mana, int *stamina)
{
  if (species < 0 || species >= NUM_SPECIES)
    return FALSE;

  if (hit)
    *hit = species_base_points[species].hit;
  if (mana)
    *mana = species_base_points[species].mana;
  if (stamina)
    *stamina = species_base_points[species].stamina;

  return TRUE;
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
