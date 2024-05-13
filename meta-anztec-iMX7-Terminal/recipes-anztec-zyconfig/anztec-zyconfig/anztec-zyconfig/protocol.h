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
   This code provides methods that encode requests to Zytronic Touchscreen
   controllers. The requests supported are:

        SET a configuration setting
        GET a configuration setting
        Read a STATUS value
        Read a VERSION string
        Restore device to default (factory) configuration

    It also provides methods to parse the reply

 */


#ifndef _ZY_PROTOCOL_H
#define _ZY_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif


#include "zytypes.h"


#define  SINGLE_BYTE_MSG_LEN        (8)
#define  DUAL_BYTE_MSG_LEN          (9)
#define  BL_REPLY_BUF_LEN           (20)    // big enough for the BL Versions report (17 bytes)


enum opCodesBL  // ToDo - move to Protocol.c, with the BL parsers
{
    BLProgramStart      = 1,
    BLRebootToApp       = 2,
    BLRebootToBL        = 3,
    BLPing              = 4,
    BLGetVersionData    = 5,        // deprecated !
    BLGetVersionStr     = 79,       // 0x4f
    /*
    GET_INTEGRITY_TEST_STATE            = 101,
    CMD_PEEK                            = 102,
    CMD_XFER_HASH                       = 103,
    CMD_XFER_RSA_SIGNATURE              = 104,
    CMD_XFER_APPLICATION_VERSION_STRING = 105,
    */
};

enum replyCodesBL
{
    BL_RSP_SIZE_ERROR  = 0x01,
    BL_RSP_BLOCK_WRITTEN,           //2
    BL_RSP_PROGRAMMING_COMPLETE,    //3
    BL_RSP_PING,                    //4
    BL_RSP_BL_VERSIONS,             //5
    BL_RSP_CRC_ERROR,               //6
    BL_RSP_PROGRAMMING_FAILED,      //7
    BL_RSP_COMMS_ERROR,             //8

    BL_RSP_Acknowledge  = 0xAA,
    BL_RSP_VersionStr   = 0x4f,
};


typedef enum versionStringIndex
{
    STR_BL, STR_FW, STR_HW, STR_AFC, STR_CPUID
} VerIndex;


/**
 * Generic encoder of a single Message-Code message (command)
 */
bool        zul_encodeSingleByteMessage (/*@out@*/ uint8_t *buffer, int bufLen, uint8_t byte);

/**
 *  Some common command encoders
 */
bool        zul_encodeRestoreDefaults   (/*@out@*/ uint8_t *buffer, int bufLen);
bool        zul_encodeResetController   (/*@out@*/ uint8_t *buffer, int bufLen);
bool        zul_encodeForceEqualisation (/*@out@*/ uint8_t *buffer, int bufLen);
bool        zul_encodeStartBootLoader   (/*@out@*/ uint8_t *buffer, int bufLen);
bool        zul_encodeSetFlashWrite     (/*@out@*/ uint8_t *buffer, int bufLen, bool enabled);
bool        zul_encodeForceFlashWrite   (/*@out@*/ uint8_t *buffer, int bufLen);


/**
 * create a message in the supplied buffer to fetch a config value from the
 * controller
 */
bool        zul_encodeGetRequest        (/*@out@*/ uint8_t *buffer, int bufLen, uint8_t index);

/**
 * create a message in the supplied buffer to fetch a status value from the
 * controller
 */
bool        zul_encodeGetStatus         (/*@out@*/ uint8_t *buffer, int bufLen, uint8_t index);

/**
 * create a message in the supplied buffer to fetch an SPI Register value from
 * the controller
 */
bool        zul_encodeGetSpiRegister    (/*@out@*/ uint8_t *buffer, int bufLen,
                                                     uint8_t device, uint8_t index);

/**
 * create a message in the supplied buffer to set a a config value to a
 * given value
 */
bool        zul_encodeSetRequest        (/*@out@*/ uint8_t *buffer, int bufLen,
                                                uint8_t index, uint16_t value);

/**
 * create a message in the supplied buffer to fetch a version string from
 * the controller
 */
bool        zul_encodeVerStrRequest     (/*@out@*/ uint8_t *buffer, int bufLen, VerIndex index);


/**
 * create a message in the supplied buffer to set the device to raw mode
 * or to return to normal mode
 */
bool        zul_encodeRawModeRequest    (/*@out@*/ uint8_t *buffer, int bufLen, int mode);
bool        zul_encodeTouchModeRequest  (/*@out@*/ uint8_t *buffer, int bufLen, int mode);

/**
 * create a message in the supplied buffer to set the device to private touch mode
 * which disabled the normal HID operation, and delivers touch data on USB Report ID #6
 */
bool        zul_encodePrivateTouchModeRequest(uint8_t *buffer, int bufLen, bool mode);


// ========================================================================================
//          ZXY500 Virtual Key Messages
// ========================================================================================

bool        zul_encodeVirtKeySet        (/*@out@*/ uint8_t *buffer, int bufLen, VirtualKey *vk);
bool        zul_encodeVirtKeyGet        (/*@out@*/ uint8_t *buffer, int bufLen, int index);
bool        zul_encodeVirtKeyClear      (/*@out@*/ uint8_t *buffer, int bufLen, int index);


// ========================================================================================
//          ZXY100 odd messages
// ========================================================================================

bool        zul_encodeGetSingleRawData  (/*@out@*/ uint8_t *buffer, int bufLen);
bool        zul_encodeGetSingleTouchData(/*@out@*/ uint8_t *buffer, int bufLen);

bool        zul_encodeOldVersionReq     (/*@out@*/ uint8_t *buffer, int bufLen);
bool        zul_encodeOldSysReportReq   (/*@out@*/ uint8_t *buffer, int bufLen);


// ========================================================================================
//          Bootloader Services
// ========================================================================================

bool        zul_encode_BL_RebootToApp   (/*@out@*/ uint8_t *buffer, int bufLen);
bool        zul_encode_BL_RebootToBL    (/*@out@*/ uint8_t *buffer, int bufLen);
bool        zul_encode_BL_PING          (/*@out@*/ uint8_t *buffer, int bufLen);
bool        zul_encode_BL_GetVerStr     (/*@out@*/ uint8_t *buffer, int bufLen, VerIndex index);

bool        zul_encode_BL_VersionF      (/*@out@*/ uint8_t *buffer, int bufLen);  // deprecated

    // this call starts the data transfer session, there is practically NO protocol.
    // see TR0318, and other firmware transfer code, ZyConfig ...
bool        zul_encode_BL_ProgDataBlock (/*@out@*/ uint8_t *buffer, int bufLen,
                                                size_t fwSize, uint8_t *pinfo);


// ========================================================================================
//          general utilities
// ========================================================================================

/**
 * general purpose CRC calculator
 */
uint16_t    zul_getCRC                  (uint8_t *buf, size_t len);
bool        zul_checkCrc                (size_t len, uint8_t *buf);


#ifdef __cplusplus
}
#endif

#endif // _ZY_PROTOCOL_H
