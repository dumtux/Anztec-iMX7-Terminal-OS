/*
 *  Copyright (c) 2016 Zytronic Displays Limited. All rights reserved.
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
   This code should provide the basic communication services required
   to:
    - discover Zytronic (USB) products connected to the system
    - connect/disconnect to/from one of the available devices
    - manage that device

    Dependancies:

        Hid_API from http://www.Signal-11.us/oss/hidapi

 */


#ifndef _ZY_USB_H
#define _ZY_USB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zytypes.h"

#define ZYTRONIC_VENDOR_ID          (0x14C8)

#define ZXY100_PRODUCT_ID           (0x0005)
#define ZXY100_BOOTLDR_ID           (0x000A)

#define ZXY110_PRODUCT_ID           (0x0009)
#define ZXY110_BOOTLDR_ID           (0x000D)

#define ZXY150_PRODUCT_ID           (0x0014)
#define ZXY150_BOOTLDR_ID           (0x0015)

#define ZXY200_PRODUCT_ID           (0x0006)
#define ZXY200_BOOTLDR_ID           (0x000B)

#define ZXY300_PRODUCT_ID           (0x0007)
#define ZXY300_BOOTLDR_ID           (0x000C)

#define ZXY500_PRODUCT_ID           (0x0016)
#define ZXY500_BOOTLDR_ID           (0x0017)

// pointer to touch data handler
typedef void(*interrupt_handler_t)(uint8_t *d);

// pointer to control data handler
typedef int(*response_handler_t)(uint8_t *d);

// periodic call to service USB needs
// void service_zxy100(interrupt_handler_t th);


/**
 * Call to initialise the library. Zero returned on success.
 * Returns: USB lib error code on failures
 */
int         zul_openLib         (void);
void        zul_closeLib        (void);
char *      zul_usbLibStr       (void);

/**
 * This service lists the available devices on the system
 */
int         zul_getDeviceList   (char *buf, int len);

/**
 * return true if the connected device is a bootloader
 */
bool        zul_isBLDevice      (int index, char * list);
bool        zul_isBLDevicePID   (int16_t pid);

/**
 * if the list contains a particular PID, then return the index
 */
int         zul_selectPIDFromList(int16_t pid, char * list);

/**
 * given a USB Product ID, return a device name string :
 *      ZXY100, ZXY110, ZXY150, ZXY200, ZXY300
   and vice versa
 */
char const *zul_getDevStrByPID  (int16_t pid);
int16_t     zul_getBLPIDByDevS  (char const *devName);
int16_t     zul_getAppPIDByDevS (char const *devName);

/**
 * Open a particular device, based on the indices provided by zul_getDeviceList()
 * NB: only open one at a time
 */
int         zul_openDevice      (int index);

/**
 * If a device is open, set the supplied pid and return 1;
 * Else, return 0
 */
bool        zul_getDevicePID    (int16_t *pid);

/**
 * Close an open device
 */
int         zul_closeDevice     ();

/**
 * Make a control-request to the connected device
 * Returns the number of bytes sent or on an error a negative code.
 */
int         zul_ControlRequest  (uint8_t *request, uint16_t reqLen,
                                 /*@null@*/ response_handler_t handle_reply);

/**
 * Make a control-request to the connected device, which expects more than
 * one control-response from the device.
 * Returns the number of bytes sent or on an error a negative code.
 */
int         zul_ControlRequestMR(uint8_t *request, uint16_t reqLen,
                                 /*@null@*/ response_handler_t handle_reply,
                                 int replyCount);


/**
 * Attempt an interrupt transfer to the connected device
 *    ...
 */
// int         zul_InterruptCheck  (interrupt_handler_t data_handler);


#ifdef __cplusplus
}
#endif


#endif // _ZY_USB_H


