/*
 * Copyright 2019 Zytronic Displays Limited, UK.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* For a module overview, see the header file

    The PC is the master of communications, and the controller is the slave.

    A packet is sent to the controller, and in normal cases a packet is returned
    either providing the data requested, or acknowledging the command receipt.

    From serial communications protocols, all packets are wrapped inside
    framing bytes. Start-of-frame is 0x02 (ascii STX), and end-of-frame is
    0x03 (ascii ETX).

    The packet itself has a header of two bytes, the first of which is the
    total length of the packet.  The second header byte is the packet-ID.

    The packet length covers all bytes in the packet, including the length byte,
    and the CRC. A frame length is 2 bytes greater than the packet length, as it
    includes the STX and ETX bytes.

    The packet-IDs that are used are master-request (102 = 0x66) and
    slave-response (106 = 0x6A).

    The final two bytes of the packet is used to carry a 16 bit CRC.

    All other bytes in the packet are payload (d1 ... dn).

        ||| STX  || LEN | TYPE || d1 | d2 ... dn || CRC1 | CRC2 || ETX  |||

    The first payload byte (d1 above) identifies the message, and is known as
    the mesage-code.

    The LEN and TYPE bytes are INCLUDED in the CRC.

    As a fram example, the following is taken from the (ZXY100) protocol
    document:

        // Restore Factory Settings
        tempBuffer2[0] = 0x02;  // STX
        tempBuffer2[1] = 0x05;  // LEN
        tempBuffer2[2] = 0x66;  // PktID
        tempBuffer2[3] = 0x29;  // MsgCode
        tempBuffer2[4] = 0x37;  // CRC-lsb
        tempBuffer2[5] = 0xff;  // CRC-msb
        tempBuffer2[6] = 0x03;  // ETX

 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include "dbg2console.h"
#include "zytypes.h"
#include "protocol.h"
#include "debug.h"


//
// --- Module Global Variables ---
//


// Zytronic Control Collection (USB terminology!)
static const uint8_t ZCC       = 0x05;

static const uint8_t STX       = 0x02;
static const uint8_t ETX       = 0x03;

enum messageCodes
{
    ReadViaSpi          =  37,  //  0x25
    RestoreDefaults     =  41,  //  0x29
    ResetController     =  61,  //  0x3D
    ForceEqualisation   =  62,  //  0x3E
    SetRawMode          =  64,  //  0x40
    SetParam            =  77,  //  0x4D
    GetParam            =  78,  //  0x4E
    GetVersionString    =  79,  //  0x4F
    SetSilentTouchMode  =  83,  //  0x53
    StartBootLoader     =  99,  //  0x63
    GetStatus           = 113,  //  0x71

    // ZXY100 Only
    SetTouchMode        =  63,  //  0x3F
    GetSingleTouch100   =  65,  //  0x41 -- still required @ FW 501.36
    GetSingleRawData100 =  66,  //  0x42 -- still required @ FW 501.36
    OLD_GetVersions     =  73,  //  0x49 -- deprecated see GetVersionString
    Old_GetSysReport    =  76,  //  0x4c -- still required @ FW 501.36

    // the following may not be available in the ZXY100/110 ? TBD
    DisableFlashWrite   =           0x82,
    EnableFlashWrite    =           0x83,
    ForceFlashWrite     =           0x84,

    SetVirtualButton    = 151,  //  0x97
    GetVirtualButton    = 152,  //  0x98
    ClearVirtualButton  = 153,  //  0x99
};


enum packetIDs
{
    MasterRequest       =  102,  // 0x66
    SlaveResponse       =  106,  // 0x6A
};


//
// --- Private Prototypes ---
//

static uint8_t         msb_16          (uint16_t val);
static uint8_t         lsb_16          (uint16_t val);
static uint8_t         lsb_int         (int val);

static bool            framePayload    (/*@out@*/  uint8_t *buffer,
                                            int bufLen,
                                            uint8_t *payload, int payloadLen);

// ============================================================================
// --- Public Implementation ---
// ============================================================================

/**
 * zul_getCRC
 */
uint16_t zul_getCRC( uint8_t *buf, size_t len )
{
    int i, j;
    uint16_t crc;

    crc = 0;
    for ( i = 0; i < (int)len; i++ )
    {
        crc = (uint16_t)( crc ^ (buf[i] << 8) );
        for ( j = 0; j < 8; j++ )
        {
            if ( (crc & 0x8000U) > 0 )
            {
                  crc = (uint16_t) ( (crc << 1) ^ 0x1021U );
            }
            else
            {
                crc = (uint16_t) ( crc << 1 );
            }
        }
    }

    return ( crc );
}



/**
 * Generic encoder of a single Message-Code message (command)
 */
bool        zul_encodeSingleByteMessage (uint8_t *buffer, int bufLen,
                                                            uint8_t byte)
{
    if (buffer == NULL)
        return false;

    if (bufLen < 8)
        return false;

    return framePayload(buffer, bufLen, &byte, 1);
}

/**
 * create a message in the supplied buffer to to revert the controller's
 * Configuration settings to their factory default values
 */
bool  zul_encodeRestoreDefaults   (uint8_t *buffer, int bufLen)
{
    return zul_encodeSingleByteMessage(buffer, bufLen, RestoreDefaults);
}

/**
 * create a message in the supplied buffer to to reset the micro-controller
 */
bool  zul_encodeResetController (uint8_t *buffer, int bufLen)
{
    return zul_encodeSingleByteMessage(buffer, bufLen, ResetController);
}

/**
 * create a message in the supplied buffer to tirgger a sensor 'equalisation'
 * see "Temperature Compensation" in the protocol docs.
 */
bool  zul_encodeForceEqualisation (uint8_t *buffer, int bufLen)
{
    return zul_encodeSingleByteMessage(buffer, bufLen, ForceEqualisation);
}

/**
* create a message in the supplied buffer to restart the device in BootLoader mode
*/
bool  zul_encodeStartBootLoader(uint8_t *buffer, int bufLen)
{
    // FYI ... this is a normal APP message, no a BL message !
    return zul_encodeSingleByteMessage(buffer, bufLen, StartBootLoader);
}


/**
 * When flash writing is disabled, it's much faster to load a set of configuration parameters
 * The normal state is enabled. After setting config values when flash writing is disabled
 * (fast), then force a flash write, (which oddly, but automatically re-enabled flash write)
 */
bool    zul_encodeSetFlashWrite (uint8_t *buffer, int bufLen, bool enabled)
{
    uint8_t command = (uint8_t)(enabled ? EnableFlashWrite : DisableFlashWrite);
    return zul_encodeSingleByteMessage(buffer, bufLen, command);
}
bool    zul_encodeForceFlashWrite (uint8_t *buffer, int bufLen)
{
    return zul_encodeSingleByteMessage(buffer, bufLen, ForceFlashWrite);
}


// --- Some ZXY100 Only single byte messages ---

bool  zul_encodeGetSingleRawData  (uint8_t *buffer, int bufLen)
{
    return zul_encodeSingleByteMessage(buffer, bufLen, GetSingleRawData100);
}

bool  zul_encodeGetSingleTouchData(uint8_t *buffer, int bufLen)
{
    return zul_encodeSingleByteMessage(buffer, bufLen, GetSingleTouch100);
}

bool  zul_encodeOldSysReportReq   (uint8_t *buffer, int bufLen)
{
    return zul_encodeSingleByteMessage(buffer, bufLen, Old_GetSysReport);
}

bool  zul_encodeOldVersionReq(uint8_t *buffer, int bufLen)
{
    return zul_encodeSingleByteMessage(buffer, bufLen, OLD_GetVersions);
}


// --- End ZXY100 Only  single byte messages ---


/**
 * create a message in the supplied buffer to set a config value to a
 * given value

     Frame ( d1 = MSG_CODE.SetParam
                | d2 = index | d3 = Value.LSB | d4 = Value.MSB
           )
 */
bool  zul_encodeSetRequest      (uint8_t *buffer, int bufLen,
                                                uint8_t index, uint16_t value)
{
    int payloadLen;
    uint8_t  payload[5] = {0,0,0,0,0};
    uint8_t  *ptr = payload;

    if (buffer == NULL)
        return false;

    if (bufLen < 11)
        return false;

    *ptr = (uint8_t)SetParam;   // message Code

    ptr++;
    *ptr = index;               // config parameter index (8bit)

    ptr++;
    *ptr = lsb_16(value);       // LS Byte value

    ptr++;
    *ptr = msb_16(value);       // MS Byte value

    ptr++;
    payloadLen = (int)(ptr - payload);

    return framePayload(buffer, bufLen, payload, payloadLen);
}


/**
 * create a message in the supplied buffer to set a a config value to a
 * given value

     Frame ( d1 = MSG_CODE.SetParam
                | d2 = index | d3 = Value.LSB | d4 = Value.MSB
           )
 */
bool  zul_encodeGetRequest      (uint8_t *buffer, int bufLen, uint8_t index)
{
    int payloadLen;
    uint8_t  payload[2] = {0,0};
    uint8_t  *ptr = payload;

    if (buffer == NULL)
    {
        return false;
    }

    if (bufLen < 8)
    {
        return false;
    }

    *ptr = (uint8_t)GetParam;   // message Code

    ptr++;
    *ptr = index;               // config parameter index (8bit)

    ptr++;
    payloadLen = (int)(ptr - payload);

    return framePayload(buffer, bufLen, payload, payloadLen);
}

/**
 * create a message in the supplied buffer to get an SPI device register

     Frame ( d1 = MSG_CODE.GetSpiRegister | d2 = device:regIndex |
           )
 */
bool  zul_encodeGetSpiRegister(/*@out@*/ uint8_t *buffer, int bufLen, uint8_t device, uint8_t index)
{
    int      payloadLen;
    uint8_t  payload[2] = {0,0};
    uint8_t  *ptr = payload;
    uint8_t  address = (device << 4) + (index << 1);

    if (buffer == NULL)
        return false;

    if (bufLen < 8)
        return false;

    *ptr = (uint8_t)ReadViaSpi;  // message Code

    ptr++;
    *ptr = address;
    ptr++;
    payloadLen = (int)(ptr - payload);

    return framePayload(buffer, bufLen, payload, payloadLen);
}

/**
 * create a message in the supplied buffer to get a status value

     Frame ( d1 = MSG_CODE.GetParam | d2 = index |
           )
 */
bool  zul_encodeGetStatus (/*@out@*/ uint8_t *buffer, int bufLen, uint8_t index)
{
    int payloadLen;
    uint8_t  payload[2] = {0,0};
    uint8_t  *ptr = payload;

    if (buffer == NULL)
        return false;

    if (bufLen < 8)
        return false;

    *ptr = (uint8_t)GetStatus;  // message Code

    ptr++;
    *ptr = index;               // config parameter index (8bit)

    ptr++;
    payloadLen = (int)(ptr - payload);

    return framePayload(buffer, bufLen, payload, payloadLen);
}


/**
 * create a message in the supplied buffer to set a a config value to a
 * given value

     Frame ( d1 = MSG_CODE.GetVersionString | d2 = string index |
           )
 */
bool  zul_encodeVerStrRequest (uint8_t *buffer, int bufLen, VerIndex index)
{
    int payloadLen;
    uint8_t  payload[8] = {0,0,0,0,0,0,0,0};
    uint8_t  *ptr = payload;

    if (buffer == NULL)
        return false;

    if (bufLen < 8)
        return false;

    *ptr = (uint8_t)GetVersionString;   // message Code

    ptr++;
    *ptr = index;                       // version string index (8bit)

    ptr++;
    payloadLen = (int)(ptr - payload);

    return framePayload(buffer, bufLen, payload, payloadLen);
}




/**
 * create a message in the supplied buffer to set the device to raw mode
 * or to return to normal mode

     Frame ( d1 = MSG_CODE.SetRawMode | d2 = bool on/off |
           )
 */
bool  zul_encodeRawModeRequest (uint8_t *buffer, int bufLen, int mode)
{
    int payloadLen;
    uint8_t  payload[8] = {0,0,0,0,0,0,0,0};
    uint8_t  *ptr = payload;

    if (buffer == NULL)
        return false;

    if (bufLen < 8)
        return false;

    *ptr = (uint8_t)SetRawMode;         // message Code

    ptr++;
    *ptr = (uint8_t)(mode > 0);         // mode 0 is normal

    ptr++;
    payloadLen = (int)(ptr - payload);

    return framePayload(buffer, bufLen, payload, payloadLen);
}



/**
 * create a message in the supplied buffer to set the device to
 * touch mode or to return to normal mode

     Frame ( d1 = MSG_CODE.SetTouchMode | d2 = bool on/off |
           )
 */
bool  zul_encodeTouchModeRequest (uint8_t *buffer, int bufLen, int mode)
{
    int payloadLen;
    uint8_t  payload[8] = {0,0,0,0,0,0,0,0};
    uint8_t  *ptr = payload;

    if (buffer == NULL)
        return false;

    if (bufLen < 8)
        return false;

    *ptr = (uint8_t)SetTouchMode;       // message Code

    ptr++;
    *ptr = (uint8_t)(mode > 0);         // mode 1 is normal

    ptr++;
    payloadLen = (int)(ptr - payload);

    return framePayload(buffer, bufLen, payload, payloadLen);
}


/**
 * create a message in the supplied buffer to set the device to private-touch
 * mode (also known as "silent touch mode") where the touch data is delivered
 * to USB Report ID #6.

      Frame ( d1 = MSG_CODE.SetSilentTouchMode | d2 = bool on/off |
            )
 */
bool zul_encodePrivateTouchModeRequest(uint8_t *buffer, int bufLen, bool mode)
{
    int payloadLen;
    uint8_t  payload[8] = {0,0,0,0,0,0,0,0};
    uint8_t  *ptr = payload;

    if (buffer == NULL)
        return false;

    if (bufLen < 8)
        return false;

    *ptr = (uint8_t)SetSilentTouchMode; // message Code

    ptr++;
    *ptr = (uint8_t)(mode > 0);         // mode 0 is normal

    ptr++;
    payloadLen = (int)(ptr - payload);

    return framePayload(buffer, bufLen, payload, payloadLen);
}


// ========================================================================================
//          Virtual Key Definition messages
// ========================================================================================

bool zul_encodeVirtKeySet  (uint8_t *buffer, int bufLen, VirtualKey *vk)
{
    int payloadLen;
    uint8_t  payload[17];
    uint8_t  *ptr = payload;
    int i;

    memset(payload, 0, 17);

    if (buffer == NULL)
        return false;

    if (bufLen < 32)                    // FixMe
        return false;

    *ptr = (uint8_t)SetVirtualButton;   // message Code

    ptr++;
    *ptr = (uint8_t)(vk->ID);           // 0..9 as of 2018 (zxy500 only)

    ptr++;
    // TopLeft X MSB first
    *ptr = (uint8_t)((vk->TopLeft.x & 0xFF00) >> 8);
    ptr++;
    *ptr = (uint8_t)((vk->TopLeft.x & 0xFF));
    ptr++;

    // TopLeft Y MSB first
    *ptr = (uint8_t)((vk->TopLeft.y & 0xFF00) >> 8);
    ptr++;
    *ptr = (uint8_t)((vk->TopLeft.y & 0xFF));
    ptr++;

    // TOPRIGHT X MSB first
    *ptr = (uint8_t)((vk->BottomRight.x & 0xFF00) >> 8);
    ptr++;
    *ptr = (uint8_t)((vk->BottomRight.x & 0xFF));
    ptr++;

    // TOPRIGHT Y MSB first
    *ptr = (uint8_t)((vk->BottomRight.y & 0xFF00) >> 8);
    ptr++;
    *ptr = (uint8_t)((vk->BottomRight.y & 0xFF));
    ptr++;

    // keyCodes
    *ptr = (uint8_t)vk->modifier;
    ptr++;
    for (i=0; i<3; i++)
    {
        *ptr = (uint8_t)vk->keycode[i];
        ptr++;
    }

    // incomplete
    printf ("Function %s is not implemented|\n", __FUNCTION__ );

    payloadLen = (int)(ptr - payload);
    return framePayload(buffer, bufLen, payload, payloadLen);
}

bool zul_encodeVirtKeyGet  (uint8_t *buffer, int bufLen, int index)
{
    int payloadLen;
    uint8_t  payload[2] = {0,0};
    uint8_t  *ptr = payload;

    if (buffer == NULL)
        return false;

    if (bufLen < 8)
        return false;

    *ptr = (uint8_t)GetVirtualButton;   // message Code

    ptr++;
    *ptr = (uint8_t)(index > 0);        // 0..9 as of 2018 (zxy500 only)

    ptr++;
    payloadLen = (int)(ptr - payload);

    printf ("Function %s is not implemented|\n", __FUNCTION__ );

    return framePayload(buffer, bufLen, payload, payloadLen);
}

bool zul_encodeVirtKeyClear(uint8_t *buffer, int bufLen, int index)
{
    int payloadLen;
    uint8_t  payload[2] = {0,0};
    uint8_t  *ptr = payload;

    if (buffer == NULL)
        return false;

    if (bufLen < 8)
        return false;

    *ptr = (uint8_t)ClearVirtualButton;   // message Code

    ptr++;
    *ptr = (uint8_t)index;        // 0..9 as of 2018 (zxy500 only)

    ptr++;
    payloadLen = (int)(ptr - payload);

    printf ("Function %s is not implemented|\n", __FUNCTION__ );

    return framePayload(buffer, bufLen, payload, payloadLen);
}


// ========================================================================================
//          Bootloader Services
// ========================================================================================



static bool  zul_encode_BL_Command(/*@out@*/uint8_t *buffer, int bufLen, uint8_t command)
{
    if (buffer == NULL) return false;
    if (bufLen < 1) return false;

    // buffer[0] = 0x0;
    buffer[0] = command;
    return true;
}

bool  zul_encode_BL_RebootToApp(uint8_t *buffer, int bufLen)
{
    return zul_encode_BL_Command(buffer, bufLen, BLRebootToApp);
}

bool  zul_encode_BL_RebootToBL(uint8_t *buffer, int bufLen)
{
    return zul_encode_BL_Command(buffer, bufLen, BLRebootToBL);
}

bool  zul_encode_BL_PING(uint8_t *buffer, int bufLen)
{
    return zul_encode_BL_Command(buffer, bufLen, BLPing);
}

bool  zul_encode_BL_GetVerStr(uint8_t *buffer, int bufLen, VerIndex index)
{
    if (buffer == NULL) return false;
    if (bufLen < 3) return false;

    if (zul_encode_BL_Command(buffer, bufLen, BLGetVersionStr))
    {
        buffer[1] = index;
        return true;
    }

    return false;
}


// OLD zxy100 BL version accessor!
bool  zul_encode_BL_VersionF(uint8_t *buffer, int bufLen) // deprecated !!
{
    return zul_encode_BL_Command(buffer, bufLen, BLGetVersionData);
}


bool  zul_encode_BL_ProgDataBlock(uint8_t *buffer, int bufLen,
                                            size_t fwSize, uint8_t *pinfo)
{
    int index = 1;
    if (bufLen < 64) return false;

    memset(buffer, 0, 64);

    zul_encode_BL_Command(buffer, bufLen, BLProgramStart);

    buffer[index++] = (uint8_t)(fwSize & 0xff);  fwSize >>= 8;
    buffer[index++] = (uint8_t)(fwSize & 0xff);  fwSize >>= 8;
    buffer[index++] = (uint8_t)(fwSize & 0xff);  fwSize >>= 8;
    buffer[index++] = (uint8_t)(fwSize & 0xff);

    buffer[index++] = *pinfo++;
    buffer[index++] = *pinfo++;

    if (BL_DEBUG) printf("ProgDataBlock: %s\n", zul_hex2String(buffer, 16));

    return true;
}




// ============================================================================
// --- Private Implementation ---
// ============================================================================

/**
 * msb_16
 */
uint8_t msb_16(uint16_t val)
{
    uint16_t retVal = val >> 8;
    return (uint8_t)(retVal & 0x00ff);
}

/**
 * lsb_16
 */
uint8_t lsb_16(uint16_t val)
{
    return (uint8_t)(val & 0x00ff);
}

uint8_t lsb_int     (int val)
{
    return (uint8_t)(val & 0xff);
}


/**
 * encode the payload in the supplied frame
 *

   ||
     | STX  |
        | LEN | TYPE |
           | d1 .. dn |     Payload
        | CRC1 | CRC2 |
     | ETX  |
   ||

 */
bool framePayload(uint8_t *buffer, int bufLen, uint8_t *payload, int payloadLen)
{
    uint8_t     *ptr = buffer;
    uint8_t     *lenPtr;
    uint16_t    crcVal;

    if (buffer == NULL)
    {
        return false;
    }

    if (bufLen < (7 + payloadLen))
    {
        fprintf(stderr, "%s buf too short", __FUNCTION__ );
        return false;
    }

    *ptr = ZCC;                 // Zytronic Control Collection ID

    ptr++;
    *ptr = STX;                 // start of frame

    ptr++;
    lenPtr = ptr;               // remember this point, where packet length is placed

    ptr++;
    *ptr = MasterRequest;

    // Add payload
    {
        int         i;
        for (i=0; i<payloadLen; i++)
        {
            ptr++;
            *ptr = payload[i];
        }
    }

    // Set packet length and calculate CRC
    {
        int packetLen = ptr - buffer - 1;                   // skip the STX
        *lenPtr = lsb_int(packetLen + 2);                   // two extra bytes for the CRC
        crcVal = zul_getCRC(buffer + 2, (size_t)packetLen); // skip the STX
    }

    ptr++;
    *ptr = lsb_16(crcVal);       // LS Byte CRC

    ptr++;
    *ptr = msb_16(crcVal);       // MS Byte CRC

    ptr++;
    *ptr = ETX;                  // LS Byte CRC

    if (PROTOCOL_DEBUG)  // zul_hex2String
    {
        uint8_t *o;

        ptr++;
        o = buffer;
        printf("  Encoded Msg\t");   // zul_hex2String
        while (o<ptr) printf ("0x%02x ", (uint)*o++);
        printf("\n");
    }

    return true;
}
