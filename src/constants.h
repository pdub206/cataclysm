/**
* @file constants.h
* Declares the global constants defined in constants.c.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*/
#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

extern const char *tbamud_version;
extern const char *dirs[];
extern const char *autoexits[];
extern const char *room_bits[];
extern const char *zone_bits[];
extern const char *exit_bits[];
extern const char *sector_types[];
extern const char *genders[];
extern const char *position_types[];
extern const char *player_bits[];
extern const char *action_bits[];
extern const char *preference_bits[];
extern const char *affected_bits[];
extern const char *connected_types[];
extern const char *wear_where[];
extern const char *equipment_types[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *apply_types[];
extern const char *container_bits[];
extern const char *drinks[];
extern const char *drinknames[];
extern const char *color_liquid[];
extern const char *fullness[];
extern const char *weekdays[];
extern const char *month_name[];
extern const struct str_app_type str_app[];
extern const struct dex_skill_type dex_app_skill[];
extern const struct dex_app_type dex_app[];
extern const struct con_app_type con_app[];
extern const struct int_app_type int_app[];
extern const struct wis_app_type wis_app[];
extern int rev_dir[];
extern int movement_loss[];
extern int drink_aff[][3];
extern const char *trig_types[];
extern const char *otrig_types[];
extern const char *wtrig_types[];
extern const char *history_types[];
extern const char *ibt_bits[];
extern size_t room_bits_count;
extern size_t action_bits_count;
extern size_t affected_bits_count;
extern size_t extra_bits_count;
extern size_t wear_bits_count;

/* 5e system helpers */

/* Armor slot constraints for AC calculation */
struct armor_slot {
  const char *name;
  int max_piece_ac;   /* max base AC contribution from this slot */
  int max_magic;      /* max magic bonus contribution from this slot */
  int bulk_weight;    /* bulk contribution for encumbrance / Dex cap */
};

/* Armor slot table (defined in constants.c) */
extern const struct armor_slot armor_slots[];
extern const int NUM_ARMOR_SLOTS;
extern const int ARMOR_WEAR_POSITIONS[];
/* Armor flags (obj->value[3]) */
extern const char *armor_flag_bits[];

/* Bounded accuracy caps */
#define MAX_TOTAL_ATTACK_BONUS     10  /* stats + prof + magic + situational */
#define MAX_WEAPON_MAGIC            3
#define MAX_SHIELD_MAGIC            3
/* We already set this earlier: */
#define MAX_TOTAL_ARMOR_MAGIC       3

#endif /* _CONSTANTS_H_ */
