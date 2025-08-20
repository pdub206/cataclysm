/* tests_5e.c — unit tests for 5e-like rules
 *
 * Build suggestion (see Makefile note below):
 *   cc -g -O2 -Wall -Wextra -o tests_5e \
 *      tests_5e.c utils.o constants.o handler.o db.o random.o \
 *      (plus whatever .o your build needs for GET_EQ/GET_SKILL, etc.)
 *
 * If you keep it simple and link against your normal object files,
 * memset(0) on char_data is enough for GET_SKILL == 0, etc.
 */

#include <math.h>
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "constants.h"

/* ---------- Tiny test framework ---------- */
static int tests_run = 0, tests_failed = 0;

#define T_ASSERT(cond, ...) \
  do { tests_run++; if (!(cond)) { \
    tests_failed++; \
    fprintf(stderr, "[FAIL] %s:%d: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
  } } while (0)

#define T_EQI(actual, expect, label) \
  T_ASSERT((actual) == (expect), "%s: got %d, expect %d", (label), (int)(actual), (int)(expect))

#define T_IN_RANGE(val, lo, hi, label) \
  T_ASSERT((val) >= (lo) && (val) <= (hi), "%s: got %.3f, expect in [%.3f, %.3f]", (label), (double)(val), (double)(lo), (double)(hi))

/* ---------- Helpers for test setup ---------- */

/* Make a simple armor object with given per-piece fields. */
static struct obj_data *make_armor(int piece_ac, int bulk, int magic, int flags) {
  struct obj_data *o = calloc(1, sizeof(*o));
  GET_OBJ_TYPE(o) = ITEM_ARMOR;
  GET_OBJ_VAL(o, VAL_ARMOR_PIECE_AC)    = piece_ac;
  GET_OBJ_VAL(o, VAL_ARMOR_BULK)        = bulk;
  GET_OBJ_VAL(o, VAL_ARMOR_MAGIC_BONUS) = magic;
  GET_OBJ_VAL(o, VAL_ARMOR_FLAGS)       = flags;
  return o;
}

/* Equip an object at a wear position. */
static void equip_at(struct char_data *ch, int wear_pos, struct obj_data *o) {
  /* Most Circle/tbaMUD trees have ch->equipment[POS] */
  ch->equipment[wear_pos] = o;
}

/* Set an ability score (helpers for readability) — adjust if your tree uses different fields. */
static void set_ability_scores(struct char_data *ch, int str, int dex, int con, int intel, int wis, int cha) {
  /* real_abils is standard in Circle; if your tree differs, tweak these lines */
  ch->real_abils.str  = str;
  ch->real_abils.dex  = dex;
  ch->real_abils.con  = con;
  ch->real_abils.intel= intel;
  ch->real_abils.wis  = wis;
  ch->real_abils.cha  = cha;
  ch->aff_abils = ch->real_abils; /* common pattern */
}

/* For hit probability sanity tests: simulate pure d20 vs ascending AC with nat 1/20. */
static double simulate_hit_rate(int attack_mod, int target_ac, int trials) {
  int hits = 0;
  for (int i = 0; i < trials; ++i) {
    int d20 = rand_number(1, 20);
    bool hit;
    if (d20 == 1) hit = FALSE;
    else if (d20 == 20) hit = TRUE;
    else hit = (d20 + attack_mod) >= target_ac;
    hits += hit ? 1 : 0;
  }
  return (double)hits / (double)trials;
}

/* Dump a breakdown (useful when a test fails) */
static void dbg_dump_ac(const char *label, struct ac_breakdown *b) {
  fprintf(stderr, "%s: total=%d (base=%d armor=%d magic=%d dexCap=%d dex=%d shield=%d situ=%d bulk=%d)\n",
    label, b->total, b->base, b->armor_piece_sum, b->armor_magic_sum,
    b->dex_cap, b->dex_mod_applied, b->shield_bonus, b->situational, b->total_bulk);
}

/* ---------- TESTS ---------- */

static void test_ability_mod(void) {
  /* Spot-check classic 5e values and a sweep */
  T_EQI(ability_mod(10), 0, "ability_mod(10)");
  T_EQI(ability_mod(8), -1, "ability_mod(8)");
  T_EQI(ability_mod(12), 1, "ability_mod(12)");
  T_EQI(ability_mod(18), 4, "ability_mod(18)");
  T_EQI(ability_mod(1), -5, "ability_mod(1)");
  /* sweep 1..30 vs floor((s-10)/2) */
  for (int s = 1; s <= 30; ++s) {
    int expect = (int)floor((s - 10) / 2.0);
    T_EQI(ability_mod(s), expect, "ability_mod sweep");
  }
}

static void test_prof_from_skill(void) {
  /* Boundaries for your <= mapping: 0..14→0, 15..29→+1, ... 91..100→+6 */
  struct { int pct, expect; const char *lbl; } cases[] = {
    { 0, 0, "0→0"}, {14,0,"14→0"},
    {15,1,"15→1"}, {29,1,"29→1"},
    {30,2,"30→2"}, {44,2,"44→2"},
    {45,3,"45→3"}, {59,3,"59→3"},
    {60,4,"60→4"}, {74,4,"74→4"},
    {75,5,"75→5"}, {90,5,"90→5"},
    {91,6,"91→6"}, {100,6,"100→6"}
  };
  for (size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
    T_EQI(prof_from_skill(cases[i].pct), cases[i].expect, cases[i].lbl);
  }
}

static void test_ac_light_medium_heavy(void) {
  /* Fresh character */
  struct char_data ch;
  memset(&ch, 0, sizeof(ch));

  /* LIGHT SETUP:
   * Bulk target: Light (<=5).
   * Use head bulk 1 (wt 1 => +1), body bulk 1 (wt 3 => +3), total 4 => Light.
   * Dex 18 => +4 fully applied.
   * Armor piece AC: head=1, body=2 => sum = 3.
   * Magic: head+1, body+2 => per-slot ok, total 3 (at global cap).
   * No shield.
   * Expect: base 10 + piece 3 + magic 3 + Dex 4 = 20.
   */
  set_ability_scores(&ch, 10, 18, 10, 10, 10, 10);
  equip_at(&ch, WEAR_HEAD, make_armor(1, 1, 1, 0));
  equip_at(&ch, WEAR_BODY, make_armor(2, 1, 2, 0));

  struct ac_breakdown b1; compute_ac_breakdown(&ch, &b1);
  /* Sanity checks */
  if (b1.total != 20) dbg_dump_ac("LIGHT", &b1);
  T_EQI(b1.dex_cap, 5, "Light dex cap 5");
  T_EQI(b1.dex_mod_applied, 4, "Light dex +4 applied");
  T_EQI(b1.armor_magic_sum, 3, "Light magic cap (global) 3");
  T_EQI(b1.total, 20, "Light total AC");

  /* MEDIUM SETUP:
   * Clear equipment and rebuild:
   * Bulk target: Medium (6..10).
   * Use legs bulk 2 (wt 2 => +4), hands bulk 1 (wt 1 => +1), feet bulk 1 (wt 1 => +1), total 6 => Medium.
   * Dex 18 => +4, but cap at +2.
   * Armor piece AC: legs=2, hands=1, feet=1 => sum = 4.
   * Magic: legs +2 only (still under global cap).
   * Expect: base 10 + piece 4 + magic 2 + Dex 2 = 18.
   */
  memset(ch.equipment, 0, sizeof(ch.equipment));
  equip_at(&ch, WEAR_LEGS,  make_armor(2, 2, 2, 0)); /* wt 2 -> bulk 4 */
  equip_at(&ch, WEAR_HANDS, make_armor(1, 1, 0, 0)); /* wt 1 -> bulk 1 */
  equip_at(&ch, WEAR_FEET,  make_armor(1, 1, 0, 0)); /* wt 1 -> bulk 1 */

  struct ac_breakdown b2; compute_ac_breakdown(&ch, &b2);
  if (b2.total != 18) dbg_dump_ac("MEDIUM", &b2);
  T_EQI(b2.dex_cap, 2, "Medium dex cap 2");
  T_EQI(b2.dex_mod_applied, 2, "Medium dex +2 applied");
  T_EQI(b2.total_bulk, 6, "Medium bulk score 6");
  T_EQI(b2.total, 18, "Medium total AC");

  /* HEAVY SETUP:
   * Bulk target: Heavy (>=11).
   * Use body bulk 3 (wt 3 => +9), legs bulk 2 (wt 2 => +4), total 13 => Heavy.
   * Dex 18 => +4 but cap 0.
   * Armor piece AC: body=3, legs=2 => sum = 5.
   * Magic: body +3 (per-slot allows up to 3), legs +3 (per-slot 1 -> runtime clamps to 1), global cap 3 -> total 3.
   * Shield: base 2 + magic +5 (clamped to +3) + prof 0 => +5 total.
   * Expect: base 10 + piece 5 + armorMagic 3 + Dex 0 + shield 5 = 23.
   */
  memset(ch.equipment, 0, sizeof(ch.equipment));
  equip_at(&ch, WEAR_BODY, make_armor(3, 3, 3, 0)); /* bulk 3*wt3 => 9 */
  equip_at(&ch, WEAR_LEGS, make_armor(2, 2, 3, 0)); /* magic will clamp via slot+global; bulk 2*wt2 => 4 */
  /* Shield: test magic cap on shield and zero prof */
  struct obj_data *shield = make_armor(0, 0, 5, 0);
  equip_at(&ch, WEAR_SHIELD, shield);

  struct ac_breakdown b3; compute_ac_breakdown(&ch, &b3);
  if (b3.total != 23) dbg_dump_ac("HEAVY", &b3);
  T_EQI(b3.dex_cap, 0, "Heavy dex cap 0");
  T_EQI(b3.dex_mod_applied, 0, "Heavy dex applied 0");
  T_EQI(b3.total_bulk, 13, "Heavy bulk score 13");
  T_EQI(b3.armor_piece_sum, 5, "Heavy piece sum 5");
  T_EQI(b3.armor_magic_sum, 3, "Heavy armor magic at global cap 3");
  T_EQI(b3.shield_bonus, 5, "Shield: base 2 + magic 3 (cap) + prof 0 = 5");
  T_EQI(b3.total, 23, "Heavy total AC");
}

static void test_hit_probability_sanity(void) {
  /* Sanity envelope checks (Monte Carlo with seed) */
  srand(42);

  /* Even-ish fight: attack_mod = +5 vs AC 16 => expect about 55–65% */
  double p1 = simulate_hit_rate(/*atk*/5, /*AC*/16, 200000);
  T_IN_RANGE(p1, 0.55, 0.65, "Hit rate ~60% (atk+5 vs AC16)");

  /* Slightly behind: atk +3 vs AC 17 => expect about 35–50% */
  double p2 = simulate_hit_rate(3, 17, 200000);
  T_IN_RANGE(p2, 0.35, 0.50, "Hit rate ~40% (atk+3 vs AC17)");

  /* Way ahead: atk +8 vs AC 14 => expect ≳80% but < 95% (nat1 auto-miss) */
  double p3 = simulate_hit_rate(8, 14, 200000);
  T_IN_RANGE(p3, 0.80, 0.95, "Hit rate high (atk+8 vs AC14)");

  /* Way behind: atk +0 vs AC 20 => expect ≲15% but > 5% (nat20 auto-hit) */
  double p4 = simulate_hit_rate(0, 20, 200000);
  T_IN_RANGE(p4, 0.05, 0.15, "Hit rate low (atk+0 vs AC20)");
}

int main(void) {
  test_ability_mod();
  test_prof_from_skill();
  test_ac_light_medium_heavy();
  test_hit_probability_sanity();

  printf("Tests run: %d, failures: %d\n", tests_run, tests_failed);
  return tests_failed ? 1 : 0;
}
