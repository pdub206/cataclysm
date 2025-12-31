/**
* @file rset.c
* Room creation/configuration and utility routines.
* 
* This set of code was not originally part of the circlemud distribution.
*/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "genolc.h"
#include "genwld.h"
#include "genzon.h"
#include "oasis.h"
#include "improved-edit.h"
#include "modify.h"

#include "rset.h"

static void rset_show_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Usage:\r\n"
    "  rset show                     - show editable fields + current values\r\n"
    "  rset set  <field> <value>     - set a scalar field\r\n"
    "  rset add  <field> <value...>  - add to a list field\r\n"
    "  rset del  <field> <value...>  - remove from a list field\r\n"
    "  rset desc                     - edit long text (main description)\r\n"
    "  rset clear                    - clears all fields\r\n"
    "  rset validate                 - run validation checks\r\n"
    "\r\n"
    "Type:\r\n"
    "  rset set\r\n"
    "  rset add\r\n"
    "  rset del\r\n"
    "\r\n"
    "For command-specific options.\r\n");
}

static void rset_show_set_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Sets specific configuration to the room.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset set name <text>                   - add a room name\r\n"
    "  rset set sector <sector>               - add terrain/sector type\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset set name \"A wind-scoured alley\"\r\n"
    "  rset set sector desert\r\n");
}

static void rset_show_set_sector_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Sets room sector type.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset set sector <sector>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset set sector desert\r\n"
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
    "  rset add flags:\r\n"
    "    <flag> [flag ...]\r\n"
    "  rset add exit <direction> <room number>\r\n"
    "  rset add door <direction> <name of door>\r\n"
    "  rset add key <direction> <key number>\r\n"
    "  rset add hidden <direction>\r\n"
    "  rset add forage <object vnum> <dc check>\r\n"
    "  rset add edesc <keyword> <description>\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset add flags INDOORS QUITSAFE\r\n"
    "  rset add exit n 101\r\n"
    "  rset add door n door\r\n"
    "  rset add key n 201\r\n"
    "  rset add hidden n\r\n"
    "  rset add forage 301 15\r\n"
    "  rset add edesc mosaic A beautiful mosaic is here on the wall.\r\n");
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

static void rset_show_clear_usage(struct char_data *ch)
{
  send_to_char(ch,
    "Clears all configuration from a room to start over fresh.\r\n"
    "\r\n"
    "Usage:\r\n"
    "  rset clear\r\n"
    "\r\n"
    "Examples:\r\n"
    "  rset clear\r\n");
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
    "  rset desc\r\n");
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

    if (exit->key != NOTHING) {
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

  if (is_abbrev(arg1, "set")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      rset_show_set_usage(ch);
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

    {
      int flag;

      flag = rset_find_room_flag(arg2);
      if (flag >= 0) {
        SET_BIT_AR(room->room_flags, flag);
        rset_mark_room_modified(rnum);
        send_to_char(ch, "Room flag set.\r\n");
        return;
      }
    }

    rset_show_set_usage(ch);
    return;
  }

  if (is_abbrev(arg1, "add")) {
    argument = one_argument(argument, arg2);
    if (!*arg2) {
      rset_show_add_usage(ch);
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
        send_to_char(ch, "Usage: rset add forage <object vnum> <dc check>\r\n");
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
        send_to_char(ch, "Usage: rset del forage <object vnum>\r\n");
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
    if (*argument) {
      rset_show_clear_usage(ch);
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
