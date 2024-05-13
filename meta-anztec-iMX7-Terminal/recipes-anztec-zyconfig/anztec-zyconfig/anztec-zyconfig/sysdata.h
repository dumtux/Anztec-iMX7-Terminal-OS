/*
 *  Copyright (c) 2019 Zytronic Displays Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact Zytronic, you can do so either via the
 * website <www.zytronic.co.uk> or by paper mail:
 * Zytronic, Whiteley Road, Blaydon on Tyne, Tyne & Wear, NE21 5NJ, UK
 */



/* Module Overview
   ===============
   This code should provide simple access to system (PC) data

 */

#ifndef _ZY_SYSDATA_H
#define _ZY_SYSDATA_H


#ifdef __cplusplus
extern "C" {
#endif

#include "zytypes.h"

/**
 * accessors to the system/OS data
 */

const char *      getUpTime               (void);

const char *      getMACs                 (void);

const char *      getOSinfo               (void);

void              cacheTouchEventPaths    (void);
int               getTouchEventPathIndex  (char const *uniqueID);

const char *      getXDisplayInfo         (void);

/**
 * Accessors to any xrandr data
 * NB:  disconnected outputs are completely ignored!
 */

typedef enum xMonitorOrientation_e
{
    xmo_Normal = 0,
    xmo_Left,
    xmo_Inverted,
    xmo_Right,
    xmo_invalid
} XMonitorOrientation;

#define X_MAX_SCRN      (4)
#define X_MAX_MONS      (8)

typedef struct XOrgMonitorData_t
{
    char                name[20];
    bool                primary;
    XMonitorOrientation orientation;
    Size2d              size;
    Location            location;
} XOrgMonitorData;

typedef struct xOrgScreenData_t
{
    uint8_t         numMonitors;
    Size2d          size;

    // MAX of 8 monitors for one user
    XOrgMonitorData monitor[X_MAX_MONS];

} XOrgScreenData;

/**
 * If zero screens are returned, the accessors should not be trusted
 */
int                     getNumberOfScreens      (void);


Size2d                  getScreenSize           (int screen);

int                     getNumberOfMonitors     (int screen);

const char *            getMonitorName          (int screen, int output);
const char *            getPrimaryMonitor       (int screen);

XMonitorOrientation     getMonitorOrientation   (int screen, int output);
Size2d                  getMonitorSize          (int screen, int output);
Location                getMonitorLocation      (int screen, int output);

#ifdef __cplusplus
}
#endif

#endif // _ZY_SYSDATA_H
