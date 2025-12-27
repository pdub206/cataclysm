/**************************************************************************
*  File: objsave.c                                         Part of tbaMUD *
*  Usage: loading/saving player objects for crash-save and login restore  *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "spells.h"
#include "act.h"
#include "class.h"
#include "config.h"
#include "modify.h"
#include "genolc.h" /* for strip_cr and sprintascii */

/* these factors should be unique integers */
#define CRYO_FACTOR    4

#define LOC_INVENTORY  0
#define MAX_BAG_ROWS   5

/* local functions */
static int Crash_save(struct obj_data *obj, FILE *fp, int location);
static void auto_equip(struct char_data *ch, struct obj_data *obj, int location);
static void Crash_restore_weight(struct obj_data *obj);
static int Crash_load_objs(struct char_data *ch);
static int handle_obj(struct obj_data *obj, struct char_data *ch, int locate, struct obj_data **cont_rows);
static void Crash_write_header(struct char_data *ch, FILE *fp, int savecode);

/* Writes one object record to FILE.  Old name: Obj_to_store().
 * Updated to save all NUM_OBJ_VAL_POSITIONS values instead of only 4. */
int objsave_save_obj_record(struct obj_data *obj, FILE *fp, int locate)
{
  int i;
  char buf1[MAX_STRING_LENGTH + 1];
  struct obj_data *temp = NULL;

  /* Build a prototype baseline to diff against so we only emit changed fields */
  if (GET_OBJ_VNUM(obj) != NOTHING)
    temp = read_object(GET_OBJ_VNUM(obj), VIRTUAL);
  else {
    temp = create_obj();
    temp->item_number = NOWHERE;
  }

  if (obj->main_description) {
    strcpy(buf1, obj->main_description);
    strip_cr(buf1);
  } else
    *buf1 = 0;

  /* Header and placement */
  fprintf(fp, "#%d\n", GET_OBJ_VNUM(obj));

  /* Top-level worn slots are positive (1..NUM_WEARS); inventory is 0.
   * Children use negative numbers from Crash_save recursion (…,-1,-2,…) — we map that to Nest. */
  if (locate > 0)
    fprintf(fp, "Loc : %d\n", locate);

  if (locate < 0) {
    int nest = -locate;            /* e.g. -1 => Nest:1, -2 => Nest:2, etc. */
    fprintf(fp, "Nest: %d\n", nest);
  } else {
    fprintf(fp, "Nest: %d\n", 0);  /* top-level object (inventory or worn) */
  }

  /* Save all object values (diffed against proto) */
  {
    bool diff = FALSE;
    for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
      if (GET_OBJ_VAL(obj, i) != GET_OBJ_VAL(temp, i)) {
        diff = TRUE;
        break;
      }
    }
    if (diff) {
      fprintf(fp, "Vals:");
      for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
        fprintf(fp, " %d", GET_OBJ_VAL(obj, i));
      fprintf(fp, "\n");
    }
  }

  /* Extra flags (array words) */
  if (GET_OBJ_EXTRA(obj) != GET_OBJ_EXTRA(temp))
    fprintf(fp, "Flag: %d %d %d %d\n",
            GET_OBJ_EXTRA(obj)[0], GET_OBJ_EXTRA(obj)[1],
            GET_OBJ_EXTRA(obj)[2], GET_OBJ_EXTRA(obj)[3]);

  /* Names/descriptions */
  if (obj->name && (!temp->name || strcmp(obj->name, temp->name)))
    fprintf(fp, "Name: %s\n", obj->name);
  if (obj->short_description && (!temp->short_description ||
      strcmp(obj->short_description, temp->short_description)))
    fprintf(fp, "Shrt: %s\n", obj->short_description);
  if (obj->description && (!temp->description ||
      strcmp(obj->description, temp->description)))
    fprintf(fp, "Desc: %s\n", obj->description);
  if (obj->main_description && (!temp->main_description ||
      strcmp(obj->main_description, temp->main_description)))
    fprintf(fp, "ADes:\n%s~\n", buf1);

  /* Core fields */
  if (GET_OBJ_TYPE(obj) != GET_OBJ_TYPE(temp))
    fprintf(fp, "Type: %d\n", GET_OBJ_TYPE(obj));
  if (GET_OBJ_WEIGHT(obj) != GET_OBJ_WEIGHT(temp))
    fprintf(fp, "Wght: %d\n", GET_OBJ_WEIGHT(obj));
  if (GET_OBJ_COST(obj) != GET_OBJ_COST(temp))
    fprintf(fp, "Cost: %d\n", GET_OBJ_COST(obj));

  /* Permanent affects (array words) */
  if (GET_OBJ_AFFECT(obj)[0] != GET_OBJ_AFFECT(temp)[0] ||
      GET_OBJ_AFFECT(obj)[1] != GET_OBJ_AFFECT(temp)[1] ||
      GET_OBJ_AFFECT(obj)[2] != GET_OBJ_AFFECT(temp)[2] ||
      GET_OBJ_AFFECT(obj)[3] != GET_OBJ_AFFECT(temp)[3])
    fprintf(fp, "Perm: %d %d %d %d\n",
            GET_OBJ_AFFECT(obj)[0], GET_OBJ_AFFECT(obj)[1],
            GET_OBJ_AFFECT(obj)[2], GET_OBJ_AFFECT(obj)[3]);

  /* Wear flags (array words) */
  if (GET_OBJ_WEAR(obj)[0] != GET_OBJ_WEAR(temp)[0] ||
      GET_OBJ_WEAR(obj)[1] != GET_OBJ_WEAR(temp)[1] ||
      GET_OBJ_WEAR(obj)[2] != GET_OBJ_WEAR(temp)[2] ||
      GET_OBJ_WEAR(obj)[3] != GET_OBJ_WEAR(temp)[3])
    fprintf(fp, "Wear: %d %d %d %d\n",
            GET_OBJ_WEAR(obj)[0], GET_OBJ_WEAR(obj)[1],
            GET_OBJ_WEAR(obj)[2], GET_OBJ_WEAR(obj)[3]);

  /* (If you also persist applies, extra descs, scripts, etc., keep that code here unchanged) */

  return 1;
}

#undef TEST_OBJS
#undef TEST_OBJN

/* AutoEQ by Burkhard Knopf. */
static void auto_equip(struct char_data *ch, struct obj_data *obj, int location)
{
  int j;

  /* Lots of checks... */
  if (location > 0) {  /* Was wearing it. */
    switch (j = (location - 1)) {
    case WEAR_LIGHT:
      break;
    case WEAR_FINGER_R:
    case WEAR_FINGER_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_FINGER)) /* not fitting :( */
        location = LOC_INVENTORY;
      break;
    case WEAR_NECK_1:
    case WEAR_NECK_2:
      if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
        location = LOC_INVENTORY;
      break;
    case WEAR_BACK:
      if (!CAN_WEAR(obj, ITEM_WEAR_BACK))
        location = LOC_INVENTORY;
      break;
    case WEAR_BODY:
      if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
        location = LOC_INVENTORY;
      break;
    case WEAR_HEAD:
      if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
        location = LOC_INVENTORY;
      break;
    case WEAR_LEGS:
      if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
        location = LOC_INVENTORY;
      break;
    case WEAR_FEET:
      if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
        location = LOC_INVENTORY;
      break;
    case WEAR_HANDS:
      if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
        location = LOC_INVENTORY;
      break;
    case WEAR_ARMS:
      if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
        location = LOC_INVENTORY;
      break;
    case WEAR_SHIELD:
      if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
        location = LOC_INVENTORY;
      break;
    case WEAR_ABOUT:
      if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
        location = LOC_INVENTORY;
      break;
    case WEAR_WAIST:
      if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
        location = LOC_INVENTORY;
      break;
    case WEAR_WRIST_R:
    case WEAR_WRIST_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
        location = LOC_INVENTORY;
      break;
    case WEAR_WIELD:
      if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
        location = LOC_INVENTORY;
      break;
    case WEAR_HOLD:
      if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
        break;
      if (IS_FIGHTER(ch) && CAN_WEAR(obj, ITEM_WEAR_WIELD) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        break;
      location = LOC_INVENTORY;
      break;
    default:
      location = LOC_INVENTORY;
    }

    if (location > 0) {      /* Wearable. */
      if (!GET_EQ(ch,j)) {
        /* Check the characters's alignment to prevent them from being zapped
	 * through the auto-equipping. */
        if (invalid_align(ch, obj) || invalid_class(ch, obj))
          location = LOC_INVENTORY;
        else
          equip_char(ch, obj, j);
      } else {  /* Oops, saved a player with double equipment? */
        mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
               "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch), location);
        location = LOC_INVENTORY;
      }
    }
  }
  if (location <= 0)  /* Inventory */
    obj_to_char(obj, ch);
}

int Crash_delete_file(char *name)
{
  char filename[MAX_INPUT_LENGTH];
  FILE *fl;

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, name))
    return FALSE;

  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT)  /* if it fails but NOT because of no file */
      log("SYSERR: deleting crash file %s (1): %s", filename, strerror(errno));
    return FALSE;
  }
  fclose(fl);

  /* if it fails, NOT because of no file */
  if (remove(filename) < 0 && errno != ENOENT)
    log("SYSERR: deleting crash file %s (2): %s", filename, strerror(errno));

  return TRUE;
}

int Crash_delete_crashfile(struct char_data *ch)
{
  char filename[MAX_INPUT_LENGTH];
  int numread;
  FILE *fl;
  int savecode;
  char line[READ_SIZE];

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, GET_NAME(ch)))
    return FALSE;

  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT)  /* if it fails, NOT because of no file */
      log("SYSERR: checking for crash file %s (3): %s", filename, strerror(errno));
    return FALSE;
  }
  numread = get_line(fl,line);
  fclose(fl);

  if (numread == FALSE)
    return FALSE;
  sscanf(line,"%d ",&savecode);

  if (savecode == SAVE_CRASH)
    Crash_delete_file(GET_NAME(ch));

  return TRUE;
}

int Crash_clean_file(char *name)
{
  char filename[MAX_INPUT_LENGTH], filetype[20];
  int numread;
  FILE *fl;
  int savecode, timed, netcost, coins, account, nitems;
  char line[READ_SIZE];

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, name))
    return FALSE;

  /* Open so that permission problems will be flagged now, at boot time. */
  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT)  /* if it fails, NOT because of no file */
      log("SYSERR: OPENING OBJECT FILE %s (4): %s", filename, strerror(errno));
    return FALSE;
  }

  numread = get_line(fl,line);
  fclose(fl);
  if (numread == FALSE)
    return FALSE;

  sscanf(line, "%d %d %d %d %d %d",&savecode,&timed,&netcost,
         &coins,&account,&nitems);

  if ((savecode == SAVE_CRASH) ||
      (savecode == SAVE_LOGOUT) ||
      (savecode == SAVE_FORCED) ||
      (savecode == SAVE_TIMEDOUT) ) {
    if (timed < time(0) - (CONFIG_CRASH_TIMEOUT * SECS_PER_REAL_DAY)) {
      Crash_delete_file(name);
      switch (savecode) {
      case SAVE_CRASH:
        strcpy(filetype, "crash");
        break;
      case SAVE_LOGOUT:
        strcpy(filetype, "legacy save");
        break;
      case SAVE_FORCED:
        strcpy(filetype, "idle save");
        break;
      case SAVE_TIMEDOUT:
        strcpy(filetype, "idle save");
        break;
      default:
        strcpy(filetype, "UNKNOWN!");
        break;
      }
      log("    Deleting %s's %s file.", name, filetype);
      return TRUE;
    }
  }
  return FALSE;
}

void update_obj_file(void)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (*player_table[i].name)
      Crash_clean_file(player_table[i].name);
}

/* Return values:
 *  0 - successful load, keep char in load room.
 *  1 - load failure or load of crash items -- put char in temple. */
int Crash_load(struct char_data *ch)
{
  return (Crash_load_objs(ch));
}

static int Crash_save(struct obj_data *obj, FILE *fp, int location)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    Crash_save(obj->next_content, fp, location);
    Crash_save(obj->contains, fp, MIN(0, location) - 1);

    result = objsave_save_obj_record(obj, fp, location);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    if (!result)
      return FALSE;
  }
  return (TRUE);
}

static void Crash_restore_weight(struct obj_data *obj)
{
  if (obj) {
    Crash_restore_weight(obj->contains);
    Crash_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}

static void Crash_write_header(struct char_data *ch, FILE *fp, int savecode)
{
  int timed = (int)time(0);
  int netcost = 0;
  int coins = ch ? GET_COINS(ch) : 0;
  int account = ch ? GET_BANK_COINS(ch) : 0;
  int nitems = 0;

  fprintf(fp, "%d %d %d %d %d %d\n", savecode, timed, netcost, coins, account, nitems);
}

void Crash_crashsave(struct char_data *ch)
{
  char buf[MAX_INPUT_LENGTH];
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(buf, sizeof(buf), CRASH_FILE, GET_NAME(ch)))
    return;

  if (!(fp = fopen(buf, "w")))
    return;

  Crash_write_header(ch, fp, SAVE_CRASH);

  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch, j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
    }

  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  Crash_restore_weight(ch->carrying);

  fprintf(fp, "$~\n");
  fclose(fp);
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);
}

/* Shortened because we don't use storage fees in this game */
void Crash_idlesave(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return;

  char buf[MAX_INPUT_LENGTH];
  int j;
  FILE *fp;

  if (!get_filename(buf, sizeof(buf), CRASH_FILE, GET_NAME(ch)))
    return;

  if (!(fp = fopen(buf, "w")))
    return;

  Crash_write_header(ch, fp, SAVE_FORCED);

  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch, j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
    }

  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  Crash_restore_weight(ch->carrying);

  fprintf(fp, "$~\n");
  fclose(fp);
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);
}

/* Shortened because we don't use storage fees in this game */
void Crash_rentsave(struct char_data *ch, int cost)
{
  if (!ch || IS_NPC(ch))
    return;

  char buf[MAX_INPUT_LENGTH];
  int j;
  FILE *fp;

  if (!get_filename(buf, sizeof(buf), CRASH_FILE, GET_NAME(ch)))
    return;

  if (!(fp = fopen(buf, "w")))
    return;

  Crash_write_header(ch, fp, SAVE_LOGOUT);

  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch, j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
    }

  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  Crash_restore_weight(ch->carrying);

  fprintf(fp, "$~\n");
  fclose(fp);
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);
}

void Crash_save_all(void)
{
  struct descriptor_data *d;

  /* Respect config: if autosave is off, do nothing. */
  if (!CONFIG_AUTO_SAVE)
    return;

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING)
      continue;
    if (!d->character || IS_NPC(d->character))
      continue;

    /* Skip characters not fully placed yet (prevents clobbering Room to 65535). */
    if (IN_ROOM(d->character) == NOWHERE)
      continue;

    /* Optional hardening: if spawn vnum is not yet established, skip this tick. */
    if (GET_LOADROOM(d->character) == NOWHERE)
      continue;

    /* IMPORTANT: Do NOT modify GET_LOADROOM here.
       Autosave should not change the player's spawn point. */

    /* Persist character and object file. */
    save_char(d->character);
    Crash_crashsave(d->character);

    if (PLR_FLAGGED(d->character, PLR_CRASH))
      REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRASH);
  }
}

/* Load all objects from file into memory. Updated to load NUM_OBJ_VAL_POSITIONS values. */
obj_save_data *objsave_parse_objects(FILE *fl)
{
  char line[MAX_STRING_LENGTH];

  obj_save_data *head = NULL, *tail = NULL;

  /* State for the object we’re currently assembling */
  struct obj_data *temp = NULL;
  int pending_locate = 0;  /* 0 = inventory, 1..NUM_WEARS = worn slot */
  int pending_nest   = 0;  /* 0 = top-level; >0 = inside container at level-1 */

  /* --- helpers (GCC nested functions OK in tbaMUD build) ---------------- */

  /* append current object to the result list with proper locate */
  void commit_current(void) {
    if (!temp) return;

    /* sanitize top-level locate range only; children will be negative later */
    int loc = pending_locate;
    if (pending_nest <= 0) {
      if (loc < 0 || loc > NUM_WEARS) {
        mudlog(NRM, LVL_IMMORT, TRUE,
               "SAVE-LOAD: bad locate %d for vnum %d; defaulting to inventory.",
               loc, GET_OBJ_VNUM(temp));
        loc = 0;
      }
    }

    /* convert Nest>0 into negative locate for handle_obj()/cont_row */
    int effective_loc = (pending_nest > 0) ? -pending_nest : loc;

    obj_save_data *node = NULL;
    CREATE(node, obj_save_data, 1);
    node->obj    = temp;
    node->locate = effective_loc;
    node->next   = NULL;

    if (!head) head = node, tail = node;
    else tail->next = node, tail = node;

    temp = NULL;
    pending_locate = 0;
    pending_nest   = 0;
  }

  /* split a line into normalized tag (no colon) and payload pointer */
  void split_tag_line(const char *src, char tag_out[6], const char **payload_out) {
    const char *s = src;

    while (*s && isspace((unsigned char)*s)) s++;        /* skip leading ws */

    const char *te = s;
    while (*te && !isspace((unsigned char)*te) && *te != ':') te++;

    size_t tlen = (size_t)(te - s);
    if (tlen > 5) tlen = 5;
    memcpy(tag_out, s, tlen);
    tag_out[tlen] = '\0';

    const char *p = te;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == ':') {
      p++;
      while (*p && isspace((unsigned char)*p)) p++;
    }
    *payload_out = p;
  }

  /* ---------------------------------------------------------------------- */

  while (get_line(fl, line)) {
    if (!*line) continue;

    /* New object header: "#<vnum>" (commit any previous one first) */
    if (*line == '#') {
      if (temp) commit_current();

      long vnum = -1L;
      vnum = strtol(line + 1, NULL, 10);

      if (vnum <= 0 && vnum != -1L) {
        mudlog(NRM, LVL_IMMORT, TRUE, "SAVE-LOAD: bad vnum header: '%s'", line);
        temp = NULL;
        pending_locate = 0;
        pending_nest   = 0;
        continue;
      }

      /* Instantiate from prototype if available, else create a blank */
      if (vnum == -1L) {
        temp = create_obj();
        temp->item_number = NOTHING;
        if (!temp->name)              temp->name = strdup("object");
        if (!temp->short_description) temp->short_description = strdup("an object");
        if (!temp->description)       temp->description = strdup("An object lies here.");
      } else {
        int rnum = real_object((obj_vnum)vnum);
        if (rnum >= 0) {
          temp = read_object(rnum, REAL);
        } else {
          temp = create_obj();
          /* Do NOT assign GET_OBJ_VNUM(temp); item_number derives vnum. */
          if (!temp->name)              temp->name = strdup("object");
          if (!temp->short_description) temp->short_description = strdup("an object");
          if (!temp->description)       temp->description = strdup("An object lies here.");
        }
      }

      pending_locate = 0;
      pending_nest   = 0;
      continue;
    }

    /* Normal data line: TAG [ : ] payload */
    char tag[6];
    const char *payload = NULL;
    split_tag_line(line, tag, &payload);

    if (!*tag) continue;
    if (!temp) {
      mudlog(NRM, LVL_IMMORT, TRUE, "SAVE-LOAD: data before header ignored: '%s'", line);
      continue;
    }

    if (!strcmp(tag, "Loc")) {
      pending_locate = (int)strtol(payload, NULL, 10);
    }
    else if (!strcmp(tag, "Nest")) {
      pending_nest = (int)strtol(payload, NULL, 10);
      if (pending_nest < 0) pending_nest = 0;
      if (pending_nest > MAX_BAG_ROWS) {
        mudlog(NRM, LVL_IMMORT, TRUE,
               "SAVE-LOAD: nest level %d too deep; clamping to %d.",
               pending_nest, MAX_BAG_ROWS);
        pending_nest = MAX_BAG_ROWS;
      }
    }
    else if (!strcmp(tag, "Vals")) {
      const char *p = payload;
      for (int i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
        if (!*p) { GET_OBJ_VAL(temp, i) = 0; continue; }
        GET_OBJ_VAL(temp, i) = (int)strtol(p, (char **)&p, 10);
      }
    }
    else if (!strcmp(tag, "Wght")) {
      GET_OBJ_WEIGHT(temp) = (int)strtol(payload, NULL, 10);
    }
    else if (!strcmp(tag, "Cost")) {
      GET_OBJ_COST(temp) = (int)strtol(payload, NULL, 10);
    }
    else if (!strcmp(tag, "Rent")) {
      /* Legacy tag ignored (cost-per-day no longer used). */
    }
    else if (!strcmp(tag, "Type")) {
      GET_OBJ_TYPE(temp) = (int)strtol(payload, NULL, 10);
    }
    else if (!strcmp(tag, "Wear")) {
      unsigned long words[4] = {0,0,0,0};
      const char *p = payload;
      for (int i = 0; i < 4 && *p; i++) words[i] = strtoul(p, (char **)&p, 10);

#if defined(TW_ARRAY_MAX) && defined(GET_OBJ_WEAR_AR)
      for (int i = 0; i < 4; i++) {
        if (i < TW_ARRAY_MAX) GET_OBJ_WEAR_AR(temp, i) = (bitvector_t)words[i];
        else if (words[i])
          mudlog(NRM, LVL_IMMORT, TRUE,
                 "SAVE-LOAD: Wear word %d (%lu) truncated (TW_ARRAY_MAX=%d).",
                 i, words[i], TW_ARRAY_MAX);
      }
#elif defined(GET_OBJ_WEAR_AR)
      for (int i = 0; i < 4; i++) GET_OBJ_WEAR_AR(temp, i) = (bitvector_t)words[i];
#endif
    }
    else if (!strcmp(tag, "Flag")) {
      unsigned long words[4] = {0,0,0,0};
      const char *p = payload;
      for (int i = 0; i < 4 && *p; i++) words[i] = strtoul(p, (char **)&p, 10);

#if defined(EF_ARRAY_MAX) && defined(GET_OBJ_EXTRA_AR)
      for (int i = 0; i < 4; i++) {
        if (i < EF_ARRAY_MAX) GET_OBJ_EXTRA_AR(temp, i) = (bitvector_t)words[i];
        else if (words[i])
          mudlog(NRM, LVL_IMMORT, TRUE,
                 "SAVE-LOAD: Extra word %d (%lu) truncated (EF_ARRAY_MAX=%d).",
                 i, words[i], EF_ARRAY_MAX);
      }
#elif defined(GET_OBJ_EXTRA_AR)
      for (int i = 0; i < 4; i++) GET_OBJ_EXTRA_AR(temp, i) = (bitvector_t)words[i];
#endif
    }
    else if (!strcmp(tag, "Name")) {
      if (temp->name) free(temp->name);
      temp->name = *payload ? strdup(payload) : strdup("object");
    }
    else if (!strcmp(tag, "Shrt")) {
      if (temp->short_description) free(temp->short_description);
      temp->short_description = *payload ? strdup(payload) : strdup("an object");
    }
    else if (!strcmp(tag, "Desc")) {
      if (temp->description) free(temp->description);
      temp->description = *payload ? strdup(payload) : strdup("An object lies here.");
    }
    else if (!strcmp(tag, "ADes")) {
      if (temp->main_description) free(temp->main_description);
      temp->main_description = *payload ? strdup(payload) : NULL;
    }
    else if (!strcmp(tag, "End")) {
      commit_current();
    }
    else {
      mudlog(NRM, LVL_IMMORT, TRUE, "SAVE-LOAD: unknown tag '%s'", tag);
    }
  }

  if (temp) commit_current();

  return head;
}

static int Crash_load_objs(struct char_data *ch) {
  FILE *fl;
  char filename[PATH_MAX];
  char line[READ_SIZE];
  char buf[MAX_STRING_LENGTH];
  int i, orig_save_code, num_objs=0;
  struct obj_data *cont_row[MAX_BAG_ROWS];
  int savecode = SAVE_UNDEF;
  int timed=0,netcost=0,coins,account,nitems;
	obj_save_data *loaded, *current;

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, GET_NAME(ch)))
    return 1;

  for (i = 0; i < MAX_BAG_ROWS; i++)
    cont_row[i] = NULL;

  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      snprintf(buf, MAX_STRING_LENGTH, "SYSERR: READING OBJECT FILE %s (5)", filename);
      perror(buf);
      send_to_char(ch, "\r\n********************* NOTICE *********************\r\n"
                       "There was a problem loading your objects from disk.\r\n"
                       "Contact a God for assistance.\r\n");
    }
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s entering game with no equipment.", GET_NAME(ch));
    if (GET_COINS(ch) > 0) {
      int old_coins = GET_COINS(ch);
      GET_COINS(ch) = 0;
      add_coins_to_char(ch, old_coins);
    }
    return 1;
  }
 
  if (!get_line(fl, line))
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "Failed to read player's save code: %s.", GET_NAME(ch));
  else
    sscanf(line,"%d %d %d %d %d %d",&savecode, &timed, &netcost,&coins,&account,&nitems);

  if (savecode == SAVE_LOGOUT || savecode == SAVE_TIMEDOUT) {
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s entering game with legacy save code; no fees applied.", GET_NAME(ch));
  }
  switch (orig_save_code = savecode) {
  case SAVE_LOGOUT:
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s restoring saved items and entering game.", GET_NAME(ch));
    break;
  case SAVE_CRASH:

    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    break;
  case SAVE_CRYO:
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s restoring cryo-saved items and entering game.", GET_NAME(ch));
    break;
  case SAVE_FORCED:
  case SAVE_TIMEDOUT:
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s retrieving idle-saved items and entering game.", GET_NAME(ch));
    break;
  default:
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "WARNING: %s entering game with undefined save code.", GET_NAME(ch));
    break;
  }

	loaded = objsave_parse_objects(fl);
	for (current = loaded; current != NULL; current=current->next)
	  num_objs += handle_obj(current->obj, ch, current->locate, cont_row);

	/* now it's safe to free the obj_save_data list - all members of it
	 * have been put in the correct lists by handle_obj() */
	while (loaded != NULL) {
		current = loaded;
		loaded = loaded->next;
		free(current);
	}

  {
    int old_coins = GET_COINS(ch);
    int coin_total = count_char_coins(ch);

    if (coin_total > 0) {
      GET_COINS(ch) = coin_total;
    } else if (old_coins > 0) {
      GET_COINS(ch) = 0;
      add_coins_to_char(ch, old_coins);
    } else {
      GET_COINS(ch) = 0;
    }
  }

  /* Little hoarding check. -gg 3/1/98 */
  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s (level %d) has %d object%s.",
         GET_NAME(ch), GET_LEVEL(ch), num_objs, num_objs != 1 ? "s" : "");

  fclose(fl);

  if ((orig_save_code == SAVE_LOGOUT) || (orig_save_code == SAVE_CRYO))
    return 0;
  else
    return 1;
}

static int handle_obj(struct obj_data *temp, struct char_data *ch, int locate, struct obj_data **cont_row)
{
  int j;
  struct obj_data *obj1;

  if (!temp)  /* this should never happen, but.... */
    return FALSE;

  auto_equip(ch, temp, locate);

  /* What to do with a new loaded item:
   * If there's a list with <locate> less than 1 below this: (equipped items
   * are assumed to have <locate>==0 here) then its container has disappeared
   * from the file   *gasp* -> put all the list back to ch's inventory if
   * there's a list of contents with <locate> 1 below this: check if it's a
   * container - if so: get it from ch, fill it, and give it back to ch (this
   * way the container has its correct weight before modifying ch) - if not:
   * the container is missing -> put all the list to ch's inventory. For items
   * with negative <locate>: If there's already a list of contents with the
   * same <locate> put obj to it if not, start a new list. Since <locate> for
   * contents is < 0 the list indices are switched to non-negative. */
  if (locate > 0) { /* item equipped */

    for (j = MAX_BAG_ROWS-1;j > 0;j--)
      if (cont_row[j]) { /* no container -> back to ch's inventory */
        for (;cont_row[j];cont_row[j] = obj1) {
          obj1 = cont_row[j]->next_content;
          obj_to_char(cont_row[j], ch);
        }
        cont_row[j] = NULL;
      }
    if (cont_row[0]) { /* content list existing */
      if (obj_is_storage(temp)) {
        /* rem item ; fill ; equip again */
        temp = unequip_char(ch, locate-1);
        temp->contains = NULL; /* should be empty - but who knows */
        for (;cont_row[0];cont_row[0] = obj1) {
          obj1 = cont_row[0]->next_content;
          obj_to_obj(cont_row[0], temp);
        }
        equip_char(ch, temp, locate-1);
      } else { /* object isn't container -> empty content list */
        for (;cont_row[0];cont_row[0] = obj1) {
          obj1 = cont_row[0]->next_content;
          obj_to_char(cont_row[0], ch);
        }
        cont_row[0] = NULL;
      }
    }
  } else { /* locate <= 0 */
    for (j = MAX_BAG_ROWS-1;j > -locate;j--)
      if (cont_row[j]) { /* no container -> back to ch's inventory */
        for (;cont_row[j];cont_row[j] = obj1) {
          obj1 = cont_row[j]->next_content;
          obj_to_char(cont_row[j], ch);
        }
        cont_row[j] = NULL;
      }

    if (j == -locate && cont_row[j]) { /* content list existing */
      if (obj_is_storage(temp)) {
        /* take item ; fill ; give to char again */
        obj_from_char(temp);
        temp->contains = NULL;
        for (;cont_row[j];cont_row[j] = obj1) {
          obj1 = cont_row[j]->next_content;
          obj_to_obj(cont_row[j], temp);
        }
        obj_to_char(temp, ch); /* add to inv first ... */
      } else { /* object isn't container -> empty content list */
        for (;cont_row[j];cont_row[j] = obj1) {
          obj1 = cont_row[j]->next_content;
          obj_to_char(cont_row[j], ch);
        }
        cont_row[j] = NULL;
      }
    }

    if (locate < 0 && locate >= -MAX_BAG_ROWS) {
      /* let obj be part of content list
         but put it at the list's end thus having the items
         in the same order as before saving */
      obj_from_char(temp);
      if ((obj1 = cont_row[-locate-1])) {
        while (obj1->next_content)
          obj1 = obj1->next_content;
        obj1->next_content = temp;
      } else
        cont_row[-locate-1] = temp;
    }
  } /* locate less than zero */

  return TRUE;
}
