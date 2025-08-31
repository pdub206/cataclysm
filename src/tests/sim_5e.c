/* tests/sim_5e.c — quick simulations for 5e-like tuning */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "constants.h"

/* ---------- local RNG for the sim (do NOT use MUD's rand_number here) ---------- */
static inline int randi_closed(int lo, int hi) {
  /* inclusive [lo, hi] using C RNG; assumes lo <= hi */
  return lo + (rand() % (hi - lo + 1));
}

static inline int d20_local(void) { return randi_closed(1, 20); }

static inline int dice_local(int ndice, int sdice) {
  int sum = 0;
  for (int i = 0; i < ndice; ++i) sum += randi_closed(1, sdice);
  return sum;
}

/* ---------- minimal helpers (same style as tests_5e) ---------- */
static void init_test_char(struct char_data *ch) {
  memset(ch, 0, sizeof(*ch));
  /* ensure skills and other saved fields exist if GET_SKILL() dereferences */
  ch->player_specials = calloc(1, sizeof(struct player_special_data));
  /* if the tree uses player_specials->saved, calloc above keeps it zeroed */
  ch->in_room = 0; /* park them in room #0 (we'll make a stub room below) */
}

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

static void equip_at(struct char_data *ch, int wear_pos, struct obj_data *o) {
  if (wear_pos < 0 || wear_pos >= NUM_WEARS) {
    fprintf(stderr, "equip_at: wear_pos %d out of bounds (NUM_WEARS=%d)\n", wear_pos, NUM_WEARS);
    abort();
  }
  ch->equipment[wear_pos] = o;
}
static void set_ability_scores(struct char_data *ch, int str, int dex, int con, int intel, int wis, int cha) {
  ch->real_abils.str   = str;
  ch->real_abils.dex   = dex;
  ch->real_abils.con   = con;
  ch->real_abils.intel = intel;
  ch->real_abils.wis   = wis;
  ch->real_abils.cha   = cha;
  ch->aff_abils        = ch->real_abils;
}

/* d20 hit sim with nat 1/20 using local RNG */
static double hit_rate(int attack_mod, int target_ac, int trials) {
  int hits = 0;
  for (int i=0;i<trials;++i) {
    int d20 = d20_local();
    int hit = (d20==20) || (d20!=1 && (d20 + attack_mod) >= target_ac);
    hits += hit;
  }
  return (double)hits / (double)trials;
}

/* simple DPR per swing: roll dice + STR mod; 0 on miss (local RNG) */
static int swing_damage(int ndice, int sdice, int str_mod) {
  return dice_local(ndice, sdice) + str_mod;
}

/* duel until someone hits 0 HP; return rounds elapsed (attacker first each round) */
static int duel_rounds(int atk_mod, int ndice, int sdice, int att_str_mod,
                       struct char_data *def, int def_hp, int trials)
{
  int rounds_sum = 0;
  int def_ac = compute_ascending_ac(def);

  for (int t=0;t<trials;++t) {
    int hp = def_hp;
    int rounds = 0;
    while (hp > 0) {
      int d20 = d20_local();
      int hit = (d20==20) || (d20!=1 && (d20 + atk_mod) >= def_ac);
      if (hit) hp -= swing_damage(ndice, sdice, att_str_mod);
      rounds++;
      if (rounds > 1000) break; /* safety */
    }
    rounds_sum += rounds;
  }
  return (int) floor((double)rounds_sum / (double)trials);
}

/* build three defenders with real slot weights and caps */
static void build_light(struct char_data *ch) {
  init_test_char(ch);
  set_ability_scores(ch, 10, 18, 10, 10, 10, 10);
  equip_at(ch, WEAR_HEAD,   make_armor(1,1,0,0,0,0));
  equip_at(ch, WEAR_BODY,   make_armor(1,1,0,0,0,0));
  equip_at(ch, WEAR_LEGS,   make_armor(1,2,0,0,0,0));
  equip_at(ch, WEAR_FEET,   make_armor(1,1,0,0,0,0));
}

static void build_medium(struct char_data *ch) {
  init_test_char(ch);
  set_ability_scores(ch, 10, 18, 10, 10, 10, 10);
  equip_at(ch, WEAR_HEAD,   make_armor(2,1,0,0,0,0));
  equip_at(ch, WEAR_BODY,   make_armor(2,2,1,0,0,0));
  equip_at(ch, WEAR_LEGS,   make_armor(2,2,0,0,0,0));
  equip_at(ch, WEAR_HANDS,  make_armor(1,1,0,0,0,0));
  equip_at(ch, WEAR_FEET,   make_armor(1,1,0,0,0,0));
}

static void build_heavy(struct char_data *ch) {
  init_test_char(ch);
  set_ability_scores(ch, 10, 18, 10, 10, 10, 10);
  equip_at(ch, WEAR_HEAD,        make_armor(2,1,1,0,0,0));
  equip_at(ch, WEAR_BODY,        make_armor(3,3,1,0,0,0));
  equip_at(ch, WEAR_LEGS,        make_armor(2,1,1,0,0,0));
  equip_at(ch, WEAR_ARMS,        make_armor(1,1,0,0,0,0));
  equip_at(ch, WEAR_HANDS,       make_armor(1,1,0,0,0,0));
  equip_at(ch, WEAR_FEET,        make_armor(1,1,0,0,0,0));
  equip_at(ch, WEAR_WRIST_L,     make_armor(1,1,0,0,0,0));
  equip_at(ch, WEAR_WRIST_R,     make_armor(1,1,0,0,0,0));
}

/* attacker profiles: compute attack_mod = STRmod + prof(skill%) + weapon_magic */
static int compute_attack_mod(int str_score, int skill_pct, int weapon_magic) {
  int mod = GET_ABILITY_MOD(str_score);
  mod += GET_PROFICIENCY(skill_pct);
  if (weapon_magic > MAX_WEAPON_MAGIC) weapon_magic = MAX_WEAPON_MAGIC;
  mod += weapon_magic;
  if (mod > MAX_TOTAL_ATTACK_BONUS) mod = MAX_TOTAL_ATTACK_BONUS;
  return mod;
}

int main(void) {
  /* seed local RNG (do not rely on MUD RNG here) */
  srand(42);

  /* 1) Hit-rate grid: atk_mod 0..10 vs AC 12..20 */
  printf("Hit-rate grid (trials=50000):\n      ");
  for (int ac=12; ac<=20; ++ac) printf(" AC%2d ", ac);
  printf("\n");
  for (int atk=0; atk<=10; ++atk) {
    printf("atk%2d ", atk);
    for (int ac=12; ac<=20; ++ac) {
      double p = hit_rate(atk, ac, 50000);
      printf(" %5.1f", p*100.0);
    }
    printf("\n");
  }
  printf("\n");

  /* 2) Real defenders AC using compute_ac_breakdown */
  struct char_data light, medium, heavy;
  build_light(&light);
  build_medium(&medium);
  build_heavy(&heavy);

  struct ac_breakdown bl, bm, bh0;
  compute_ac_breakdown(&light,  &bl);
  compute_ac_breakdown(&medium, &bm);
  compute_ac_breakdown(&heavy, &bh0);

  printf("Defender AC breakdowns:\n");
  printf(" Light : total=%d (base=%d armor=%d magic=%d dexCap=%d dex=%+d situ=%+d bulk=%d)\n",
         bl.total, bl.base, bl.armor_piece_sum, bl.armor_magic_sum, bl.dex_cap,
         bl.dex_mod_applied, bl.situational, bl.total_bulk);
  printf(" Medium: total=%d (base=%d armor=%d magic=%d dexCap=%d dex=%+d situ=%+d bulk=%d)\n",
         bm.total, bm.base, bm.armor_piece_sum, bm.armor_magic_sum, bm.dex_cap,
         bm.dex_mod_applied, bm.situational, bm.total_bulk);
  printf(" Heavy : total=%d (base=%d armor=%d magic=%d dexCap=%d dex=%+d situ=%+d bulk=%d)\n",
         bh0.total, bh0.base, bh0.armor_piece_sum, bh0.armor_magic_sum, bh0.dex_cap,
         bh0.dex_mod_applied, bh0.situational, bh0.total_bulk);

  /* 3) Attacker profiles vs defenders (TTK & hit%, 1d8 weapon) */
  struct { int str; int skill; int wmag; const char *name; } atk[] = {
    {14, 30, 0, "Novice (STR14, skill30, wm+0)"},
    {16, 60, 1, "Skilled (STR16, skill60, wm+1)"},
    {18, 90, 3, "Expert  (STR18, skill90, wm+3)"},
  };
  struct { struct char_data *def; const char *name; int hp; } def[] = {
    { &light,  "Light",  40 },
    { &medium, "Medium", 50 },
    { &heavy, "Heavy",  60 },
  };

  printf("Matchups (trials=20000, 1d8 weapon):\n");
  for (size_t i=0;i<sizeof(atk)/sizeof(atk[0]);++i) {
    int str_mod = GET_ABILITY_MOD(atk[i].str);
    int atk_mod = compute_attack_mod(atk[i].str, atk[i].skill, atk[i].wmag);
    printf(" Attacker: %-28s => attack_mod %+d (STRmod %+d, prof %+d, wm %+d)\n",
           atk[i].name, atk_mod, str_mod, GET_PROFICIENCY(atk[i].skill), MIN(atk[i].wmag, MAX_WEAPON_MAGIC));
    for (size_t j=0;j<sizeof(def)/sizeof(def[0]);++j) {
      int ac = compute_ascending_ac(def[j].def);
      double p = hit_rate(atk_mod, ac, 20000);
      int r  = duel_rounds(atk_mod, 1, 8, str_mod, def[j].def, def[j].hp, 20000);
      printf("   vs %-16s AC=%2d  hit%%=%5.1f   avg rounds-to-kill ~ %d\n", def[j].name, ac, p*100.0, r);
    }
    printf("\n");
  }

  return 0;
}
