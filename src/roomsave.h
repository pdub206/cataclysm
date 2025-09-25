/**
* @file roomsave.h
* Core structures used within the core mud code.
*
* An addition to the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*/
#ifndef ROOMSAVE_H_
#define ROOMSAVE_H_

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

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

#endif /* ROOMSAVE_H_ */
