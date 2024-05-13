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
   This code provides user services for the Zytronic USB Touchscreen devices

   The services offered are:

        - service initialisation/termination

        - device discovery, connection & disconnection

        - Clear the on-board calibration
        - Set the on-board calibration

        - Get and Set configuration Parameters
        - Get status values
        - Get version data

        - Firmware update
        - Raw data fetch
 */

#ifndef _ZY_SERVICES_H
#define _ZY_SERVICES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zytypes.h"
#include "protocol.h"
#include "version.h"
#include "zxy100.h"
#include "zxy110.h"
#include "zxymt.h"

#define BL_RESET_DELAY_MS       (4000)


// === Useful Datatypes =======================================================

typedef struct calibration_t
{
    uint16_t val[ZXY100_CN_ONBOARD_CAL_COUNT];

    /*  Assignmemt of array elements:
     *  TOP LEFT
     *    0 - Screen  X in range 0..4095 (target)
     *    1 - Screen  Y in range 0..4095
     *    2 - Touched X in range 0..4095 (measured)
     *    3 - Touched Y in range 0..4095
     *  BOTTOM RIGHT
     *    4 - Screen  X in range 0..4095 (target)
     *    5 - Screen  Y in range 0..4095
     *    6 - Touched X in range 0..4095 (measured)
     *    7 - Touched Y in range 0..4095
     *
     * typically screen top left target X & Y values are 5% of 4095 = 205 (0x00cc)
     * and screen bottom right target X & Y values are 95% of 4095 = 3890 (0x0F32)
     *
     * FYI, 4096 = 0x1000; 4095 = 0x0FFF
     */

} Calibration;

typedef struct Contact_t
{
    uint8_t ID, flags;
    int     x,y;
} Contact;

typedef struct ZXY_sensorSize_t
{
    uint16_t        xWires, yWires;
} ZXY_sensorSize;

// === Services ===============================================================

/**
 * Reset all internal state, ready for first use
 */
int             zul_InitServices                (void);


/**
 * Terminate all services, free all resources
 */
void            zul_EndServices                 (void);


/**
 * Get the Zytronic Library version
 *      TODO provide string "long" to get libUSB version also
 */
void            zul_getVersion                  (char *buffer, int len);

/**
 * provide access to the underlying libUSB version
 */
char *          zul_usbLibStr                   (void);


/**
 * sleep for a number of milliseconds
 */
void            zy_msleep                       (uint32_t ms);

/**
 * return true if the connected device is a bootloader
 */
bool            zul_isBLDevice                  (int index, char * list);
bool            zul_isBLDevicePID               (int16_t pid);

/**
 * return true if connected to a ZXY500 Application firmware
 */
bool            zul_isZXY500AppPID              (int16_t *pid);

/**
 * given a USB Product ID, return a device name string :
 *      ZXY100, ZXY110, ZXY150, ZXY200, ZXY300
   and vice versa
 */
char const *    zul_getDevStrByPID              (int16_t pid);
int16_t         zul_getBLPIDByDevS              (char const *devName);
int16_t         zul_getAppPIDByDevS             (char const *devName);
char const *    zul_getZYFFilter                (void);


/**
 * return a string listing the connected Zytronic Touchscreens, one per line
 * return count is the number of devices
 * negative return values indicate a fault
 */
int             zul_getDeviceList               (char *buf, int len);


/**
 * if the list contains a particular PID, then return the index
 */
int             zul_selectPIDFromList           (int16_t pid, char * list);


/**
 * if the args list contains a "deviceKey=" option, remove it and return the key
 */
char *          zul_removeDeviceTargetKey           (int *pNumArgs, char *argv[]);

/**
 * if the args list contains a "Addr=" option, remove it and return the key
 */
 char *         zul_removeDeviceTargetAddr          (int *pNumArgs, char *argv[]);

/**
 * Open a particular device, based on the indices provided by zul_getDeviceList()
 * NB: only open one at a time
 */
int             zul_openDevice                  (int index);

/**
 * If a device is open, copy the USB address string to the supplied buffer,
 * returning SUCCESS.  Else return a negative error number.
 */
int             zul_getAddrStr                  (char * addrStr);

/**
 * Open a particular device, based on the address string provided
 * NB: only open one at a time
 */
int             zul_openDeviceByAddr            (char *addrStr);

/**
 *  Re-Open the last device closed, by index provided by zul_getDeviceList()
 */
int             zul_reOpenLastDevice            (void);

/**
 * If a device is open, set the supplied pid and return true
 * Else, return false
 */
bool            zul_getDevicePID                (int16_t *pid);

/**
 * If a device is open, set the supplied sensor size struct and return true
 * Else, return false
 */
bool            zul_getSensorSize               (ZXY_sensorSize *sz);
/**
 * Set the connected interface to #0 with parameter 'true' or to the auxilliary
 * interface with parameter 'false'.
 * The auxilliary interface #1 was introduced with ZXY500 for management of
 * device, without taking the touch services from the linux kernel.
 */
int             zul_useKernelIFace              (bool kernel);

/**
 * Close an open device - the index is remembered.
 */
int             zul_closeDevice                 (void);


// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//  Control the "robustness" of the communications
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

typedef enum    commsEnduranceCode
{
    COM_ENDUR_NORM, COM_ENDUR_MEDIUM, COM_ENDUR_HIGH
} Endurance;

void            zul_setCommsEndurance           (Endurance code);
Endurance       zul_getCommsEndurance           (void);


// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// -  Standard get/set/status accessors
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

int             zul_getStatusByID               (uint8_t ID, uint16_t *status);
int             zul_getSpiRegister              (uint8_t device, uint8_t reg, uint16_t *value);
int             zul_getConfigParamByID          (uint8_t ID, uint16_t *config);
int             zul_setConfigParamByID          (uint8_t ID, uint16_t config);
// test the setting of an option bit
bool            zul_optionAvailable             (uint16_t optionBit);

/**
 * Standard device version string accessors
 */
int             zul_getVersionStr               (VerIndex verType, char *v, int len);
int             zul_Firmware                    (char *v, int len);
int             zul_Bootloader                  (char *v, int len);
int             zul_Hardware                    (char *v, int len);
int             zul_Customization               (char *v, int len);
int             zul_CpuID                       (char *v, int len);

/*
 * The ZXY500 Failsafe mode has a number of different reason codes but the No sensor
 * reason code is dealt with differently by the ZyConfig Tool displaying a message
 * to the user. This is an accessor to a the Boolean that records whether this
 * message has been displayed for this controller.
 */
bool            getShowNoSensor                 (void);

/*
 * set the Boolean that records whether the no sensor message has been shown
 */
void            setShowNoSensor                (bool b);

/**
 * When flash writing is disabled, it's much faster to load a set of
 * configuration parameters.  The controller's default is that flash
 * writing is enabled, it may be temporarily disabled to accelerate a
 * load-configuration process.
 */
void            zul_inhibitFlashWrites          (bool inhibit);

/**
 * General service to send a single byte message holding only the message-code.
 */
void            zul_sendMessageCode             (uint8_t msgCode);

/**
 * Restart the micro-controller
 */
void            zul_resetController             (void);

/**
 * Reset the device configuration to factory defaults
 */
void            zul_restoreDefaults             (void);

/**
 * Force sensor equalisation
 */
void            zul_forceEqualisation           (void);

/**
 * stop touch application, start Boot Loader mode
 *   NB bootloader protocol is VERY different to application protocol.
 */
void            zul_StartBootLoader             (void);


// ============================================================================
// --- Calibration Support Functions (OnBoard Calibraion  Only) ---
// ============================================================================

/**
 * Clear the on board calibration
 */
void            zul_clearOnBoardCal             (void);

/**
 * Set a test calibration
 */
void            zul_TestSetOnBoardCal           (void);

/**
 * set the on board calibration
 */
void            zul_setOnBoardCal               (Calibration *c);


// ============================================================================
// ---  Virtual Key Programming Services ---
// ============================================================================

/**
 * Clear a virtual key definition
 */
void            zul_clearVirtKey                (int index);

/**
 * Set a virtual key definition
 */
void            zul_setVirtKey                  (VirtualKey *vk);

/**
 * Get a virtual key definition
 */
void            zul_getVirtKey                  (int index, VirtualKey *vk);



// ============================================================================
// ---  Interrupt Transfer Services ---
// ============================================================================


/**
 *   TODO ... why this and that below
 */
void            zul_SetupStandardInHandlers     (void);

/**
 * By default, when an interrupt transfer occurs, and no handler has been
 * assigned [usb_RegisterHandler] the data is printed in hex.  This is not
 * ideal, so default handlers are provided for the likely REPORT_IDs. These are
 * enabled by this service:
 * TODO - encapsulate in usb.c !
 */
void            zul_ResetDefaultInHandlers      (void);

void            zul_setRawDataHandler               (void);

void            zul_SetSpecialHandler           (UsbReportID_t ReportID,
                                                interrupt_handler_t handler);

/**
 * Some basic routines to access touch event packets
 *   latest info only available, if not serviced often enough,
 *   packets may be missed.
 */
/*@null@*/
uint8_t *       zul_GetTouchData                (void);
bool            zul_Get1TouchFromData           (uint8_t *data, Contact *c);
/* Ideally, the library would maintain up to 100 touch histories, updating them
 * as the data is received, and making the current touch state available to the
 * program on demand.    - CFM 05/10/2017  -- ToDo -- */

/**
 * Return true if a touch is available.
 * Both private/silent or HID mode are catered for here.
 */
bool            zul_TouchAvailable              (Contact *c);

/**
 * Wait for a touch-release event, or the supplied timeout
 * Both private/silent or HID mode are catered for here.
 */
bool            zul_GetTouchUp                  (int timeout_ms, Contact *c);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
 * MSGCODE_SET_SILENT_TOUCH_DATA_MODE is available on some devices
 * When enabled, the HID touch events are NOT generated, but the touch data
 * is relayed (in the same format) to USB Report ID #6. (same as raw data !)
 * This 'disables' normal touch operation on most systems, and makes the touch
 * data available to a single receiving application. One use of this is for a
 * TUIO server */
void            zul_SetPrivateTouchMode         (bool enabled);

/**
 * Get any touch data into a user supplied buffer
 */
int             zul_GetPrivateTouchData         (uint8_t *buffer, int bufSize);


// ============================================================================
// --- Raw Data Mode services ---
// ============================================================================

/**
 *  set the application image buffer to receive interrupt data
 */
void            zul_SetRawDataBuffer            (void *buffer);

/**
 * set the device mode - normal or raw data
 */
void            zul_SetRawMode                  (int rawMode);
void            zul_SetTouchMode                (int touchMode);

/**
 * Wait for a interrupt packet, or a time-out
 * If an interrupt packet is received, store data in the 'configured'
 * buffer. [see g_image]
 * 'timeout_ms' not used as yet...
 */
bool            zul_GetRawData                  (void);

/**
 *  Provide a monitor that reports the age of the last Raw Data frame received
 */
long            zul_getRawInAgeMS               (void);

/**
 * There is some special data packets delivered in raw mode - ask PR.
 */
uint8_t *       zul_GetSpecialRawData           (void);
/*@null@*/
uint8_t *       zul_GetHeartBeatData            (void);

// ============================================================================
// --- High-level methods to support Firmware Update ---
//
// NB July 2016: FW Update is NOT supported on the HID-API/OSX implementation
//               as yet
// ============================================================================

bool            zul_checkZYFmatchesHW           (char const * hwID, char const *filename);

bool            zul_BLPingOK                    (void);
bool            zul_BLgetVersion                (char *VerStr, int len, VerIndex index);
bool            zul_BLgetUniqID                 (char *IDStr, int len);

bool            zul_BLgetVersionFromResponse    (char *VerStr, int len);
bool            zul_BLgetUniqIDFromResponse     (char *IDStr, int len);

bool            zul_BL_RebootToApp              (void);
bool            zul_BL_RebootToBL               (void);

void            zul_BLresetPktCount             (void);   // hide ??

int             zul_loadAndValidateZyf          (char const *Firmware);
int             zul_getFwTransferCount          (void);
char *          zul_getZyfXferResultStr         (void);

int             zul_testProgDataBlock           (void);
int             zul_transferFirmware            (bool track);       // pthreaded !?
int             zul_transferFirmwareStatus      (uint32_t *Size, uint32_t *LeftToWrite);

#ifdef __cplusplus
}
#endif

#endif // _ZY_SERVICES_H
