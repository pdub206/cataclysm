/**************************************************************************
*  File: act.other.c                                       Part of tbaMUD *
*  Usage: Miscellaneous player-level commands.                             *
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
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "mail.h"  /* for has_mail() */
#include "shop.h"
#include "quest.h"
#include "modify.h"
#include "roomsave.h"

/* Local defined utility functions */
/* do_group utility functions */
static void print_group(struct char_data *ch);
static void display_group_list(struct char_data * ch);

ACMD(do_quit)
{
  char first[MAX_INPUT_LENGTH];
  char *rest;

  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Parse optional "ooc" sub-arg: quit ooc <message> */
  rest = (char *)argument;
  skip_spaces(&rest);
  rest = one_argument(rest, first);
  bool quit_ooc = (*first && is_abbrev(first, "ooc")) ? TRUE : FALSE;

  /* Keep original safety controls */
  if (!quit_ooc && subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL);
  }
  /* New: normal quit must be in a QUIT room (mortals only). */
  else if (!quit_ooc && GET_LEVEL(ch) < LVL_IMMORT &&
           !ROOM_FLAGGED(IN_ROOM(ch), ROOM_QUIT)) {
    send_to_char(ch, "You cannot quit here. Find a room marked [Quit].\r\n");
  }
  /* For quit ooc, require a message for staff context. */
  else if (quit_ooc) {
    skip_spaces(&rest);
    if (!*rest) {
      send_to_char(ch, "Usage must include a reason to quit ooc: quit ooc <message>\r\n");
      return;
    }

    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s used QUIT OOC in room %d: %s",
           GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), rest);

    if (GET_QUEST_TIME(ch) != -1)
      quest_timeout(ch);

    send_to_char(ch, "You step out-of-character and leave the world...\r\n");

    /* We used to check here for duping attempts, but we may as well do it right
     * in extract_char(), since there is no check if a player rents out and it
     * can leave them in an equally screwy situation. */

    if (CONFIG_FREE_RENT)
      Crash_rentsave(ch, 0);

    /* Requirement: respawn in the same (possibly non-QUIT) room. */
    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    /* Stop snooping so you can't see passwords during deletion or change. */
    if (ch->desc->snoop_by) {
      write_to_output(ch->desc->snoop_by, "Your victim is no longer among us.\r\n");
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }

    extract_char(ch);   /* Char is saved before extracting. */
  }
  else {
    /* Normal quit (in a QUIT room, or immortal bypass) */
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s has quit the game.", GET_NAME(ch));

    if (GET_QUEST_TIME(ch) != -1)
      quest_timeout(ch);

    send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");

    /* We used to check here for duping attempts, but we may as well do it right
     * in extract_char(), since there is no check if a player rents out and it
     * can leave them in an equally screwy situation. */

    if (CONFIG_FREE_RENT)
      Crash_rentsave(ch, 0);

    /* Requirement: respawn in the same QUIT room they logged out in. */
    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    /* Stop snooping so you can't see passwords during deletion or change. */
    if (ch->desc->snoop_by) {
      write_to_output(ch->desc->snoop_by, "Your victim is no longer among us.\r\n");
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }

    extract_char(ch);   /* Char is saved before extracting. */
  }
}

ACMD(do_save)
{
  char a1[MAX_INPUT_LENGTH], a2[MAX_INPUT_LENGTH];

  /* Parse up to two words so we can accept "save room" or "room save". */
  two_arguments(argument, a1, a2);

  /* Does either token equal "room"? (order-agnostic) */
  const bool wants_room = ((*a1 && !str_cmp(a1, "room")) ||
                           (*a2 && !str_cmp(a2, "room")));

  if (wants_room) {
    room_rnum rnum = IN_ROOM(ch);

    if (rnum == NOWHERE) {
      send_to_char(ch, "You're not in a valid room.\r\n");
      return;
    }

    /* Not a SAVE room? Fall back to normal character save semantics. */
    if (!ROOM_FLAGGED(rnum, ROOM_SAVE)) {
      send_to_char(ch, "Saving %s.\r\n", GET_NAME(ch));
      save_char(ch);
      Crash_crashsave(ch);   /* keep whatever your tree normally calls */
      return;
    }

    /* Room is flagged SAVE → persist its contents */
    if (RoomSave_now(rnum)) {
      send_to_char(ch, "Saving room.\r\n");
    } else {
      send_to_char(ch, "Room save failed; see logs.\r\n");
    }
    return;
  }

  /* No "room" token present → normal character save */
  send_to_char(ch, "Saving %s.\r\n", GET_NAME(ch));
  save_char(ch);
  Crash_crashsave(ch);
}

/* Generic function for commands which are normally overridden by special
 * procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}

ACMD(do_sneak)
{
  struct affected_type af;
  int rolla, rollb, roll, bonus, total, dc;
  bool disadv = FALSE;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SNEAK)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (FIGHTING(ch)){
    send_to_char(ch, "While fighting!?\r\n");
    return; 
  }    /* you can't sneak while in active melee */

  send_to_char(ch, "Okay, you'll try to move silently for a while.\r\n");

  /* Remove prior sneak affect if present (refresh logic) */
  if (AFF_FLAGGED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  /* --- 5e-style Stealth check (DEX + proficiency) --- */
  bonus = GET_ABILITY_MOD(GET_DEX(ch)) +
          GET_PROFICIENCY(GET_SKILL(ch, SKILL_SNEAK));

  dc = 10;

  disadv = has_stealth_disadv(ch) ? TRUE : FALSE;

  rolla = rand_number(1, 20);
  if (disadv) {
    rollb = rand_number(1, 20);
    roll  = MIN(rolla, rollb);                  /* disadvantage: take lower roll */
  } else {
    roll  = rolla;
  }

  total = roll + bonus;

  if (total < dc) {
    gain_skill(ch, "sneak", FALSE);
    WAIT_STATE(ch, PULSE_VIOLENCE / 2);
    GET_MOVE(ch) -= 10;
    return;
  }

  /* Success: apply Sneak affect */
  new_affect(&af);
  af.spell    = SKILL_SNEAK;
  af.location = APPLY_NONE;
  af.modifier = 0;
  af.duration = GET_LEVEL(ch);               /* keep stock duration; adjust if desired */
  memset(af.bitvector, 0, sizeof(af.bitvector));
  SET_BIT_AR(af.bitvector, AFF_SNEAK);
  affect_to_char(ch, &af);

  /* Store a stealth check value for movement contests (reuse Hide’s field) */
  /* If you’ve already hidden with a higher roll, keep the stronger value. */
  SET_STEALTH_CHECK(ch, MAX(GET_STEALTH_CHECK(ch), total));

  gain_skill(ch, "sneak", TRUE);
  GET_MOVE(ch) -= 10;
}

ACMD(do_hide)
{
  int rolla, rollb, roll, bonus, total, dc;
  bool disadv = FALSE;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDE)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  send_to_char(ch, "You attempt to hide yourself.\r\n");

  /* If already hidden, drop it before re-attempting */
  if (AFF_FLAGGED(ch, AFF_HIDE)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    GET_STEALTH_CHECK(ch) = 0;
  }

  if (FIGHTING(ch)){
    send_to_char(ch, "While fighting!?\r\n");
    return; 
  }    /* you can't hide while in active melee */

  /* --- 5e Stealth (DEX) ability check --- */
  bonus = GET_ABILITY_MOD(GET_DEX(ch)) + GET_PROFICIENCY(GET_SKILL(ch, SKILL_HIDE));

  /* Baseline difficulty: hiding in general */
  /* TODO: Maybe change dc based on terrain/populated rooms in the future */
  dc = 10;

  /* Armor/gear can impose disadvantage */
  disadv = has_stealth_disadv(ch) ? TRUE : FALSE;

  rolla = rand_number(1, 20);
  if (disadv) {
    rollb = rand_number(1, 20);
    roll  = MIN(rolla, rollb);      /* disadvantage: take the lower */
  } else {
    roll  = rolla;
  }

  total = roll + bonus;

  if (total < dc) {
    /* Failure */
    gain_skill(ch, "hide", FALSE);
    WAIT_STATE(ch, PULSE_VIOLENCE / 2);
    GET_MOVE(ch) -= 10;
    return;
  }

  /* Success: set flag and store this specific Stealth result */
  SET_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
  GET_STEALTH_CHECK(ch) = total;

  send_to_char(ch, "You hide yourself as best you can.\r\n");
  gain_skill(ch, "hide", TRUE);
  WAIT_STATE(ch, PULSE_VIOLENCE / 2);
  GET_MOVE(ch) -= 10;
}

/* Perception: scan the room for hidden creatures and objects */
ACMD(do_perception)
{
  struct char_data *tch;
  struct obj_data  *obj, *next_obj;
  int roll, bonus, total;
  int found_chars = 0, found_objs = 0;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_PERCEPTION)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  /* Roll once for this scan (active check) */
  bonus = GET_ABILITY_MOD(GET_WIS(ch)) +
          GET_PROFICIENCY(GET_SKILL(ch, SKILL_PERCEPTION));

  roll   = rand_number(1, 20);
  total = roll + bonus;

  /* Optional: it’s harder to actively scan while in melee */
  if (FIGHTING(ch))
    total -= 4;

  /* --- Scan characters in the room (PCs & NPCs) --- */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
    if (!AFF_FLAGGED(tch, AFF_HIDE))
      continue;

    /* Safety default if some legacy code set AFF_HIDE without a stored check */
    if (GET_STEALTH_CHECK(tch) <= 0)
      SET_STEALTH_CHECK(tch, 5);

    if (total >= GET_STEALTH_CHECK(tch)) {
      /* Spotted! Reveal them. */
      REMOVE_BIT_AR(AFF_FLAGS(tch), AFF_HIDE);
      SET_STEALTH_CHECK(tch, 0);
      ++found_chars;

      act("You spot $N hiding!", FALSE, ch, 0, tch, TO_CHAR);
      act("$n seems to look right at you — you've been spotted!", FALSE, ch, 0, tch, TO_VICT);
      act("$n spots $N hiding nearby!", FALSE, ch, 0, tch, TO_NOTVICT);
    }
  }

  /* --- Scan objects in the room (requires an ITEM_HIDDEN extra flag) --- */
  for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj) {
    next_obj = obj->next_content;

    /* If you don't have ITEM_HIDDEN yet, add it to your extra flags table and OBJ flag names. */
#ifdef ITEM_HIDDEN
    if (OBJ_FLAGGED(obj, ITEM_HIDDEN)) {
      /* Simple baseline DC for hidden objects; tune as desired or add per-object difficulty. */
      int obj_dc = 12;
      if (FIGHTING(ch)) obj_dc += 2;

      if (total >= obj_dc) {
        /* Reveal the object. */
        REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_HIDDEN);
        ++found_objs;

        act("You spot $p tucked out of sight.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n notices $p tucked out of sight.", FALSE, ch, obj, 0, TO_ROOM);
      }
    }
#endif
  }

  if (!found_chars && !found_objs) {
    send_to_char(ch, "You search carefully but don’t uncover anything hidden.\r\n");
    gain_skill(ch, "perception", FALSE);
  } else {
    gain_skill(ch, "perception", TRUE);
  }

  /* Small action taxes do disincline players from spamming perception */
  WAIT_STATE(ch, PULSE_VIOLENCE / 2);
  GET_MOVE(ch) -= 10;
}

ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STEAL)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Steal what from who?\r\n");
    return;
  } else if (vict == ch) {
    send_to_char(ch, "Come on now, that's rather stupid!\r\n");
    return;
  }

  /* 101% is a complete failure */
  percent = rand_number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;		/* ALWAYS SUCCESS, unless heavy object. */

  if (!CONFIG_PT_ALLOWED && !IS_NPC(vict))
    pcsteal = 1;

  if (!AWAKE(vict))	/* Easier to steal from sleeping people. */
    percent -= 50;

  /* No stealing if not allowed. If it is no stealing from Imm's or Shopkeepers. */
  if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal || GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;		/* Failure */

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

    if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char(ch, "Steal the equipment now?  Impossible!\r\n");
	  return;
	} else {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj) ) {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {			/* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */

      if (percent > GET_SKILL(ch, SKILL_STEAL)) {
        ohoh = TRUE;
        send_to_char(ch, "Oops..\r\n");
        act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
        act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
        gain_skill(ch, "steal", FALSE);
      } else {			/* Steal the item */
          if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
            if (!give_otrigger(obj, vict, ch) || !receive_mtrigger(ch, vict, obj) ) {
              send_to_char(ch, "Impossible!\r\n");
              return;
          }
	  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char(ch, "Got it!\r\n");
	  }
	} else
	  send_to_char(ch, "You cannot carry that much.\r\n");
      }
    }
  } else {			/* Steal some coins */
    if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
      ohoh = TRUE;
      send_to_char(ch, "Oops..\r\n");
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      gain_skill(ch, "steal", FALSE);
    } else {
      /* Steal some gold coins */
      gold = (GET_GOLD(vict) * rand_number(1, 10)) / 100;
      gold = MIN(1782, gold);
      if (gold > 0) {
        increase_gold(ch, gold);
        decrease_gold(vict, gold);
        gain_skill(ch, "steal", TRUE);
        if (gold > 1)
          send_to_char(ch, "Bingo!  You got %d gold coins.\r\n", gold);
        else
          send_to_char(ch, "You manage to swipe a solitary gold coin.\r\n");
          } else {
      send_to_char(ch, "You couldn't get any gold...\r\n");
          }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}

ACMD(do_skills)
{

  if (IS_NPC(ch))
    return;

  list_skills(ch);

}

ACMD(do_visible)
{
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  } else
    send_to_char(ch, "You are already visible.\r\n");
}

ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);
  parse_at(argument);

  if (IS_NPC(ch))
    send_to_char(ch, "Your title is fine... go away.\r\n");
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char(ch, "You can't title yourself -- you shouldn't have abused it!\r\n");
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
  else if (strlen(argument) > MAX_TITLE_LENGTH)
    send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
  else {
    set_title(ch, argument);
    send_to_char(ch, "Okay, you're now %s%s%s.\r\n", GET_NAME(ch), *GET_TITLE(ch) ? " " : "", GET_TITLE(ch));
  }
}

static void print_group(struct char_data *ch)
{
  struct char_data * k;

  send_to_char(ch, "Your group consists of:\r\n");

  while ((k = (struct char_data *) simple_list(ch->group->members)) != NULL)
    send_to_char(ch, "%-*s: %s[%4d/%-4d]H [%4d/%-4d]M [%4d/%-4d]V%s\r\n",
	    count_color_chars(GET_NAME(k))+22, GET_NAME(k), 
	    GROUP_LEADER(GROUP(ch)) == k ? CBGRN(ch, C_NRM) : CCGRN(ch, C_NRM),
	    GET_HIT(k), GET_MAX_HIT(k),
	    GET_MANA(k), GET_MAX_MANA(k),
	    GET_MOVE(k), GET_MAX_MOVE(k),
	    CCNRM(ch, C_NRM));
}

static void display_group_list(struct char_data * ch)
{
  struct group_data * group;
  int count = 0;
	
  if (group_list->iSize) {
    send_to_char(ch, "#   Group Leader     # of Members    In Zone\r\n"
                     "---------------------------------------------------\r\n");
		
    while ((group = (struct group_data *) simple_list(group_list)) != NULL) {
			if (IS_SET(GROUP_FLAGS(group), GROUP_NPC))
			  continue;
      if (GROUP_LEADER(group) && !IS_SET(GROUP_FLAGS(group), GROUP_ANON))
        send_to_char(ch, "%-2d) %s%-12s     %-2d              %s%s\r\n", 
          ++count,
          IS_SET(GROUP_FLAGS(group), GROUP_OPEN) ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM), 
          GET_NAME(GROUP_LEADER(group)), group->members->iSize, zone_table[world[IN_ROOM(GROUP_LEADER(group))].zone].name,
          CCNRM(ch, C_NRM));
      else
        send_to_char(ch, "%-2d) Hidden\r\n", ++count);
				
		}
  }
  if (count)
    send_to_char(ch, "\r\n"
                     "%sSeeking Members%s\r\n"
                     "%sClosed%s\r\n", 
                     CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
  else
    send_to_char(ch, "\r\n"
                     "Currently no groups formed.\r\n");
}

/* Vatiken's Group System: Version 1.1 */
ACMD(do_group)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;

  argument = one_argument(argument, buf);

  if (!*buf) {
    if (GROUP(ch))
      print_group(ch);
    else
      send_to_char(ch, "You must specify a group option, or type HELP GROUP for more info.\r\n");
    return;
  }
  
  if (is_abbrev(buf, "new")) {
    if (GROUP(ch))
      send_to_char(ch, "You are already in a group.\r\n");
    else
      create_group(ch);
  } else if (is_abbrev(buf, "list"))
    display_group_list(ch);
  else if (is_abbrev(buf, "join")) {
    skip_spaces(&argument);
    if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Join who?\r\n");
      return;
    } else if (vict == ch) {
      send_to_char(ch, "That would be one lonely grouping.\r\n");
      return;
    } else if (GROUP(ch)) {
      send_to_char(ch, "But you are already part of a group.\r\n");
      return;
    } else if (!GROUP(vict)) {
      act("$E$u is not part of a group!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    } else if (!IS_SET(GROUP_FLAGS(GROUP(vict)), GROUP_OPEN)) {
      send_to_char(ch, "That group isn't accepting members.\r\n");
      return;
    }   
    join_group(ch, GROUP(vict)); 
  } else if (is_abbrev(buf, "kick")) {
    skip_spaces(&argument);
    if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Kick out who?\r\n");
      return;
    } else if (vict == ch) {
      send_to_char(ch, "There are easier ways to leave the group.\r\n");
      return;
    } else if (!GROUP(ch) ) {
      send_to_char(ch, "But you are not part of a group.\r\n");
      return;
    } else if (GROUP_LEADER(GROUP(ch)) != ch ) {
      send_to_char(ch, "Only the group's leader can kick members out.\r\n");
      return;
    } else if (GROUP(vict) != GROUP(ch)) {
      act("$E$u is not a member of your group!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    } 
    send_to_char(ch, "You have kicked %s out of the group.\r\n", GET_NAME(vict));
    send_to_char(vict, "You have been kicked out of the group.\r\n"); 
    leave_group(vict);
  } else if (is_abbrev(buf, "regroup")) {
    if (!GROUP(ch)) {
      send_to_char(ch, "But you aren't part of a group!\r\n");
      return;
    }
    vict = GROUP_LEADER(GROUP(ch));
    if (ch == vict) {
      send_to_char(ch, "You are the group leader and cannot re-group.\r\n");
    } else {
      leave_group(ch);
      join_group(ch, GROUP(vict));
    }
  } else if (is_abbrev(buf, "leave")) {
    
    if (!GROUP(ch)) {
      send_to_char(ch, "But you aren't part of a group!\r\n");
      return;
    }
		
    leave_group(ch);
  } else if (is_abbrev(buf, "option")) {
    skip_spaces(&argument);
    if (!GROUP(ch)) {
      send_to_char(ch, "But you aren't part of a group!\r\n");
      return;
    } else if (GROUP_LEADER(GROUP(ch)) != ch) {
      send_to_char(ch, "Only the group leader can adjust the group flags.\r\n");
      return;
    }
    if (is_abbrev(argument, "open")) {
      TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN);
      send_to_char(ch, "The group is now %s to new members.\r\n", IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN) ? "open" : "closed");
    } else if (is_abbrev(argument, "anonymous")) {
      TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_ANON);
      send_to_char(ch, "The group location is now %s to other players.\r\n", IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_ANON) ? "invisible" : "visible");
    } else 
      send_to_char(ch, "The flag options are: Open, Anonymous\r\n");
  } else {
    send_to_char(ch, "You must specify a group option, or type HELP GROUP for more info.\r\n");		
  }

}

ACMD(do_report)
{
  struct group_data *group;

  if ((group = GROUP(ch)) == NULL) {
    send_to_char(ch, "But you are not a member of any group!\r\n");
    return;
  }

  send_to_group(NULL, group, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	  GET_NAME(ch),
	  GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));
}

ACMD(do_split)
{
  char buf[MAX_INPUT_LENGTH];
  int amount, num = 0, share, rest;
  size_t len;
  struct char_data *k;
  
  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char(ch, "Sorry, you can't do that.\r\n");
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char(ch, "You don't seem to have that much gold to split.\r\n");
      return;
    }
    
    if (GROUP(ch))
      while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
        if (IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k))
          num++;

    if (num && GROUP(ch)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char(ch, "With whom do you wish to share your gold?\r\n");
      return;
    }

    decrease_gold(ch, share * (num - 1));

    /* Abusing signed/unsigned to make sizeof work. */
    len = snprintf(buf, sizeof(buf), "%s splits %d coins; you receive %d.\r\n",
		GET_NAME(ch), amount, share);
    if (rest && len < sizeof(buf)) {
      snprintf(buf + len, sizeof(buf) - len,
		"%d coin%s %s not splitable, so %s keeps the money.\r\n", rest,
		(rest == 1) ? "" : "s", (rest == 1) ? "was" : "were", GET_NAME(ch));
    }

    while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
      if (k != ch && IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k)) {
	      increase_gold(k, share);
	      send_to_char(k, "%s", buf);
			}

    send_to_char(ch, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);

    if (rest) {
      send_to_char(ch, "%d coin%s %s not splitable, so you keep the money.\r\n",
		rest, (rest == 1) ? "" : "s", (rest == 1) ? "was" : "were");
    }
  } else {
    send_to_char(ch, "How many coins do you wish to split with your group?\r\n");
    return;
  }
}

ACMD(do_use)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	return;
      }
      break;
    case SCMD_USE:
      send_to_char(ch, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      /* SYSERR_DESC: This is the same as the unhandled case in do_gen_ps(),
       * but in the function which handles 'quaff', 'recite', and 'use'. */
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char(ch, "You can only quaff potions.\r\n");
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char(ch, "You can't seem to figure out how to use it.\r\n");
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters don't need displays.  Go away.\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: prompt { { H | M | V } | all | auto | none }\r\n");
    return;
  }

  if (!str_cmp(argument, "auto")) {
    TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPAUTO);
    send_to_char(ch, "Auto prompt %sabled.\r\n", PRF_FLAGGED(ch, PRF_DISPAUTO) ? "en" : "dis");
    return;
  }

  if (!str_cmp(argument, "on") || !str_cmp(argument, "all")) {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
  } else if (!str_cmp(argument, "off") || !str_cmp(argument, "none")) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
  } else {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);

    for (i = 0; i < strlen(argument); i++) {
      switch (LOWER(argument[i])) {
      case 'h':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
	break;
      case 'm':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
	break;
      case 'v':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
	break;
      default:
	send_to_char(ch, "Usage: prompt { { H | M | V } | all | auto | none }\r\n");
	return;
      }
    }
  }

  send_to_char(ch, "%s", CONFIG_OK);
}

#define TOG_OFF 0
#define TOG_ON  1
ACMD(do_gen_tog)
{
  long result;
  int i;
  char arg[MAX_INPUT_LENGTH];

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear shouts.\r\n",
    "You are now deaf to shouts.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
    "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
    "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
    "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
    "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
    "Autoexits enabled.\r\n"},
    {"Will no longer track through doors.\r\n",
    "Will now track through doors.\r\n"},
    {"Will no longer clear screen in OLC.\r\n",
    "Will now clear screen in OLC.\r\n"},
    {"Buildwalk Off.\r\n",
    "Buildwalk On.\r\n"},
    {"AFK flag is now off.\r\n",
    "AFK flag is now on.\r\n"},
    {"Autoloot disabled.\r\n",
    "Autoloot enabled.\r\n"},
    {"Autogold disabled.\r\n",
    "Autogold enabled.\r\n"},
    {"Autosplit disabled.\r\n",
    "Autosplit enabled.\r\n"},
    {"Autoassist disabled.\r\n",
    "Autoassist enabled.\r\n"},
    {"Automap disabled.\r\n",
    "Automap enabled.\r\n"},
    {"Autokey disabled.\r\n",
    "Autokey enabled.\r\n"},
    {"Autodoor disabled.\r\n",
    "Autodoor enabled.\r\n"},
    {"ZoneResets disabled.\r\n",
    "ZoneResets enabled.\r\n"}
  };

  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOSHOUT:
    result = PRF_TOG_CHK(ch, PRF_NOSHOUT);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_SHOWVNUMS:
    result = PRF_TOG_CHK(ch, PRF_SHOWVNUMS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_CLS:
    result = PRF_TOG_CHK(ch, PRF_CLS);
    break;    
  case SCMD_BUILDWALK:
    if (GET_LEVEL(ch) < LVL_BUILDER) {
      send_to_char(ch, "Builders only, sorry.\r\n");
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
    if (PRF_FLAGGED(ch, PRF_BUILDWALK)) {
      one_argument(argument, arg);
      for (i=0; *arg && *(sector_types[i]) != '\n'; i++)
        if (is_abbrev(arg, sector_types[i]))
          break;
      if (*(sector_types[i]) == '\n') 
        i=0;
      GET_BUILDWALK_SECTOR(ch) = i;
      send_to_char(ch, "Default sector type is %s\r\n", sector_types[i]);
  
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk on. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    } else
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk off. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    if (PRF_FLAGGED(ch, PRF_AFK))
      act("$n has gone AFK.", TRUE, ch, 0, 0, TO_ROOM);
    else {
      act("$n has come back from AFK.", TRUE, ch, 0, 0, TO_ROOM);
      if (has_mail(GET_IDNUM(ch)))
        send_to_char(ch, "You have mail waiting.\r\n");
    }
    break;
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;
  case SCMD_AUTOGOLD:
    result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;
  case SCMD_AUTOMAP:
    result = PRF_TOG_CHK(ch, PRF_AUTOMAP);
    break;
  case SCMD_AUTOKEY:
    result = PRF_TOG_CHK(ch, PRF_AUTOKEY);
    break;
  case SCMD_AUTODOOR:
    result = PRF_TOG_CHK(ch, PRF_AUTODOOR);
    break;
  case SCMD_ZONERESETS:
    result = PRF_TOG_CHK(ch, PRF_ZONERESETS);
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
  }

  if (result)
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
  else
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

  return;
}