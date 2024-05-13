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



/* Overview
   ===============
    This allows a consistent data type set to be used across
    the implementation files
 */


#ifndef _ZY_TYPES_H
#define _ZY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __APPLE__
#include <hidapi.h>
typedef unsigned int uint;
#endif

// ---

#define ZYTRONIC_VENDOR_ID          (0x14C8)

#define UNKNWN_PRODUCT_ID           (0x0001)    // INVALID ID

#define ZXYZXY_PRODUCT_ID           (0x0003)    // NOT SUPPORTED
#define USB32C_PRODUCT_ID           (0x0004)    // NOT SUPPORTED

#define ZXY100_PRODUCT_ID           (0x0005)
#define ZXY100_BOOTLDR_ID           (0x000A)

#define ZXY110_PRODUCT_ID           (0x0009)
#define ZXY110_BOOTLDR_ID           (0x000D)

#define ZXY150_PRODUCT_ID           (0x0014)
#define ZXY150_BOOTLDR_ID           (0x0015)

#define ZXY200_PRODUCT_ID           (0x0006)
#define ZXY200_BOOTLDR_ID           (0x000B)
#define ZXY200_PRODUCT_ID_ALT1      (0x0018)

#define ZXY300_PRODUCT_ID           (0x0007)
#define ZXY300_BOOTLDR_ID           (0x000C)

#define ZXY500_PRODUCT_ID           (0x0016)
#define ZXY500_BOOTLDR_ID           (0x0017)
#define ZXY500_PRODUCT_ID_ALT1      (0x0019)

// ---

// pointer to touch data handler
typedef void(*interrupt_handler_t)(uint8_t *d);

typedef enum
{
    UNUSED_00    = 0,
    TOUCH_OS        ,   //Used to send touch data packets (Touchscreen Mode) to the OS              (Interrupt transfers)
    MAX_CONTACTS    ,   //Used to tell OS about Maximum Number of Contacts that this device supports
    DEVICE_MODE     ,   //Used by OS to configure the Device Mode - HID mouse, Single-touch, Multi-touch
    MOUSE_OS        ,   //Used to send touch data packets (Mouse Mode) to the OS
    CONFIGURATION   ,   //Used to communicate with Zytronic protocol layer.                         (Control transfers)
    RAW_DATA        ,   //Used to send ZYTRONIC RAW DATA.-- AND -- TOUCH_APP (silent mode touches)  (Interrupt transfers)
    HEARTBEAT_REPORT,   //Used to send ZYTRONIC USB HEARTBEAT PACKET / DEBUG REPORTS                (Interrupt transfers)
    KEYBOARD_OS     ,   //Used to send HID Keyboard packets to the OS                               (Interrupt transfers)
    UNUSED_09       ,
    UNUSED_10       ,
} UsbReportID_t;

#define  MAX_REPORT_ID              (10 + 1)

// ---

#define     SUCCESS             (1)
#define     FAILURE             (0)

// ---

typedef struct size2d_t
{
    uint16_t        x, y;
} Size2d;

typedef struct location_t
{
    uint16_t        x, y;
} Location;

#include "keycodes.h"
typedef struct virtualKey_t
{
    uint8_t     ID;
    Location    TopLeft;
    Location    BottomRight;
    KbdModifier modifier;
    KeyScanCode keycode[6];
} VirtualKey;


#ifdef __cplusplus
}
#endif

#endif  // _ZY_TYPES_H
