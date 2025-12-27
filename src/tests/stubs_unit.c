/* tests/stubs_unit.c – minimal stubs to satisfy utils.o linkage for unit tests */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

/* --- Globals expected by utils.c --- */
FILE *logfile = NULL;                    /* used by mudlog/basic_mud_vlog */
struct descriptor_data *descriptor_list = NULL;
struct player_special_data dummy_mob;    /* dummy specials area for mobs */

/* --- Minimal world so in_room lookups are safe in tests/sims --- */
struct room_data stub_room;        /* zeroed room */
struct room_data *world = &stub_room;
int top_of_world = 0;                    /* room/world info */
struct weather_data weather_info;

/* A few arrays symbols utils.c references in helpers (keep minimal) */
const char *pc_class_types[] = { "class", NULL };

/* --- Functions utils.c references that we don't need in tests --- */
bool has_save_proficiency(int class_num, int ability) {
  (void)class_num; (void)ability;
  return FALSE;
}

void send_to_char(struct char_data *ch, const char *messg, ...) {
  /* no-op for tests */
  (void)ch; (void)messg;
}

void act(const char *str, int hide_invisible, struct char_data *ch,
         struct obj_data *obj, void *vict_obj, int type) {
  /* no-op */
  (void)str; (void)hide_invisible; (void)ch; (void)obj; (void)vict_obj; (void)type;
}

int affected_by_spell(struct char_data *ch, int skill) {
  (void)ch; (void)skill; return 0;
}

void affect_from_char(struct char_data *ch, int type) {
  (void)ch; (void)type;
}

void page_string(struct descriptor_data *d, char *str, int keep_internal) {
  (void)d; (void)str; (void)keep_internal;
}

int is_abbrev(const char *arg1, const char *arg2) {
  (void)arg1; (void)arg2; return 0;
}

void parse_tab(const char *buf, char *out, size_t outlen) {
  /* trivial passthrough */
  if (outlen) {
    size_t i = 0;
    for (; buf[i] && i + 1 < outlen; ++i) out[i] = buf[i];
    out[i] = '\0';
  }
}
