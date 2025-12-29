/**
* @file roomsave.h
* Room file loading/saving and utility headers.
* 
* This set of code was not originally part of the circlemud distribution.
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
