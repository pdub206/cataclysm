/**
* @file set.c
* Builder room/object creation and utility functions.
* 
* This set of code was not originally part of the circlemud distribution.
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
#include "comm.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "boards.h"
#include "genolc.h"
#include "genwld.h"
#include "genzon.h"
#include "oasis.h"
#include "improved-edit.h"
#include "modify.h"
#include "genobj.h"
#include "genmob.h"
#include "dg_scripts.h"
#include "fight.h"

#include "set.h"

static void rset_show_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Usage:\r\n"
    "  rset show\r\n"
    "  rset add name <text>\r\n"
    "  rset add sector <sector>\r\n"
    "  rset add flags <flag> [flag ...]\r\n"
    "  rset add exit <direction> <room number>\r\n"
    "  rset add door <direction> <name of door>\r\n"
    "  rset add key <direction> <key number>\r\n"
    "  rset add hidden <direction>\r\n"
    "  rset add forage <object vnum> <dc check>\r\n"
    "  rset add edesc <keyword> <description>\r\n"
    "  rset add desc\r\n"
    "  rset del <field>\r\n"
    "  rset clear force\r\n"
    "  rset validate\r\n");
}

static void rset_show_set_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds basic configuration to the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add name <text>\r\n"
    "  rset add sector <sector>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add name \"A wind-scoured alley\"\r\n"
    "  rset add sector desert\r\n");
}

static void rset_show_set_sector_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds room sector type.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add sector <sector>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add sector desert\r\n"
    "\r\n"
    "Sectors:\r\n");
  column_list(ch, 0, sector_types, NUM_ROOM_SECTORS, FALSE);
}

static void rset_show_add_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds specific configuration to the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add name <text>\r\n"
    "  rset add sector <sector>\r\n"
    "  rset add flags <flag> [flag ...]\r\n"
    "  rset add exit <direction> <room number>\r\n"
    "  rset add door <direction> <name of door>\r\n"
    "  rset add key <direction> <key number>\r\n"
    "  rset add hidden <direction>\r\n"
    "  rset add forage <object vnum> <dc check>\r\n"
    "  rset add edesc <keyword> <description>\r\n"
    "  rset add desc\r\n");
}

static void rset_show_del_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes specific configuration from the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del flags <flag> [flag ...]\r\n"
    "  rset del exit <direction>\r\n"
    "  rset del door <direction>\r\n"
    "  rset del key <direction>\r\n"
    "  rset del hidden <direction>\r\n"
    "  rset del forage <object vnum>\r\n"
    "  rset del edesc <keyword>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del flags INDOORS QUITSAFE\r\n"
    "  rset del exit n\r\n"
    "  rset del door n\r\n"
    "  rset del key n\r\n"
    "  rset del hidden n\r\n"
    "  rset del forage 301\r\n"
    "  rset del edesc mosaic\r\n");
}

static void rset_show_validate_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Verifies your new room meets building standards and looks for any errors. If\r\n"
    "correct, you can use the rsave command to finish building.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset validate\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset validate\r\n");
}

static void rset_show_desc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Enters text editor for editing the main description of the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add desc\r\n");
}

static void rset_show_rcreate_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Creates a new unfinished room which can be entered and configured.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rcreate <vnum>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rcreate 1001\r\n");
}

static void rset_show_add_flags_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds room flags.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add flags <flag> [flag ...]\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add flags INDOORS QUITSAFE\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, room_bits, NUM_ROOM_FLAGS, FALSE);
}

static void rset_show_del_flags_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes room flags.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del flags <flag> [flag ...]\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del flags INDOORS QUITSAFE\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, room_bits, NUM_ROOM_FLAGS, FALSE);
}

static void rset_show_add_exit_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds an exit to the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add exit <direction> <room number>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add exit n 101\r\n");
}

static void rset_show_add_door_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a door to an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add door <direction> <name of door>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add door n door\r\n");
}

static void rset_show_add_key_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a key to an existing door.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add key <direction> <key number>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add key n 201\r\n");
}

static void rset_show_add_hidden_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds the hidden flag to an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add hidden <direction>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add hidden n\r\n");
}

static void rset_show_add_forage_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a forage entry to the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add forage <object vnum> <dc check>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add forage 301 15\r\n");
}

static void rset_show_add_edesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds an extra description to the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset add edesc <keyword> <description>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add edesc mosaic A beautiful mosaic is here on the wall.\r\n");
}

static void rset_show_del_exit_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes an exit from the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del exit <direction>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del exit n\r\n");
}

static void rset_show_del_door_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes a door from an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del door <direction>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del door n\r\n");
}

static void rset_show_del_key_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes a key from an existing door.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del key <direction>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del key n\r\n");
}

static void rset_show_del_hidden_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes the hidden flag from an existing exit.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del hidden <direction>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del hidden n\r\n");
}

static void rset_show_del_forage_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes a forage entry from the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del forage <object vnum>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del forage 301\r\n");
}

static void rset_show_del_edesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes an extra description from the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset del edesc <keyword>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset del edesc mosaic\r\n");
}

static int rset_find_room_flag(const char *arg)
{
  int i;

  for (i = 0; i < NUM_ROOM_FLAGS; i++)
    if (is_abbrev(arg, room_bits[i]))
      return i;

  return -1;
}

static int rset_find_sector(const char *arg)
{
  int i;

  for (i = 0; i < NUM_ROOM_SECTORS; i++)
    if (is_abbrev(arg, sector_types[i]))
      return i;

  return -1;
}

static int rset_find_dir(const char *arg)
{
  char dirbuf[MAX_INPUT_LENGTH];
  int dir;

  strlcpy(dirbuf, arg, sizeof(dirbuf));
  dir = search_block(dirbuf, dirs, FALSE);
  if (dir == -1) {
    strlcpy(dirbuf, arg, sizeof(dirbuf));
    dir = search_block(dirbuf, autoexits, FALSE);
  }

  if (dir >= DIR_COUNT)
    return -1;

  return dir;
}

static void rset_mark_room_modified(room_rnum rnum)
{
  if (rnum == NOWHERE || rnum < 0 || rnum > top_of_world)
    return;

  add_to_save_list(zone_table[world[rnum].zone].number, SL_WLD);
}

static void rset_show_room(struct char_data *ch, struct room_data *room)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int i, count = 0;

  send_to_char(ch, "Room [%d]: %s\r\n", room->number, room->name ? room->name : "<None>");
  send_to_char(ch, "Zone [%d]: %s\r\n", zone_table[room->zone].number, zone_table[room->zone].name);

  sprinttype(room->sector_type, sector_types, buf, sizeof(buf));
  send_to_char(ch, "Sector: %s\r\n", buf);

  sprintbitarray(room->room_flags, room_bits, RF_ARRAY_MAX, buf);
  send_to_char(ch, "Flags: %s\r\n", buf);

  send_to_char(ch, "Description:\r\n%s", room->description ? room->description : "  <None>\r\n");

  send_to_char(ch, "Exits:\r\n");
  for (i = 0; i < DIR_COUNT; i++) {
    struct room_direction_data *exit = room->dir_option[i];
    room_rnum to_room;
    const char *dir = dirs[i];
    char keybuf[32];

    if (!exit)
      continue;

    to_room = exit->to_room;
    if (to_room == NOWHERE || to_room < 0 || to_room > top_of_world)
      snprintf(buf2, sizeof(buf2), "NOWHERE");
    else
      snprintf(buf2, sizeof(buf2), "%d", world[to_room].number);

    sprintbit(exit->exit_info, exit_bits, buf, sizeof(buf));
    if (IS_SET(exit->exit_info, EX_HIDDEN))
      strlcat(buf, "HIDDEN ", sizeof(buf));

    if (exit->key == NOTHING)
      strlcpy(keybuf, "None", sizeof(keybuf));
    else
      snprintf(keybuf, sizeof(keybuf), "%d", exit->key);

    send_to_char(ch, "  %-5s -> %s (door: %s, key: %s, flags: %s)\r\n",
      dir,
      buf2,
      exit->keyword ? exit->keyword : "None",
      keybuf,
      buf);
    count++;
  }
  if (!count)
    send_to_char(ch, "  None.\r\n");

  send_to_char(ch, "Forage:\r\n");
  count = 0;
  for (struct forage_entry *entry = room->forage; entry; entry = entry->next) {
    obj_rnum rnum = real_object(entry->obj_vnum);
    const char *sdesc = (rnum != NOTHING) ? obj_proto[rnum].short_description : "Unknown object";
    send_to_char(ch, "  [%d] DC %d - %s\r\n", entry->obj_vnum, entry->dc, sdesc);
    count++;
  }
  if (!count)
    send_to_char(ch, "  None.\r\n");

  send_to_char(ch, "Extra Descs:\r\n");
  count = 0;
  for (struct extra_descr_data *desc = room->ex_description; desc; desc = desc->next) {
    send_to_char(ch, "  %s\r\n", desc->keyword ? desc->keyword : "<None>");
    count++;
  }
  if (!count)
    send_to_char(ch, "  None.\r\n");
}

static void rset_desc_edit(struct char_data *ch, struct room_data *room)
{
  char *oldtext = NULL;

  send_editor_help(ch->desc);
  write_to_output(ch->desc, "Enter room description:\r\n\r\n");

  if (room->description) {
    write_to_output(ch->desc, "%s", room->description);
    oldtext = strdup(room->description);
  }

  string_write(ch->desc, &room->description, MAX_ROOM_DESC, 0, oldtext);
  rset_mark_room_modified(IN_ROOM(ch));
}

static bool rset_room_has_flags(struct room_data *room)
{
  int i;

  for (i = 0; i < RF_ARRAY_MAX; i++)
    if (room->room_flags[i])
      return TRUE;

  return FALSE;
}

static void rset_validate_room(struct char_data *ch, struct room_data *room)
{
  int errors = 0;
  int i;

  if (!room->name || !*room->name) {
    send_to_char(ch, "Error: room name is not set.\r\n");
    errors++;
  }

  if (!room->description || !*room->description) {
    send_to_char(ch, "Error: room description is not set.\r\n");
    errors++;
  }

  if (room->sector_type < 0 || room->sector_type >= NUM_ROOM_SECTORS) {
    send_to_char(ch, "Error: sector type is invalid.\r\n");
    errors++;
  }

  if (!rset_room_has_flags(room)) {
    send_to_char(ch, "Error: at least one room flag should be set.\r\n");
    errors++;
  }

  for (i = 0; i < DIR_COUNT; i++) {
    struct room_direction_data *exit = room->dir_option[i];
    bool exit_valid;

    if (!exit)
      continue;

    exit_valid = !(exit->to_room == NOWHERE || exit->to_room < 0 || exit->to_room > top_of_world);

    if (!exit_valid) {
      send_to_char(ch, "Error: exit %s does not point to a valid room.\r\n", dirs[i]);
      errors++;
    }

    if (IS_SET(exit->exit_info, EX_ISDOOR) && !exit_valid) {
      send_to_char(ch, "Error: door on %s has no valid exit.\r\n", dirs[i]);
      errors++;
    }

    if (exit->key > 0) {
      if (!IS_SET(exit->exit_info, EX_ISDOOR)) {
        send_to_char(ch, "Error: key on %s is set without a door.\r\n", dirs[i]);
        errors++;
      }
      if (real_object(exit->key) == NOTHING) {
        send_to_char(ch, "Error: key vnum %d on %s does not exist.\r\n", exit->key, dirs[i]);
        errors++;
      }
    }

    if (IS_SET(exit->exit_info, EX_HIDDEN) && !exit_valid) {
      send_to_char(ch, "Error: hidden flag on %s has no valid exit.\r\n", dirs[i]);
      errors++;
    }
  }

  for (struct forage_entry *entry = room->forage; entry; entry = entry->next) {
    if (entry->obj_vnum <= 0 || real_object(entry->obj_vnum) == NOTHING) {
      send_to_char(ch, "Error: forage object vnum %d is invalid.\r\n", entry->obj_vnum);
      errors++;
    }
    if (entry->dc <= 0) {
      send_to_char(ch, "Error: forage object vnum %d is missing a valid DC.\r\n", entry->obj_vnum);
      errors++;
    }
  }

  for (struct extra_descr_data *desc = room->ex_description; desc; desc = desc->next) {
    if (!desc->keyword || !*desc->keyword) {
      send_to_char(ch, "Error: extra description is missing a keyword.\r\n");
      errors++;
    }
    if (!desc->description || !*desc->description) {
      send_to_char(ch, "Error: extra description for %s is missing text.\r\n",
        desc->keyword ? desc->keyword : "<None>");
      errors++;
    }
  }

  if (!errors)
    send_to_char(ch, "Room validates cleanly.\r\n");
  else
    send_to_char(ch, "Validation failed: %d issue%s.\r\n", errors, errors == 1 ? "" : "s");
}

ACMD(do_rcreate)
{
  char arg[MAX_INPUT_LENGTH];
  char namebuf[MAX_INPUT_LENGTH];
  char descbuf[MAX_STRING_LENGTH];
  char timestr[64];
  struct room_data room;
  room_vnum vnum;
  room_rnum rnum;
  zone_rnum znum;
  time_t now;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "rcreate is only usable by connected players.\r\n");
    return;
  }

  argument = one_argument(argument, arg);
  if (!*arg) {
    rset_show_rcreate_usage(ch);
    return;
  }

  if (!is_number(arg)) {
    rset_show_rcreate_usage(ch);
    return;
  }

  vnum = atoi(arg);
  if (vnum <= 0) {
    send_to_char(ch, "That is not a valid room vnum.\r\n");
    return;
  }

  if (real_room(vnum) != NOWHERE) {
    send_to_char(ch, "Room %d already exists.\r\n", vnum);
    return;
  }

  if ((znum = real_zone_by_thing(vnum)) == NOWHERE) {
    send_to_char(ch, "That room number is not in a valid zone.\r\n");
    return;
  }

  if (!can_edit_zone(ch, znum)) {
    send_to_char(ch, "You do not have permission to modify that zone.\r\n");
    return;
  }

  now = time(0);
  strftime(timestr, sizeof(timestr), "%c", localtime(&now));
  snprintf(namebuf, sizeof(namebuf), "Unfinished room made by %s", GET_NAME(ch));
  snprintf(descbuf, sizeof(descbuf),
           "This is an unfinished room created by %s on %s\r\n",
           GET_NAME(ch), timestr);

  memset(&room, 0, sizeof(room));
  room.number = vnum;
  room.zone = znum;
  room.sector_type = SECT_INSIDE;
  room.name = strdup(namebuf);
  room.description = strdup(descbuf);
  room.ex_description = NULL;
  room.forage = NULL;

  rnum = add_room(&room);

  free(room.name);
  free(room.description);

  if (rnum == NOWHERE) {
    send_to_char(ch, "Room creation failed.\r\n");
    return;
  }

  send_to_char(ch, "Room %d created.\r\n", vnum);
}

ACMD(do_rset)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  struct room_data *room;
  room_rnum rnum;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "rset is only usable by connected players.\r\n");
    return;
  }

  rnum = IN_ROOM(ch);
  if (rnum == NOWHERE || rnum < 0 || rnum > top_of_world) {
    send_to_char(ch, "You are not in a valid room.\r\n");
    return;
  }

  if (!can_edit_zone(ch, world[rnum].zone)) {
    send_to_char(ch, "You do not have permission to modify this zone.\r\n");
    return;
  }

  room = &world[rnum];

  argument = one_argument(argument, arg1);
  if (!*arg1) {
    rset_show_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "show")) {
    rset_show_room(ch, room);
    return;
  }

  if (is_abbrev(arg1, "add") || is_abbrev(arg1, "set")) {
    bool set_alias = is_abbrev(arg1, "set");

    argument = one_argument(argument, arg2);
    if (!*arg2) {
      rset_show_add_usage(ch);
      return;
    }

    if (is_abbrev(arg2, "name")) {
      skip_spaces(&argument);
      if (!*argument) {
        rset_show_set_usage(ch);
        return;
      }
      genolc_checkstring(ch->desc, argument);
      if (count_non_protocol_chars(argument) > MAX_ROOM_NAME / 2) {
        send_to_char(ch, "Size limited to %d non-protocol characters.\r\n", MAX_ROOM_NAME / 2);
        return;
      }
      if (room->name)
        free(room->name);
      argument[MAX_ROOM_NAME - 1] = '\0';
      room->name = str_udup(argument);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Room name set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "sector")) {
      int sector;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_set_sector_usage(ch);
        return;
      }

      if (is_number(arg3))
        sector = atoi(arg3) - 1;
      else
        sector = rset_find_sector(arg3);

      if (sector < 0 || sector >= NUM_ROOM_SECTORS) {
        send_to_char(ch, "Invalid sector type.\r\n");
        return;
      }

      room->sector_type = sector;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Sector type set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "desc")) {
      if (*argument) {
        rset_show_add_usage(ch);
        return;
      }
      rset_desc_edit(ch, room);
      return;
    }

    if (is_abbrev(arg2, "flags")) {
      bool any = FALSE;

      if (!*argument) {
        rset_show_add_flags_usage(ch);
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = rset_find_room_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown room flag: %s\r\n", arg3);
          continue;
        }

        SET_BIT_AR(room->room_flags, flag);
        any = TRUE;
      }

      if (any) {
        rset_mark_room_modified(rnum);
        send_to_char(ch, "Room flags updated.\r\n");
      }
      return;
    }

    if (is_abbrev(arg2, "exit")) {
      int dir;
      room_rnum to_room;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg1);
      if (!*arg3 || !*arg1) {
        rset_show_add_exit_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!is_number(arg1)) {
        send_to_char(ch, "Room number must be numeric.\r\n");
        return;
      }

      to_room = real_room(atoi(arg1));
      if (to_room == NOWHERE) {
        send_to_char(ch, "That room does not exist.\r\n");
        return;
      }

      if (!room->dir_option[dir]) {
        CREATE(room->dir_option[dir], struct room_direction_data, 1);
        room->dir_option[dir]->general_description = NULL;
        room->dir_option[dir]->keyword = NULL;
        room->dir_option[dir]->exit_info = 0;
        room->dir_option[dir]->key = NOTHING;
      }

      room->dir_option[dir]->to_room = to_room;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Exit %s set to room %d.\r\n", dirs[dir], world[to_room].number);
      return;
    }

    if (is_abbrev(arg2, "door")) {
      int dir;
      char *door_name;

      argument = one_argument(argument, arg3);
      skip_spaces(&argument);
      door_name = argument;

      if (!*arg3 || !*door_name) {
        rset_show_add_door_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir] || room->dir_option[dir]->to_room == NOWHERE) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      genolc_checkstring(ch->desc, door_name);
      if (room->dir_option[dir]->keyword)
        free(room->dir_option[dir]->keyword);
      room->dir_option[dir]->keyword = str_udup(door_name);
      SET_BIT(room->dir_option[dir]->exit_info, EX_ISDOOR);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Door added to %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "key")) {
      int dir;
      int key_vnum;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg1);
      if (!*arg3 || !*arg1) {
        rset_show_add_key_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir] || room->dir_option[dir]->to_room == NOWHERE) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      if (!IS_SET(room->dir_option[dir]->exit_info, EX_ISDOOR)) {
        send_to_char(ch, "That exit has no door.\r\n");
        return;
      }

      if (!is_number(arg1)) {
        send_to_char(ch, "Key number must be numeric.\r\n");
        return;
      }

      key_vnum = atoi(arg1);
      if (real_object(key_vnum) == NOTHING) {
        send_to_char(ch, "That key vnum does not exist.\r\n");
        return;
      }

      room->dir_option[dir]->key = key_vnum;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Key set on %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "hidden")) {
      int dir;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_add_hidden_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir] || room->dir_option[dir]->to_room == NOWHERE) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      SET_BIT(room->dir_option[dir]->exit_info, EX_HIDDEN);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Hidden flag set on %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "forage")) {
      int vnum, dc;
      struct forage_entry *entry;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg1);
      if (!*arg3 || !*arg1) {
        rset_show_add_forage_usage(ch);
        return;
      }

      if (!is_number(arg3) || !is_number(arg1)) {
        rset_show_add_forage_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      dc = atoi(arg1);

      if (vnum <= 0 || dc <= 0) {
        send_to_char(ch, "Both vnum and DC must be positive.\r\n");
        return;
      }

      if (real_object(vnum) == NOTHING) {
        send_to_char(ch, "That object vnum does not exist.\r\n");
        return;
      }

      CREATE(entry, struct forage_entry, 1);
      entry->obj_vnum = vnum;
      entry->dc = dc;
      entry->next = room->forage;
      room->forage = entry;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Forage entry added.\r\n");
      return;
    }

    if (is_abbrev(arg2, "edesc")) {
      struct extra_descr_data *desc;
      char *keyword;
      char *edesc;

      argument = one_argument(argument, arg3);
      skip_spaces(&argument);
      keyword = arg3;
      edesc = argument;

      if (!*keyword || !*edesc) {
        rset_show_add_edesc_usage(ch);
        return;
      }

      genolc_checkstring(ch->desc, edesc);
      genolc_checkstring(ch->desc, keyword);

      CREATE(desc, struct extra_descr_data, 1);
      desc->keyword = str_udup(keyword);
      desc->description = str_udup(edesc);
      desc->next = room->ex_description;
      room->ex_description = desc;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Extra description added.\r\n");
      return;
    }

    if (set_alias) {
      int flag;

      flag = rset_find_room_flag(arg2);
      if (flag >= 0) {
        SET_BIT_AR(room->room_flags, flag);
        rset_mark_room_modified(rnum);
        send_to_char(ch, "Room flag set.\r\n");
        return;
      }
    }

    rset_show_add_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "del")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      rset_show_del_usage(ch);
      return;
    }

    if (is_abbrev(arg2, "flags")) {
      bool any = FALSE;

      if (!*argument) {
        rset_show_del_flags_usage(ch);
        return;
      }

      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = rset_find_room_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown room flag: %s\r\n", arg3);
          continue;
        }

        REMOVE_BIT_AR(room->room_flags, flag);
        any = TRUE;
      }

      if (any) {
        rset_mark_room_modified(rnum);
        send_to_char(ch, "Room flags updated.\r\n");
      }
      return;
    }

    if (is_abbrev(arg2, "exit")) {
      int dir;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_exit_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir]) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      if (room->dir_option[dir]->general_description)
        free(room->dir_option[dir]->general_description);
      if (room->dir_option[dir]->keyword)
        free(room->dir_option[dir]->keyword);
      free(room->dir_option[dir]);
      room->dir_option[dir] = NULL;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Exit %s removed.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "door")) {
      int dir;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_door_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir]) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      if (room->dir_option[dir]->keyword) {
        free(room->dir_option[dir]->keyword);
        room->dir_option[dir]->keyword = NULL;
      }

      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_ISDOOR);
      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_CLOSED);
      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_LOCKED);
      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_PICKPROOF);
      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_HIDDEN);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Door removed from %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "key")) {
      int dir;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_key_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir]) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      room->dir_option[dir]->key = NOTHING;
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Key removed from %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "hidden")) {
      int dir;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_hidden_usage(ch);
        return;
      }

      dir = rset_find_dir(arg3);
      if (dir < 0) {
        send_to_char(ch, "Invalid direction.\r\n");
        return;
      }

      if (!room->dir_option[dir]) {
        send_to_char(ch, "That exit does not exist.\r\n");
        return;
      }

      REMOVE_BIT(room->dir_option[dir]->exit_info, EX_HIDDEN);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Hidden flag removed from %s.\r\n", dirs[dir]);
      return;
    }

    if (is_abbrev(arg2, "forage")) {
      int vnum;
      struct forage_entry *entry = room->forage;
      struct forage_entry *prev = NULL;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_forage_usage(ch);
        return;
      }

      if (!is_number(arg3)) {
        rset_show_del_forage_usage(ch);
        return;
      }

      vnum = atoi(arg3);
      while (entry) {
        if (entry->obj_vnum == vnum)
          break;
        prev = entry;
        entry = entry->next;
      }

      if (!entry) {
        send_to_char(ch, "No forage entry found for vnum %d.\r\n", vnum);
        return;
      }

      if (prev)
        prev->next = entry->next;
      else
        room->forage = entry->next;
      free(entry);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Forage entry removed.\r\n");
      return;
    }

    if (is_abbrev(arg2, "edesc")) {
      struct extra_descr_data *desc = room->ex_description;
      struct extra_descr_data *prev = NULL;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        rset_show_del_edesc_usage(ch);
        return;
      }

      while (desc) {
        if (desc->keyword && isname(arg3, desc->keyword))
          break;
        prev = desc;
        desc = desc->next;
      }

      if (!desc) {
        send_to_char(ch, "No extra description found for %s.\r\n", arg3);
        return;
      }

      if (prev)
        prev->next = desc->next;
      else
        room->ex_description = desc->next;

      if (desc->keyword)
        free(desc->keyword);
      if (desc->description)
        free(desc->description);
      free(desc);
      rset_mark_room_modified(rnum);
      send_to_char(ch, "Extra description removed.\r\n");
      return;
    }

    rset_show_del_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "desc")) {
    rset_show_desc_usage(ch);
    rset_desc_edit(ch, room);
    return;
  }

  if (is_abbrev(arg1, "clear")) {
    argument = one_argument(argument, arg2);
    if (!*arg2 || !is_abbrev(arg2, "force")) {
      rset_show_usage(ch);
      return;
    }
    if (*argument) {
      rset_show_usage(ch);
      return;
    }

    free_room_strings(room);
    room->name = strdup("An unfinished room");
    room->description = strdup("You are in an unfinished room.\r\n");
    room->sector_type = SECT_INSIDE;
    memset(room->room_flags, 0, sizeof(room->room_flags));
    rset_mark_room_modified(rnum);
    send_to_char(ch, "Room cleared.\r\n");
    return;
  }

  if (is_abbrev(arg1, "validate")) {
    if (*argument) {
      rset_show_validate_usage(ch);
      return;
    }
    rset_validate_room(ch, room);
    return;
  }

  rset_show_usage(ch);
}

static void oset_show_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Usage:\r\n"
    "  oset show <obj>\r\n"
    "  oset add keywords <obj> <keyword> [keywords]\r\n"
    "  oset add sdesc <obj> <text>\r\n"
    "  oset add ldesc <obj> <text>\r\n"
    "  oset add desc <obj>\r\n"
    "  oset add type <obj> <item type>\r\n"
    "  oset add flags <obj> <flags> [flags]\r\n"
    "  oset add wear <obj> <wear type> [wear types]\r\n"
    "  oset add weight <obj> <value>\r\n"
    "  oset add cost <obj> <value>\r\n"
    "  oset add oval <obj> <oval number> <value>\r\n"
    "  oset del <obj> <field>\r\n"
    "  oset clear <obj> force\r\n"
    "  oset validate <obj>\r\n");
}

static void oset_show_add_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds specific configuration to the object.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset add keywords <obj> <keyword> [keywords]\r\n"
    "  oset add sdesc <obj> <text>\r\n"
    "  oset add ldesc <obj> <text>\r\n"
    "  oset add desc <obj>\r\n"
    "  oset add type <obj> <item type>\r\n"
    "  oset add flags <obj> <flags> [flags]\r\n"
    "  oset add wear <obj> <wear type> [wear types]\r\n"
    "  oset add weight <obj> <value>\r\n"
    "  oset add cost <obj> <value>\r\n"
    "  oset add oval <obj> <oval number> <value>\r\n");
}

static void oset_show_add_keywords_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds keywords to object. Can add a single keyword or several at once.\r\n"
    "It is always best to use the most specific keyword as the first entry.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add keywords sword steel\r\n"
    "  oset add keywords armor padded\r\n"
    "  oset add keywords staff gnarled oak\r\n");
}

static void oset_show_add_sdesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a short description to the object. This is what is seen in an inventory,\r\n"
    "in a container, on furniture, or worn by someone.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add sdesc sword a wooden sword\r\n"
    "  oset add sdesc cloak a dark, hooded cloak\r\n"
    "  oset add sdesc maul a massive, obsidian-studded maul\r\n");
}

static void oset_show_add_ldesc_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Adds a long description to the object. This is what everyone sees when an item\r\n"
    "is in a room.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add ldesc cloak A pile of dark fabric is here in a heap.\r\n"
    "  oset add ldesc maul Made of solid wood, a massive, obsidian-studded maul is here.\r\n"
    "  oset add ldesc bread A piece of bread has been discarded here.\r\n");
}

static void oset_show_add_type_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies the object type. Can only be one type.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add type chest container\r\n"
    "  oset add type armor armor\r\n"
    "  oset add type sword weapon\r\n"
    "\r\n"
    "Types:\r\n");
  column_list(ch, 0, item_types, NUM_ITEM_TYPES, FALSE);
}

static void oset_show_add_flags_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies object flags. Can be a single flag or multiples.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add flags sword no_drop\r\n"
    "  oset add flags staff hum glow\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, extra_bits, NUM_EXTRA_FLAGS, FALSE);
}

static void oset_show_add_wear_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies object wear types. Can be a single type or multiples.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add wear sword wield\r\n"
    "  oset add wear staff wield hold\r\n"
    "\r\n"
    "Wear types:\r\n");
  column_list(ch, 0, wear_bits, NUM_ITEM_WEARS, FALSE);
}

static void oset_show_add_weight_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies object weight. Affects carrying capacity of PC/NPC.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add weight sword 1\r\n");
}

static void oset_show_add_cost_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Specifies object cost. Determines how much it sells for and is bought for.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add cost sword 100\r\n");
}

static void oset_show_add_oval_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Sets an oval property on an object. Each object type has its own respective\r\n"
    "ovals. Some of these influence different code paramters, such as the type\r\n"
    "CONTAINER having oval1 \"capacity\". This sets how much weight the container\r\n"
    "object can hold.\r\n"
    "\r\n"
    "For a list of ovals each item has, check HELP OVALS.\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset add oval chest capacity 100 (for a container)\r\n"
    "  oset add oval armor piece_ac 2 (for armor)\r\n"
    "  oset add oval sword weapon_type slashing (for weapons)\r\n");
}

static void oset_show_del_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes specific configuration from the object.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset del <obj> keywords <keyword> [keywords]\r\n"
    "  oset del <obj> flags <flags> [flags]\r\n"
    "  oset del <obj> wear <wear type> [wear types]\r\n"
    "  oset del <obj> oval <oval number|oval name>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset del sword keywords sword\r\n"
    "  oset del sword flags hum\r\n"
    "  oset del sword wear wield\r\n"
    "  oset del sword oval 2\r\n");
}

static void oset_show_del_flags_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes object flags.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset del <obj> flags <flag> [flags]\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset del sword flags hum\r\n"
    "\r\n"
    "Flags:\r\n");
  column_list(ch, 0, extra_bits, NUM_EXTRA_FLAGS, FALSE);
}

static void oset_show_del_wear_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Deletes object wear types.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset del <obj> wear <wear type> [wear types]\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset del sword wear wield\r\n"
    "\r\n"
    "Wear types:\r\n");
  column_list(ch, 0, wear_bits, NUM_ITEM_WEARS, FALSE);
}

static void oset_show_clear_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Clears all configuration from an object to start over fresh.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  oset clear <obj> force\r\n"
    "\r\n"
    "Examples:\r\n"
    "  oset clear sword force\r\n");
}

static struct obj_data *oset_get_target_obj_keyword(struct char_data *ch, char *keyword)
{
  struct obj_data *obj;

  if (!ch || !keyword || !*keyword)
    return NULL;

  obj = get_obj_in_list_vis(ch, keyword, NULL, ch->carrying);
  if (obj)
    return obj;

  return get_obj_in_list_vis(ch, keyword, NULL, world[IN_ROOM(ch)].contents);
}

static void oset_replace_string(struct obj_data *obj, char **field, const char *value, const char *proto_field)
{
  if (*field && (!proto_field || *field != proto_field))
    free(*field);

  *field = strdup(value ? value : "");
}

static int oset_find_item_type(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return -1;

  for (i = 0; *item_types[i] != '\n'; i++) {
    if (is_abbrev(arg, item_types[i]))
      return i;
  }

  return -1;
}

static int oset_find_extra_flag(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return -1;

  for (i = 0; i < NUM_EXTRA_FLAGS; i++) {
    if (is_abbrev(arg, extra_bits[i]))
      return i;
  }

  return -1;
}

static int oset_find_wear_flag(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return -1;

  for (i = 0; i < NUM_ITEM_WEARS; i++) {
    if (is_abbrev(arg, wear_bits[i]))
      return i;
  }

  return -1;
}

static int oset_find_oval_by_name(const char *arg, const char * const *labels)
{
  int i;

  if (!arg || !*arg || !labels)
    return -1;

  for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
    if (labels[i] && *labels[i] && is_abbrev(arg, labels[i]))
      return i;
  }

  return -1;
}

static int oset_find_attack_type(const char *arg)
{
  int i;

  if (!arg || !*arg)
    return -1;

  for (i = 0; i < NUM_ATTACK_TYPES; i++) {
    if (is_abbrev(arg, attack_hit_text[i].singular) ||
        is_abbrev(arg, attack_hit_text[i].plural))
      return i;
  }

  if (!str_cmp(arg, "slashing"))
    return oset_find_attack_type("slash");
  if (!str_cmp(arg, "piercing"))
    return oset_find_attack_type("pierce");
  if (!str_cmp(arg, "bludgeoning"))
    return oset_find_attack_type("bludgeon");

  return -1;
}

static bool oset_add_keywords(struct char_data *ch, struct obj_data *obj, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  bool changed = FALSE;
  obj_rnum rnum = GET_OBJ_RNUM(obj);
  const char *proto_name = (rnum != NOTHING) ? obj_proto[rnum].name : NULL;

  buf[0] = '\0';
  if (obj->name && *obj->name)
    strlcpy(buf, obj->name, sizeof(buf));

  while (*argument) {
    argument = one_argument(argument, arg);
    if (!*arg)
      break;

    if (!isname(arg, buf)) {
      size_t needed = strlen(buf) + strlen(arg) + 2;
      if (needed >= sizeof(buf)) {
        send_to_char(ch, "Keyword list is too long.\r\n");
        return FALSE;
      }
      if (*buf)
        strlcat(buf, " ", sizeof(buf));
      strlcat(buf, arg, sizeof(buf));
      changed = TRUE;
    }
  }

  if (!changed) {
    send_to_char(ch, "Keywords unchanged.\r\n");
    return TRUE;
  }

  oset_replace_string(obj, &obj->name, buf, proto_name);
  send_to_char(ch, "Keywords updated.\r\n");
  return TRUE;
}

static bool oset_del_keywords(struct char_data *ch, struct obj_data *obj, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char work[MAX_STRING_LENGTH];
  char word[MAX_INPUT_LENGTH];
  bool changed = FALSE;
  obj_rnum rnum = GET_OBJ_RNUM(obj);
  const char *proto_name = (rnum != NOTHING) ? obj_proto[rnum].name : NULL;
  char *ptr;

  buf[0] = '\0';
  if (!obj->name || !*obj->name) {
    send_to_char(ch, "Object has no keywords to remove.\r\n");
    return TRUE;
  }

  strlcpy(work, obj->name, sizeof(work));
  ptr = work;
  while (*ptr) {
    ptr = one_argument(ptr, word);
    if (!*word)
      break;

    if (isname(word, argument)) {
      changed = TRUE;
      continue;
    }

    if (*buf)
      strlcat(buf, " ", sizeof(buf));
    strlcat(buf, word, sizeof(buf));
  }

  if (!changed) {
    send_to_char(ch, "No matching keywords found.\r\n");
    return TRUE;
  }

  oset_replace_string(obj, &obj->name, buf, proto_name);
  send_to_char(ch, "Keywords updated.\r\n");
  return TRUE;
}

static void oset_show_object(struct char_data *ch, struct obj_data *obj)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int i;
  const char * const *labels = obj_value_labels(GET_OBJ_TYPE(obj));

  sprinttype(GET_OBJ_TYPE(obj), item_types, buf, sizeof(buf));
  sprintbitarray(GET_OBJ_EXTRA(obj), extra_bits, EF_ARRAY_MAX, buf2);

  send_to_char(ch, "Object [%d]: %s\r\n",
    GET_OBJ_VNUM(obj), obj->short_description ? obj->short_description : "<None>");
  send_to_char(ch, "Keywords: %s\r\n", obj->name ? obj->name : "<None>");
  send_to_char(ch, "Short desc: %s\r\n", obj->short_description ? obj->short_description : "<None>");
  send_to_char(ch, "Long desc: %s\r\n", obj->description ? obj->description : "<None>");
  send_to_char(ch, "Main desc:\r\n%s", obj->main_description ? obj->main_description : "  <None>\r\n");
  send_to_char(ch, "Type: %s\r\n", buf);
  send_to_char(ch, "Flags: %s\r\n", buf2);
  sprintbitarray(GET_OBJ_WEAR(obj), wear_bits, TW_ARRAY_MAX, buf);
  send_to_char(ch, "Wear: %s\r\n", buf);
  send_to_char(ch, "Weight: %d\r\n", GET_OBJ_WEIGHT(obj));
  send_to_char(ch, "Cost: %d\r\n", GET_OBJ_COST(obj));

  send_to_char(ch, "Ovals:\r\n");
  for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
    const char *label = labels ? labels[i] : "Value";
    send_to_char(ch, "  [%d] %s: %d\r\n", i, label, GET_OBJ_VAL(obj, i));
  }
}

static void oset_desc_edit(struct char_data *ch, struct obj_data *obj)
{
  char *oldtext = NULL;

  send_editor_help(ch->desc);
  write_to_output(ch->desc, "Enter object description:\r\n\r\n");

  if (obj->main_description) {
    write_to_output(ch->desc, "%s", obj->main_description);
    oldtext = strdup(obj->main_description);
  }

  string_write(ch->desc, &obj->main_description, MAX_MESSAGE_LENGTH, 0, oldtext);
}

static void oset_clear_object(struct obj_data *obj)
{
  obj_rnum rnum = GET_OBJ_RNUM(obj);

  if (rnum != NOTHING)
    free_object_strings_proto(obj);
  else
    free_object_strings(obj);

  obj->name = NULL;
  obj->description = NULL;
  obj->short_description = NULL;
  obj->main_description = NULL;
  obj->ex_description = NULL;

  obj->name = strdup("unfinished object");
  obj->description = strdup("An unfinished object is lying here.");
  obj->short_description = strdup("an unfinished object");

  GET_OBJ_TYPE(obj) = 0;
  GET_OBJ_WEIGHT(obj) = 0;
  GET_OBJ_COST(obj) = 0;
  GET_OBJ_COST_PER_DAY(obj) = 0;
  GET_OBJ_TIMER(obj) = 0;
  GET_OBJ_LEVEL(obj) = 0;

  memset(obj->obj_flags.extra_flags, 0, sizeof(obj->obj_flags.extra_flags));
  memset(obj->obj_flags.wear_flags, 0, sizeof(obj->obj_flags.wear_flags));
  memset(obj->obj_flags.value, 0, sizeof(obj->obj_flags.value));
  memset(obj->obj_flags.bitvector, 0, sizeof(obj->obj_flags.bitvector));
  memset(obj->affected, 0, sizeof(obj->affected));

  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
}

static void oset_validate_object(struct char_data *ch, struct obj_data *obj)
{
  int errors = 0;

  if (!obj->name || !*obj->name) {
    send_to_char(ch, "Error: keywords are not set.\r\n");
    errors++;
  }

  if (!obj->short_description || !*obj->short_description) {
    send_to_char(ch, "Error: short description is not set.\r\n");
    errors++;
  }

  if (!obj->description || !*obj->description) {
    send_to_char(ch, "Error: long description is not set.\r\n");
    errors++;
  }

  if (!obj->main_description || !*obj->main_description) {
    send_to_char(ch, "Error: main description is not set.\r\n");
    errors++;
  }

  if (GET_OBJ_TYPE(obj) < 1 || GET_OBJ_TYPE(obj) >= NUM_ITEM_TYPES) {
    send_to_char(ch, "Error: object type is not set.\r\n");
    errors++;
  }

  if (GET_OBJ_WEIGHT(obj) <= 0) {
    send_to_char(ch, "Error: weight must be above zero.\r\n");
    errors++;
  }

  if (GET_OBJ_COST(obj) <= 0) {
    send_to_char(ch, "Error: cost must be above zero.\r\n");
    errors++;
  }

  if (!errors)
    send_to_char(ch, "Object validates cleanly.\r\n");
  else
    send_to_char(ch, "Validation failed: %d issue%s.\r\n", errors, errors == 1 ? "" : "s");
}

ACMD(do_oset)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char arg4[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "oset is only usable by connected players.\r\n");
    return;
  }

  argument = one_argument(argument, arg1);

  if (!*arg1) {
    oset_show_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "show")) {
    char *keyword;

    skip_spaces(&argument);
    keyword = argument;
    if (!*keyword) {
      send_to_char(ch, "Provide a keyword of an object to show.\r\n");
      return;
    }
    obj = oset_get_target_obj_keyword(ch, keyword);
    if (!obj) {
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(keyword), keyword);
      return;
    }
    oset_show_object(ch, obj);
    return;
  }

  if (is_abbrev(arg1, "add")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      oset_show_add_usage(ch);
      return;
    }

    if (is_abbrev(arg2, "keywords")) {
      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_keywords_usage(ch);
        return;
      }
      if (!*argument) {
        oset_show_add_keywords_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      oset_add_keywords(ch, obj, argument);
      return;
    }

    if (is_abbrev(arg2, "sdesc")) {
      static size_t max_len = 64;
      obj_rnum rnum;
      const char *proto_sdesc;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_sdesc_usage(ch);
        return;
      }
      skip_spaces(&argument);
      if (!*argument) {
        oset_show_add_sdesc_usage(ch);
        return;
      }
      if (strlen(argument) > max_len) {
        send_to_char(ch, "Short description is too long.\r\n");
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      rnum = GET_OBJ_RNUM(obj);
      proto_sdesc = (rnum != NOTHING) ? obj_proto[rnum].short_description : NULL;
      oset_replace_string(obj, &obj->short_description, argument, proto_sdesc);
      send_to_char(ch, "Short description set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "ldesc")) {
      static size_t max_len = 128;
      obj_rnum rnum;
      const char *proto_ldesc;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_ldesc_usage(ch);
        return;
      }
      skip_spaces(&argument);
      if (!*argument) {
        oset_show_add_ldesc_usage(ch);
        return;
      }
      if (strlen(argument) > max_len) {
        send_to_char(ch, "Long description is too long.\r\n");
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      rnum = GET_OBJ_RNUM(obj);
      proto_ldesc = (rnum != NOTHING) ? obj_proto[rnum].description : NULL;
      oset_replace_string(obj, &obj->description, argument, proto_ldesc);
      send_to_char(ch, "Long description set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "desc")) {
      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_usage(ch);
        return;
      }
      if (*argument) {
        oset_show_add_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      oset_desc_edit(ch, obj);
      return;
    }

    if (is_abbrev(arg2, "type")) {
      int type;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_type_usage(ch);
        return;
      }
      skip_spaces(&argument);
      if (!*argument) {
        oset_show_add_type_usage(ch);
        return;
      }
      type = oset_find_item_type(argument);
      if (type < 0 || type >= NUM_ITEM_TYPES) {
        send_to_char(ch, "Invalid object type.\r\n");
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      GET_OBJ_TYPE(obj) = type;
      send_to_char(ch, "Object type set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "flags")) {
      bool any = FALSE;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_flags_usage(ch);
        return;
      }
      if (!*argument) {
        oset_show_add_flags_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = oset_find_extra_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown flag: %s\r\n", arg3);
          continue;
        }

        SET_BIT_AR(GET_OBJ_EXTRA(obj), flag);
        any = TRUE;
      }
      if (any)
        send_to_char(ch, "Object flags updated.\r\n");
      return;
    }

    if (is_abbrev(arg2, "wear")) {
      bool any = FALSE;

      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_wear_usage(ch);
        return;
      }
      if (!*argument) {
        oset_show_add_wear_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = oset_find_wear_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown wear type: %s\r\n", arg3);
          continue;
        }

        SET_BIT_AR(GET_OBJ_WEAR(obj), flag);
        any = TRUE;
      }
      if (any)
        send_to_char(ch, "Wear flags updated.\r\n");
      return;
    }

    if (is_abbrev(arg2, "weight")) {
      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_weight_usage(ch);
        return;
      }
      skip_spaces(&argument);
      if (!*argument || !is_number(argument)) {
        oset_show_add_weight_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      GET_OBJ_WEIGHT(obj) = LIMIT(atoi(argument), 0, MAX_OBJ_WEIGHT);
      send_to_char(ch, "Weight set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "cost")) {
      argument = one_argument(argument, arg3);
      if (!*arg3) {
        oset_show_add_cost_usage(ch);
        return;
      }
      skip_spaces(&argument);
      if (!*argument || !is_number(argument)) {
        oset_show_add_cost_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      GET_OBJ_COST(obj) = LIMIT(atoi(argument), 0, MAX_OBJ_COST);
      send_to_char(ch, "Cost set.\r\n");
      return;
    }

    if (is_abbrev(arg2, "oval")) {
      int pos;
      int value;
      const char * const *labels;

      argument = one_argument(argument, arg3);
      argument = one_argument(argument, arg4);
      if (!*arg3 || !*arg4) {
        oset_show_add_oval_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg3);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg3), arg3);
        return;
      }
      argument = one_argument(argument, arg1);
      if (!*arg1) {
        oset_show_add_oval_usage(ch);
        return;
      }

      labels = obj_value_labels(GET_OBJ_TYPE(obj));
      if (is_number(arg4)) {
        pos = atoi(arg4);
      } else {
        pos = oset_find_oval_by_name(arg4, labels);
      }

      if (pos < 0 || pos >= NUM_OBJ_VAL_POSITIONS) {
        send_to_char(ch, "Invalid oval position.\r\n");
        return;
      }

      if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && pos == 2 &&
          !is_number(arg1)) {
        value = oset_find_attack_type(arg1);
        if (value < 0) {
          send_to_char(ch, "Unknown weapon type: %s\r\n", arg1);
          return;
        }
      } else {
        if (!is_number(arg1)) {
          send_to_char(ch, "Oval value must be numeric.\r\n");
          return;
        }
        value = atoi(arg1);
      }

      GET_OBJ_VAL(obj, pos) = value;
      send_to_char(ch, "Oval set.\r\n");
      return;
    }

    oset_show_add_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "del")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      oset_show_del_usage(ch);
      return;
    }

    argument = one_argument(argument, arg3);
    if (!*arg3) {
      oset_show_del_usage(ch);
      return;
    }

    if (is_abbrev(arg3, "keywords")) {
      if (!*argument) {
        oset_show_del_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg2);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
        return;
      }
      oset_del_keywords(ch, obj, argument);
      return;
    }

    if (is_abbrev(arg3, "flags")) {
      bool any = FALSE;

      if (!*argument) {
        oset_show_del_flags_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg2);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
        return;
      }
      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = oset_find_extra_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown flag: %s\r\n", arg3);
          continue;
        }

        REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), flag);
        any = TRUE;
      }
      if (any)
        send_to_char(ch, "Object flags updated.\r\n");
      return;
    }

    if (is_abbrev(arg3, "wear")) {
      bool any = FALSE;

      if (!*argument) {
        oset_show_del_wear_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg2);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
        return;
      }
      while (*argument) {
        int flag;

        argument = one_argument(argument, arg3);
        if (!*arg3)
          break;

        flag = oset_find_wear_flag(arg3);
        if (flag < 0) {
          send_to_char(ch, "Unknown wear type: %s\r\n", arg3);
          continue;
        }

        REMOVE_BIT_AR(GET_OBJ_WEAR(obj), flag);
        any = TRUE;
      }
      if (any)
        send_to_char(ch, "Wear flags updated.\r\n");
      return;
    }

    if (is_abbrev(arg3, "oval")) {
      int pos;
      const char * const *labels;

      argument = one_argument(argument, arg4);
      if (!*arg4) {
        oset_show_del_usage(ch);
        return;
      }
      obj = oset_get_target_obj_keyword(ch, arg2);
      if (!obj) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
        return;
      }

      labels = obj_value_labels(GET_OBJ_TYPE(obj));
      if (is_number(arg4)) {
        pos = atoi(arg4);
      } else {
        pos = oset_find_oval_by_name(arg4, labels);
      }

      if (pos < 0 || pos >= NUM_OBJ_VAL_POSITIONS) {
        send_to_char(ch, "Invalid oval position.\r\n");
        return;
      }

      GET_OBJ_VAL(obj, pos) = 0;
      send_to_char(ch, "Oval cleared.\r\n");
      return;
    }

    oset_show_del_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "clear")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      oset_show_clear_usage(ch);
      return;
    }
    obj = oset_get_target_obj_keyword(ch, arg2);
    if (!obj) {
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg2), arg2);
      return;
    }
    argument = one_argument(argument, arg3);
    if (!*arg3 || !is_abbrev(arg3, "force")) {
      oset_show_clear_usage(ch);
      return;
    }
    if (*argument) {
      oset_show_clear_usage(ch);
      return;
    }
    oset_clear_object(obj);
    send_to_char(ch, "Object cleared.\r\n");
    return;
  }

  if (is_abbrev(arg1, "validate")) {
    char *keyword;

    skip_spaces(&argument);
    keyword = argument;
    if (!*keyword) {
      send_to_char(ch, "Provide a keyword of an object to validate.\r\n");
      return;
    }
    obj = oset_get_target_obj_keyword(ch, keyword);
    if (!obj) {
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(keyword), keyword);
      return;
    }
    oset_validate_object(ch, obj);
    return;
  }

  oset_show_usage(ch);
}

static struct obj_data *find_obj_vnum_nearby(struct char_data *ch, obj_vnum vnum)
{
  struct obj_data *obj;

  if (!ch || IN_ROOM(ch) == NOWHERE)
    return NULL;

  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj) && GET_OBJ_VNUM(obj) == vnum)
      return obj;

  for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj) && GET_OBJ_VNUM(obj) == vnum)
      return obj;

  return NULL;
}

ACMD(do_ocreate)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char namebuf[MAX_NAME_LENGTH];
  char timestr[MAX_STRING_LENGTH];
  struct obj_data *newobj;
  struct obj_data *obj;
  obj_vnum vnum;
  zone_rnum znum;
  time_t ct;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "ocreate is only usable by connected players.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch,
      "Creates a new unfinished object which can be configured.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  ocreate <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  ocreate 1001\r\n");
    return;
  }

  if (!is_number(arg)) {
    send_to_char(ch,
      "Creates a new unfinished object which can be configured.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  ocreate <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  ocreate 1001\r\n");
    return;
  }

  vnum = atoi(arg);
  if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
    send_to_char(ch, "That object VNUM can't exist.\r\n");
    return;
  }

  if (real_object(vnum) != NOTHING) {
    send_to_char(ch, "Object %d already exists.\r\n", vnum);
    return;
  }

  znum = real_zone_by_thing(vnum);
  if (znum == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    return;
  }

  if (!can_edit_zone(ch, znum)) {
    send_cannot_edit(ch, zone_table[znum].number);
    return;
  }

  CREATE(newobj, struct obj_data, 1);
  clear_object(newobj);

  newobj->name = strdup("unfinished object");
  strlcpy(namebuf, GET_NAME(ch), sizeof(namebuf));
  snprintf(buf, sizeof(buf), "unfinished object made by %s", namebuf);
  newobj->short_description = strdup(buf);
  ct = time(0);
  strftime(timestr, sizeof(timestr), "%c", localtime(&ct));
  snprintf(buf, sizeof(buf),
    "This is an unfinished object created by %s on ", namebuf);
  strlcat(buf, timestr, sizeof(buf));
  newobj->description = strdup(buf);
  SET_BIT_AR(GET_OBJ_WEAR(newobj), ITEM_WEAR_TAKE);

  if (add_object(newobj, vnum) == NOTHING) {
    free_object_strings(newobj);
    free(newobj);
    send_to_char(ch, "ocreate: failed to add object %d.\r\n", vnum);
    return;
  }

  if (in_save_list(zone_table[znum].number, SL_OBJ))
    remove_from_save_list(zone_table[znum].number, SL_OBJ);

  free_object_strings(newobj);
  free(newobj);

  obj = read_object(vnum, VIRTUAL);
  if (obj == NULL) {
    send_to_char(ch, "ocreate: failed to instantiate object %d.\r\n", vnum);
    return;
  }

  obj_to_char(obj, ch);
  send_to_char(ch,
    "Object %d created (temporary). Use osave to write it to disk.\r\n",
    vnum);
}

ACMD(do_osave)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct obj_data *proto;
  obj_rnum robj_num;
  obj_vnum vnum;
  zone_rnum znum;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "osave is only usable by connected players.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch,
      "Saves an object and its current properties to disk, which will load upon next boot.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  osave <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  osave 1001\r\n");
    return;
  }

  if (!is_number(arg)) {
    send_to_char(ch,
      "Saves an object and its current properties to disk, which will load upon next boot.\r\n"
      "\r\n"
      "Usage:\r\n"
      "  osave <vnum>\r\n"
      "\r\n"
      "Examples:\r\n"
      "  osave 1001\r\n");
    return;
  }

  vnum = atoi(arg);
  if (vnum < IDXTYPE_MIN || vnum > IDXTYPE_MAX) {
    send_to_char(ch, "That object VNUM can't exist.\r\n");
    return;
  }

  obj = find_obj_vnum_nearby(ch, vnum);
  if (obj == NULL) {
    send_to_char(ch,
      "osave: object %d is not in your inventory or room.\r\n", vnum);
    return;
  }

  znum = real_zone_by_thing(vnum);
  if (znum == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    return;
  }

  if (!can_edit_zone(ch, znum)) {
    send_cannot_edit(ch, zone_table[znum].number);
    return;
  }

  CREATE(proto, struct obj_data, 1);
  clear_object(proto);
  copy_object(proto, obj);
  proto->in_room = NOWHERE;
  proto->carried_by = NULL;
  proto->worn_by = NULL;
  proto->worn_on = NOWHERE;
  proto->in_obj = NULL;
  proto->contains = NULL;
  proto->next_content = NULL;
  proto->next = NULL;
  proto->sitting_here = NULL;
  proto->events = NULL;
  proto->script = NULL;
  proto->script_id = 0;

  if ((robj_num = add_object(proto, vnum)) == NOTHING) {
    free_object_strings(proto);
    free(proto);
    send_to_char(ch, "osave: failed to update object %d.\r\n", vnum);
    return;
  }

  free_object_strings(proto);
  free(proto);

  for (obj = object_list; obj; obj = obj->next) {
    if (obj->item_number != robj_num)
      continue;
    if (SCRIPT(obj))
      extract_script(obj, OBJ_TRIGGER);
    free_proto_script(obj, OBJ_TRIGGER);
    copy_proto_script(&obj_proto[robj_num], obj, OBJ_TRIGGER);
    assign_triggers(obj, OBJ_TRIGGER);
  }

  save_objects(znum);
  send_to_char(ch, "osave: object %d saved to disk.\r\n", vnum);
}

/* ====== Builder snapshot: save a staged mob's gear as its prototype loadout ====== */
static void msave_loadout_append(struct mob_loadout **head,
                                 struct mob_loadout **tail,
                                 obj_vnum vnum, sh_int wear_pos,
                                 int *eq_count, int *inv_count) {
  struct mob_loadout *e;
  CREATE(e, struct mob_loadout, 1);
  e->vnum = vnum;
  e->wear_pos = wear_pos;
  e->quantity = 1;
  e->next = NULL;

  if (*tail)
    (*tail)->next = e;
  else
    *head = e;
  *tail = e;

  if (wear_pos >= 0)
    (*eq_count)++;
  else
    (*inv_count)++;
}

static void msave_capture_obj_tree(struct mob_loadout **head,
                                   struct mob_loadout **tail,
                                   struct obj_data *obj, sh_int base_wear_pos,
                                   int depth, int *eq_count, int *inv_count) {
  sh_int wear_pos;
  struct obj_data *cont;

  if (!obj || GET_OBJ_VNUM(obj) <= 0)
    return;

  if (depth <= 0)
    wear_pos = base_wear_pos;
  else
    wear_pos = (sh_int)(-(depth + 1));

  msave_loadout_append(head, tail, GET_OBJ_VNUM(obj), wear_pos,
                       eq_count, inv_count);

  for (cont = obj->contains; cont; cont = cont->next_content)
    msave_capture_obj_tree(head, tail, cont, base_wear_pos, depth + 1,
                           eq_count, inv_count);
}

ACMD(do_msave)
{
  char a1[MAX_INPUT_LENGTH], a2[MAX_INPUT_LENGTH];
  char target[MAX_INPUT_LENGTH] = {0}, flags[MAX_INPUT_LENGTH] = {0};
  struct char_data *vict = NULL, *tmp = NULL;
  mob_rnum rnum;
  int include_inv = 0;      /* -all */
  int clear_first = 1;      /* default replace; -append flips this to 0 */
  int equips_added = 0, inv_entries = 0;
  int pos;
  struct obj_data *o;
  struct mob_loadout *lo_head = NULL;
  struct mob_loadout *lo_tail = NULL;

  two_arguments(argument, a1, a2);
  if (*a1 && *a1 == '-') {
    /* user wrote: msave -flags <mob> */
    strcpy(flags, a1);
    strcpy(target, a2);
  } else {
    /* user wrote: msave <mob> [-flags] */
    strcpy(target, a1);
    strcpy(flags, a2);
  }

  /* Parse flags (space-separated, any order) */
  if (*flags) {
    char buf[MAX_INPUT_LENGTH], *p = flags;
    while (*p) {
      p = one_argument(p, buf);
      if (!*buf) break;
      if (!str_cmp(buf, "-all")) include_inv = 1;
      else if (!str_cmp(buf, "-append")) clear_first = 0;
      else if (!str_cmp(buf, "-clear")) clear_first = 1;
      else {
        send_to_char(ch, "Unknown flag '%s'. Try -all, -append, or -clear.\r\n", buf);
        return;
      }
    }
  }

  /* Find target mob in the room */
  if (*target)
    vict = get_char_vis(ch, target, NULL, FIND_CHAR_ROOM);
  else {
    /* No name: pick the first NPC only if exactly one exists */
    for (tmp = world[IN_ROOM(ch)].people; tmp; tmp = tmp->next_in_room) {
      if (IS_NPC(tmp)) {
        if (vict) { vict = NULL; break; } /* more than one — force explicit name */
        vict = tmp;
      }
    }
  }

  if (!vict || !IS_NPC(vict)) {
    send_to_char(ch, "Target an NPC in this room: msave <mob> [-all] [-append|-clear]\r\n");
    return;
  }

  /* Resolve prototype and permission to edit its zone */
  rnum = GET_MOB_RNUM(vict);
  if (rnum < 0) {
    send_to_char(ch, "I can’t resolve that mob's prototype.\r\n");
    return;
  }

#ifdef CAN_EDIT_ZONE
  if (!can_edit_zone(ch, real_zone_by_thing(GET_MOB_VNUM(vict)))) {
    send_to_char(ch, "You don’t have permission to modify that mob’s zone.\r\n");
    return;
  }
#endif

  /* Build the new loadout into the PROTOTYPE */
  if (clear_first) {
    loadout_free_list(&mob_proto[rnum].proto_loadout);
    mob_proto[rnum].proto_loadout = NULL;
  }

  lo_head = mob_proto[rnum].proto_loadout;
  lo_tail = lo_head;
  while (lo_tail && lo_tail->next)
    lo_tail = lo_tail->next;

  /* Capture equipment: one entry per worn slot */
  for (pos = 0; pos < NUM_WEARS; pos++) {
    o = GET_EQ(vict, pos);
    if (!o) continue;
    if (GET_OBJ_VNUM(o) <= 0) continue;
    msave_capture_obj_tree(&lo_head, &lo_tail, o, (sh_int)pos, 0,
                           &equips_added, &inv_entries);
  }

  /* Capture inventory (with nesting) if requested */
  if (include_inv) {
    for (o = vict->carrying; o; o = o->next_content) {
      if (GET_OBJ_VNUM(o) <= 0) continue;
      msave_capture_obj_tree(&lo_head, &lo_tail, o, -1, 0,
                             &equips_added, &inv_entries);
    }
  }

  mob_proto[rnum].proto_loadout = lo_head;

  /* Persist to disk: save the zone owning this mob vnum */
  {
    zone_rnum zr = real_zone_by_thing(GET_MOB_VNUM(vict));
    if (zr == NOWHERE) {
      mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
             "msave: could not resolve zone for mob %d", GET_MOB_VNUM(vict));
      send_to_char(ch, "Saved in memory, but couldn’t resolve zone to write disk.\r\n");
    } else {
      save_mobiles(zr);
      send_to_char(ch,
        "Loadout saved for mob [%d]. Equipped: %d, Inventory lines: %d%s\r\n",
        GET_MOB_VNUM(vict), equips_added, inv_entries, include_inv ? "" : " (use -all to include inventory)");
      mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
             "msave: %s saved loadout for mob %d (eq=%d, inv=%d) in zone %d",
             GET_NAME(ch), GET_MOB_VNUM(vict), equips_added, inv_entries,
             zone_table[zr].number);
    }
  }

}

ACMD(do_rsave)
{
  room_rnum rnum;
  zone_rnum znum;
  int ok;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Mobiles can’t use this.\r\n");
    return;
  }

  /* IN_ROOM(ch) is already a room_rnum (index into world[]). Do NOT pass it to real_room(). */
  rnum = IN_ROOM(ch);

  if (rnum == NOWHERE || rnum < 0 || rnum > top_of_world) {
    send_to_char(ch, "You are not in a valid room.\r\n");
    return;
  }

  znum = world[rnum].zone;
  if (znum < 0 || znum > top_of_zone_table) {
    send_to_char(ch, "This room is not attached to a valid zone.\r\n");
    return;
  }

  /* Optional: permission check */
  if (!can_edit_zone(ch, znum)) {
    send_to_char(ch, "You don’t have permission to modify zone %d.\r\n",
                 zone_table[znum].number);
    return;
  }

  /* Save the owning zone's .wld file so the room data persists */
  ok = save_rooms(znum);
  if (ok)
    ok = RoomSave_now(rnum);

  if (!ok) {
    send_to_char(ch, "rsave: failed.\r\n");
    mudlog(BRF, GET_LEVEL(ch), TRUE,
           "RSAVE FAIL: %s room %d (rnum=%d) zone %d (znum=%d)",
           GET_NAME(ch), GET_ROOM_VNUM(rnum), rnum,
           zone_table[znum].number, znum);
    return;
  }

  send_to_char(ch, "rsave: room %d saved to world file for zone %d.\r\n",
               GET_ROOM_VNUM(rnum), zone_table[znum].number);

  mudlog(CMP, GET_LEVEL(ch), TRUE,
         "RSAVE OK: %s room %d (rnum=%d) -> world/wld/%d.wld",
         GET_NAME(ch), GET_ROOM_VNUM(rnum), rnum, zone_table[znum].number);
}

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

    /* Parse object header: O vnum timer weight cost unused */
    int vnum, timer, weight, cost, unused_cost;
    if (sscanf(line, "O %d %d %d %d %d", &vnum, &timer, &weight, &cost, &unused_cost) != 5)
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
    GET_OBJ_COST_PER_DAY(obj)   = 0;

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
    if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
      update_money_obj(obj);

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
O <vnum> <timer> <extra_flags> <wear_flags> <weight> <cost> <unused>
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
  if (rnum == NOWHERE || rnum < 0 || rnum > top_of_world) return 0;
  zone_rnum znum = world[rnum].zone;
  if (znum < 0 || znum > top_of_zone_table) return 0;
  return zone_table[znum].number; /* e.g., 1 for rooms 100–199, 2 for 200–299, etc. */
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
          GET_OBJ_COST_PER_DAY(obj));

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

  {
    int n = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (n < 0 || n >= (int)sizeof(tmp)) {
      mudlog(NRM, LVL_IMMORT, TRUE,
             "SYSERR: RoomSave: temp path too long for %s", path);
      return 0;
    }
  }

  if (!(out = fopen(tmp, "w"))) {
    mudlog(NRM, LVL_IMMORT, TRUE,
           "SYSERR: RoomSave: fopen(%s) failed: %s",
           tmp, strerror(errno));
    return 0;
  }

  if ((in = fopen(path, "r")) != NULL) {
    while (fgets(line, sizeof(line), in)) {
      if (strncmp(line, "#R ", 3) == 0) {
        int file_rvnum;
        long ts;
        if (sscanf(line, "#R %d %ld", &file_rvnum, &ts) == 2) {
          if (file_rvnum == (int)rvnum) {
            /* Skip old block completely until and including '.' line */
            while (fgets(line, sizeof(line), in)) {
              if (line[0] == '.') {
                /* consume it and break */
                break;
              }
            }
            continue; /* do NOT write skipped lines */
          }
        }
      }
      fputs(line, out); /* keep unrelated lines */
    }
    fclose(in);
  }

  /* Append new block */
  fprintf(out, "#R %d %ld\n", rvnum, (long)time(0));

  RS_write_room_mobs(out, rnum);

  for (struct obj_data *obj = world[rnum].contents; obj; obj = obj->next_content)
    write_one_object(out, obj);

  /* Always terminate block */
  fprintf(out, ".\n");

  if (fclose(out) != 0) {
    mudlog(NRM, LVL_IMMORT, TRUE,
           "SYSERR: RoomSave: fclose(%s) failed: %s",
           tmp, strerror(errno));
    return 0;
  }
  if (rename(tmp, path) != 0) {
    mudlog(NRM, LVL_IMMORT, TRUE,
           "SYSERR: RoomSave: rename(%s -> %s) failed: %s",
           tmp, path, strerror(errno));
    return 0;
  }

  return 1;
}

/* --- M/E/G/P load helpers (mob restore) -------------------------------- */

struct rs_load_ctx {
  room_rnum rnum;
  struct char_data *cur_mob;      /* last mob spawned by 'M' */
  struct obj_data  *stack[16];    /* container stack by depth for 'P' */
  int saw_inventory;             /* saw any inventory lines (G/P from inventory) */
  int last_item_was_inventory;   /* last item source was inventory (G) */
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

static void RS_apply_inventory_loadout(struct char_data *mob) {
  mob_rnum rnum;
  const struct mob_loadout *e;
  struct obj_data *stack[16];
  int i;

  if (!mob || !IS_NPC(mob)) return;
  rnum = GET_MOB_RNUM(mob);
  if (rnum < 0) return;

  for (i = 0; i < (int)(sizeof(stack) / sizeof(stack[0])); i++)
    stack[i] = NULL;

  for (e = mob_proto[rnum].proto_loadout; e; e = e->next) {
    int qty, n;
    if (e->wear_pos >= 0)
      continue;
    qty = (e->quantity > 0) ? e->quantity : 1;
    for (n = 0; n < qty; n++) {
      struct obj_data *obj = RS_create_obj_by_vnum(e->vnum);
      if (!obj) {
        log("SYSERR: RS_apply_inventory_loadout: bad obj vnum %d on mob %d",
            e->vnum, GET_MOB_VNUM(mob));
        continue;
      }
      if (e->wear_pos == -1) {
        for (i = 0; i < (int)(sizeof(stack) / sizeof(stack[0])); i++)
          stack[i] = NULL;
        obj_to_char(obj, mob);
        if (obj_is_storage(obj) || GET_OBJ_TYPE(obj) == ITEM_FURNITURE)
          stack[0] = obj;
        continue;
      }

      {
        int depth = -(e->wear_pos) - 1;
        if (depth <= 0 ||
            depth >= (int)(sizeof(stack) / sizeof(stack[0])) ||
            !stack[depth - 1]) {
          obj_to_char(obj, mob);
          continue;
        }
        obj_to_obj(obj, stack[depth - 1]);
        if (obj_is_storage(obj) || GET_OBJ_TYPE(obj) == ITEM_FURNITURE) {
          stack[depth] = obj;
          for (i = depth + 1; i < (int)(sizeof(stack) / sizeof(stack[0])); i++)
            stack[i] = NULL;
        }
      }
    }
  }
}

static void RS_finalize_mob_loadout(struct rs_load_ctx *ctx) {
  if (!ctx || !ctx->cur_mob)
    return;
  if (!ctx->saw_inventory)
    RS_apply_inventory_loadout(ctx->cur_mob);
}


/* Reset the loader context before reading a new #R block */
static void RS_ctx_clear(struct rs_load_ctx *ctx) {
  if (!ctx)
    return;

  /* DO NOT reset ctx->rnum — each #R block sets this explicitly
   * before parsing mobs or objects. Resetting it causes cross-room
   * bleed (e.g., mobs from one room spawning in another).
   */

  ctx->cur_mob = NULL;
  ctx->saw_inventory = 0;
  ctx->last_item_was_inventory = 0;

  /* Clear all container stack pointers */
  for (int i = 0; i < 16; i++)
    ctx->stack[i] = NULL;
}

/* Optional autosave hook (invoked by limits.c:point_update). */
void RoomSave_autosave_tick(void) {
  /* Iterate all rooms; only save flagged ones. */
  for (room_rnum rnum = 0; rnum <= top_of_world; ++rnum) {
    if (ROOM_FLAGGED(rnum, ROOM_SAVE))
      RoomSave_now(rnum);
  }
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
    case 'M': {
      mob_vnum mv;
      if (sscanf(line+1, " %d", (int *)&mv) != 1) return 0;

      RS_finalize_mob_loadout(ctx);
      ctx->cur_mob = RS_create_mob_by_vnum(mv);
      if (!ctx->cur_mob) return 1;

      /* Place in the block's room */
      char_to_room(ctx->cur_mob, ctx->rnum);

      /* Safety: if anything put it elsewhere, force it back */
      if (IN_ROOM(ctx->cur_mob) != ctx->rnum)
        char_to_room(ctx->cur_mob, ctx->rnum);

      RS_stack_clear(ctx); /* clear only container stack */
      ctx->saw_inventory = 0;
      ctx->last_item_was_inventory = 0;
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
      ctx->last_item_was_inventory = 0;
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
      ctx->saw_inventory = 1;
      ctx->last_item_was_inventory = 1;
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
      if (ctx->last_item_was_inventory)
        ctx->saw_inventory = 1;
      return 1;
    }

    default:
      return 0;
  }
}

/* Forward decls for mob restore helpers */
static void RS_stack_clear(struct rs_load_ctx *ctx);

void RoomSave_boot(void)
{
  DIR *dirp;
  struct dirent *dp;

  ensure_dir_exists(ROOMSAVE_PREFIX);

  dirp = opendir(ROOMSAVE_PREFIX);
  if (!dirp) {
    mudlog(NRM, LVL_IMMORT, TRUE,
           "SYSERR: RoomSave_boot: cannot open %s", ROOMSAVE_PREFIX);
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
        mudlog(NRM, LVL_IMMORT, TRUE,
               "SYSERR: RoomSave_boot: path too long: %s%s",
               ROOMSAVE_PREFIX, dp->d_name);
        continue;
      }

      FILE *fl = fopen(path, "r");
      if (!fl) {
        mudlog(NRM, LVL_IMMORT, TRUE,
               "SYSERR: RoomSave_boot: fopen(%s) failed: %s",
               path, strerror(errno));
        continue;
      }

      log("RoomSave: reading %s", path);

      int blocks = 0;
      int restored_objs_total = 0;
      int restored_mobs_total = 0;

      /* Outer loop: read every #R block in this .rsv file */
      char line[512];
      while (fgets(line, sizeof(line), fl)) {

        /* Skip until a valid #R header */
        if (strncmp(line, "#R ", 3) != 0)
          continue;

        /* Parse header line */
        int rvnum; long ts;
        if (sscanf(line, "#R %d %ld", &rvnum, &ts) != 2) {
          mudlog(NRM, LVL_IMMORT, TRUE,
                 "RoomSave: malformed #R header in %s: %s", path, line);
          /* Skip malformed block */
          while (fgets(line, sizeof(line), fl))
            if (line[0] == '.') break;
          continue;
        }

        blocks++;

        /* Resolve the room for this block */
        room_rnum rnum = real_room((room_vnum)rvnum);
        if (rnum == NOWHERE) {
          mudlog(NRM, LVL_IMMORT, FALSE,
                 "RoomSave: unknown room vnum %d in %s (skipping)",
                 rvnum, path);
          /* Skip to next block */
          while (fgets(line, sizeof(line), fl))
            if (line[0] == '.') break;
          continue;
        }

        /* Clear this room's ground contents before restoring */
        while (world[rnum].contents)
          extract_obj(world[rnum].contents);

        /* Clear and set mob context for this block */
        struct rs_load_ctx mctx;
        RS_ctx_clear(&mctx);
        mctx.rnum = rnum;

        /* Per-block counts */
        int count_objs = 0, count_mobs = 0;
        char inner[512];

        /* Inner loop: read this #R block until '.' */
        while (fgets(inner, sizeof(inner), fl)) {

          /* Trim spaces */
          while (inner[0] == ' ' || inner[0] == '\t')
            memmove(inner, inner + 1, strlen(inner));

          /* Stop at end of block */
          if (inner[0] == '.')
            break;

          /* Defensive: stop if another #R starts (malformed file) */
          if (!strncmp(inner, "#R ", 3)) {
            fseek(fl, -((long)strlen(inner)), SEEK_CUR);
            break;
          }

          /* Handle object blocks */
          if (inner[0] == 'O') {
            long pos = ftell(fl);
            fseek(fl, pos - strlen(inner), SEEK_SET);
            struct obj_data *list = roomsave_read_list(fl);
            for (struct obj_data *it = list, *next; it; it = next) {
              next = it->next_content;
              it->next_content = NULL;
              obj_to_room(it, rnum);
              count_objs++;
            }
            continue;
          }

          /* Handle mob & equipment/inventory */
          if (RS_parse_mob_line(&mctx, inner)) {
            if (inner[0] == 'M')
              count_mobs++;
            continue;
          }

          /* Unknown token: ignore gracefully */
        }

        RS_finalize_mob_loadout(&mctx);

        restored_objs_total += count_objs;
        restored_mobs_total += count_mobs;

        if (count_mobs > 0)
          log("RoomSave: room %d <- %d object(s) and %d mob(s)",
              rvnum, count_objs, count_mobs);
        else
          log("RoomSave: room %d <- %d object(s)", rvnum, count_objs);
      }

      log("RoomSave: finished %s (blocks=%d, objects=%d, mobs=%d)",
          path, blocks, restored_objs_total, restored_mobs_total);

      fclose(fl);
    }
  }

  closedir(dirp);
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
