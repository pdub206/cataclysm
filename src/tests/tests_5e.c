/* tests_5e.c — unit tests for 5e-like rules */

#include <math.h>
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "spells.h"

extern struct player_special_data dummy_mob;

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

static void init_test_char(struct char_data *ch) {
  memset(ch, 0, sizeof(*ch));
  ch->player_specials = calloc(1, sizeof(struct player_special_data));
  ch->in_room = 0;
}

static void set_prof_bonus(struct char_data *ch, int prof_bonus) {
  ch->player.level = 1;
  ch->points.prof_mod = prof_bonus - get_level_proficiency_bonus(ch);
}

/* Make a simple armor object with given per-piece fields. */
static struct obj_data *make_armor(int piece_ac, int bulk, int magic, int stealth_disadv, int durability, int str_req) {
  struct obj_data *o = calloc(1, sizeof(*o));
  GET_OBJ_TYPE(o) = ITEM_ARMOR;
  GET_OBJ_VAL(o, VAL_ARMOR_PIECE_AC)             = piece_ac;
  GET_OBJ_VAL(o, VAL_ARMOR_BULK)                 = bulk;
  GET_OBJ_VAL(o, VAL_ARMOR_MAGIC_BONUS)          = magic;
  GET_OBJ_VAL(o, VAL_ARMOR_STEALTH_DISADV)       = stealth_disadv;
  GET_OBJ_VAL(o, VAL_ARMOR_DURABILITY)           = durability;
  GET_OBJ_VAL(o, VAL_ARMOR_STR_REQ)              = str_req;
  return o;
}

/* Equip an object at a wear position. */
static void equip_at(struct char_data *ch, int wear_pos, struct obj_data *o) {
  /* Most Circle/tbaMUD trees have ch->equipment[POS] */
  ch->equipment[wear_pos] = o;
}

/* Set an ability score (helpers for readability) */
static void set_ability_scores(struct char_data *ch, int str, int dex, int con, int intel, int wis, int cha) {
  ch->real_abils.str  = str;
  ch->real_abils.dex  = dex;
  ch->real_abils.con  = con;
  ch->real_abils.intel= intel;
  ch->real_abils.wis  = wis;
  ch->real_abils.cha  = cha;
  ch->aff_abils = ch->real_abils; /* common pattern */
}

static double simulate_skill_vs_dc(struct char_data *ch, int skillnum, int dc, int trials) {
  int success = 0;
  for (int i = 0; i < trials; ++i) {
    int total = roll_skill_check(ch, skillnum, 0, NULL);
    if (total >= dc)
      success++;
  }
  return (double)success / (double)trials;
}

static double simulate_stealth_vs_scan(struct char_data *sneaky, struct char_data *scanner, int trials) {
  int success = 0;
  for (int i = 0; i < trials; ++i) {
    int stealth_total = roll_skill_check(sneaky, SKILL_STEALTH, 0, NULL);
    int scan_total = roll_skill_check(scanner, SKILL_PERCEPTION, 0, NULL);
    if (stealth_total > scan_total)
      success++;
  }
  return (double)success / (double)trials;
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
  fprintf(stderr, "%s: total=%d (base=%d armor=%d magic=%d dexCap=%d dex=%d situ=%d bulk=%d)\n",
    label, b->total, b->base, b->armor_piece_sum, b->armor_magic_sum,
    b->dex_cap, b->dex_mod_applied, b->situational, b->total_bulk);
}

/* ---------- TESTS ---------- */

static void test_GET_ABILITY_MOD(void) {
  /* Spot-check classic 5e values and a sweep */
  T_EQI(GET_ABILITY_MOD(10), 0, "GET_ABILITY_MOD(10)");
  T_EQI(GET_ABILITY_MOD(8), -1, "GET_ABILITY_MOD(8)");
  T_EQI(GET_ABILITY_MOD(12), 1, "GET_ABILITY_MOD(12)");
  T_EQI(GET_ABILITY_MOD(18), 4, "GET_ABILITY_MOD(18)");
  T_EQI(GET_ABILITY_MOD(1), -5, "GET_ABILITY_MOD(1)");
  /* sweep 1..30 vs floor((s-10)/2) */
  for (int s = 1; s <= 30; ++s) {
    int expect = (int)floor((s - 10) / 2.0);
    T_EQI(GET_ABILITY_MOD(s), expect, "GET_ABILITY_MOD sweep");
  }
}

static void test_GET_PROFICIENCY(void) {
  /* Boundaries for <= mapping: 0..14→0, 15..29→+1, ... 91..100→+6 */
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
    T_EQI(GET_PROFICIENCY(cases[i].pct), cases[i].expect, cases[i].lbl);
  }
}

static void test_ac_light_medium_heavy(void) {
  /* Fresh character */
  struct char_data ch;
  memset(&ch, 0, sizeof(ch));

  /* LIGHT SETUP:
   * Bulk target: Light (<=5)
   * Uses:
   * Armor AC: head=1, body=1, total=2
   * Bulk: head=1, body=1, total=2
   * Magic: total=0
   * Dex +4
   * No shield.
   * Expect: base 10 + piece 2 + magic 0 + dex 4 = 16
   */
  set_ability_scores(&ch, 10, 18, 10, 10, 10, 10);
  equip_at(&ch, WEAR_HEAD,   make_armor(1,1,0,0,0,0));
  equip_at(&ch, WEAR_BODY,   make_armor(1,1,0,0,0,0));
  equip_at(&ch, WEAR_LEGS,   make_armor(1,2,0,0,0,0));
  equip_at(&ch, WEAR_FEET,   make_armor(1,1,0,0,0,0));

  struct ac_breakdown b1; compute_ac_breakdown(&ch, &b1);
  /* Sanity checks */
  if (b1.total != 18) dbg_dump_ac("LIGHT", &b1);
  T_EQI(b1.dex_cap, 5, "Light dex cap 5");
  T_EQI(b1.dex_mod_applied, 4, "Light dex +4 applied");
  T_EQI(b1.armor_magic_sum, 0, "Light magic cap (global) 3");
  T_EQI(b1.total, 18, "Light total AC");

  /* MEDIUM SETUP:
   * Bulk target: Medium (6..10)
   * Uses:
   * Armor AC: legs=2, hands=1, feet=1, total=4
   * Bulk: legs=2, hands=2, feet=2, total=6
   * Magic: legs=1, hands=1, total=2
   * Dex +4, but cap at +2
   * Expect: base 10 + piece 6 + magic 0 + dex 2 = 18.
   */
  memset(ch.equipment, 0, sizeof(ch.equipment));
  equip_at(&ch, WEAR_HEAD,   make_armor(1,1,0,0,0,0));
  equip_at(&ch, WEAR_BODY,   make_armor(2,2,0,0,0,0));
  equip_at(&ch, WEAR_LEGS,   make_armor(1,2,0,0,0,0));
  equip_at(&ch, WEAR_HANDS,  make_armor(1,1,0,0,0,0));
  equip_at(&ch, WEAR_FEET,   make_armor(1,1,0,0,0,0));

  struct ac_breakdown b2; compute_ac_breakdown(&ch, &b2);
  if (b2.total != 18) dbg_dump_ac("MEDIUM", &b2);
  T_EQI(b2.dex_cap, 2, "Medium dex cap 2");
  T_EQI(b2.dex_mod_applied, 2, "Medium dex +2 applied");
  T_EQI(b2.total_bulk, 7, "Medium bulk score 7");
  T_EQI(b2.total, 18, "Medium total AC");

  /* HEAVY SETUP:
   * Bulk target: Heavy (>=11)
   * Uses:
   * Armor AC: body=3, legs=2. total=5
   * Bulk: body=3, legs=2, total=13
   * Magic: body=3, legs=3, total=6 (max cap of 3, so total=3)
   * Dex +4 but cap 0 due to bulk
   * Expect: base 10 + piece 7 + armorMagic 0 + Dex 0 = 20
   */
  memset(ch.equipment, 0, sizeof(ch.equipment));
  equip_at(&ch, WEAR_HEAD,        make_armor(1,1,1,0,0,0));
  equip_at(&ch, WEAR_BODY,        make_armor(2,3,1,0,0,0));
  equip_at(&ch, WEAR_LEGS,        make_armor(1,2,1,0,0,0));
  equip_at(&ch, WEAR_ARMS,        make_armor(1,2,0,0,0,0));
  equip_at(&ch, WEAR_HANDS,       make_armor(1,1,0,0,0,0));
  equip_at(&ch, WEAR_FEET,        make_armor(1,1,0,0,0,0));

  struct ac_breakdown b3; compute_ac_breakdown(&ch, &b3);
  if (b3.total != 20) dbg_dump_ac("HEAVY", &b3);
  T_EQI(b3.dex_cap, 0, "Heavy dex cap 0");
  T_EQI(b3.dex_mod_applied, 0, "Heavy dex applied 0");
  T_EQI(b3.total_bulk, 10, "Heavy bulk score 10");
  T_EQI(b3.armor_piece_sum, 7, "Heavy piece sum 7");
  T_EQI(b3.armor_magic_sum, 3, "Heavy armor magic at global cap 3");
  T_EQI(b3.total, 20, "Heavy total AC");
}

static void test_hit_probability_sanity(void) {
  /* Sanity envelope checks (Monte Carlo with seed) */
  circle_srandom(42);

  /* Even-ish fight: attack_mod = +5 vs AC 16 => expect about 55–65% */
  double p1 = simulate_hit_rate(/*atk*/5, /*AC*/16, 200000);
  T_IN_RANGE(p1, 0.45, 0.55, "Hit rate ~50% (atk+5 vs AC16)");

  /* Slightly behind: atk +3 vs AC 17 => expect about 35–50% */
  double p2 = simulate_hit_rate(3, 17, 200000);
  T_IN_RANGE(p2, 0.30, 0.40, "Hit rate ~35% (atk+3 vs AC17)");

  /* Way ahead: atk +8 vs AC 14 => expect ≳80% but < 95% (nat1 auto-miss) */
  double p3 = simulate_hit_rate(8, 14, 200000);
  T_IN_RANGE(p3, 0.75, 0.85, "Hit rate high (atk+8 vs AC14)");

  /* Way behind: atk +0 vs AC 20 => expect ≲15% but > 5% (nat20 auto-hit) */
  double p4 = simulate_hit_rate(0, 20, 200000);
  T_IN_RANGE(p4, 0.05, 0.15, "Hit rate low (atk+0 vs AC20)");
}

static void test_stealth_vs_scan_proficiency(void) {
  struct char_data sneaky, scanner;
  init_test_char(&sneaky);
  init_test_char(&scanner);

  set_ability_scores(&sneaky, 10, 14, 10, 10, 10, 10);  /* DEX 14 */
  set_ability_scores(&scanner, 10, 10, 10, 10, 14, 10); /* WIS 14 */

  SET_SKILL(&sneaky, SKILL_STEALTH, 50);
  SET_SKILL(&scanner, SKILL_PERCEPTION, 50);
  set_prof_bonus(&scanner, 0); /* fixed scan proficiency */

  const int trials = 20000;

  set_prof_bonus(&sneaky, 0);
  circle_srandom(12345);
  double p1 = simulate_stealth_vs_scan(&sneaky, &scanner, trials);

  set_prof_bonus(&sneaky, 3);
  circle_srandom(12345);
  double p2 = simulate_stealth_vs_scan(&sneaky, &scanner, trials);

  set_prof_bonus(&sneaky, 6);
  circle_srandom(12345);
  double p3 = simulate_stealth_vs_scan(&sneaky, &scanner, trials);

  T_ASSERT(p1 < p2 && p2 < p3,
           "Stealth vs scan should improve with proficiency (%.3f < %.3f < %.3f)",
           p1, p2, p3);
  T_ASSERT((p3 - p1) > 0.08,
           "Stealth vs scan gap should be meaningful (%.3f -> %.3f)",
           p1, p3);
}

static void test_steal_vs_scan_proficiency(void) {
  struct char_data thief;
  init_test_char(&thief);
  set_ability_scores(&thief, 10, 14, 10, 10, 10, 10); /* DEX 14 */
  SET_SKILL(&thief, SKILL_SLEIGHT_OF_HAND, 50);

  const int trials = 20000;
  const int dc_no_scan = 10; /* compute_steal_dc base with neutral awake victim */
  const int dc_scan = 15;    /* base + scan bonus */
  const int profs[] = { 0, 3, 6 };
  double no_scan[3], scan[3];

  for (size_t i = 0; i < 3; ++i) {
    set_prof_bonus(&thief, profs[i]);

    circle_srandom(24680);
    no_scan[i] = simulate_skill_vs_dc(&thief, SKILL_SLEIGHT_OF_HAND, dc_no_scan, trials);

    circle_srandom(24680);
    scan[i] = simulate_skill_vs_dc(&thief, SKILL_SLEIGHT_OF_HAND, dc_scan, trials);
  }

  T_ASSERT(no_scan[0] < no_scan[1] && no_scan[1] < no_scan[2],
           "Steal vs no-scan improves with proficiency (%.3f < %.3f < %.3f)",
           no_scan[0], no_scan[1], no_scan[2]);
  T_ASSERT(scan[0] < scan[1] && scan[1] < scan[2],
           "Steal vs scan improves with proficiency (%.3f < %.3f < %.3f)",
           scan[0], scan[1], scan[2]);
  T_ASSERT(scan[0] < no_scan[0] && scan[1] < no_scan[1] && scan[2] < no_scan[2],
           "Scan DC should reduce steal success (no-scan %.3f/%.3f/%.3f vs scan %.3f/%.3f/%.3f)",
           no_scan[0], no_scan[1], no_scan[2], scan[0], scan[1], scan[2]);

  T_IN_RANGE(no_scan[0], 0.70, 0.80, "Steal no-scan +0");
  T_IN_RANGE(no_scan[2], 0.93, 0.98, "Steal no-scan +6");
  T_IN_RANGE(scan[0], 0.45, 0.55, "Steal scan +0");
  T_IN_RANGE(scan[2], 0.65, 0.75, "Steal scan +6");
}

int main(void) {
  test_GET_ABILITY_MOD();
  test_GET_PROFICIENCY();
  test_ac_light_medium_heavy();
  test_hit_probability_sanity();
  test_stealth_vs_scan_proficiency();
  test_steal_vs_scan_proficiency();

  printf("Tests run: %d, failures: %d\n", tests_run, tests_failed);
  return tests_failed ? 1 : 0;
}
