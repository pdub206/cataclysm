/**************************************************************************
*  File: act.offensive.c                                   Part of tbaMUD *
*  Usage: Player-level commands of an offensive nature.                   *
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
#include "act.h"
#include "fight.h"
#include "mud_event.h"

ACMD(do_assist)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char(ch, "You're already fighting!  How can you assist someone else?\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Whom do you wish to assist?\r\n");
  else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (helpee == ch)
    send_to_char(ch, "You can't help yourself any more than this!\r\n");
  else {
    /*
     * Hit the same enemy the person you're helping is.
     */
    if (FIGHTING(helpee))
      opponent = FIGHTING(helpee);
    else
      for (opponent = world[IN_ROOM(ch)].people;
	   opponent && (FIGHTING(opponent) != helpee);
	   opponent = opponent->next_in_room);

    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
         /* prevent accidental pkill */
    else {
      send_to_char(ch, "You join the fight!\r\n");
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch, opponent, TYPE_UNDEFINED);
    }
  }
}

ACMD(do_hit)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

 one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Hit who?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "That player is not here.\r\n");
  else if (vict == ch) {
    send_to_char(ch, "You hit yourself...OUCH!.\r\n");
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) { 
      if (GET_DEX(ch) > GET_DEX(vict) || (GET_DEX(ch) == GET_DEX(vict) && rand_number(1, 2) == 1))  /* if faster */
        hit(ch, vict, TYPE_UNDEFINED);  /* first */
      else hit(vict, ch, TYPE_UNDEFINED);  /* or the victim is first */
        WAIT_STATE(ch, PULSE_VIOLENCE + 2); 
    } else 
      send_to_char(ch, "You're fighting the best you can!\r\n"); 
  } 
}

ACMD(do_kill)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  if (GET_LEVEL(ch) < LVL_GRGOD || IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Kill who?\r\n");
  } else {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "That player is not here.\r\n");
    else if (ch == vict)
      send_to_char(ch, "Your mother would be so sad.. :(\r\n");
    else {
      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      raw_kill(vict, ch);
    }
  }
}

ACMD(do_backstab)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct obj_data *weap;
  int roll, atk_bonus, total, target_ac;
  bool crit_success = FALSE, crit_fail = FALSE;

  if (!GET_SKILL(ch, SKILL_BACKSTAB)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  one_argument(argument, buf);

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Backstab who?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "How can you sneak up on yourself?\r\n");
    return;
  }
  if (!(weap = GET_EQ(ch, WEAR_WIELD))) {
    send_to_char(ch, "You need to wield a weapon to make it a success.\r\n");
    return;
  }
  /* Only piercing weapons allowed */
  if (GET_OBJ_VAL(weap, 2) != TYPE_PIERCE - TYPE_HIT) {
    send_to_char(ch, "Only piercing weapons can be used for backstabbing.\r\n");
    return;
  }
  if (FIGHTING(vict)) {
    send_to_char(ch, "You can't backstab a fighting person -- they're too alert!\r\n");
    return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    hit(vict, ch, TYPE_UNDEFINED);
    return;
  }

  /* --- d20 vs ascending AC --- */
  atk_bonus = GET_ABILITY_MOD(GET_DEX(ch)) +
              GET_PROFICIENCY(GET_SKILL(ch, SKILL_BACKSTAB));

  roll              = rand_number(1, 20);
  crit_fail         = (roll == 1);
  crit_success      = (roll == 20);

  total     = roll + atk_bonus;
  target_ac = compute_ascending_ac(vict);

  if (!crit_fail && (crit_success || total >= target_ac)) {
    /* Successful backstab */
    act("You slip behind $N and drive your weapon home!", TRUE, ch, 0, vict, TO_CHAR);
    act("$n slips behind you and drives a weapon into your back!", TRUE, ch, 0, vict, TO_VICT);
    act("$n slips behind $N and drives a weapon home!", TRUE, ch, 0, vict, TO_NOTVICT);
    
    /* Keeping this logic really simple so it can be adjusted later if need be */
    if (crit_success) {
      /* Simple crit = 2x damage */
      int base = dice(GET_OBJ_VAL(weap, 0), GET_OBJ_VAL(weap, 1));
      int dmg = base + GET_ABILITY_MOD(GET_DEX(ch));
      if (dmg < 1) dmg = 1;
      dmg *= 2;
      damage(ch, vict, dmg, SKILL_BACKSTAB);
    } else {
      /* Hit but not crit = 1.5x damage */
      int base = dice(GET_OBJ_VAL(weap, 0), GET_OBJ_VAL(weap, 1));
      int dmg = base + GET_ABILITY_MOD(GET_DEX(ch));
      if (dmg < 1) dmg = 1;
      dmg *= 1.5;
      damage(ch, vict, dmg, SKILL_BACKSTAB);
    }

    gain_skill(ch, "backstab", TRUE);

  } else {
    /* Missed backstab */
    act("You lunge at $N, but miss the mark.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n lunges at you, but misses the mark!", TRUE, ch, 0, vict, TO_VICT);
    act("$n lunges at $N, but misses the mark!", TRUE, ch, 0, vict, TO_NOTVICT);

    damage(ch, vict, 0, SKILL_BACKSTAB);
    gain_skill(ch, "backstab", FALSE);
  }

  WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
}

ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  bool found = FALSE;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char(ch, "Order who to do what?\r\n");
  else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM)) && !is_abbrev(name, "followers"))
    send_to_char(ch, "That person isn't here.\r\n");
  else if (ch == vict)
    send_to_char(ch, "You obviously suffer from skitzofrenia.\r\n");
  else {
    if (AFF_FLAGGED(ch, AFF_CHARM)) {
      send_to_char(ch, "Your superior would not aprove of you giving orders.\r\n");
      return;
    }
    if (vict) {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
        act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
        send_to_char(ch, "%s", CONFIG_OK);
        command_interpreter(vict, message);
      }
    } else {			/* This is order "followers" */
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      for (k = ch->followers; k; k = k->next) {
        if (IN_ROOM(ch) == IN_ROOM(k->follower))
          if (AFF_FLAGGED(k->follower, AFF_CHARM)) {
            found = TRUE;
            command_interpreter(k->follower, message);
          }
      }
      if (found)
        send_to_char(ch, "%s", CONFIG_OK);
      else
        send_to_char(ch, "Nobody here is a loyal subject of yours!\r\n");
    }
  }
}

ACMD(do_flee)
{
  int i, attempt, loss;
  struct char_data *was_fighting;

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char(ch, "You are in pretty bad shape, unable to flee!\r\n");
    return;
  }

  for (i = 0; i < 6; i++) {
    attempt = rand_number(0, DIR_COUNT - 1); /* Select a random direction */
    if (CAN_GO(ch, attempt) &&
	!ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      was_fighting = FIGHTING(ch);
      if (do_simple_move(ch, attempt, TRUE)) {
	send_to_char(ch, "You flee head over heels.\r\n");
        if (was_fighting && !IS_NPC(ch)) {
	  loss = GET_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
	  loss *= GET_LEVEL(was_fighting);
        }
      if (FIGHTING(ch)) 
        stop_fighting(ch); 
      if (was_fighting && ch == FIGHTING(was_fighting))
        stop_fighting(was_fighting); 
      } else {
	        act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char(ch, "PANIC!  You couldn't escape!\r\n");
}

ACMD(do_bash)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int roll, atk_bonus, total, target_ac;
  bool crit_success = FALSE, crit_fail = FALSE;

  one_argument(argument, arg);

  if (!GET_SKILL(ch, SKILL_BASH)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char(ch, "You need to wield a weapon to make it a success.\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Bash who?\r\n");
      return;
    }
  }
  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  if (MOB_FLAGGED(vict, MOB_NOKILL)) {
    send_to_char(ch, "This mob is protected.\r\n");
    return;
  }

  /* --- 5e-like attack roll vs ascending AC --- */
  atk_bonus = GET_ABILITY_MOD(GET_STR(ch)) +
              GET_PROFICIENCY(GET_SKILL(ch, SKILL_BASH));

  roll = rand_number(1, 20);
  crit_success = (roll == 1);
  crit_fail      = (roll == 20);

  total = roll + atk_bonus;
  target_ac = compute_ascending_ac(vict);

  /* Some mobs simply can't be bashed: force a miss like legacy code did. */
  if (MOB_FLAGGED(vict, MOB_NOBASH))
    crit_fail = TRUE;

  if (!crit_fail && (crit_success || total >= target_ac)) {
    /* ---- HIT: small damage + knockdown ---- */

    /* Damage = 1 + STR mod (min 1); double on crit */
    int dmg = 1 + GET_ABILITY_MOD(GET_STR(ch));
    if (dmg < 1) dmg = 1;
    if (crit_success) dmg *= 2;

    if (crit_success) {
      act("You slam into $N with a crushing bash!", TRUE, ch, 0, vict, TO_CHAR);
      act("$n slams into you with a crushing bash!", TRUE, ch, 0, vict, TO_VICT);
      act("$n slams into $N with a crushing bash!", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      act("You bash $N off balance.", TRUE, ch, 0, vict, TO_CHAR);
      act("$n bashes you off balance.", TRUE, ch, 0, vict, TO_VICT);
      act("$n bashes $N off balance.", TRUE, ch, 0, vict, TO_NOTVICT);
    }

    /* Apply damage; legacy: >0 means still alive & not a pure miss */
    if (damage(ch, vict, dmg, SKILL_BASH) > 0) {
      WAIT_STATE(vict, PULSE_VIOLENCE);  /* brief stun */
      if (IN_ROOM(ch) == IN_ROOM(vict)) {
        GET_POS(vict) = POS_SITTING;     /* knockdown */
        gain_skill(ch, "bash", TRUE);
      }
    }

  } else {
    /* ---- MISS: you eat the floor (attacker sits) ---- */
    act("You miss your bash at $N and lose your footing!", TRUE, ch, 0, vict, TO_CHAR);
    act("$n misses a bash at you and loses $s footing!", TRUE, ch, 0, vict, TO_VICT);
    act("$n misses a bash at $N and loses $s footing!", TRUE, ch, 0, vict, TO_NOTVICT);

    damage(ch, vict, 0, SKILL_BASH);
    GET_POS(ch) = POS_SITTING;
    gain_skill(ch, "bash", FALSE);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

ACMD(do_rescue)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict, *tmp_ch;
  int roll, bonus, total, dc;

  if (!GET_SKILL(ch, SKILL_RESCUE)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Whom do you want to rescue?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "What about fleeing instead?\r\n");
    return;
  }
  if (FIGHTING(ch) == vict) {
    send_to_char(ch, "How can you rescue someone you are trying to kill?\r\n");
    return;
  }

  /* Find someone who is fighting the victim */
  for (tmp_ch = world[IN_ROOM(ch)].people; tmp_ch && (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room)
    ;

  /* Handle the “already rescued” edge case from your original */
  if ((FIGHTING(vict) != NULL) && (FIGHTING(ch) == FIGHTING(vict)) && (tmp_ch == NULL)) {
    tmp_ch = FIGHTING(vict);
    if (FIGHTING(tmp_ch) == ch) {
      send_to_char(ch, "You have already rescued %s from %s.\r\n", GET_NAME(vict), GET_NAME(FIGHTING(ch)));
      return;
    }
  }

  if (!tmp_ch) {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  /* --- STR + proficiency ability check vs DC (no nat 1/20 rules) --- */
  bonus = GET_ABILITY_MOD(GET_STR(ch)) + GET_PROFICIENCY(GET_SKILL(ch, SKILL_RESCUE));
  dc = 10;
  if (FIGHTING(ch)) dc += 5; /* harder to pull off while already in melee */

  roll   = rand_number(1, 20);
  total  = roll + bonus;

  if (total < dc) {
    send_to_char(ch, "You fail the rescue!\r\n");
    gain_skill(ch, "rescue", FALSE);
    return;
  }

  /* Success: swap aggro */
  send_to_char(ch, "Banzai!  To the rescue...\r\n");
  act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

  if (FIGHTING(vict) == tmp_ch)
    stop_fighting(vict);
  if (FIGHTING(tmp_ch))
    stop_fighting(tmp_ch);
  if (FIGHTING(ch))
    stop_fighting(ch);

  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);

  gain_skill(ch, "rescue", TRUE);

  WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
}

/* 5e-like whirlwind tick: random characters (PCs & NPCs), d20 vs AC, friendly fire possible */
EVENTFUNC(event_whirlwind)
{
  struct char_data *ch, *tch;
  struct mud_event_data *pMudEvent;
  struct list_data *room_list;
  int count;

  if (event_obj == NULL)
    return 0;

  pMudEvent = (struct mud_event_data *) event_obj;
  ch = (struct char_data *) pMudEvent->pStruct;

  if (!ch || IN_ROOM(ch) == NOWHERE || GET_POS(ch) < POS_FIGHTING)
    return 0;

  room_list = create_list();

  /* === Target pool: everyone except self; skip protected mobs === */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (tch == ch) continue;
    if (IS_NPC(tch) && MOB_FLAGGED(tch, MOB_NOKILL)) continue;
    if (GET_POS(tch) <= POS_DEAD) continue;
    add_to_list(tch, room_list);
  }

  if (room_list->iSize == 0) {
    free_list(room_list);
    send_to_char(ch, "There is no one here to whirlwind!\r\n");
    return 0;
  }

  send_to_char(ch, "\t[f313]You deliver a vicious \t[f014]\t[b451]attack, spinning wildly!!!\tn\r\n");

  /* Up to 1–4 rapid strikes (friendly fire possible) */
  for (count = dice(1, 4); count > 0; count--) {
    int roll, atk_bonus, total, target_ac, dmg;
    bool crit_success = FALSE, crit_fail = FALSE;

    tch = (struct char_data *) random_from_list(room_list);
    if (!tch || IN_ROOM(tch) != IN_ROOM(ch) || GET_POS(tch) <= POS_DEAD)
      continue;

    /* Attack roll vs ascending AC */
    atk_bonus = GET_ABILITY_MOD(GET_STR(ch)) + GET_PROFICIENCY(GET_SKILL(ch, SKILL_WHIRLWIND));
    roll       = rand_number(1, 20);
    crit_fail = (roll == 1);
    crit_success      = (roll == 20);

    total     = roll + atk_bonus;
    target_ac = compute_ascending_ac(tch);

    if (!crit_fail && (crit_success || total >= target_ac)) {
      dmg = dice(1, 4) + GET_ABILITY_MOD(GET_STR(ch)) + GET_PROFICIENCY(GET_SKILL(ch, SKILL_WHIRLWIND));
      if (dmg < 1) dmg = 1;
      if (crit_success)   dmg *= 2;

      if (crit_success) {
        act("Your whirlwind catches $N with a devastating slash!", TRUE, ch, 0, tch, TO_CHAR);
        act("$n's whirlwind catches you with a devastating slash!", TRUE, ch, 0, tch, TO_VICT | TO_SLEEP);
        act("$n's whirlwind catches $N with a devastating slash!", TRUE, ch, 0, tch, TO_NOTVICT);
      } else {
        act("Your whirlwind slices into $N.", TRUE, ch, 0, tch, TO_CHAR);
        act("$n's whirlwind slices into you.", TRUE, ch, 0, tch, TO_VICT | TO_SLEEP);
        act("$n's whirlwind slices into $N.", TRUE, ch, 0, tch, TO_NOTVICT);
      }

      damage(ch, tch, dmg, SKILL_WHIRLWIND);

      /* Learning tick on a strong (crit) hit */
      if (crit_success)
        gain_skill(ch, "whirlwind", TRUE);

    } else {
      act("Your whirlwind arcs wide past $N.", TRUE, ch, 0, tch, TO_CHAR);
      act("$n's whirlwind arcs wide past you.", TRUE, ch, 0, tch, TO_VICT | TO_SLEEP);
      act("$n's whirlwind arcs wide past $N.", TRUE, ch, 0, tch, TO_NOTVICT);

      damage(ch, tch, 0, SKILL_WHIRLWIND);
      gain_skill(ch, "whirlwind", FALSE);
    }
  }

  free_list(room_list);

  /* Continue spinning or stop */
  if (GET_SKILL(ch, SKILL_WHIRLWIND) < rand_number(1, 101)) {
    send_to_char(ch, "You stop spinning, but the world around you doesn't.\r\n");
    return 0;
  }
  return (int)(1.5 * PASSES_PER_SEC);
}

/* The "Whirlwind" skill is designed to provide a basic understanding of the
 * mud event and list systems. */
ACMD(do_whirlwind)
{
  
  if (!GET_SKILL(ch, SKILL_WHIRLWIND)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char(ch, "You must be on your feet to perform a whirlwind.\r\n");
    return;    
  }

  /* First thing we do is check to make sure the character is not in the middle
   * of a whirl wind attack.
   * 
   * "char_had_mud_event() will sift through the character's event list to see if
   * an event of type "eWHIRLWIND" currently exists. */
  if (char_has_mud_event(ch, eWHIRLWIND)) {
    send_to_char(ch, "You are already attempting that!\r\n");
    return;   
  }

  send_to_char(ch, "You begin to spin rapidly in circles.\r\n");
  act("$n begins to rapidly spin in a circle!", FALSE, ch, 0, 0, TO_ROOM);
  
  /* NEW_EVENT() will add a new mud event to the event list of the character.
   * This function below adds a new event of "eWHIRLWIND", to "ch", and passes "NULL" as
   * additional data. The event will be called in "3 * PASSES_PER_SEC" or 3 seconds */
  NEW_EVENT(eWHIRLWIND, ch, NULL, 3 * PASSES_PER_SEC);
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_kick)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int roll, atk_bonus, total, target_ac;
  bool crit_success = FALSE, crit_miss = FALSE;

  if (!GET_SKILL(ch, SKILL_KICK)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Kick who?\r\n");
      return;
    }
  }

  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  /* --- 5e-like attack roll vs ascending AC --- */
  atk_bonus = GET_ABILITY_MOD(GET_STR(ch)) +
              GET_PROFICIENCY(GET_SKILL(ch, SKILL_KICK));

  roll = rand_number(1, 20);
  crit_miss = (roll == 1);
  crit_success      = (roll == 20);

  total = roll + atk_bonus;
  target_ac = compute_ascending_ac(vict);

  if (!crit_miss && (crit_success || total >= target_ac)) {
    /* HIT */

    /* Damage = 1 + STR mod, floored at 1 */
    int dmg = 1 + GET_ABILITY_MOD(GET_STR(ch));
    if (dmg < 1)
      dmg = 1;

    if (crit_success) {
      dmg *= 2;  /* simple crit rule: double damage */
      act("You land a brutal, bone-jarring kick on $N!", TRUE, ch, 0, vict, TO_CHAR);
      act("$n lands a brutal, bone-jarring kick on you!", TRUE, ch, 0, vict, TO_VICT);
      act("$n lands a brutal, bone-jarring kick on $N!", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      act("You kick $N solidly.", TRUE, ch, 0, vict, TO_CHAR);
      act("$n kicks you solidly.", TRUE, ch, 0, vict, TO_VICT);
      act("$n kicks $N solidly.", TRUE, ch, 0, vict, TO_NOTVICT);
    }

    damage(ch, vict, dmg, SKILL_KICK);
    gain_skill(ch, "kick", TRUE);

  } else {
    /* MISS */
    act("You miss your kick at $N.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n misses a kick at you.", TRUE, ch, 0, vict, TO_VICT);
    act("$n misses a kick at $N.", TRUE, ch, 0, vict, TO_NOTVICT);

    damage(ch, vict, 0, SKILL_KICK);
    gain_skill(ch, "kick", FALSE);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_bandage)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int roll, bonus, total, dc;

  if (!GET_SKILL(ch, SKILL_BANDAGE)) {
    send_to_char(ch, "You are unskilled in the art of bandaging.\r\n");
    return;
  }

  if (GET_POS(ch) != POS_STANDING) {
    send_to_char(ch, "You are not in a proper position for that!\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Who do you want to bandage?\r\n");
    return;
  }

  if (AFF_FLAGGED(vict, AFF_BANDAGED)) {
    send_to_char(ch, "That person has already been bandaged recently.\r\n");
    return;
  }

  if (GET_HIT(vict) >= GET_MAX_HIT(vict)) {
    send_to_char(ch, "They don’t need bandaging right now.\r\n");
    return;
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);

  /* --- WIS + proficiency ability check vs DC --- */
  bonus = GET_ABILITY_MOD(GET_WIS(ch)) + GET_PROFICIENCY(GET_SKILL(ch, SKILL_BANDAGE));
  dc = 10;
  if (FIGHTING(ch)) dc += 2;  /* harder to bandage in combat */

  roll   = rand_number(1, 20);
  total = roll + bonus;

  if (total < dc) {
    /* Failure: hurt the patient slightly */
    act("Your attempt to bandage fails.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n tries to bandage $N, but fails miserably.", TRUE, ch, 0, vict, TO_NOTVICT);
    act("$n fumbles the bandage work on you. That hurts!", TRUE, ch, 0, vict, TO_VICT);

    damage(vict, vict, 2, TYPE_SUFFERING);
    gain_skill(ch, "bandage", FALSE);
    return;
  }

  /* Success: heal 1d4 + WIS mod + proficiency */
  int heal = dice(1, 4) + GET_ABILITY_MOD(GET_WIS(ch)) +
             GET_PROFICIENCY(GET_SKILL(ch, SKILL_BANDAGE));
  if (heal < 1) heal = 1;

  GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + heal);

  act("You successfully bandage $N.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n bandages $N, who looks a bit better now.", TRUE, ch, 0, vict, TO_NOTVICT);
  act("Someone bandages you, and you feel a bit better now.", FALSE, ch, 0, vict, TO_VICT);

  /* Apply the bandaged cooldown: 30 minutes real-time (1800 sec) */
  struct affected_type af;
  new_affect(&af);

  /* Field name is 'spell' in your headers (not 'type') */
  af.spell    = SKILL_BANDAGE;
  af.duration = 30 RL_SEC;          /* 30 minutes real time */
  af.modifier = 0;
  af.location = APPLY_NONE;

  /* bitvector is an array; clear then set the AFF_BANDAGED bit */
  memset(af.bitvector, 0, sizeof(af.bitvector));    /* clear all bits; macro usually zeros the array */
  SET_BIT_AR(af.bitvector, AFF_BANDAGED);

  affect_to_char(vict, &af);

  gain_skill(ch, "bandage", TRUE);
}
