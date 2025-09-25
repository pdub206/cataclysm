/**
* @file roomsave.c
* Numeric and string contants used by the MUD.
*
* An addition to the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
*/
#include "conf.h"
#include "sysdep.h"

#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "comm.h"
#include "constants.h"
#include "roomsave.h"

/* Write saved rooms under lib/world/rsv/<vnum>.rsv (like wld/ zon/ obj/). */
#ifndef ROOMSAVE_PREFIX
#define ROOMSAVE_PREFIX  LIB_WORLD "rsv/"
#endif
#ifndef ROOMSAVE_EXT
#define ROOMSAVE_EXT     ".rsv"
#endif

static unsigned char *roomsave_dirty = NULL;

void RoomSave_init_dirty(void) {
  free(roomsave_dirty);
  roomsave_dirty = calloc((size_t)top_of_world + 1, 1);
}

void RoomSave_mark_dirty_room(room_rnum rnum) {
  if (!roomsave_dirty) return;
  if (rnum != NOWHERE && rnum >= 0 && rnum <= top_of_world)
    roomsave_dirty[rnum] = 1;
}

/* Where does an object “live” (topmost location -> room)? */
room_rnum RoomSave_room_of_obj(struct obj_data *o) {
  if (!o) return NOWHERE;
  while (o->in_obj) o = o->in_obj;
  if (o->carried_by) return IN_ROOM(o->carried_by);
  if (o->worn_by)    return IN_ROOM(o->worn_by);
  return o->in_room;
}

/* --- helper: read a list of objects until '.' or 'E' and return the head --- */
/* Context-aware implementation: stop_on_E = 1 for nested B..E, 0 for top-level. */
static struct obj_data *roomsave_read_list_ctx(FILE *fl, int stop_on_E)
{
  char line[256];
  struct obj_data *head = NULL, *tail = NULL;

  while (fgets(line, sizeof(line), fl)) {
    if (line[0] == '.') {
      /* End of this list scope */
      break;
    }

    if (stop_on_E && line[0] == 'E') {
      /* End of nested (B..E) scope */
      break;
    }

    /* For top-level reads (stop_on_E==0), or any non-'O', push back
       so the outer #R reader can handle M/E/G/P or '.' */
    if (line[0] != 'O') {
      long back = -((long)strlen(line));
      fseek(fl, back, SEEK_CUR);
      break;
    }

    /* Parse object header: O vnum timer weight cost rent */
    int vnum, timer, weight, cost, rent;
    if (sscanf(line, "O %d %d %d %d %d", &vnum, &timer, &weight, &cost, &rent) != 5)
      continue;

    /* IMPORTANT: read by VNUM (VIRTUAL), not real index */
    struct obj_data *obj = read_object((obj_vnum)vnum, VIRTUAL);
    if (!obj) {
      mudlog(NRM, LVL_IMMORT, TRUE, "RoomSave: read_object(vnum=%d) failed.", vnum);
      /* Skip to next object/header or end-of-scope */
      long backpos;
      while (fgets(line, sizeof(line), fl)) {
        if (line[0] == 'O' || line[0] == '.' || (stop_on_E && line[0] == 'E')) {
          backpos = -((long)strlen(line));
          fseek(fl, backpos, SEEK_CUR);
          break;
        }
      }
      continue;
    }

    /* Apply core scalars */
    GET_OBJ_TIMER(obj)  = timer;
    GET_OBJ_WEIGHT(obj) = weight;
    GET_OBJ_COST(obj)   = cost;
    GET_OBJ_RENT(obj)   = rent;

    /* Clear array flags so missing slots don't keep proto bits */
#ifdef EF_ARRAY_MAX
# ifdef GET_OBJ_EXTRA_AR
    for (int i = 0; i < EF_ARRAY_MAX; i++) GET_OBJ_EXTRA_AR(obj, i) = 0;
# else
    for (int i = 0; i < EF_ARRAY_MAX; i++) GET_OBJ_EXTRA(obj)[i] = 0;
# endif
#endif
#ifdef TW_ARRAY_MAX
    for (int i = 0; i < TW_ARRAY_MAX; i++) GET_OBJ_WEAR(obj)[i] = 0;
#endif

    /* Read per-object lines until next 'O' or '.' or 'E'(when nested) */
    long backpos;
    while (fgets(line, sizeof(line), fl)) {
      if (line[0] == 'V') {
        int idx, val;
        if (sscanf(line, "V %d %d", &idx, &val) == 2) {
#ifdef NUM_OBJ_VAL_POSITIONS
          if (idx >= 0 && idx < NUM_OBJ_VAL_POSITIONS) GET_OBJ_VAL(obj, idx) = val;
#else
          if (idx >= 0 && idx < 6) GET_OBJ_VAL(obj, idx) = val;
#endif
        }
        continue;
      } else if (line[0] == 'X') { /* extra flags */
        int idx, val;
        if (sscanf(line, "X %d %d", &idx, &val) == 2) {
#if defined(EF_ARRAY_MAX) && defined(GET_OBJ_EXTRA_AR)
          if (idx >= 0 && idx < EF_ARRAY_MAX) GET_OBJ_EXTRA_AR(obj, idx) = val;
#elif defined(EF_ARRAY_MAX)
          if (idx >= 0 && idx < EF_ARRAY_MAX) GET_OBJ_EXTRA(obj)[idx] = val;
#else
          if (idx == 0) GET_OBJ_EXTRA(obj) = val;
#endif
        }
        continue;
      } else if (line[0] == 'W') { /* wear flags */
        int idx, val;
        if (sscanf(line, "W %d %d", &idx, &val) == 2) {
#ifdef TW_ARRAY_MAX
          if (idx >= 0 && idx < TW_ARRAY_MAX) GET_OBJ_WEAR(obj)[idx] = val;
#else
          if (idx == 0) GET_OBJ_WEAR(obj) = val;
#endif
        }
        continue;
      } else if (line[0] == 'B') {
        /* Nested contents until matching 'E' */
        struct obj_data *child_head = roomsave_read_list_ctx(fl, 1 /* stop_on_E */);

        /* Detach each node before obj_to_obj(), otherwise we lose siblings */
        for (struct obj_data *it = child_head, *next; it; it = next) {
          next = it->next_content;  /* remember original sibling */
          it->next_content = NULL;  /* detach from temp list */
          obj_to_obj(it, obj);      /* push into container (LIFO) */
        }
        continue;
      } else if (line[0] == 'O' || line[0] == '.' || (stop_on_E && line[0] == 'E')) {
        /* Next object / end-of-scope: rewind one line for outer loop to see it */
        backpos = -((long)strlen(line));
        fseek(fl, backpos, SEEK_CUR);
        break;
      } else {
        /* ignore unknown lines */
        continue;
      }
    }

    /* Append to this scope's list */
    obj->next_content = NULL;
    if (!head) head = tail = obj;
    else { tail->next_content = obj; tail = obj; }
  }

  return head;
}

/* Keep your existing one-arg API for callers: top-level semantics (stop_on_E = 0). */
static struct obj_data *roomsave_read_list(FILE *fl)
{
  return roomsave_read_list_ctx(fl, 0);
}

/* ---------- Minimal line format ----------
#R <vnum> <unix_time>
O <vnum> <timer> <extra_flags> <wear_flags> <weight> <cost> <rent>
V <i> <val[i]>    ; repeated for all value slots present on this obj
B                 ; begin contents of this object (container)
E                 ; end contents of this object
.                 ; end of room
------------------------------------------ */

static void ensure_dir_exists(const char *path) {
  if (mkdir(path, 0775) == -1 && errno != EEXIST) {
    mudlog(CMP, LVL_IMMORT, TRUE, "SYSERR: roomsave mkdir(%s): %s", path, strerror(errno));
  }
}

/* zone vnum for a given room rnum (e.g., 134 -> zone 1) */
static int roomsave_zone_for_rnum(room_rnum rnum) {
  if (rnum == NOWHERE) return -1;
  if (world[rnum].zone < 0 || world[rnum].zone > top_of_zone_table) return -1;
  return zone_table[ world[rnum].zone ].number; /* zone virtual number (e.g., 1) */
}

/* lib/world/rsv/<zone>.rsv */
static void roomsave_zone_filename(int zone_vnum, char *out, size_t outsz) {
  snprintf(out, outsz, "%s%d%s", ROOMSAVE_PREFIX, zone_vnum, ROOMSAVE_EXT);
}

/* Write one object (and its recursive contents) */
static void write_one_object(FILE *fl, struct obj_data *obj) {
  int i;

  /* Core scalars (flags printed separately per-slot) */
  fprintf(fl, "O %d %d %d %d %d\n",
          GET_OBJ_VNUM(obj),
          GET_OBJ_TIMER(obj),
          GET_OBJ_WEIGHT(obj),
          GET_OBJ_COST(obj),
          GET_OBJ_RENT(obj));

/* Extra flags array */
#if defined(EF_ARRAY_MAX) && defined(GET_OBJ_EXTRA_AR)
  for (i = 0; i < EF_ARRAY_MAX; i++)
    fprintf(fl, "X %d %d\n", i, GET_OBJ_EXTRA_AR(obj, i));
#elif defined(EF_ARRAY_MAX)
  for (i = 0; i < EF_ARRAY_MAX; i++)
    fprintf(fl, "X %d %d\n", i, GET_OBJ_EXTRA(obj)[i]);
#else
  fprintf(fl, "X %d %d\n", 0, GET_OBJ_EXTRA(obj));
#endif

/* Wear flags array */
#ifdef TW_ARRAY_MAX
  for (i = 0; i < TW_ARRAY_MAX; i++)
    fprintf(fl, "W %d %d\n", i, GET_OBJ_WEAR(obj)[i]);
#else
  fprintf(fl, "W %d %d\n", 0, GET_OBJ_WEAR(obj));
#endif

  /* Values[] (durability, liquids, charges, etc.) */
#ifdef NUM_OBJ_VAL_POSITIONS
  for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
    fprintf(fl, "V %d %d\n", i, GET_OBJ_VAL(obj, i));
#else
  for (i = 0; i < 6; i++)
    fprintf(fl, "V %d %d\n", i, GET_OBJ_VAL(obj, i));
#endif

  /* Contents (recursive) */
  if (obj->contains) {
    struct obj_data *cont;
    fprintf(fl, "B\n");
    for (cont = obj->contains; cont; cont = cont->next_content)
      write_one_object(fl, cont);
    fprintf(fl, "E\n");
  }
}

/* Forward declaration for RoomSave_now*/
static void RS_write_room_mobs(FILE *out, room_rnum rnum);

/* Public: write the entire room’s contents */
int RoomSave_now(room_rnum rnum) {
  char path[PATH_MAX], tmp[PATH_MAX], line[512];
  FILE *in = NULL, *out = NULL;
  room_vnum rvnum;
  int zvnum;

  if (rnum == NOWHERE)
    return 0;

  rvnum = world[rnum].number;
  zvnum = roomsave_zone_for_rnum(rnum);
  if (zvnum < 0)
    return 0;

  ensure_dir_exists(ROOMSAVE_PREFIX);
  roomsave_zone_filename(zvnum, path, sizeof(path));

  /* Build temp file path safely (same dir as final file). */
  {
    int n = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (n < 0 || n >= (int)sizeof(tmp)) {
      mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: RoomSave: temp path too long for %s", path);
      return 0;
    }
  }

  /* Open output temp; copy existing zone file minus this room’s block */
  if (!(out = fopen(tmp, "w"))) {
    mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: RoomSave: fopen(%s) failed: %s", tmp, strerror(errno));
    return 0;
  }

  if ((in = fopen(path, "r")) != NULL) {
    while (fgets(line, sizeof(line), in)) {
      if (strncmp(line, "#R ", 3) == 0) {
        int file_rvnum;
        long ts;
        if (sscanf(line, "#R %d %ld", &file_rvnum, &ts) == 2) {
          if (file_rvnum == (int)rvnum) {
            /* Skip old block for this room until '.' line */
            while (fgets(line, sizeof(line), in)) {
              if (line[0] == '.')
                break;
            }
            continue; /* do NOT write skipped lines */
          }
        }
      }
      /* Keep unrelated content */
      fputs(line, out);
    }
    fclose(in);
  }

  /* Append fresh block for this room */
  fprintf(out, "#R %d %ld\n", rvnum, (long)time(0));

  /* Save NPCs (and their gear/inventory) present in the room */
  RS_write_room_mobs(out, rnum);

  /* Then write ground objects */
  for (struct obj_data *obj = world[rnum].contents; obj; obj = obj->next_content)
    write_one_object(out, obj);

  fprintf(out, ".\n");

  /* Finish and atomically replace */
  if (fclose(out) != 0) {
    mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: RoomSave: fclose(%s) failed: %s", tmp, strerror(errno));
    return 0;
  }
  if (rename(tmp, path) != 0) {
    mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: RoomSave: rename(%s -> %s) failed: %s", tmp, path, strerror(errno));
    /* Leave tmp in place for debugging */
    return 0;
  }

  return 1;
}

/* --- M/E/G/P load helpers (mob restore) -------------------------------- */

struct rs_load_ctx {
  room_rnum rnum;
  struct char_data *cur_mob;      /* last mob spawned by 'M' */
  struct obj_data  *stack[16];    /* container stack by depth for 'P' */
};

static struct obj_data *RS_create_obj_by_vnum(obj_vnum ov) {
  obj_rnum ornum;
  if (ov <= 0) return NULL;
  ornum = real_object(ov);
  if (ornum == NOTHING) return NULL;
  return read_object(ornum, REAL);
}

static struct char_data *RS_create_mob_by_vnum(mob_vnum mv) {
  mob_rnum mrnum;
  if (mv <= 0) return NULL;
  mrnum = real_mobile(mv);
  if (mrnum == NOBODY) return NULL;
  return read_mobile(mrnum, REAL);
}

/* Forward decl so RS_parse_mob_line can use it without implicit declaration */
static void RS_stack_clear(struct rs_load_ctx *ctx);

/* Handle one line inside a #R block. Returns 1 if handled here. */
static int RS_parse_mob_line(struct rs_load_ctx *ctx, char *line)
{
  if (!line) return 0;
  while (*line == ' ' || *line == '\t') ++line;
  if (!*line) return 0;

  switch (line[0]) {
    case 'M': { /* M <mob_vnum> */
      mob_vnum mv;
      if (sscanf(line+1, " %d", (int *)&mv) != 1) return 0;

      /* Start a fresh mob context: set new mob, clear container stack (NOT cur_mob). */
      ctx->cur_mob = RS_create_mob_by_vnum(mv);
      if (!ctx->cur_mob) return 1; /* bad vnum -> ignore line */
      char_to_room(ctx->cur_mob, ctx->rnum);
      RS_stack_clear(ctx);
      return 1;
    }

    case 'E': { /* E <wear_pos> <obj_vnum> */
      int pos; obj_vnum ov; struct obj_data *obj;
      if (!ctx->cur_mob) return 1;       /* orphan -> ignore */
      if (sscanf(line+1, " %d %d", &pos, (int *)&ov) != 2) return 0;
      obj = RS_create_obj_by_vnum(ov);
      if (!obj) return 1;

      if (pos < 0 || pos >= NUM_WEARS) pos = WEAR_HOLD; /* clamp */
      equip_char(ctx->cur_mob, obj, pos);

      /* Reset ONLY container stack for following P-lines; keep cur_mob */
      RS_stack_clear(ctx);
      ctx->stack[0] = obj;
      return 1;
    }

    case 'G': { /* G <obj_vnum> */
      obj_vnum ov; struct obj_data *obj;
      if (!ctx->cur_mob) return 1;       /* orphan -> ignore */
      if (sscanf(line+1, " %d", (int *)&ov) != 1) return 0;
      obj = RS_create_obj_by_vnum(ov);
      if (!obj) return 1;

      obj_to_char(obj, ctx->cur_mob);

      RS_stack_clear(ctx);
      ctx->stack[0] = obj;
      return 1;
    }

    case 'P': { /* P <depth> <obj_vnum> : put into last obj at (depth-1) */
      int depth; obj_vnum ov; struct obj_data *parent, *obj;
      if (sscanf(line+1, " %d %d", &depth, (int *)&ov) != 2) return 0;
      if (depth <= 0 || depth >= (int)(sizeof(ctx->stack)/sizeof(ctx->stack[0])))
        return 1;
      parent = ctx->stack[depth-1];
      if (!parent) return 1;

      obj = RS_create_obj_by_vnum(ov);
      if (!obj) return 1;
      obj_to_obj(obj, parent);

      ctx->stack[depth] = obj;
      { int d; for (d = depth+1; d < (int)(sizeof(ctx->stack)/sizeof(ctx->stack[0])); ++d) ctx->stack[d] = NULL; }
      return 1;
    }

    default:
      return 0;
  }
}

/* Forward decls for mob restore helpers */
static void RS_stack_clear(struct rs_load_ctx *ctx);

/* Reads a single #R room block (until a line starting with '.') and restores:
 * - ground objects via existing roomsave_read_list()
 * - mobs + E/G/P via RS_parse_mob_line()
 */
static void RS_read_room_block(FILE *fl, room_rnum rnum, room_vnum rvnum,
                               int *restored_objs, int *restored_mobs)
{
  char line[512];
  long pos;
  struct rs_load_ctx mctx;

  if (restored_objs) *restored_objs = 0;
  if (restored_mobs) *restored_mobs = 0;

  mctx.rnum = rnum;
  mctx.cur_mob = NULL;
  RS_stack_clear(&mctx);

  for (;;) {
    pos = ftell(fl);
    if (!fgets(line, sizeof(line), fl))
      break;                       /* EOF */

    /* Trim leading space for dispatch */
    char *p = line;
    while (*p == ' ' || *p == '\t') ++p;

    if (*p == '.')
      break;                       /* end of room block */

    if (*p == 'O') {
      /* Rewind to start of this 'O' line and let the object reader consume O-lines;
         top-level wrapper will not consume E/G/P. */
      struct obj_data *list, *it, *next;
      fseek(fl, pos, SEEK_SET);
      list = roomsave_read_list(fl);
      for (it = list; it; it = next) {
        next = it->next_content;
        it->next_content = NULL;
        obj_to_room(it, rnum);
        if (restored_objs) (*restored_objs)++;
      }
      continue;
    }

    /* Try M/E/G/P (mob + equipment/inventory + nested P) */
    if (RS_parse_mob_line(&mctx, p)) {
      if (*p == 'M' && restored_mobs) (*restored_mobs)++;
      continue;
    }

    /* Unknown token in room block: ignore */
  }
}

void RoomSave_boot(void)
{
  DIR *dirp;
  struct dirent *dp;

  ensure_dir_exists(ROOMSAVE_PREFIX);

  dirp = opendir(ROOMSAVE_PREFIX);
  if (!dirp) {
    mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: RoomSave_boot: cannot open %s", ROOMSAVE_PREFIX);
    return;
  }

  log("RoomSave: scanning %s for *.rsv", ROOMSAVE_PREFIX);

  while ((dp = readdir(dirp))) {
    size_t n = strlen(dp->d_name);
    if (n < 5) continue; /* skip . and .. */
    if (strcmp(dp->d_name + n - 4, ROOMSAVE_EXT) != 0) continue;

    {
      char path[PATH_MAX];
      int wn = snprintf(path, sizeof(path), "%s%s", ROOMSAVE_PREFIX, dp->d_name);
      if (wn < 0 || wn >= (int)sizeof(path)) {
        mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: RoomSave_boot: path too long: %s%s",
               ROOMSAVE_PREFIX, dp->d_name);
        continue;
      }

      FILE *fl = fopen(path, "r");
      if (!fl) {
        mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: RoomSave_boot: fopen(%s) failed: %s",
               path, strerror(errno));
        continue;
      }

      log("RoomSave: reading %s", path);

      /* Per-file counters */
      int blocks = 0;
      int restored_objs_total = 0;
      int restored_mobs_total = 0;

      /* Read lines looking for #R <room_vnum> <timestamp> headers */
      for (;;) {
        char line[512];
        if (!fgets(line, sizeof(line), fl))
          break; /* EOF */

        if (strncmp(line, "#R ", 3) != 0)
          continue;

        /* Parse header */
        {
          int rvnum; long ts;
          if (sscanf(line, "#R %d %ld", &rvnum, &ts) != 2) {
            mudlog(NRM, LVL_IMMORT, TRUE, "RoomSave: malformed #R header in %s: %s", path, line);
            /* skip to end-of-block '.' */
            while (fgets(line, sizeof(line), fl)) if (line[0] == '.') break;
            continue;
          }

          blocks++;

          /* Resolve room */
          {
            room_rnum rnum = real_room((room_vnum)rvnum);
            if (rnum == NOWHERE) {
              mudlog(NRM, LVL_IMMORT, FALSE,
                     "RoomSave: unknown room vnum %d in %s (skipping)", rvnum, path);
              /* skip to end-of-block '.' */
              while (fgets(line, sizeof(line), fl)) if (line[0] == '.') break;
              continue;
            }

            /* Clear this room's current ground contents (idempotent at boot). */
            while (world[rnum].contents)
              extract_obj(world[rnum].contents);

            /* Use unified reader: restores both objects and mobs from this #R block. */
            {
              int got_o = 0, got_m = 0;
              RS_read_room_block(fl, rnum, (room_vnum)rvnum, &got_o, &got_m);
              restored_objs_total += got_o;
              restored_mobs_total += got_m;

              if (got_m > 0)
                log("RoomSave: room %d <- %d object(s) and %d mob(s)", rvnum, got_o, got_m);
              else
                log("RoomSave: room %d <- %d object(s)", rvnum, got_o);
            }
          }
        }
      }

      log("RoomSave: finished %s (blocks=%d, objects=%d, mobs=%d)",
          path, blocks, restored_objs_total, restored_mobs_total);

      fclose(fl);
    }
  }

  closedir(dirp);
}

/* Save all rooms flagged ROOM_SAVE. Called from point_update() on a cadence. */
void RoomSave_autosave_tick(void) {
  for (room_rnum rnum = 0; rnum <= top_of_world; rnum++) {
    if (!ROOM_FLAGGED(rnum, ROOM_SAVE)) continue;
    if (!roomsave_dirty || !roomsave_dirty[rnum]) continue;
    if (RoomSave_now(rnum))
      roomsave_dirty[rnum] = 0;
  }
}

/* ======== MOB SAVE: write NPCs and their equipment/inventory ========== */

/* Depth-aware writer for container contents under a parent object.
 * Writes: P <depth> <obj_vnum>
 * depth starts at 1 for direct children. */
static void RS_write_P_chain(FILE *fp, struct obj_data *parent, int depth) {
  struct obj_data *c;
  for (c = parent->contains; c; c = c->next_content) {
    obj_vnum cv = GET_OBJ_VNUM(c);
    if (cv <= 0) continue;                          /* skip non-proto / invalid */
    fprintf(fp, "P %d %d\n", depth, (int)cv);
    if (c->contains)
      RS_write_P_chain(fp, c, depth + 1);
  }
}

/* Writes: E <wear_pos> <obj_vnum>  (then P-chain) */
static void RS_write_mob_equipment(FILE *fp, struct char_data *mob) {
  int w;
  for (w = 0; w < NUM_WEARS; ++w) {
    struct obj_data *eq = GET_EQ(mob, w);
    if (!eq) continue;
    if (GET_OBJ_VNUM(eq) <= 0) continue;
    fprintf(fp, "E %d %d\n", w, (int)GET_OBJ_VNUM(eq));
    if (eq->contains) RS_write_P_chain(fp, eq, 1);
  }
}

/* Writes: G <obj_vnum> for inventory items (then P-chain) */
static void RS_write_mob_inventory(FILE *fp, struct char_data *mob) {
  struct obj_data *o;
  for (o = mob->carrying; o; o = o->next_content) {
    if (GET_OBJ_VNUM(o) <= 0) continue;
    fprintf(fp, "G %d\n", (int)GET_OBJ_VNUM(o));
    if (o->contains) RS_write_P_chain(fp, o, 1);
  }
}

/* Top-level writer: for each NPC in room, emit:
 *   M <mob_vnum>
 *   [E ...]*
 *   [G ...]*
 * (Players are ignored.) */
static void RS_write_room_mobs(FILE *out, room_rnum rnum) {
  struct char_data *mob;
  for (mob = world[rnum].people; mob; mob = mob->next_in_room) {
    if (!IS_NPC(mob)) continue;
    if (GET_MOB_VNUM(mob) <= 0) continue;
    fprintf(out, "M %d\n", (int)GET_MOB_VNUM(mob));
    RS_write_mob_equipment(out, mob);
    RS_write_mob_inventory(out, mob);
  }
}

/* Clear only the container stack, NOT cur_mob */
static void RS_stack_clear(struct rs_load_ctx *ctx) {
  int i;
  for (i = 0; i < (int)(sizeof(ctx->stack)/sizeof(ctx->stack[0])); ++i)
    ctx->stack[i] = NULL;
}
