/**
* @file act.h
* Header file for the core act* c files.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
* @todo Utility functions that could easily be moved elsewhere have been
* marked. Suggest a review of all utility functions (aka. non ACMDs) and
* determine if the utility functions should be placed into a lower level
* (non-ACMD focused) shared module.
*
*/
#ifndef _ACT_H_
#define _ACT_H_

#include "utils.h" /* for the ACMD macro */

#ifndef MAX_EMOTE_TOKENS
#define MAX_EMOTE_TOKENS 16
#endif

struct emote_token {
  char op;
  char name[MAX_NAME_LENGTH];
  struct char_data *tch;
  struct obj_data *tobj;
};

struct targeted_phrase {
  char template[MAX_STRING_LENGTH];
  int token_count;
  struct emote_token tokens[MAX_EMOTE_TOKENS];
};

/*****************************************************************************
 * Begin Functions and defines for act.comm.c
 ****************************************************************************/
/* functions with subcommands */
/* do_gen_comm */
ACMD(do_gen_comm);
#define SCMD_SHOUT    0
#define SCMD_GEMOTE   1
/* do_qcomm */
ACMD(do_qcomm);
#define SCMD_QECHO    0
/* do_spec_com */
ACMD(do_spec_comm);
#define SCMD_WHISPER  0
#define SCMD_ASK      1
/* functions without subcommands */
ACMD(do_say);
ACMD(do_ooc);
ACMD(do_feel);
ACMD(do_think);
ACMD(do_page);
ACMD(do_reply);
ACMD(do_tell);
ACMD(do_write);
ACMD(do_talk);
bool build_targeted_phrase(struct char_data *ch, const char *input, bool allow_actor_at, struct targeted_phrase *phrase);
void render_targeted_phrase(struct char_data *actor, const struct targeted_phrase *phrase, bool actor_possessive_for_at, struct char_data *viewer, char *out, size_t outsz);
/*****************************************************************************
 * Begin Functions and defines for act.informative.c
 ****************************************************************************/
/* Utility Functions */
/** @todo Move to a utility library */
char *find_exdesc(char *word, struct extra_descr_data *list);
/** @todo Move to a mud centric string utility library */
void space_to_minus(char *str);
/** @todo Move to a help module? */
int search_help(const char *argument, int level);
void free_history(struct char_data *ch, int type);
void free_recent_players(void);
/* functions with subcommands */
/* do_commands */
ACMD(do_commands);
#define SCMD_COMMANDS 0
#define SCMD_SOCIALS  1
/* do_gen_ps */
ACMD(do_gen_ps);
#define SCMD_INFO      0
#define SCMD_HANDBOOK  1
#define SCMD_CREDITS   2
#define SCMD_NEWS      3
#define SCMD_WIZLIST   4
#define SCMD_POLICIES  5
#define SCMD_VERSION   6
#define SCMD_IMMLIST   7
#define SCMD_MOTD      8
#define SCMD_IMOTD     9
#define SCMD_CLEAR     10
#define SCMD_WHOAMI    11
/* do_look */
ACMD(do_look);
#define SCMD_LOOK 0
#define SCMD_READ 1
/* functions without subcommands */
ACMD(do_areas);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exits);
ACMD(do_coins);
ACMD(do_help);
ACMD(do_history);
ACMD(do_inventory);
ACMD(do_scan);
ACMD(do_score);
ACMD(do_time);
ACMD(do_toggle);
ACMD(do_users);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_whois);

/*****************************************************************************
 * Begin Functions and defines for act.item.c
 ****************************************************************************/
/* Utility Functions */
/** @todo Compare with needs of find_eq_pos_script. */
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void name_from_drinkcon(struct obj_data *obj);
void name_to_drinkcon(struct obj_data *obj, int type);
void weight_change_object(struct obj_data *obj, int weight);
/* functions with subcommands */
/* do_drop */
ACMD(do_drop);
#define SCMD_DROP   0
#define SCMD_JUNK   1
/* do_eat */
ACMD(do_eat);
#define SCMD_EAT    0
#define SCMD_TASTE  1
#define SCMD_DRINK  2
#define SCMD_SIP    3
/* do_pour */
ACMD(do_pour);
#define SCMD_POUR  0
#define SCMD_FILL  1
/* do_raise_lower_hood */
ACMD(do_raise_lower_hood);
#define SCMD_RAISE_HOOD  0
#define SCMD_LOWER_HOOD  1

/* functions without subcommands */
ACMD(do_drink);
ACMD(do_get);
ACMD(do_give);
ACMD(do_grab);
ACMD(do_put);
ACMD(do_remove);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_skin);
ACMD(do_forage);

/*****************************************************************************
 * Begin Functions and defines for act.movement.c
 ****************************************************************************/
/* Functions with subcommands */
/* do_gen_door */
ACMD(do_gen_door);
#define SCMD_OPEN       0
#define SCMD_CLOSE      1
#define SCMD_UNLOCK     2
#define SCMD_LOCK       3
#define SCMD_PICK       4
/* Functions without subcommands */
ACMD(do_enter);
ACMD(do_follow);
ACMD(do_hitch);
ACMD(do_leave);
ACMD(do_mount);
ACMD(do_move);
ACMD(do_pack);
ACMD(do_rest);
ACMD(do_dismount);
ACMD(do_sit);
ACMD(do_sleep);
ACMD(do_stand);
ACMD(do_unhitch);
ACMD(do_unpack);
ACMD(do_unfollow);
ACMD(do_wake);
/* Global variables from act.movement.c */
extern const char *cmd_door[];


/*****************************************************************************
 * Begin Functions and defines for act.offensive.c
 ****************************************************************************/
/* Functions with subcommands */
/* do_hit */
ACMD(do_hit);
#define SCMD_HIT    0
/* Functions without subcommands */
ACMD(do_assist);
ACMD(do_bash);
ACMD(do_backstab);
ACMD(do_flee);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_order);
ACMD(do_rescue);
ACMD(do_whirlwind);
ACMD(do_bandage);

/*****************************************************************************
 * Begin Functions and defines for act.other.c
 ****************************************************************************/
/* Functions with subcommands */
/* do_gen_tog */
ACMD(do_gen_tog);
#define SCMD_NOSUMMON    0
#define SCMD_NOHASSLE    1
#define SCMD_BRIEF       2
#define SCMD_COMPACT     3
#define SCMD_NOTELL      4
#define SCMD_NOSHOUT     5
#define SCMD_NOWIZ       6
#define SCMD_QUEST       7
#define SCMD_SHOWVNUMS   8
#define SCMD_NOREPEAT    9
#define SCMD_HOLYLIGHT   10
#define SCMD_SLOWNS      11
#define SCMD_AUTOEXIT    12
#define SCMD_TRACK       13
#define SCMD_CLS         14
#define SCMD_BUILDWALK   15
#define SCMD_AFK         16
#define SCMD_AUTOLOOT    17
#define SCMD_AUTOSPLIT   18
#define SCMD_AUTOASSIST  19
#define SCMD_AUTOMAP     20
#define SCMD_AUTOKEY     21
#define SCMD_AUTODOOR    22
#define SCMD_ZONERESETS  23
#define SCMD_SYSLOG      24
#define SCMD_WIMPY       25
#define SCMD_PAGELENGTH  26
#define SCMD_SCREENWIDTH 27
#define SCMD_COLOR       28

/* do_quit */
ACMD(do_quit);
#define SCMD_QUI  0
#define SCMD_QUIT 1
/* do_use */
ACMD(do_use);
#define SCMD_USE  0
#define SCMD_QUAFF  1
#define SCMD_RECITE 2
/* Functions without subcommands */
ACMD(do_display);
ACMD(do_group);
ACMD(do_hide);
ACMD(do_listen);
ACMD(do_not_here);
ACMD(do_change);
ACMD(do_reroll);
ACMD(do_report);
ACMD(do_save);
ACMD(do_skills);
ACMD(do_palm);
ACMD(do_slip);
ACMD(do_sneak);
ACMD(do_split);
ACMD(do_steal);
ACMD(do_visible);
bool perform_scan_sweep(struct char_data *ch);
void clear_scan_results(struct char_data *ch);
bool scan_can_target(struct char_data *ch, struct char_data *tch);
bool scan_confirm_target(struct char_data *ch, struct char_data *tch);
void stealth_process_room_movement(struct char_data *ch, room_rnum room, int dir, bool leaving);
int get_stealth_skill_value(struct char_data *ch);
int roll_stealth_check(struct char_data *ch);
int roll_sleight_check(struct char_data *ch);
bool can_scan_for_sneak(struct char_data *ch);
int roll_scan_perception(struct char_data *ch);


/*****************************************************************************
 * Begin Functions and defines for act.social.c
 ****************************************************************************/
/* Utility Functions */
void free_social_messages(void);
/** @todo free_action should be moved to a utility function module. */
void free_action(struct social_messg *mess);
/** @todo command list functions probably belong in interpreter */
void free_command_list(void);
/** @todo command list functions probably belong in interpreter */
void create_command_list(void);
/* Functions without subcommands */
ACMD(do_action);
ACMD(do_gmote);



/*****************************************************************************
 * Begin Functions and defines for act.wizard.c
 ****************************************************************************/
/* Utility Functions */
/** @todo should probably be moved to a more general file handler module */
void clean_llog_entries(void);
/** @todo This should be moved to a more general utility file */
int script_command_interpreter(struct char_data *ch, char *arg);
room_rnum find_target_room(struct char_data *ch, char *rawroomstr);
void perform_immort_vis(struct char_data *ch);
void snoop_check(struct char_data *ch);
bool change_player_name(struct char_data *ch, struct char_data *vict, char *new_name);
bool AddRecentPlayer(char *chname, char *chhost, bool newplr, bool cpyplr);
void perform_emote(struct char_data *ch, char *argument, bool possessive, bool hidden);
/* Functions with subcommands */
/* do_date */
ACMD(do_date);
#define SCMD_DATE   0
#define SCMD_UPTIME 1
/* do_echo */
ACMD(do_echo);
#define SCMD_ECHO   0
/* do emote */
ACMD(do_emote);
ACMD(do_pemote);
ACMD(do_hemote);
ACMD(do_phemote);
#define SCMD_EMOTE   0
#define SCMD_PEMOTE  1
#define SCMD_HEMOTE  2
#define SCMD_PHEMOTE 3
/* do_last */
ACMD(do_last);
#define SCMD_LIST_ALL 1
/* do_shutdown */
ACMD(do_shutdown);
#define SCMD_SHUTDOW   0
#define SCMD_SHUTDOWN  1
/* do_wizutil */
ACMD(do_wizutil);
#define SCMD_REROLL   0
#define SCMD_PARDON   1
#define SCMD_MUTE     2
#define SCMD_FREEZE   3
#define SCMD_THAW     4
#define SCMD_UNAFFECT 5
/* Functions without subcommands */
ACMD(do_audit);
ACMD(do_advance);
ACMD(do_at);
ACMD(do_checkloadstatus);
ACMD(do_copyover);
ACMD(do_dc);
ACMD(do_changelog);
ACMD(do_file);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_goto);
ACMD(do_invis);
ACMD(do_links);
ACMD(do_load);
ACMD(do_msave);
ACMD(do_oset);
ACMD(do_peace);
ACMD(do_plist);
ACMD(do_purge);
ACMD(do_recent);
ACMD(do_restore);
void return_to_char(struct char_data * ch);
ACMD(do_return);
ACMD(do_rsave);
ACMD(do_saveall);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_snoop);
ACMD(do_stat);
ACMD(do_switch);
ACMD(do_teleport);
ACMD(do_trans);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wizhelp);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizupdate);
ACMD(do_zcheck);
ACMD(do_zlock);
ACMD(do_zpurge);
ACMD(do_zreset);
ACMD(do_zunlock);

#endif /* _ACT_H_ */
