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

/* --- helper: read a list of objects until '.' or 'E' and return the head --- */
static struct obj_data *roomsave_read_list(FILE *fl) {
  char line[256];
  struct obj_data *head = NULL, *tail = NULL;

  while (fgets(line, sizeof(line), fl)) {
    if (line[0] == '.' || line[0] == 'E') {
      /* End of this list scope */
      break;
    }

    if (line[0] != 'O')
      continue; /* ignore junk / blank lines */

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
        if (line[0] == 'O' || line[0] == '.' || line[0] == 'E') {
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

    /* Read per-object lines until next 'O' or '.' or 'E' */
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
        struct obj_data *child_head = roomsave_read_list(fl);

        /* CRITICAL FIX: detach each node before obj_to_obj(), otherwise we lose siblings */
        for (struct obj_data *it = child_head, *next; it; it = next) {
          next = it->next_content;  /* remember original sibling */
          it->next_content = NULL;  /* detach from temp list */
          obj_to_obj(it, obj);      /* push into container (LIFO) */
        }
        continue;
      } else if (line[0] == 'O' || line[0] == '.' || line[0] == 'E') {
        /* Next object / end-of-scope: rewind one line for outer loop to see it */
        backpos = -((long)strlen(line));
        fseek(fl, backpos, SEEK_CUR);
        break;
      } else {
        /* ignore unknown lines (e.g., stray '...') */
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

  /* Top-level room contents (write_one_object handles recursion into containers) */
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

void RoomSave_boot(void) {
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
    if (n < 5) continue; /* skip . .. */
    if (strcmp(dp->d_name + n - 4, ROOMSAVE_EXT) != 0) continue;

    char path[PATH_MAX];
    int wn = snprintf(path, sizeof(path), "%s%s", ROOMSAVE_PREFIX, dp->d_name);
    if (wn < 0 || wn >= (int)sizeof(path)) {
      mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: RoomSave_boot: path too long: %s%s", ROOMSAVE_PREFIX, dp->d_name);
      continue;
    }

    FILE *fl = fopen(path, "r");
    if (!fl) {
      mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: RoomSave_boot: fopen(%s) failed: %s", path, strerror(errno));
      continue;
    }

    log("RoomSave: reading %s", path);

    char line[512];
    int blocks = 0, restored_total = 0;

    while (fgets(line, sizeof(line), fl)) {
      if (!strncmp(line, "#R ", 3)) {
        int rvnum; long ts;
        if (sscanf(line, "#R %d %ld", &rvnum, &ts) != 2) {
          mudlog(NRM, LVL_IMMORT, TRUE, "RoomSave: malformed #R header in %s: %s", path, line);
          /* skip to next block end */
          while (fgets(line, sizeof(line), fl)) if (line[0] == '.') break;
          continue;
        }

        room_rnum rnum = real_room((room_vnum)rvnum);
        blocks++;

        if (rnum == NOWHERE) {
          mudlog(NRM, LVL_IMMORT, FALSE, "RoomSave: unknown room vnum %d in %s (skipping)", rvnum, path);
          while (fgets(line, sizeof(line), fl)) if (line[0] == '.') break;
          continue;
        }

        /* Clear this room's current contents */
        while (world[rnum].contents)
          extract_obj(world[rnum].contents);

        /* Read list between this #R and '.' */
        struct obj_data *list = roomsave_read_list(fl);
        int count = 0;

        for (struct obj_data *it = list; it; ) {
          struct obj_data *next = it->next_content;
          it->next_content = NULL;
          obj_to_room(it, rnum);
          count++;
          it = next;
        }

        restored_total += count;
        log("RoomSave: room %d <- %d object(s)", rvnum, count);
      }
    }

    log("RoomSave: finished %s (blocks=%d, restored=%d)", path, blocks, restored_total);
    fclose(fl);
  }

  closedir(dirp);
}

/* Save all rooms flagged ROOM_SAVE. Called from point_update() on a cadence. */
void RoomSave_autosave_tick(void) {
  for (room_rnum rnum = 0; rnum <= top_of_world; rnum++) {
    if (ROOM_FLAGGED(rnum, ROOM_SAVE)) {
      RoomSave_now(rnum);
    }
  }
}