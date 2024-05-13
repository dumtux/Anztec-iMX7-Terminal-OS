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
   This code should provide the basic USB communication services required
   to:
    - discover Zytronic (USB) products connected to the Linux system
    - connect/disconnect to/from one of the available devices
    - manage that device

    Dependancies:

        http://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.9/

 */

#ifndef _ZY_USB_H
#define _ZY_USB_H

#include "zytypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  USB_PACKET_LEN             (64)

// pointer to control data handler
typedef int(*response_handler_t)(uint8_t *d);


/**
 * Call to initialise the library. Zero returned on success.
 * Returns: USB lib error code on failures
 */
int         usb_openLib         (void);
void        usb_closeLib        (void);
char *      usb_usbLibStr       (void);

/**
 * This service lists the available devices on the system
 */
int         usb_getDeviceList   (char *buf, int len);

/**
 * return true if the connected device is a bootloader
 */
bool        usb_isBLDevicePID(int16_t pid);




// ============================================================================
// --- Device Interactions ---
// ============================================================================

/**
 * Open a particular device, based on the indices provided by zul_getDeviceList()
 * NB: only open one at a time
 */
int         usb_openDevice      (int index);
/**
 * Open a particular device, based on the supplied usb bus address string
 * NB: only open one at a time
 */
int         usb_openDeviceByAddr(char * addrStr);

/**
 * If a device is open, copy the USB address string to the supplied buffer,
 * returning SUCCESS.  Else return a negative error number.
 */
int         usb_getAddrStr      (char * addrStr);

/**
 *  Re-Open the last device closed, by index provided by zul_getDeviceList()
 */
 int        usb_reOpenLastDevice(void);

/**
 * If a device is open, set the supplied pid and return SUCCESS;
 * Else, return FAILURE
 */
bool        usb_getDevicePID    (int16_t *pid);

/**
 * If a device is open, attempt to switch to the indicated interface
 * Return true => SUCCESS else failed (interface not available)
 */
bool        usb_switchIFace     (uint8_t iface);

/**
 * Close an open device - the index is remembered.
 */
int         usb_closeDevice     (void);



/**
 * Make a control request to the connected device
 * Returns the number of bytes sent or on an error a negative code.
 */
int         usb_ControlRequest          (uint8_t *request, uint16_t reqLen,
                                 /*@null@*/ response_handler_t handle_reply);
/**
 * Make a control-request to the connected device, which expects more than
 * one control-response from the device.  [ZXY100 get single raw data]
 * Returns the number of bytes sent or on an error a negative code.
 */
int         usb_ControlRequestMR        (   uint8_t *request, uint16_t reqLen,
                                    /*@null@*/ response_handler_t handle_reply,
                                            int replyCount);

/**
 * Alter the USB control comms delay, in microseconds. It has been useful to
 * lengthen the period between the TX and RX transfers in certain conditions.
 * The default is 5000 (see var msv_CtrlDelay) but if issues occur a value of
 * 50,000 has been seen to help.
 */
void        usb_setCtrlDelay            (int delay);
void        usb_defaultCtrlDelay        (void);

void        usb_setCtrlRetry            (int delay);
void        usb_defaultCtrlRetry(void);

void        usb_setCtrlTimeout          (int delay);
void        usb_defaultCtrlTimeout      (void);

// ============================================================================
// --- Asynchronous Interrupt Transfer Support ---
// ============================================================================


/**
 * Register a handler function for a reportID (Interrupt Transfers)
 */
void        usb_RegisterHandler         ( UsbReportID_t ReportID,
                                            /*@null@*/
                                            interrupt_handler_t handler );

/**
 * By default, when an interrupt transfer occurs, and no handler has been
 * assigned [usb_RegisterHandler] the data is printed in hex.  This is not
 * ideal, so default handlers are provided for the likely REPORT_IDs. These are
 * enabled by this service:
 */
void        usb_ResetDefaultInHandlers  (void);


// ============================================================================
// --- Interrupt Data Handlers ---
// ============================================================================




// ============================================================================
// --- Control Data Handlers ---
// ============================================================================



#ifdef __cplusplus
}
#endif

#endif // _ZY_USB_H
