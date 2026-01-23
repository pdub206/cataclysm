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
#include "toml.h"
#include "toml_utils.h"

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

static int toml_get_int_default(toml_table_t *tab, const char *key, int def)
{
  toml_datum_t d = toml_int_in(tab, key);

  if (!d.ok)
    return def;

  return (int)d.u.i;
}

static char *toml_get_string_dup(toml_table_t *tab, const char *key)
{
  toml_datum_t d = toml_string_in(tab, key);

  if (!d.ok)
    return NULL;

  return d.u.s;
}

static void toml_read_int_array(toml_array_t *arr, int *out, int out_count, int def)
{
  int i;

  for (i = 0; i < out_count; i++)
    out[i] = def;

  if (!arr)
    return;

  for (i = 0; i < out_count; i++) {
    toml_datum_t d = toml_int_at(arr, i);
    if (d.ok)
      out[i] = (int)d.u.i;
  }
}

/* Writes one object record to FILE.  Old name: Obj_to_store().
 * Updated to save all NUM_OBJ_VAL_POSITIONS values instead of only 4. */
int objsave_save_obj_record(struct obj_data *obj, FILE *fp, int locate)
{
  int i;
  char buf1[MAX_STRING_LENGTH + 1];
  int out_locate = (locate > 0) ? locate : 0;
  int nest = (locate < 0) ? -locate : 0;

  fprintf(fp, "\n[[object]]\n");
  fprintf(fp, "vnum = %d\n", GET_OBJ_VNUM(obj));
  fprintf(fp, "locate = %d\n", out_locate);
  fprintf(fp, "nest = %d\n", nest);

  fprintf(fp, "values = [");
  for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
    if (i > 0)
      fputs(", ", fp);
    fprintf(fp, "%d", GET_OBJ_VAL(obj, i));
  }
  fprintf(fp, "]\n");

  fprintf(fp, "extra_flags = [%d, %d, %d, %d]\n",
          GET_OBJ_EXTRA(obj)[0], GET_OBJ_EXTRA(obj)[1],
          GET_OBJ_EXTRA(obj)[2], GET_OBJ_EXTRA(obj)[3]);
  fprintf(fp, "wear_flags = [%d, %d, %d, %d]\n",
          GET_OBJ_WEAR(obj)[0], GET_OBJ_WEAR(obj)[1],
          GET_OBJ_WEAR(obj)[2], GET_OBJ_WEAR(obj)[3]);

  toml_write_kv_string_opt(fp, "name", obj->name);
  toml_write_kv_string_opt(fp, "short", obj->short_description);
  toml_write_kv_string_opt(fp, "description", obj->description);
  if (obj->main_description && *obj->main_description) {
    strlcpy(buf1, obj->main_description, sizeof(buf1));
    strip_cr(buf1);
    toml_write_kv_string(fp, "main_description", buf1);
  }

  fprintf(fp, "type = %d\n", GET_OBJ_TYPE(obj));
  fprintf(fp, "weight = %d\n", GET_OBJ_WEIGHT(obj));
  fprintf(fp, "cost = %d\n", GET_OBJ_COST(obj));

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
        if (invalid_class(ch, obj))
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
  FILE *fl;
  char errbuf[256];
  toml_table_t *tab = NULL;
  toml_table_t *header = NULL;
  int savecode;

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, GET_NAME(ch)))
    return FALSE;

  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT)  /* if it fails, NOT because of no file */
      log("SYSERR: checking for crash file %s (3): %s", filename, strerror(errno));
    return FALSE;
  }
  tab = toml_parse_file(fl, errbuf, sizeof(errbuf));
  fclose(fl);

  if (!tab)
    return FALSE;

  header = toml_table_in(tab, "header");
  if (!header) {
    toml_free(tab);
    return FALSE;
  }
  savecode = toml_get_int_default(header, "save_code", SAVE_UNDEF);
  toml_free(tab);

  if (savecode == SAVE_CRASH)
    Crash_delete_file(GET_NAME(ch));

  return TRUE;
}

int Crash_clean_file(char *name)
{
  char filename[MAX_INPUT_LENGTH], filetype[20];
  FILE *fl;
  char errbuf[256];
  toml_table_t *tab = NULL;
  toml_table_t *header = NULL;
  int savecode, timed;

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, name))
    return FALSE;

  /* Open so that permission problems will be flagged now, at boot time. */
  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT)  /* if it fails, NOT because of no file */
      log("SYSERR: OPENING OBJECT FILE %s (4): %s", filename, strerror(errno));
    return FALSE;
  }

  tab = toml_parse_file(fl, errbuf, sizeof(errbuf));
  fclose(fl);
  if (!tab)
    return FALSE;

  header = toml_table_in(tab, "header");
  if (!header) {
    toml_free(tab);
    return FALSE;
  }

  savecode = toml_get_int_default(header, "save_code", SAVE_UNDEF);
  timed = toml_get_int_default(header, "timed", 0);
  toml_free(tab);

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

  fprintf(fp, "[header]\n");
  fprintf(fp, "save_code = %d\n", savecode);
  fprintf(fp, "timed = %d\n", timed);
  fprintf(fp, "net_cost = %d\n", netcost);
  fprintf(fp, "coins = %d\n", coins);
  fprintf(fp, "account = %d\n", account);
  fprintf(fp, "item_count = %d\n", nitems);
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

/* Load all objects from TOML into memory. */
static obj_save_data *objsave_parse_objects_from_toml(toml_table_t *tab, const char *filename)
{
  obj_save_data *head = NULL, *tail = NULL;
  toml_array_t *arr = NULL;
  int count, i;

  arr = toml_array_in(tab, "object");
  if (!arr)
    return NULL;

  count = toml_array_nelem(arr);
  for (i = 0; i < count; i++) {
    toml_table_t *obj_tab = toml_table_at(arr, i);
    struct obj_data *temp = NULL;
    int locate, nest;
    int values[NUM_OBJ_VAL_POSITIONS];
    int flags[4];
    char *str = NULL;

    if (!obj_tab)
      continue;

    {
      int vnum = toml_get_int_default(obj_tab, "vnum", -1);
      if (vnum == -1) {
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
          if (!temp->name)              temp->name = strdup("object");
          if (!temp->short_description) temp->short_description = strdup("an object");
          if (!temp->description)       temp->description = strdup("An object lies here.");
        }
      }
    }

    locate = toml_get_int_default(obj_tab, "locate", 0);
    if (locate < 0 || locate > NUM_WEARS)
      locate = 0;

    nest = toml_get_int_default(obj_tab, "nest", 0);
    if (nest < 0)
      nest = 0;
    if (nest > MAX_BAG_ROWS) {
      if (filename)
        mudlog(NRM, LVL_IMMORT, TRUE,
               "SAVE-LOAD: nest level %d too deep in %s; clamping to %d.",
               nest, filename, MAX_BAG_ROWS);
      else
        mudlog(NRM, LVL_IMMORT, TRUE,
               "SAVE-LOAD: nest level %d too deep; clamping to %d.",
               nest, MAX_BAG_ROWS);
      nest = MAX_BAG_ROWS;
    }

    toml_read_int_array(toml_array_in(obj_tab, "values"), values, NUM_OBJ_VAL_POSITIONS, 0);
    for (int j = 0; j < NUM_OBJ_VAL_POSITIONS; j++)
      GET_OBJ_VAL(temp, j) = values[j];

    toml_read_int_array(toml_array_in(obj_tab, "extra_flags"), flags, 4, 0);
    for (int j = 0; j < 4; j++)
      GET_OBJ_EXTRA(temp)[j] = flags[j];

    toml_read_int_array(toml_array_in(obj_tab, "wear_flags"), flags, 4, 0);
    for (int j = 0; j < 4; j++)
      GET_OBJ_WEAR(temp)[j] = flags[j];

    str = toml_get_string_dup(obj_tab, "name");
    if (str) {
      if (temp->name)
        free(temp->name);
      temp->name = str;
    }

    str = toml_get_string_dup(obj_tab, "short");
    if (str) {
      if (temp->short_description)
        free(temp->short_description);
      temp->short_description = str;
    }

    str = toml_get_string_dup(obj_tab, "description");
    if (str) {
      if (temp->description)
        free(temp->description);
      temp->description = str;
    }

    str = toml_get_string_dup(obj_tab, "main_description");
    if (str) {
      if (temp->main_description)
        free(temp->main_description);
      temp->main_description = str;
    }

    GET_OBJ_TYPE(temp) = toml_get_int_default(obj_tab, "type", GET_OBJ_TYPE(temp));
    GET_OBJ_WEIGHT(temp) = toml_get_int_default(obj_tab, "weight", GET_OBJ_WEIGHT(temp));
    GET_OBJ_COST(temp) = toml_get_int_default(obj_tab, "cost", GET_OBJ_COST(temp));

    if (GET_OBJ_TYPE(temp) == ITEM_MONEY)
      update_money_obj(temp);

    {
      int effective_loc = (nest > 0) ? -nest : locate;
      obj_save_data *node = NULL;
      CREATE(node, obj_save_data, 1);
      node->obj = temp;
      node->locate = effective_loc;
      node->next = NULL;

      if (!head)
        head = node, tail = node;
      else
        tail->next = node, tail = node;
    }
  }

  return head;
}

obj_save_data *objsave_parse_objects(FILE *fl)
{
  char errbuf[256];
  toml_table_t *tab = toml_parse_file(fl, errbuf, sizeof(errbuf));
  obj_save_data *list = NULL;

  if (!tab)
    return NULL;

  list = objsave_parse_objects_from_toml(tab, NULL);
  toml_free(tab);
  return list;
}

static int Crash_load_objs(struct char_data *ch) {
  FILE *fl;
  char filename[PATH_MAX];
  char buf[MAX_STRING_LENGTH];
  int i, orig_save_code, num_objs=0;
  struct obj_data *cont_row[MAX_BAG_ROWS];
  int savecode = SAVE_UNDEF;
  int timed=0,netcost=0,coins=0,account=0,nitems=0;
  char errbuf[256];
  toml_table_t *tab = NULL;
  toml_table_t *header = NULL;
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
 
  tab = toml_parse_file(fl, errbuf, sizeof(errbuf));
  fclose(fl);
  if (!tab) {
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "Failed to parse player's object file %s: %s.", filename, errbuf);
    return 1;
  }

  header = toml_table_in(tab, "header");
  if (!header) {
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "Player object file %s missing header.", filename);
    toml_free(tab);
    return 1;
  }

  savecode = toml_get_int_default(header, "save_code", SAVE_UNDEF);
  timed = toml_get_int_default(header, "timed", 0);
  netcost = toml_get_int_default(header, "net_cost", 0);
  coins = toml_get_int_default(header, "coins", 0);
  account = toml_get_int_default(header, "account", 0);
  nitems = toml_get_int_default(header, "item_count", 0);

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

	loaded = objsave_parse_objects_from_toml(tab, filename);
  toml_free(tab);
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
