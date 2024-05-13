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
   This code provides services for Zytronic ZXY100 USB Touchscreen devices
   where those services can't be accessed via the later (MT) service set.

   The services offered are separated from the general services (see services.h)
   as the following are only required for unusual ZXY100 (and perhaps ZXY110)
   queries.

   These are used as an extension of the general services as required.

   In many cases, the later API calls these services so that the user need not
   be concerned with the older protocols. This is the general intent.
 */

#ifndef _ZY_SERVCS_SC_H
#define _ZY_SERVCS_SC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zytypes.h"
#include "protocol.h"

#include "version.h"
#include "zxy100.h"
#include "zxy110.h"
#include "zxymt.h"
#include "services.h"


// === Useful Datatypes =======================================================

/**
 * Data structures to support the self-capacitance raw-data graphics
 */

typedef struct ZXY100_rawImage_t
{
    ZXY_sensorSize  sensorSz;
    bool            allValid;
    uint8_t         wireSig[128];       // NB 8 bit
    // ToDo  -- make 16 bit and merge with ZXY110
} ZXY100_rawImage;

typedef struct ZXY110_rawImage_t
{
    ZXY_sensorSize  sensorSz;
    bool            allValid;
    uint16_t        wireSig[128];       // NB 16 bit
    // there are no 128 wire ZXY110s yet, ... better safe than sorry. 24/05/2018
} ZXY110_rawImage;


// === Services ===============================================================

void            zul_InitServSelfCap             (void);
void            zul_ResetSelfCapData            (void);

int             zul_getZxy100VersionStr         (VerIndex verType, char *v, int len);
bool            zul_old_BLgetVersion            (void);

uint16_t        zul_getZxy100StatusCount        (void);
uint16_t        zul_getZxy100ConfigCount        (void);

int             zul_getOldZxy100WireCnt         (uint16_t *xWC, uint16_t *yWC);

int             zul_getOldSysReport             (Zxy100SysReport *d);
int             zul_getNoiseAlgoMetric          (Zxy100SysReport *d);
int             zul_getSingleRawData            (Zxy100RawData *d);

int             zul_getOldTouchReport           (Zxy100TouchReport *d);

void            zul_SetRawDataBuffer100         (void *buffer);
void            zul_setRawMode100               (int mode);

struct timeb *  zul_zxy100RawInTime             (void);

void            handle_IN_rawdata_100           (uint8_t *data);
void            handle_IN_rawdata_110           (uint8_t *data);
void            handle_IN_rawdata_110_Clipped   (uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif // _ZY_SERVCS_SC_H
