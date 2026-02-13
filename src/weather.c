/**
* @file weather.c                                          
* Functions that handle the in game progress of time and weather changes.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"

static void another_hour(int mode);
static void weather_change(void);

/* Hours per calendar unit, used for epoch-based moon cycle calculation. */
#define MOON_HOURS_PER_DAY    9
#define MOON_HOURS_PER_MONTH  810    /* 90 days * 9 hours */
#define MOON_HOURS_PER_YEAR   6480   /* 8 months * 810 hours */

/* Guthay: 3h on, 5h off (8h cycle). Ral: 6h on, 8h off (14h cycle). */
#define GUTHAY_CYCLE  8
#define RAL_CYCLE     14

/** Compute moon states from the absolute in-game hour count derived from
 * time_info.  Called on boot (reset_time) and each hour (another_hour).
 * Guthay cycle: 3h on (east/high/west), 5h off = 8h total.
 * Ral cycle:    6h on (2h each position), 8h off = 14h total.
 */
void update_moon_states(void)
{
  long total_hours;
  int  guthay_cycle, ral_cycle;

  total_hours = (long)time_info.year  * MOON_HOURS_PER_YEAR  +
                (long)time_info.month * MOON_HOURS_PER_MONTH +
                (long)time_info.day   * MOON_HOURS_PER_DAY   +
                (time_info.hours - 1);

  guthay_cycle = (int)(total_hours % GUTHAY_CYCLE);
  ral_cycle    = (int)(total_hours % RAL_CYCLE);

  if (guthay_cycle == 0)
    weather_info.guthay = MOON_EAST;
  else if (guthay_cycle == 1)
    weather_info.guthay = MOON_HIGH;
  else if (guthay_cycle == 2)
    weather_info.guthay = MOON_WEST;
  else
    weather_info.guthay = MOON_NONE;

  if (ral_cycle <= 1)
    weather_info.ral = MOON_EAST;
  else if (ral_cycle <= 3)
    weather_info.ral = MOON_HIGH;
  else if (ral_cycle <= 5)
    weather_info.ral = MOON_WEST;
  else
    weather_info.ral = MOON_NONE;
}

/** Call this function every mud hour to increment the gametime (by one hour)
 * and the weather patterns.
 * @param mode Really, this parameter has the effect of a boolean. In the
 * current incarnation of the function and utility functions, as long as mode
 * is non-zero, the gametime will increment one hour and the weather will be
 * changed.
 */
void weather_and_time(int mode)
{
  another_hour(mode);
  if (mode)
    weather_change();
}

/** Increment the game time by one hour (no matter what) and display any time 
 * dependent messages via send_to_outdoors() (if parameter is non-zero).
 * @param mode Really, this parameter has the effect of a boolean. If non-zero,
 * display day/night messages to all eligible players.
 */
static void another_hour(int mode)
{
  int old_guthay = weather_info.guthay;
  int old_ral    = weather_info.ral;

  time_info.hours++;

  if (time_info.hours > 9) {
    time_info.hours = 1;
    time_info.day++;

    if (time_info.day > 89) {
      time_info.day = 0;
      time_info.month++;

      if (time_info.month > 7) {
        time_info.month = 0;
        time_info.year++;
      }
    }
  }

  if (mode) {
    switch (time_info.hours) {
    case 1:
      weather_info.sunlight = SUN_RISE;
      send_to_outdoor("The sun rises in the east.\r\n");
      break;
    case 2:
      weather_info.sunlight = SUN_LIGHT;
      send_to_outdoor("The day has begun.\r\n");
      break;
    case 7:
      weather_info.sunlight = SUN_SET;
      send_to_outdoor("The sun slowly disappears in the west.\r\n");
      break;
    case 8:
      weather_info.sunlight = SUN_DARK;
      send_to_outdoor("The night has begun.\r\n");
      break;
    default:
      break;
    }

    update_moon_states();

    if (old_guthay != weather_info.guthay) {
      if (old_guthay == MOON_NONE && weather_info.guthay == MOON_EAST)
        send_to_outdoor("Guthay begins to ascend in the sky to the east.\r\n");
      else if (old_guthay == MOON_EAST && weather_info.guthay == MOON_HIGH) {
        if (weather_info.sunlight == SUN_DARK)
          send_to_outdoor("The darkness fades as Guthay illuminates the landscape in a golden light.\r\n");
        else
          send_to_outdoor("Guthay reaches the middle of the sky.\r\n");
      }
      else if (old_guthay == MOON_HIGH && weather_info.guthay == MOON_WEST)
        send_to_outdoor("Guthay begins to descend in the sky to the west.\r\n");
      else if (old_guthay == MOON_WEST && weather_info.guthay == MOON_NONE)
        send_to_outdoor("Guthay disappears over the horizon to the west.\r\n");
    }

    if (old_ral != weather_info.ral) {
      if (old_ral == MOON_NONE && weather_info.ral == MOON_EAST)
        send_to_outdoor("Ral begins to ascend in the sky to the east.\r\n");
      else if (old_ral == MOON_EAST && weather_info.ral == MOON_HIGH) {
        if (weather_info.sunlight == SUN_DARK)
          send_to_outdoor("Ral rises into the sky, bathing the ground in a soft green light.\r\n");
        else
          send_to_outdoor("Ral reaches the middle of the sky.\r\n");
      }
      else if (old_ral == MOON_HIGH && weather_info.ral == MOON_WEST)
        send_to_outdoor("Ral begins to descend in the sky to the west.\r\n");
      else if (old_ral == MOON_WEST && weather_info.ral == MOON_NONE)
        send_to_outdoor("Ral disappears over the horizon to the west.\r\n");
    }
  }
}

/** Controls the in game weather system. If the weather changes, an information
 * update is sent via send_to_outdoors().
 * @todo There are some hard coded values that could be extracted to make
 * customizing the weather patterns easier.
 */  
static void weather_change(void)
{
  int diff, change;
  
  if (time_info.month >= 4)
    diff = (weather_info.pressure > 985 ? -2 : 2);
  else
    diff = (weather_info.pressure > 1015 ? -2 : 2);

  weather_info.change += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));

  weather_info.change = MIN(weather_info.change, 12);
  weather_info.change = MAX(weather_info.change, -12);

  weather_info.pressure += weather_info.change;

  weather_info.pressure = MIN(weather_info.pressure, 1040);
  weather_info.pressure = MAX(weather_info.pressure, 960);

  change = 0;

  switch (weather_info.sky) {
  case SKY_CLOUDLESS:
    if (weather_info.pressure < 990)
      change = 1;
    else if (weather_info.pressure < 1010)
      if (dice(1, 4) == 1)
	change = 1;
    break;
  case SKY_CLOUDY:
    if (weather_info.pressure < 970)
      change = 2;
    else if (weather_info.pressure < 990) {
      if (dice(1, 4) == 1)
	change = 2;
      else
	change = 0;
    } else if (weather_info.pressure > 1030)
      if (dice(1, 4) == 1)
	change = 3;

    break;
  case SKY_RAINING:
    if (weather_info.pressure < 970) {
      if (dice(1, 4) == 1)
	change = 4;
      else
	change = 0;
    } else if (weather_info.pressure > 1030)
      change = 5;
    else if (weather_info.pressure > 1010)
      if (dice(1, 4) == 1)
	change = 5;

    break;
  case SKY_LIGHTNING:
    if (weather_info.pressure > 1010)
      change = 6;
    else if (weather_info.pressure > 990)
      if (dice(1, 4) == 1)
	change = 6;

    break;
  default:
    change = 0;
    weather_info.sky = SKY_CLOUDLESS;
    break;
  }

  switch (change) {
  case 0:
    break;
  case 1:
    send_to_outdoor("The sky starts to get cloudy.\r\n");
    weather_info.sky = SKY_CLOUDY;
    break;
  case 2:
    send_to_outdoor("It starts to rain.\r\n");
    weather_info.sky = SKY_RAINING;
    break;
  case 3:
    send_to_outdoor("The clouds disappear.\r\n");
    weather_info.sky = SKY_CLOUDLESS;
    break;
  case 4:
    send_to_outdoor("Lightning starts to show in the sky.\r\n");
    weather_info.sky = SKY_LIGHTNING;
    break;
  case 5:
    send_to_outdoor("The rain stops.\r\n");
    weather_info.sky = SKY_CLOUDY;
    break;
  case 6:
    send_to_outdoor("The lightning stops.\r\n");
    weather_info.sky = SKY_RAINING;
    break;
  default:
    break;
  }
}
