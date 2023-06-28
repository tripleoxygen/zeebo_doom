/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *   Joystick handling for Linux
 *
 *-----------------------------------------------------------------------------
 */

#ifndef lint
#endif /* lint */

#include <stdlib.h>

#include "doomdef.h"
#include "doomtype.h"
#include "m_argv.h"
#include "d_event.h"
#include "d_main.h"
#include "i_joy.h"
#include "lprintf.h"
#include "prboom.h"
#include "AEEHIDDevice_Joystick.h"
#include "AEEHIDButtons.h"

extern PrBoomApp *pApp;

int joyleft;
int joyright;
int joyup;
int joydown;

int usejoystick;

#define JOYSTICK_OFFSET 128
#define JOYSTICK_THRESHOLD 20

extern void M_QuitDOOM(int choice);
static void I_DispatchEvent();

static void I_EndJoystick(void)
{
  lprintf(LO_DEBUG, "I_EndJoystick : closing joystick\n");
}

void I_EventFromKey(PrBoomApp *pApp, uint16 key, byte state)
{
  int bid = -1;

  switch (key)
  {
  case AVK_LEFT:
    pApp->keyboard_nX = state ? -1 : 0;
    break;
  case AVK_RIGHT:
    pApp->keyboard_nX = state ? 1 : 0;
    break;
  case AVK_UP:
    pApp->keyboard_nY = state ? -1 : 0;
    break;
  case AVK_DOWN:
    pApp->keyboard_nY = state ? 1 : 0;
    break;
  case AVK_SOFT1:
    bid = ZEEBO_BUTTON_HOME;
    break;
  case AVK_SELECT:
    bid = ZEEBO_BUTTON_SHOULDER_RIGHT;
    break;
  case AVK_1:
    bid = ZEEBO_BUTTON_NORTH;
    break;
  case AVK_2:
    bid = ZEEBO_BUTTON_WEST;
    break;
  case AVK_3:
    bid = ZEEBO_BUTTON_EAST;
    break;
  case AVK_4:
    bid = ZEEBO_BUTTON_SHOULDER_LEFT;
    break;
  }

  if (bid != -1)
    pApp->m_JoystickButtonStates[bid] = state;

  I_DispatchEvent();
}

void I_PollJoystick(void)
{
  AEEResult ret = EFAILED;
  AEEHIDButtonInfo bi;
  AEEHIDPositionInfo pi;
  uint32 pdwTimestamp;
  boolean pbDroppedEvents;
  int axis_value;
  event_t ev;
  int i;

  if (pApp->m_IHIDDevice == NULL || pApp->m_JoystickButtonStates == NULL)
    return;

  if (AEEHIDButton_GetNextButtonEvent(pApp->m_IHIDDevice, &bi, &pdwTimestamp, &pbDroppedEvents) == SUCCESS)
  {
    pApp->m_JoystickButtonStates[bi.nButtonID] = (boolean)bi.nState;

    // printf("%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d",
    //        pApp->m_JoystickButtonStates[0],
    //        pApp->m_JoystickButtonStates[1],
    //        pApp->m_JoystickButtonStates[2],
    //        pApp->m_JoystickButtonStates[3],
    //        pApp->m_JoystickButtonStates[4],
    //        pApp->m_JoystickButtonStates[5],
    //        pApp->m_JoystickButtonStates[6],
    //        pApp->m_JoystickButtonStates[7],
    //        pApp->m_JoystickButtonStates[8],
    //        pApp->m_JoystickButtonStates[9],
    //        pApp->m_JoystickButtonStates[10],
    //        pApp->m_JoystickButtonStates[11],
    //        pApp->m_JoystickButtonStates[12],
    //        pApp->m_JoystickButtonStates[13],
    //        pApp->m_JoystickButtonStates[14],
    //        pApp->m_JoystickButtonStates[15]
    //     );
  }

  if (IHIDDevice_GetPositionState(pApp->m_IHIDDevice, &pi) == SUCCESS)
  {
    // Doom X + Strafe
    pApp->axis_nX = 0;
    axis_value = (pi.nX - JOYSTICK_OFFSET);
    if (abs(axis_value) > JOYSTICK_THRESHOLD)
    {
      pApp->axis_nX = (axis_value < 0) ? -1 : 1;
    }

    // Doom X
    pApp->axis_nZ = 0;
    axis_value = (pi.nZ - JOYSTICK_OFFSET);
    if (abs(axis_value) > JOYSTICK_THRESHOLD)
    {
      pApp->axis_nZ = (axis_value < 0) ? -1 : 1;
    }

    // Doom Y
    pApp->axis_nY = 0;
    axis_value = pi.nY - JOYSTICK_OFFSET;
    if (abs(axis_value) > JOYSTICK_THRESHOLD)
    {
      pApp->axis_nY = (axis_value < 0) ? -1 : 1;
    }
  }

  //     printf("bRelativeAxes = %d", pi.bRelativeAxes);
  //     printf("nX %d nY %d nZ %d", pi.nX, pi.nY, pi.nZ);
  //     printf("nRx %d nRy %d nRz %d", pi.nRx, pi.nRy, pi.nRz);
  //     printf("nVX %d nVY %d nVZ %d", pi.nVX, pi.nVY, pi.nVZ);
  //     printf("nVRx %d nVRy %d nVRz %d", pi.nVRx, pi.nVRy, pi.nVRz);
  //     printf("nAX %d nAY %d nAZ %d", pi.nAX, pi.nAY, pi.nAZ);
  //     printf("nARx %d nARy %d nARz %d", pi.nARx, pi.nARy, pi.nARz);
  //     printf("nFX %d nFY %d nFZ %d", pi.nFX, pi.nFY, pi.nFZ);
  //     printf("nFRx %d nFRy %d nFRz %d", pi.nFRx, pi.nFRy, pi.nFRz);

  // if(ev.data1 != 0 || ev.data2 != 0 || ev.data3 != 0) {
  //   printf("ev.data1 = %d", ev.data1);
  //   printf("ev.data2 = %d", ev.data2);
  //   printf("ev.data3 = %d", ev.data3);
  // }

  I_DispatchEvent();
}

static void I_DispatchEvent()
{
  event_t ev;

  ev.type = ev_joystick;
  ev.data1 = ev.data2 = ev.data3 = 0;

  ev.data1 =
      (pApp->m_JoystickButtonStates[ZEEBO_BUTTON_SHOULDER_RIGHT] << 0) | // Fire
      (pApp->m_JoystickButtonStates[ZEEBO_BUTTON_NORTH] << 1) |          // Strafe
      (pApp->m_JoystickButtonStates[ZEEBO_BUTTON_SOUTH] << 2) |          // Run
      (pApp->m_JoystickButtonStates[ZEEBO_BUTTON_WEST] << 3) |           // Use
      (pApp->m_JoystickButtonStates[ZEEBO_BUTTON_HOME] << 4) |           // Escape
      (pApp->m_JoystickButtonStates[ZEEBO_BUTTON_SHOULDER_LEFT] << 5) |  // 'y'
      (pApp->m_JoystickButtonStates[ZEEBO_BUTTON_EAST] << 6);            // change weapon

  if (pApp->keyboard_nX || pApp->keyboard_nY)
  {
    ev.data2 = pApp->keyboard_nX;
    ev.data3 = pApp->keyboard_nY;
  }
  else
  {
    ev.data3 = pApp->axis_nY;

    if (pApp->axis_nX != 0)
    {
      ev.data1 |= (1 << 1);
      ev.data2 = pApp->axis_nX;
    }

    if (pApp->axis_nZ != 0)
    {
      ev.data1 &= ~(1 << 1);
      ev.data2 = pApp->axis_nZ;
    }
  }

  D_PostEvent(&ev);
}

void I_InitJoystick(void)
{
#ifdef HAVE_SDL_JOYSTICKGETAXIS
  const char *fname = "I_InitJoystick : ";
  int num_joysticks;

  if (!usejoystick)
    return;
  SDL_InitSubSystem(SDL_INIT_JOYSTICK);
  num_joysticks = SDL_NumJoysticks();
  if (M_CheckParm("-nojoy") || (usejoystick > num_joysticks) || (usejoystick < 0))
  {
    if ((usejoystick > num_joysticks) || (usejoystick < 0))
      lprintf(LO_WARN, "%sinvalid joystick %d\n", fname, usejoystick);
    else
      lprintf(LO_INFO, "%suser disabled\n", fname);
    return;
  }
  joystick = SDL_JoystickOpen(usejoystick - 1);
  if (!joystick)
    lprintf(LO_ERROR, "%serror opening joystick %s\n", fname, SDL_JoystickName(usejoystick - 1));
  else
  {
    atexit(I_EndJoystick);
    lprintf(LO_INFO, "%sopened %s\n", fname, SDL_JoystickName(usejoystick - 1));
    joyup = 32767;
    joydown = -32768;
    joyright = 32767;
    joyleft = -32768;
  }
#endif
}
