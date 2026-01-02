/**
* @file set.h
* Builder room/object creation and utility headers.
* 
* This set of code was not originally part of the circlemud distribution.
*/

#ifndef SET_H
#define SET_H

#include "structs.h"

ACMD(do_rset);
ACMD(do_rcreate);
ACMD(do_ocreate);
ACMD(do_mcreate);
ACMD(do_oset);
ACMD(do_osave);
ACMD(do_msave);
ACMD(do_rsave);

/* Boot-time loader: scans ROOMSAVE_DIR and restores contents into rooms. */
void RoomSave_boot(void);

/* Immediate save for a single room (room_rnum). Safe to call anytime. */
/* saves ground objs AND NPCs (+their E/G/P trees) in rsv. */
int  RoomSave_now(room_rnum rnum);

/* Autosave pass for all rooms flagged ROOM_SAVE. */
void RoomSave_autosave_tick(void);

/* Only save rooms when modified */
void RoomSave_init_dirty(void);
void RoomSave_mark_dirty_room(room_rnum rnum);
/* For container edits: find the room an object ultimately lives in */
room_rnum RoomSave_room_of_obj(struct obj_data *obj);

#endif
