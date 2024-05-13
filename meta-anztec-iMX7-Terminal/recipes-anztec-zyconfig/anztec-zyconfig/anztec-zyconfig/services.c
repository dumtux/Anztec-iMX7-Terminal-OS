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
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "dbg2console.h"
#include "zytypes.h"
#include "protocol.h"
#include "usb.h"
#include "services.h"
#include "services_sc.h"
//#include "comms.h"
#include "debug.h"

//
// --- Module Global Variables/Consts ---
//

/*   --- NB: the library is not designed to be multi-thread safe ---  */


// msv => module static variable

static uint8_t              msv_xfrIndex;
static uint16_t             msv_getConfigParam;
static uint16_t             msv_getStatusVal;
static bool                 msv_flashWriteDisabled = false;
static char                 msv_resp_string[80];

static uint8_t              msv_privateTouches[64];

static bool                 msv_showNoSensor;

/*@null@*/          // for splint
static uint8_t         *    msv_pvtTouchUserBuf = NULL;
static uint8_t              msv_BL_reply[BL_REPLY_BUF_LEN];
static uint16_t             msv_xWires = 0, msv_yWires = 0;
static bool                 msv_privateTouchMode = false;
static int                  msv_RawDataMode = 0;


// data buffers for interrupt data storage

 /*@null@*/
static void             *   msv_image = 0;
static uint8_t              msv_rawDataStatus[64];
static uint8_t              msv_heartBeatData[64];
static uint8_t              msv_touchData[64];
static Contact              msv_lastTouchLocation;


//
// --- Private Prototypes ---
//
void            zul_initFwData                  (void);


/**
 * INterrupt transfer handlers
 */

void            handle_IN_touchdata             (uint8_t *data);
void            handle_IN_heartbeat             (uint8_t *data);
void            handle_IN_rawdata_mt            (uint8_t *data);

void            handle_privateTouches           (uint8_t *data);

/**
 * CTRL tansaction reply handlers
 */
int             default_CTRL_handler            (uint8_t *data);
int             get_response                    (uint8_t *data);
int             status_response                 (uint8_t *data);
int             set_response                    (uint8_t *data);
int             get_str_response                (uint8_t *data);
int             handle_BL_response              (uint8_t *data);

// ============================================================================
// --- Public Implementation ---
// ============================================================================


/**
 * Reset all internal state, ready for first use
 * return zero on success, else a negative error code
 */
int zul_InitServices(void)
{
    zul_logf(3, "%s", __FUNCTION__);
    zul_InitServSelfCap();
    zul_initFwData();
    return usb_openLib();
}

/**
 * Terminate all services, free all resources
 */
void zul_EndServices(void)
{
    usb_closeLib();
}

/**
 * return a string listing the connected Zytronic Touchscreens, one per line
 * return count is the number of devices
 * negative return values indicate a fault
 */
int zul_getDeviceList(char *buf, int len)
{
    bool    done = false;
    char *  bufPtr = buf;
    int16_t pid = 0;

    int     numZyDevices = usb_getDeviceList(buf, len);

    if ( numZyDevices > 0 )
    {

        while (!done)
        {
            bufPtr = strstr( buf, "PID:"  );
            if (bufPtr != NULL)
            {
                const char *dn = NULL;
                // mark start of hex PID value
                bufPtr += 4;
                pid = (int16_t) strtol(  bufPtr, NULL, 16 );


                // move past the "PID:" token for next iteration
                buf = bufPtr;

                // replace NNNNNN tokens with actual names
                dn = zul_getDevStrByPID(pid);
                bufPtr = strstr( buf, "NNNNNN" );

                if (bufPtr != NULL)
                {
                    memcpy( bufPtr, dn, 6);

                    // replace MMM tokens with either APP or BL markers
                    dn = zul_isBLDevicePID(pid) ? "BL " : "APP" ;
                    bufPtr = strstr( buf, "MMM" );
                    if (bufPtr != NULL)
                    {
                        memcpy( bufPtr, dn, 3);
                    }
                    else
                    {
                        done = true;
                    }
                    dn = NULL;
                }
                else
                {
                    done = true;
                }
            }
            else
            {
                done = true;
            }
        }
    }
    return numZyDevices;
}


/**
 * Get a version string. There is now only one Version string for library/zyconfig/apps.
 */
void zul_getVersion(char *buffer, int len)
{
    size_t lenStr = strlen(VERSION_STRING);
    if (lenStr > 10)
    {
        char *p = VERSION_STRING;
        p += lenStr;  // work from end of string
        strncpy(buffer, p-10, (size_t)(len-1));
        buffer[8] = '\0';
    }
}


/**
 * provide access to the underlying libUSB version
 */
char *      zul_usbLibStr(void)
{
    return usb_usbLibStr();
}


/**
 * sleep for a number of milliseconds
 */
void zy_msleep(uint32_t ms)
{
    struct timespec t;
    struct timespec r;
    t.tv_sec = ms/1000;
    ms %= 1000;
    t.tv_nsec = 1000000L * ms;
    nanosleep(&t, &r);
}

void zul_byteSwap(uint16_t *status)
{
    int left  = (*status & 0xff00) >> 8;
    int right = (*status & 0x00ff) << 8;
    *status = (uint16_t) ( left | right );
}

/**
 * return true if the supplied Product ID is a bootloader device
 */
bool zul_isBLDevicePID(int16_t pid)
{
    return usb_isBLDevicePID(pid);
}

/**
 * return true if the indexed device in the list is a bootloader device
 */
bool zul_isBLDevice(int index, char * list)
{
    char    copy[1001];
    char   *devToken = copy;
    int     devIndex;
    bool    keepGoing = true;

    strncpy(copy, list, 1000);
    copy[1000] = '\0';
    do
    {
        devToken = strtok(devToken, "\n");
        if (devToken == NULL)
        {
            keepGoing = false;
            break;
        }

        devIndex = atoi(devToken);
        if (devIndex == index)
        {
            int16_t pid = 0;
            char *p = strstr(devToken, "PID:");
            if (p != NULL)
            {
                pid = (int16_t)strtol((p+4), NULL, 16);

                return zul_isBLDevicePID(pid);
            }
        }
        devToken = NULL;
    }
    while (keepGoing);
    return false;
}

bool zul_isZXY500AppPID (int16_t *pid)
{
    if (( *pid == ZXY500_PRODUCT_ID ) ||
        ( *pid == ZXY500_PRODUCT_ID_ALT1 ))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * given a USB Product ID, return a device name string :
 *      ZXY100, ZXY110, ZXY150, ZXY200, ZXY300
 */
char const * zul_getDevStrByPID  (int16_t pid)
{
    switch (pid)
    {
        case ZXYZXY_PRODUCT_ID:
            return "ZXYZXY";

        case USB32C_PRODUCT_ID:
            return "USB32C";

        case ZXY100_PRODUCT_ID:
        case ZXY100_BOOTLDR_ID:
            return "ZXY100";

        case ZXY110_PRODUCT_ID:
        case ZXY110_BOOTLDR_ID:
            return "ZXY110";

        case ZXY150_PRODUCT_ID:
        case ZXY150_BOOTLDR_ID:
            return "ZXY150";

        case ZXY200_PRODUCT_ID:
        case ZXY200_PRODUCT_ID_ALT1:
        case ZXY200_BOOTLDR_ID:
            return "ZXY200";

        case ZXY300_PRODUCT_ID:
        case ZXY300_BOOTLDR_ID:
            return "ZXY300";

        case ZXY500_PRODUCT_ID:
        case ZXY500_PRODUCT_ID_ALT1:
        case ZXY500_BOOTLDR_ID:
            return "ZXY500";

        default:
            return "UNKNWN";
    }
}
/**
 * Given a ZXYxxx device name, return a device number {100, 110, 150, ...}
 * This service can remove (path) chars before the first 'ZXY' string.
 * If we fail, return -1
 */
int zul_getProdNumFromDevS(char const *devName)
{
    int         retVal = -1;    // failure indication
    char const *startLoc;
    char const *zxyLoc;

    // if a path is provided, it might also hold 'ZXYxxx' str. Skip any path!
    startLoc = strrchr(devName, '/');
    if (startLoc == NULL) startLoc = devName;

    zxyLoc = strstr(startLoc, "ZXY");
    if (zxyLoc == NULL) return retVal;

    return atoi(zxyLoc+3);
}


/**
 * Given a ZXYxxx device name, return a bootloader USB PID.
 * This service can remove (path) chars before the first 'ZXY' string.
 * If we fail, return -1
 */
int16_t zul_getBLPIDByDevS(char const *devName)
{
    // example devName: .path/to/file/ZXY100...
    //                  ZXY100-U-OFF-23123

    int16_t retVal = -1;    // failure indication
    int     devNum = zul_getProdNumFromDevS(devName);

    switch (devNum)
    {
        case 100:    retVal = ZXY100_BOOTLDR_ID;  break;
        case 110:    retVal = ZXY110_BOOTLDR_ID;  break;
        case 150:    retVal = ZXY150_BOOTLDR_ID;  break;
        case 200:    retVal = ZXY200_BOOTLDR_ID;  break;
        case 300:    retVal = ZXY300_BOOTLDR_ID;  break;
        case 500:    retVal = ZXY500_BOOTLDR_ID;  break;
    }

    return retVal;
}


/**
 * given a USB Product ID, return a device name string :
 *      ZXY100, ZXY110, ZXY150, ZXY200, ZXY300
 */
const char * zul_getZYFFilter(void)
{
    static char filterBuffer[21];
    int16_t pid;

    strcpy ( filterBuffer, " (" );
    // if not connected, return a generic filter
    if (!zul_getDevicePID(&pid)) return (" (ZXY*.zyf)");

    strcat( filterBuffer, zul_getDevStrByPID(pid) );
    strcat( filterBuffer, "*.zyf)");

    // ZXY500 device need further qualification (uC variation)
    if ( (zul_isZXY500AppPID(&pid)) || (pid == ZXY500_BOOTLDR_ID) )
    {
        char hwStr[33] = "-" ;

        if (zul_isBLDevicePID(pid))
        {
            if (!zul_BLgetVersion (hwStr, 32,  STR_HW))
            {
                strcpy(hwStr, "ZXY500");
            }
        }
        else
        {
            (void)zul_Hardware( hwStr, 32 );
        }
        if (strstr( hwStr, "500-U-OFF-256-" ) )
        {
            strcpy( filterBuffer, " (ZXY500_256*.zyf)");
        }
        if (strstr( hwStr, "500-U-OFF-128-" ) )
        {
            strcpy( filterBuffer, " (ZXY500_128*.zyf)");
        }
        if (strstr( hwStr, "500-U-OFF-64-" ) )
        {
            strcpy( filterBuffer, " (ZXY500_64*.zyf)");
        }
    }

    return filterBuffer;
}


/**
 * Given a ZXYxxx device name, return the application USB PID.
 * This service can remove (path) chars before the first 'ZXY' string.
 * If we fail, return -1
 */
int16_t zul_getAppPIDByDevS (char const *devName)
{
    int16_t retVal = -1;    // failure indication
    int     devNum = zul_getProdNumFromDevS(devName);

    switch (devNum)
    {
        case 100:    retVal = ZXY100_PRODUCT_ID;  break;
        case 110:    retVal = ZXY110_PRODUCT_ID;  break;
        case 150:    retVal = ZXY150_PRODUCT_ID;  break;
        case 200:    retVal = ZXY200_PRODUCT_ID;  break;
        case 300:    retVal = ZXY300_PRODUCT_ID;  break;
        case 500:    retVal = ZXY500_PRODUCT_ID;  break;
    }

    return retVal;
}

/**
 * if the list contains a particular PID, then return the index
 */
int zul_selectPIDFromList(int16_t pid, char * list)
{
    char    copy[1001];
    char   *devToken = copy;
    int     devIndex;
    int16_t linePid;
    char   *pidToken;

    strncpy(copy, list, 1000); copy[1000] = '\0';
    do
    {
        devToken = strtok(devToken, "\n");
        if (devToken == NULL) return -1;

        devIndex = atoi(devToken);

        linePid = 0;
        pidToken = strstr(devToken, "PID:");
        if (pidToken != NULL)
        {
            linePid = (int16_t)strtol((pidToken+4), NULL, 16);

            if (pid == linePid) return devIndex;
        }

        devToken = NULL;
    }
    while (true);
    return -1;
}

char * zul_removeDeviceTargetKey(int *pNumArgs, char *argv[])
{
    char * retStr = NULL;
    int n = *pNumArgs;
    int i;
    int found = 0;

    for( i=0; i<n; i++)
    {
        if ( !found && (0 == strncasecmp(argv[i], "deviceKey=", 10)) )
        {
            retStr = argv[i] + 10;
            found = 1;
        }
        if (found)
        {
            // remove the deviceKey from the argv list
            if (i < n-1)
            {
                argv[i] = argv[i+1];
            }
        }
    }
    if (found)
    {
        // update the number of args expected in argv
        n -= 1;
        *pNumArgs = n;
    }

    return retStr;
}

/**
 * Edit the command line parameters to remove a USB address specification,
 * if present address spec is a string leading with "addr="
 * The leader is matched regardless of case
 * The USB address value should be og the form "BB_PP" where
 *    BB is a 2 digit bus number
 *    PP is a 2 digit address number
 * See the references provide by the "list" command
 */
char * zul_removeDeviceTargetAddr(int *pNumArgs, char *argv[])
{
    char * retStr = NULL;
    int n = *pNumArgs;
    int i;
    int found = 0;
    for( i=0; i<n; i++)
    {
        // std::cout << "cmdline_args.at(" << i << ") = " << argv[i] << std::endl;
        if ( !found && (0 == strncasecmp(argv[i], "Addr=", 5)) )
        {
            retStr = argv[i] + 5;
            found = 1;
        }

        if (found)
        {
            // remove the address from the argv list
            if (i < n-1)
            {
                argv[i] = argv[i+1];
            }
        }
    }
    if (found)
    {
        // update the number of args expected in argv
        n -= 1;
        *pNumArgs = n;
    }
    if (found)
    {
        if (strlen(retStr) != 5)  return NULL;
        // validate addr format
        if (!isxdigit(retStr[0])) return NULL;
        if (!isxdigit(retStr[1])) return NULL;
        if (!ispunct (retStr[2])) return NULL;
        if (!isxdigit(retStr[3])) return NULL;
        if (!isxdigit(retStr[4])) return NULL;
        retStr[2] = '_';    // bus address separator expected to be '_'
    }

    return retStr;
}

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// -  Device access functions
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -


/**
 * if a device is open, set the raw data IN handler according to the device PID
 */
void zul_setRawDataHandler(void)
{
    int16_t pid;
    if (usb_getDevicePID(&pid))
    {
        switch (pid)
        {
            case ZXY100_PRODUCT_ID:
                usb_RegisterHandler( RAW_DATA, handle_IN_rawdata_100 );
                break;
            case ZXY110_PRODUCT_ID:
                usb_RegisterHandler( RAW_DATA, handle_IN_rawdata_110 );
                break;
            default:
                usb_RegisterHandler( RAW_DATA, handle_IN_rawdata_mt );
                break;
        }
    }
}

/**
 * If a device is open, copy the USB address string to the supplied buffer,
 * returning SUCCESS.  Else return a negative error number.
 */
int zul_getAddrStr(char * addrStr)
{
    return usb_getAddrStr(addrStr);
}

/**
 * Open a particular device, based on the supplied address string
 * NB: only open one at a time
 */
int zul_openDeviceByAddr(char *portAddr)
{
    msv_showNoSensor = true;
    int retVal = usb_openDeviceByAddr(portAddr);
    zul_setRawDataHandler();
    return retVal;
}

/**
 * Open a particular device, based on the indices provided by zul_getDeviceList()
 * NB: only open one at a time
 */
int zul_openDevice(int index)
{
    msv_showNoSensor = true;
    int retVal = usb_openDevice(index);
    zul_setRawDataHandler();
    return retVal;
}

/**
 *  Re-Open the last device closed, by index provided by zul_getDeviceList()
 */
int zul_reOpenLastDevice(void)
{
    int retVal = usb_reOpenLastDevice();
    zul_setRawDataHandler();
    return retVal;
}

/**
 * If a device is open, set the supplied pid and return true
 * Else, return false
 */
bool zul_getDevicePID(int16_t *pid)
{
    return usb_getDevicePID(pid);
}

/**
 * If a device is open, set the supplied sensor size struct and return true
 * Else, return false
 */
bool zul_getSensorSize (ZXY_sensorSize *sz)
{
    int16_t     PID;
    uint16_t    cellCountX,cellCountY;

    if (usb_getDevicePID(&PID))
    {
        switch (PID)
        {
            case ZXY100_PRODUCT_ID:
            case ZXY110_PRODUCT_ID:
                (void)zul_getOldZxy100WireCnt(&cellCountX, &cellCountY);
                break;

            default:
                (void)zul_getStatusByID(ZXYMT_SI_NUM_X_WIRES, &cellCountX);
                (void)zul_getStatusByID(ZXYMT_SI_NUM_Y_WIRES, &cellCountY);
                break;
        }
        sz->xWires = cellCountX;
        sz->yWires = cellCountY;
        return true;
    }
    return false;
}

/**
 * Control the "robustness" of the communications
 * see
        void        usb_setCtrlDelay            (int delay);
        void        usb_defaultCtrlDelay        (void);

    TODO - clean this up
 */
static Endurance   msv_commEndurance = COM_ENDUR_NORM;
void  zul_setCommsEndurance(Endurance endurance)
{
    msv_commEndurance = endurance;

    switch (endurance)
    {
        case COM_ENDUR_MEDIUM:
            usb_setCtrlDelay(40);       // ms
            usb_setCtrlRetry(50);
            usb_setCtrlTimeout(10000);   // ms
            break;

        case COM_ENDUR_HIGH:
            usb_setCtrlDelay(40);       // ms
            usb_setCtrlRetry(200);
            usb_setCtrlTimeout(10000);  // ms
            break;

        default:
            msv_commEndurance = COM_ENDUR_NORM;
            /*@fallthrough@*/   // intended fallthrough

        case COM_ENDUR_NORM:
            usb_defaultCtrlDelay();
            usb_defaultCtrlRetry();
            usb_defaultCtrlTimeout();
            break;
    }
}

Endurance zul_getCommsEndurance(void)
{
    return msv_commEndurance;
}

/**
 * Set the connected interface to #0 with parameter 'true' or to the auxilliary
 * interface with parameter 'false'.
 * The auxilliary interface #1 was introduced with ZXY500 for management of
 * device, without taking the touch services from the linux kernel.
 */
int zul_useKernelIFace(bool kernel)
{
    int16_t     PID;
    // if connected,
    if (usb_getDevicePID(&PID))
    {
        // if connected to a ZXY500 Application device
        if ( zul_isZXY500AppPID(&PID) )
        {
            // switch to interface ZERO if main is true
            usb_switchIFace( (uint8_t) ( kernel ? 0 : 1 ) );
            return SUCCESS;
        }
    }
    return FAILURE;   // no change made
}

/**
 * Close an open device - the index is remembered.
 */
int zul_closeDevice(void)
{
    zul_ResetSelfCapData();
    return usb_closeDevice();
}



// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
// -  Standard get/set/status accessors
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

int zul_getStatusByID(uint8_t ID, uint16_t *status)
{
    bool    ok;
    uint8_t msgBuf[DUAL_BYTE_MSG_LEN];
    int     retVal;

    bzero(msgBuf, DUAL_BYTE_MSG_LEN);
    ok = zul_encodeGetStatus(msgBuf, DUAL_BYTE_MSG_LEN, ID);
    msv_xfrIndex = ID;
    if (ok)
    {
        retVal = usb_ControlRequest(msgBuf, DUAL_BYTE_MSG_LEN, status_response);
        if (retVal > 0) *status = msv_getStatusVal;
        retVal = (retVal > 0) ? SUCCESS : FAILURE;
    }
    else
    {
        retVal = FAILURE;
    }
    return retVal;
}

int zul_getSpiRegister(uint8_t device, uint8_t reg, uint16_t *value)
{
    bool    ok;
    uint8_t msgBuf[DUAL_BYTE_MSG_LEN];
    int     retVal;

    bzero(msgBuf, DUAL_BYTE_MSG_LEN);
    ok = zul_encodeGetSpiRegister(msgBuf, DUAL_BYTE_MSG_LEN, device, reg);
    msv_xfrIndex = reg;
    if (ok)
    {
        retVal = usb_ControlRequest(msgBuf, DUAL_BYTE_MSG_LEN, get_response);
        if (retVal > 0)
        {
            *value = msv_getConfigParam;
            retVal = SUCCESS;
        } else {
            retVal = FAILURE;
        }
    }
    else
    {
        retVal = FAILURE;
    }
    return retVal;
}

int zul_getConfigParamByID(uint8_t ID, uint16_t *value)
{
    bool    ok;
    uint8_t msgBuf[DUAL_BYTE_MSG_LEN];
    int     retVal;

    bzero(msgBuf, DUAL_BYTE_MSG_LEN);
    ok = zul_encodeGetRequest(msgBuf, DUAL_BYTE_MSG_LEN, ID);
    msv_xfrIndex = ID;
    if (ok)
    {
        retVal = usb_ControlRequest(msgBuf, DUAL_BYTE_MSG_LEN, get_response);
        if (retVal > 0)
        {
            *value = msv_getConfigParam;
            retVal = SUCCESS;
        }
        else
        {
            retVal = FAILURE;
        }
    }
    else
    {
        retVal = FAILURE;
    }
    return retVal;
}

int zul_setConfigParamByID(uint8_t ID, uint16_t value)
{
    bool    ok;
    uint8_t msgBuf[DUAL_BYTE_MSG_LEN + 2];
    int     retVal;
    Endurance e = zul_getCommsEndurance();

    zul_setCommsEndurance(COM_ENDUR_MEDIUM);
    bzero(msgBuf, DUAL_BYTE_MSG_LEN + 2);
    ok = zul_encodeSetRequest(msgBuf, DUAL_BYTE_MSG_LEN + 2, ID, value);
    msv_xfrIndex = ID;
    if (ok)
    {
        retVal = usb_ControlRequest(msgBuf, DUAL_BYTE_MSG_LEN + 2, default_CTRL_handler);
        retVal = (retVal > 0) ? SUCCESS : FAILURE;
    }
    else
    {
        retVal = FAILURE;
    }
    zul_setCommsEndurance(e);
    return retVal;
}

/**
 * Standard device version string accessors
 */
int zul_getVersionStr(VerIndex verType, char *v, int len)
{
    bool        ok;
    int16_t     pid;
    uint8_t     msgBuf[DUAL_BYTE_MSG_LEN];

    zul_logf(3, "%s %d", __FUNCTION__, verType);

    if ( zul_getDevicePID(&pid))
    {
        if (pid == ZXY100_PRODUCT_ID)
        {
            int retval = zul_getZxy100VersionStr(verType, v, len);
            if (retval == SUCCESS) return retval;   // else continue ...
        }

        // fake a string of the hex CPU Unique ID
        if (verType == STR_CPUID)
        {
            char hxStr[6*4+1];
            int x;
            int baseCI = ZXY110_SI_PROCESSOR_ID_0;
            if (pid != ZXY110_PRODUCT_ID)
            {
                baseCI = ZXYMT_SI_PROCESSOR_ID_BASE;
            }

            for (x=0; x<6; x++)
            {
                uint16_t status;
                zul_getStatusByID(baseCI+x, &status);
                zul_byteSwap(&status);
                sprintf(hxStr+(x*4), "%04X", status); // case is important
            }
            hxStr[6*4] = '\0';
            snprintf(v, len, "%s", hxStr);
            return SUCCESS;
        }

        // Normal path :
        bzero(msgBuf, 9);
        ok = zul_encodeVerStrRequest(msgBuf, DUAL_BYTE_MSG_LEN, verType);

        if (ok)
        {
            msv_resp_string[0] = '\0';
            (void)usb_ControlRequest(msgBuf, DUAL_BYTE_MSG_LEN, get_str_response);
            if (msv_resp_string[0] != '\0')
            {
                strncpy (v, msv_resp_string, (size_t)len);
                v[len-1] = '\0';    // force string termination
                return SUCCESS;
            }
        }
    }

    return FAILURE;
}

int zul_Firmware        (char *v, int len)
{
    return zul_getVersionStr(STR_FW, v, len);
}

int zul_Bootloader      (char *v, int len)
{
    return zul_getVersionStr(STR_BL, v, len);
}

int zul_Hardware        (char *v, int len)
{
    return zul_getVersionStr(STR_HW, v, len);
}

int zul_Customization   (char *v, int len)
{
    return zul_getVersionStr(STR_AFC, v, len);
}

int zul_CpuID(char *v, int len)
{
    return zul_getVersionStr(STR_CPUID, v, len);
}


bool getShowNoSensor(void)
{
    return msv_showNoSensor;
}

void setShowNoSensor(bool b)
{
    msv_showNoSensor = b;
}
/**
 * Test if an option bit is set in the STATUS_BITS value for the
 * connected device.
 */
bool zul_optionAvailable (uint16_t requestedBit)
{
    int16_t     pid;
    uint16_t    optionBits;
    uint8_t     optionsIndex;

    if (zul_getDevicePID(&pid))
    {
        switch (pid)
        {
            case ZXY100_PRODUCT_ID:
                optionsIndex = ZXY100_SI_OPTION_BITS;
                break;
            case ZXY110_PRODUCT_ID:
                optionsIndex = ZXY110_SI_OPTION_BITS;
                break;
            default:
                optionsIndex = ZXYMT_SI_OPTION_BITS;
        }

        if (zul_getStatusByID(optionsIndex, &optionBits) == SUCCESS)
        {
            zul_logf(3, "PID:%04x OptionIndex:%d BITS:%04X",
                pid, optionsIndex, optionBits);
            return (optionBits & requestedBit) > 0;
        }
    }
    return false;
}


/**
 * When flash writing is disabled, it's much faster to load a set of
 * configuration parameters.  The controller's default is that flash
 * writing is enabled, it may be temporarily disabled to accelerate a
 * load-configuration process.
 */
void zul_inhibitFlashWrites(bool inhibit)
{
    zul_logf(3, "%s %d", __FUNCTION__, inhibit);
    if (msv_flashWriteDisabled == inhibit) return;

    msv_flashWriteDisabled = inhibit;

    {
        int     retVal;
        uint8_t msgBuf[DUAL_BYTE_MSG_LEN];

        bzero(msgBuf, DUAL_BYTE_MSG_LEN);
        if (msv_flashWriteDisabled)
        {
            // NB: there is a logic inversion here: inhibited <=> not enabled
            zul_encodeSetFlashWrite(msgBuf, DUAL_BYTE_MSG_LEN, !msv_flashWriteDisabled);
            retVal = usb_ControlRequest(msgBuf, DUAL_BYTE_MSG_LEN, default_CTRL_handler);
            retVal = (retVal > 0) ? SUCCESS : FAILURE;
        }
        else
        {
            zul_encodeForceFlashWrite(msgBuf, SINGLE_BYTE_MSG_LEN);
            retVal = usb_ControlRequest(msgBuf, SINGLE_BYTE_MSG_LEN, default_CTRL_handler);
            retVal = (retVal > 0) ? SUCCESS : FAILURE;
        }
    }
}


/**
 * MSGCODE_SET_SILENT_TOUCH_DATA_MODE is available on some devices
 * When enabled, the HID touch events are NOT generated, but the touch data
 * is relayed (in the same format) to USB Report ID #6.
 * This 'disables' normal touch operation on most systems, and makes the touch
 * data available to a single receiving application. One use of this is for a
 * TUIO server */
void zul_SetPrivateTouchMode(bool enabled)
{
    int     retVal;
    uint8_t msgBuf[DUAL_BYTE_MSG_LEN];

    // if no change - return! ToDo : error message?
    if (msv_privateTouchMode == enabled) return;

    msv_privateTouchMode = enabled;

    bzero(msgBuf, DUAL_BYTE_MSG_LEN);

    zul_encodePrivateTouchModeRequest( msgBuf, DUAL_BYTE_MSG_LEN, msv_privateTouchMode );

    retVal = usb_ControlRequest(msgBuf, DUAL_BYTE_MSG_LEN, default_CTRL_handler);
    retVal = (retVal > 0) ? SUCCESS : FAILURE;
}



/**
 * General service to send a single byte message holding only the message-code.
 */
void zul_sendMessageCode (uint8_t msgCode)
{
    bool    ok;
    uint8_t msgBuf[SINGLE_BYTE_MSG_LEN];

    bzero(msgBuf, SINGLE_BYTE_MSG_LEN);
    ok = zul_encodeSingleByteMessage(msgBuf, 8, msgCode);

    if (ok)
    {
        (void)usb_ControlRequest(msgBuf, SINGLE_BYTE_MSG_LEN, default_CTRL_handler);
    }
}

/**
 * Reset the controller -- NB:ZXY110 can take ~10 seconds before this returns!
 */
void zul_restoreDefaults (void)
{
    bool    ok;
    uint8_t msgBuf[SINGLE_BYTE_MSG_LEN];
    zul_logf(3, "%s", __FUNCTION__);

    bzero(msgBuf, SINGLE_BYTE_MSG_LEN);
    ok = zul_encodeRestoreDefaults(msgBuf, 8);

    if (ok)
    {
        zul_setCommsEndurance(COM_ENDUR_HIGH);
        (void)usb_ControlRequest(msgBuf, SINGLE_BYTE_MSG_LEN, default_CTRL_handler);
        usb_defaultCtrlDelay();
    }
}


/**
 * Reset the controller
 */
void zul_resetController (void)
{
    bool    ok;
    uint8_t msgBuf[SINGLE_BYTE_MSG_LEN];
    zul_logf(3, "%s", __FUNCTION__);

    bzero(msgBuf, SINGLE_BYTE_MSG_LEN);
    ok = zul_encodeResetController(msgBuf, 8);

    if (ok)
    {
        (void)usb_ControlRequest(msgBuf, SINGLE_BYTE_MSG_LEN, default_CTRL_handler);
    }
}

/**
 * Force sensor equalisation
 */
void zul_forceEqualisation (void)
{
    bool    ok;
    uint8_t msgBuf[SINGLE_BYTE_MSG_LEN];
    zul_logf(3, "%s", __FUNCTION__);

    bzero(msgBuf, SINGLE_BYTE_MSG_LEN);
    ok = zul_encodeForceEqualisation(msgBuf, 8);

    if (ok)
    {
        (void)usb_ControlRequest(msgBuf, SINGLE_BYTE_MSG_LEN, default_CTRL_handler);
    }
}


/**
 * Re Start in Bootloader mode
 */
void zul_StartBootLoader (void)
{
    bool    ok;
    uint8_t msgBuf[SINGLE_BYTE_MSG_LEN];

    bzero(msgBuf, SINGLE_BYTE_MSG_LEN);
    ok = zul_encodeStartBootLoader(msgBuf, 8);

    if (ok)
    {
        int16_t pid = 6;    // guess that it's a MT type
        response_handler_t  handFunc = default_CTRL_handler;
        zul_getDevicePID(&pid);
        if ( (pid == ZXY100_PRODUCT_ID) || (pid == ZXY110_PRODUCT_ID) )
        {
            // a reply is not always sent, and even if it is, it is not used
            handFunc = NULL;
        }
        (void)usb_ControlRequest(msgBuf, SINGLE_BYTE_MSG_LEN, handFunc);
    }
}

/**
 * clear the on board calibration
 */
void zul_clearOnBoardCal (void)
{
    Calibration c;

    uint8_t     num;
    uint8_t     i;
    int16_t     pid = -1;

    if (zul_getDevicePID(&pid))
    {
        switch (pid)
        {
            case ZXY100_PRODUCT_ID:
                num  = ZXY100_CN_ONBOARD_CAL_COUNT;
                break;

            case ZXY110_PRODUCT_ID:
                num  = ZXY110_CN_ONBOARD_CAL_COUNT;
                break;

            default:        // Multitouch devices
                num  = ZXYMT_CN_ONBOARD_CAL_COUNT;
        }

        // check that the required number will fit in the array
        assert (num <= ZXY100_CN_ONBOARD_CAL_COUNT);

        bzero ( &c, sizeof(c));
        for (i=num/2; i<num; i++)
        {
            c.val[i] = (uint16_t)0xFFF;
        }

        zul_setOnBoardCal (&c);
    }
}


/**
 * clear the on board calibration
 */
void zul_TestSetOnBoardCal(void)
{
    Calibration c;

    // printf("  #  send test calibration\n");

    // the following calibration moves the dislpayed contacts inwards
    // from the finger touches, by a fixed quantity
    c.val[0] = 0x0202;
    c.val[1] = 0x0202;
    c.val[2] = 0x0101;
    c.val[3] = 0x0101;

    c.val[4] = 0x0E0E;
    c.val[5] = 0x0E0E;
    c.val[6] = 0x0F0F;
    c.val[7] = 0x0F0F;

    zul_setOnBoardCal (&c);
}



/**
 * Auto-Configure the FIRST_TOUCH_MODE_ON_THRESHOLD based on current
 * noise level measurements.
 *
 * PAR, June 2016:  Now that FIRST_TOUCH_MODE_OFF_THRESHOLD is effectively
 *                  removed, we can simplify the algorithm for configuring
 *                  first touch mode to the following:
 *
 *    - Read ZXYMT_CI_FIRST_TOUCH_MODE_ON_THRESHOLD
 *
 *    - Read ZXYMT_SI_NUM_ZEROS_WARNING_MAX, and add 10 to it
 *
 *    - If this is higher than the existing threshold then write this as the
 *      new value of ZXYMT_CI_FIRST_TOUCH_MODE_ON_THRESHOLD
 *
 * This is implemented privately, as it's hoped that the process can be hidden
 * from all customers inside the calibration process.
 */
static void configFtmMT(void)
{
    uint16_t    onThreshold, noiseLimit;

    (void)zul_getConfigParamByID(ZXYMT_CI_FTM_ON_THRESHOLD, &onThreshold);
    (void)zul_getStatusByID(ZXYMT_SI_NUM_ZEROS_WARNING_MAX, &noiseLimit);
    noiseLimit += 10;


    if ( (noiseLimit) > onThreshold)
    {
        (void)zul_setConfigParamByID(ZXYMT_CI_FTM_ON_THRESHOLD, noiseLimit);
    }
}

/**
 * set the on board calibration
 */
void zul_setOnBoardCal (Calibration *c)
{
    uint8_t     num;
    uint8_t     base;
    uint8_t     i;
    int16_t     pid = -1;
    bool        mtFTMconfigReqd = false;

    if (zul_getDevicePID(&pid))
    {
        switch (pid)
        {
            case ZXY100_PRODUCT_ID:
                num  = ZXY100_CN_ONBOARD_CAL_COUNT;
                base = ZXY100_CI_ONBOARD_CAL_BASE;
                break;

            case ZXY110_PRODUCT_ID:
                num  = ZXY110_CN_ONBOARD_CAL_COUNT;
                base = ZXY110_CI_ONBOARD_CAL_BASE;
                break;

            default:        // Multitouch devices
                num  = ZXYMT_CN_ONBOARD_CAL_COUNT;
                base = ZXYMT_CI_ONBOARD_CAL_BASE;
                mtFTMconfigReqd = true;
        }

        // check that the required number will fit in the array
        assert (num <= ZXY100_CN_ONBOARD_CAL_COUNT);

        for (i=0; i<num; i++)
        {
            (void)zul_setConfigParamByID(i+base, c->val[i]);
        }
        if (mtFTMconfigReqd)
        {
            configFtmMT();
        }
    }
}



// ============================================================================
// ---  Virtual Key Programming Services ---
// ============================================================================

/**
 * Clear a virtual key definition
 */
void            zul_clearVirtKey                (int index)
{
    int16_t pid = UNKNWN_PRODUCT_ID;
    response_handler_t  handFunc = default_CTRL_handler;
    zul_getDevicePID(&pid);
    if ( zul_isZXY500AppPID(&pid) )
    {
        bool    ok;
        uint8_t msgBuf[DUAL_BYTE_MSG_LEN];

        bzero(msgBuf, DUAL_BYTE_MSG_LEN);
        ok = zul_encodeVirtKeyClear(msgBuf, DUAL_BYTE_MSG_LEN, index);

        if (ok)
        {
            (void)usb_ControlRequest(msgBuf, DUAL_BYTE_MSG_LEN, handFunc);
        }
    }
}

/**
 * Set a virtual key definition
 */
void            zul_setVirtKey                  (VirtualKey *vk)
{
    int16_t pid = UNKNWN_PRODUCT_ID;
    response_handler_t  handFunc = default_CTRL_handler;
    zul_getDevicePID(&pid);
    if ( zul_isZXY500AppPID(&pid) )
    {
        bool    ok;
        uint8_t msgBuf[DUAL_BYTE_MSG_LEN+17];

        bzero(msgBuf, DUAL_BYTE_MSG_LEN+17);
        ok = zul_encodeVirtKeySet(msgBuf, DUAL_BYTE_MSG_LEN+17, vk);

        if (ok)
        {
            (void)usb_ControlRequest(msgBuf, DUAL_BYTE_MSG_LEN, handFunc);
        }
    }
}


/**
 * Get a virtual key definition
 */
void            zul_getVirtKey                  (int index, VirtualKey *vk)
{
    int16_t pid = UNKNWN_PRODUCT_ID;
    response_handler_t  handFunc = default_CTRL_handler;
    zul_getDevicePID(&pid);
    if ( zul_isZXY500AppPID(&pid) )
    {
        bool    ok;
        uint8_t msgBuf[DUAL_BYTE_MSG_LEN];

        bzero(msgBuf, DUAL_BYTE_MSG_LEN);
        ok = zul_encodeVirtKeyGet(msgBuf, DUAL_BYTE_MSG_LEN, index);

        if (ok)
        {
            (void)usb_ControlRequest(msgBuf, DUAL_BYTE_MSG_LEN, handFunc);
        }
    }
}


// ============================================================================
// ============================================================================


/**
 * get touch data (from Report ID 6)
 *  Not currently used - but could be useful in future ... TBD
 */
int zul_GetPrivateTouchData(uint8_t *buffer, int bufSize)
{
    int retVal = -1;

    if (!msv_privateTouchMode) return FAILURE;

    if (bufSize >= 64) msv_pvtTouchUserBuf = buffer;
    // ToDo carp on failure wrt above

    // retVal = zul_InterruptCheck(handle_privateTouches);
    msv_pvtTouchUserBuf = NULL;
    return retVal;
}


/**
 * Return true if a touch is available.
 *
 * Both private/silent data are catered for here.
 */
bool zul_Get1TouchFromData (uint8_t *data, Contact *c)
{
    bool touched = false;
    uint8_t *p;
    uint8_t flags;

    if (data == NULL)
    {
        return false;
    }

    p = data + 1;
    flags = (uint)*p++;

    flags &= 0x07;      // only consider 3 bits
    c->flags = flags;

    switch (flags)
    {
        // ZXY100 formats
        case 4:     // intended fall-through
        case 7:     // intended fall-through
            ;       // do nothing !
            break;

        // ZXY110, 150, 200 etc formats
        case 3:     // intended fall-through
        case 0:     // intended fall-through
            // consume the MT Contact ID
            c->ID = *p++;
            break;

        default:
            fprintf(stderr, "FLAGS: %02x\n", (unsigned int) flags);
    }


    //     ZXY100    &    ZXY110/MT
    //   ----------      ----------
    if ((flags == 7) || (flags == 3))
    {
        touched = true;
    }

    // extract the X & Y values
    c->x  = (uint)*p++;
    c->x += 0x100 * (uint)(*p++);

    c->y  = (uint)*p++;
    c->y += 0x100 * (uint)(*p++);

    // fprintf(stderr, "FLAGS: %02x %d %d -=-\n", flags, c->x, c->y);

    return touched;
}

// return true if a touch is available, and set the supplied contact
bool zul_TouchAvailable(Contact *c)
{
    static bool touched = false;

    uint8_t *tch = zul_GetTouchData();

    if (tch == NULL)
    {
        // use data from from last reading - no update has been received
        return touched;
    }

    touched = zul_Get1TouchFromData (tch, c);

    return touched;
}

/**
 * Assume that there is a touch down ...
 * Wait for a touch-up event, or a time-out
 */
bool zul_GetTouchUp(int timeout_ms, Contact *c)
{
    bool            timeUp = false, touchUp=false;

    struct timeval  tStart, tNow, tDiff, tAllowed;
    int             result;

    result = ftime(&tStart);

    if (result != 0) return false;

    c->flags = 0x7;

    timerclear(&tAllowed);
    tAllowed.tv_sec = timeout_ms/1000;
    timeout_ms %= 1000;
    tAllowed.tv_usec = timeout_ms*1000;

    #ifdef __APPLE__
    zul_logf(4, "T1 %ld  %d --\n", tAllowed.tv_sec, tAllowed.tv_usec);
    #else
    zul_logf(4, "T1:: %03ld %06ld  --\n", tAllowed.tv_sec, tAllowed.tv_usec);
    #endif

    while (!timeUp)
    {
        // set contact flags, only if a new touch report is available
        zul_TouchAvailable(c);

        result = ftime(&tNow);
        if (result !=0) return false;
        timersub(&tNow, &tStart, &tDiff);

        #ifdef __APPLE__
        zul_logf(4, "T2 %ld  %d --\n", tDiff.tv_sec, tDiff.tv_usec);
        #else
        zul_logf(4, "T2:: %03ld %06ld --\n", tDiff.tv_sec, tDiff.tv_usec);
        #endif


#ifndef S_SPLINT_S
        timeUp = timercmp(&tDiff, &tAllowed, >);
#endif

        // only return on timeout or touch lift-off

        // consider:        ZXY100      &     ZXY110/MT
        //               -------------      -------------
        touchUp =       (c->flags == 4) || (c->flags == 0);

        if (touchUp)
        {
            /* kloodge -- works well enough for single touches
             * which is all that is needed for calibration/clicks */
            c->x = msv_lastTouchLocation.x;
            c->y = msv_lastTouchLocation.y;
            return true;
        }
    }

    return false;
}

static struct timeb    rawInTimeMs = {0, 0, 0, 0};
void zul_SetRawDataBuffer(void *buffer)
{
    int16_t pid;
    if (usb_getDevicePID(&pid))
    {
        switch (pid)
        {
            case ZXY100_PRODUCT_ID:
            case ZXY110_PRODUCT_ID:
                zul_SetRawDataBuffer100(buffer);
                break;
            default:    // Multitouch devices
                msv_image = buffer;

                // MT array setup
                (void)zul_getStatusByID(ZXYMT_SI_NUM_X_WIRES, &msv_xWires);
                (void)zul_getStatusByID(ZXYMT_SI_NUM_Y_WIRES, &msv_yWires);

                (void)ftime(&rawInTimeMs);
                zul_logf(3, "MT Raw Buffer setup %d %d\n", msv_xWires, msv_yWires);
        }
    }
}

uint8_t *zul_GetSpecialRawData(void)
{
    return msv_rawDataStatus;
}

uint8_t *zul_GetHeartBeatData(void)
{
    /*@null@*/
    uint8_t *retVal = NULL;
    if (msv_heartBeatData[0] == HEARTBEAT_REPORT)
    {
        // mark report as read. Don't re-read same report.
        msv_heartBeatData[0] = 0xFF;

        retVal = msv_heartBeatData;
    }
    return retVal;
}

/**
 * Return a pointer to the raw data buffer.
 * Mark the buffer as read when supplied,
 * and if not updated return NULL
 */
uint8_t *zul_GetTouchData(void)
{
    uint8_t *retVal = NULL;
    if (msv_touchData[0] != 0xFF)
    {
        // mark report as read. Don't re-read same report.
        msv_touchData[0] = 0xFF;

        retVal = msv_touchData;
    }

    return retVal;
}


/**
 * set the device mode - normal or raw data
 */
void zul_SetRawMode(int newMode)
{
    bool    ok;
    uint8_t msgBuf[SINGLE_BYTE_MSG_LEN + 1];
    int16_t pid;

    zul_logf(3, "%s %d", __FUNCTION__, newMode);

    if (usb_getDevicePID(&pid))
    {
        switch (pid)
        {
            case ZXY100_PRODUCT_ID:
            case ZXY110_PRODUCT_ID:
                zul_setRawMode100(newMode);
                break;
            default:
                msv_RawDataMode = newMode;
        }

        bzero(msgBuf, SINGLE_BYTE_MSG_LEN + 1);
        ok = zul_encodeRawModeRequest(msgBuf, 9, newMode);

        if (ok)
        {
            (void)usb_ControlRequest(msgBuf, SINGLE_BYTE_MSG_LEN + 1, default_CTRL_handler);
            zul_logf(4, "   RawMode=%d command sent", newMode );
        }
    }
}


/**
 * set the device Touch mode - normal or private/silent touch data
 */
void zul_SetTouchMode(int newMode)
{
    bool    ok;
    uint8_t msgBuf[SINGLE_BYTE_MSG_LEN + 1];
    zul_logf(3, "%s %d", __FUNCTION__, newMode);

    bzero(msgBuf, SINGLE_BYTE_MSG_LEN + 1);
    ok = zul_encodeTouchModeRequest(msgBuf, SINGLE_BYTE_MSG_LEN + 1, newMode);

    if (ok)
    {
        if (newMode != 0)
        {
            usb_RegisterHandler(RAW_DATA, handle_privateTouches);
        }
        else
        {
            // revert the raw-data handler to normal
            usb_RegisterHandler(RAW_DATA, handle_IN_rawdata_mt);
        }
        (void)usb_ControlRequest(msgBuf, SINGLE_BYTE_MSG_LEN + 1, default_CTRL_handler);
    }
}



// ============================================================================
// --- High-level methods to support Firmware Update ---
// ============================================================================

struct fwInfo_t
{
    size_t      byteCount;
    size_t      unWrittenBytes;
    uint8_t     pinfo[2];
    uint16_t    crc;
    /*@null@*/
    uint8_t    *content;
};
typedef struct fwInfo_t ZyfInfo;

static int          msv_packetCounter = 0;
static char         *msv_fwXferResultStr;
static ZyfInfo      msv_fwInfo;

#define             ZY_MAX_FW_FILE_SIZE         (128*1024) /* 128 kB */
#define             ZY_BL_MAX_DATA              (64)
#define             ZXY100_FW_CRC_LEN           (2)
#define             ZXY100_PINFO_LEN            (2)

/**
 * init the binary storage for FW update
 */
void zul_initFwData(void)
{
    msv_fwInfo.content = NULL;
    msv_fwInfo.byteCount = 0;
    msv_fwXferResultStr = "NoResult";
}
/**
 * provide users with a successful-packet-transfer counter
 */
void zul_BLresetPktCount(void)
{
    msv_packetCounter = 0;
}

/**
 * provide users with reasons for failures
 */
char * zul_getZyfXferResultStr(void)
{
    return msv_fwXferResultStr;
}

/**
 * supply the caller with the number of packet transfers that are required
 * for 'this' ZYF.
 * Returns -1, if the ZYF has not been loaded successfully.
 */
int zul_getFwTransferCount(void)
{
    if (msv_fwInfo.byteCount == 0)  return -1;
    return (int)(msv_fwInfo.byteCount / ZY_BL_MAX_DATA);
}

/**
 * Assure that the ZYF file supplied has a valid CRC, and load into memory
 */
int zul_loadAndValidateZyf(char const *Firmware)
{
    FILE           *fptr;
    uint16_t        file_crc;
    uint16_t        test_crc;
    size_t          bytesRead;
    struct stat     st;
    int             retVal;

    zul_ResetSelfCapData(); //msv_oldVerInfo[0] = 0x00;

    retVal = stat(Firmware, &st);
    if (retVal < 0)
    {
        msv_fwXferResultStr = strerror(errno);
        return FAILURE;
    }

    if ((st.st_size > ZY_MAX_FW_FILE_SIZE) ||
        (st.st_size < 5))       // hmmm ... ?
    {
        msv_fwXferResultStr = "size error";
        return FAILURE;
    }

    msv_fwInfo.byteCount = (size_t)st.st_size - 4; /* max firmware size = 128Kb, April 2016 */
    msv_fwInfo.unWrittenBytes = msv_fwInfo.byteCount;

    fptr = fopen(Firmware, "r");    // Windows "rb"
    if (NULL == fptr)
    {
        msv_fwXferResultStr = strerror(errno);
        return FAILURE;
    }

    /* Clear any existing data and allocate memory for new data */
    if (NULL != msv_fwInfo.content)
    {
        free(msv_fwInfo.content);
        msv_fwInfo.content = NULL;
    }

    msv_fwInfo.content = (uint8_t *)malloc((size_t)msv_fwInfo.byteCount + 4);

    if (NULL == msv_fwInfo.content)
    {
        (void)fclose(fptr);
        msv_fwXferResultStr = "malloc error";
        return FAILURE;
    }

    /* Stash data into buffer */

    bytesRead = fread(msv_fwInfo.content, 1, 4 + msv_fwInfo.byteCount, fptr);
    if (bytesRead < msv_fwInfo.byteCount)
    {
        free(msv_fwInfo.content);
        msv_fwInfo.content = NULL;
        (void)fclose(fptr);
        msv_fwXferResultStr = "fread error";
        return FAILURE;
    }

    /* Copy and check CRC of file data */
    if (NULL == memcpy(&file_crc, &(msv_fwInfo.content[msv_fwInfo.byteCount + ZXY100_PINFO_LEN]), ZXY100_FW_CRC_LEN))
    {
        free(msv_fwInfo.content);
        msv_fwInfo.content = NULL;
        (void)fclose(fptr);
        msv_fwXferResultStr = "copy error"; // unlikely !
        return FAILURE;
    }
    msv_fwInfo.crc = file_crc;
    memcpy(&msv_fwInfo.pinfo, &(msv_fwInfo.content[msv_fwInfo.byteCount]), ZXY100_PINFO_LEN);

    if (BL_DEBUG)
    {
        fprintf(stdout, "  CRC read from file 0x%04X, PINFO:%02X%02X\n",
                    (uint)file_crc, (uint)msv_fwInfo.pinfo[0], (uint)msv_fwInfo.pinfo[1]);
    }

    test_crc = zul_getCRC(msv_fwInfo.content, msv_fwInfo.byteCount);

    if (BL_DEBUG)
    {
        fprintf(stdout, "  CRC calculated from file 0x%04X\n", (uint)test_crc);
    }

    if (file_crc == test_crc)
    {
        (void)fclose(fptr);
        msv_fwXferResultStr = "ZYF CRC pass. File OK.";
        return SUCCESS;
    }
    else
    {
        msv_fwInfo.crc = -1;
        msv_fwInfo.pinfo[0] = 0;
        msv_fwInfo.byteCount = 0;
        free(msv_fwInfo.content);
        msv_fwInfo.content = NULL;
        (void)fclose(fptr);
        msv_fwXferResultStr = "CRC filecheck failed";
        return FAILURE;
    }
}

/**
 * send the Program DataBlock to the device, and if Acked, return SUCCESS
 * otherwise, return FAILURE
 */
int zul_testProgDataBlock(void)
{
    uint8_t     txBuffer[65];
    uint16_t    reqLen = 64;

    if (msv_fwInfo.byteCount == 0) return FAILURE;
    memset(txBuffer, 0, 64);

    // Send the Program DataBlock
    zul_encode_BL_ProgDataBlock(txBuffer, 64, msv_fwInfo.byteCount, msv_fwInfo.pinfo);

    if (TIMING_DEBUG) zul_log_ts(2, "Start application transfer ...");
    (void)usb_ControlRequest(txBuffer, reqLen, handle_BL_response);

    if (TIMING_DEBUG) zul_log_ts(2, "reply..");
    if (msv_BL_reply[0] != BL_RSP_Acknowledge)
    {
        return FAILURE;
    }

    return SUCCESS;
}

/**
 *
 */
int zul_transferFirmware(bool track)
{
    static char progress[26] = "";
    static int errorCount = 0;

    int block_start = 0, block_end = 0;
    int loopCount = 0;
    int numBlocks = (int) msv_fwInfo.byteCount / ZY_BL_MAX_DATA;
    int ctrlReqStatus;

    zul_BLresetPktCount();

    while (block_end < (int)msv_fwInfo.byteCount)
    {
        if ((block_start + ZY_BL_MAX_DATA) < (int)msv_fwInfo.byteCount)
        {
            block_end = (block_start + ZY_BL_MAX_DATA);
        }
        else
        {
            block_end = (int) msv_fwInfo.byteCount;
        }

        if (TIMING_DEBUG) zul_log_ts(3, "BLOCK");
        if (BL_DEBUG) printf("  FW Data: %s\t... \n",
            zul_hex2String(msv_fwInfo.content + block_start,16));

        ctrlReqStatus = usb_ControlRequest((uint8_t*)(msv_fwInfo.content + block_start),
                                    ZY_BL_MAX_DATA, handle_BL_response);


        if ( (ctrlReqStatus < 0) || (errorCount > 4) )
        {
            if (BL_DEBUG) printf("  BL COMMS ERRORS %d %d\n",ctrlReqStatus, errorCount);
            msv_BL_reply[0] = BL_RSP_COMMS_ERROR; // => exit!
        }

        ++loopCount;
        (void)snprintf(progress, 25, "%3d/%03d  %02d%%\r",
                loopCount, numBlocks, 100*loopCount/numBlocks );
        progress[25] = '\0';

        // ToDo - There are 2 of these switch blocks, remove one?

        switch (msv_BL_reply[0])   // msv_BL_reply set by handle_BL_response
        {
            case BL_RSP_Acknowledge:
                // simple ack
                errorCount=0;
                break;
            case BL_RSP_SIZE_ERROR:
                msv_fwXferResultStr = "Size error";
                return FAILURE;
            case BL_RSP_BLOCK_WRITTEN:
                // "The bootloader has successfully written the DOWNLOAD
                //   data into flash memory."
                msv_fwXferResultStr = progress;
                errorCount = 0;
                break;
            case BL_RSP_PROGRAMMING_COMPLETE:
                msv_fwXferResultStr = "Programming is complete";
                return SUCCESS;
            case BL_RSP_PING:
                msv_fwXferResultStr = "PING reply during download - unexpected!";
                return FAILURE;
            case BL_RSP_BL_VERSIONS:
                msv_fwXferResultStr = "VERSIONS reply during download - unexpected!";
                return FAILURE;
            case BL_RSP_CRC_ERROR:
                msv_fwXferResultStr = "The data sent to the controller has been corrupted. CRC.";
                return FAILURE;
            case BL_RSP_PROGRAMMING_FAILED:
                msv_fwXferResultStr = "Programming has failed.";
                return FAILURE;
            case BL_RSP_COMMS_ERROR:
                msv_fwXferResultStr = "Unspecified error in communications.";
                return FAILURE;
            case 0:
                errorCount++;
        }

        block_start += ZY_BL_MAX_DATA;

        if (track && (loopCount % 10 == 0))
        {
            printf("                %s\n", progress);
            zul_CursorUp(1);
        }

        if ((BL_DEBUG) || (track && (loopCount == numBlocks)))
            printf("                %s\n\n", progress);

        msv_fwInfo.unWrittenBytes = msv_fwInfo.byteCount - block_end;
    }

    return SUCCESS;
}


/**
 * provide access to the transfer progress info
 */
int zul_transferFirmwareStatus(uint32_t *Size, uint32_t *LeftToWrite)
{
    if (msv_fwInfo.byteCount == 0) return FAILURE;
    *Size = msv_fwInfo.byteCount;
    *LeftToWrite = msv_fwInfo.unWrittenBytes;
    return SUCCESS;
}

// ============================================================================
// --- Basic Bootloader Services ---
// ============================================================================

// ToDo = move the BL parsers to the protocol module

/**
 * return 1=SUCCESS if the supplied ZYF filename is valid for the hardware
 * identified.     NB - ZXY500s must also match the wirecounts!
 */
bool zul_checkZYFmatchesHW(char const * hwID, char const *filename)
{
    char fileFilter[21];
    char * wildcard;

    // some checks on the filename
    if (strstr(filename, ".zyf") == NULL) return false;

    zul_logf( 3, "%s -- |%s|\n", __FUNCTION__, hwID);
    strncpy( fileFilter,  zul_getZYFFilter(), 20);
    fileFilter[20] = '\0';
    wildcard = strchr(fileFilter, '*');
    if (wildcard != NULL)
    {
        *wildcard = '\0';
    }
    if (strlen(fileFilter) < 6)
    {
        strcpy ( fileFilter, "  " );
        strcat ( fileFilter, hwID );
    }

    zul_logf( 3, "%s -- |%s|\n", __FUNCTION__, fileFilter + 2);

    // validate that the supplied filename has the device HW ID
    if ( NULL == strstr(filename, fileFilter + 2) ) return false ;

    return true;
}


/**
 */
bool zul_BLPingOK(void)
{
    bool    ok;
    uint8_t msgBuf[3];
    bool    retVal;

    bzero(msgBuf, 3);
    ok = zul_encode_BL_PING(msgBuf, 3);

    if (ok)
    {
        (void)usb_ControlRequest(msgBuf, 2, handle_BL_response);
        // .. ?
        retVal = (msv_BL_reply[0] == BLPing) ? true : false;
    }
    else
    {
        retVal = false;
    }
    return retVal;
}

/**
 * This is valid for all but some older ZXY100 firmwares (circa 401.89)
 * See services_sc for earlier implementation: zul_getZxy100VersionStr
 */
bool zul_BLgetVersion(char *VerStr, int len, VerIndex index)
{
    bool    ok;
    uint8_t msgBuf[4];
    bool    retVal;

    bzero(msgBuf, 4);
    ok = zul_encode_BL_GetVerStr(msgBuf, 4, index);

    if (ok)
    {
        (void)usb_ControlRequest(msgBuf, 2, handle_BL_response);

        retVal = (msv_BL_reply[0] == BLGetVersionStr) ? true : false;
    }
    else
    {
        retVal = false;
    }

    if (retVal)
    {
        strncpy(VerStr, (char *)msv_BL_reply+2, (size_t)len-1);
        VerStr[len-1] = '\0';
    }

    return retVal;
}

/**
 */
bool zul_BL_RebootToApp (void)
{
    bool    ok;
    uint8_t msgBuf[2];
    int     retVal;

    bzero(msgBuf, 2);
    ok = zul_encode_BL_RebootToApp(msgBuf, 2);

    if (!ok) return false;

    // a reply is not always sent, and even if it is, it is not used
    retVal = usb_ControlRequest(msgBuf, 2, NULL); // not handle_BL_response!

    return retVal > 0;
}

/**
 */
bool zul_BL_RebootToBL (void)
{
    bool    ok;
    uint8_t msgBuf[2];
    int     retVal;

    bzero(msgBuf, 2);
    ok = zul_encode_BL_RebootToBL(msgBuf, 2);

    if (!ok) return false;

    // a reply is not always sent, and even if it is, it is not used
    retVal = usb_ControlRequest(msgBuf, 2, NULL);  // not handle_BL_response!

    return retVal > 0;
}




/**
 *  debug handler to dump the reply data
 */
int handle_cal_response(uint8_t *data)
{
    if (0)
    {
        int x;
        printf("HANDLE CAL RESP:\n\t");
        for (x=0; x<12; x++)
            printf(" 0x%02x", (uint)(data[x]));

        printf("\n");
    }
    return 5;
}

// ============================================================================
// --- Interrupt Data Handler Implementation ---
//     ToDo Move these handlers to the protocol module
// ============================================================================


/*
 * install ONLY standard handlers for the IN data that is expected from a
 * Zytronic Touchscreen Controller.
 */
void zul_SetupStandardInHandlers  (void)
{
    int i;

    zul_logf(3, "%s", __FUNCTION__);
    for (i = 0; i < MAX_REPORT_ID; i++)
    {
        usb_RegisterHandler( i, NULL );
    }

    // as of 2017, IN data is only expected on the following ReportIDs
    usb_RegisterHandler( TOUCH_OS,          handle_IN_touchdata );
    usb_RegisterHandler( RAW_DATA,          handle_IN_rawdata_mt );
    usb_RegisterHandler( HEARTBEAT_REPORT,  handle_IN_heartbeat );
}

void zul_ResetDefaultInHandlers(void)
{
    // To explain: this is a debug service. Nothing else happens.
    // Users can be informed of the data arriving, see default_IN_handler()
    zul_logf(3, "%s", __FUNCTION__);
    usb_ResetDefaultInHandlers();
}

/**
 * Register a handler function for a reportID (Interrupt Transfers)
 */
void zul_SetSpecialHandler ( UsbReportID_t ReportID, interrupt_handler_t handler )
{
    if (ReportID != RAW_DATA) return;
    zul_logf(3, "Special IN Handler ReportID:%d", ReportID);
    usb_RegisterHandler( ReportID, handler );
}


/**
 *  handler to store the private touch data
 */
void handle_privateTouches(uint8_t *data)
{
    uint8_t * destination = msv_privateTouches;

    if (msv_pvtTouchUserBuf != NULL)
    {
        destination = msv_pvtTouchUserBuf;
    }
    memcpy(destination, data, 64);
    zul_log_hex(3 - TOUCH_DEBUG, "PVT Raw Touch: ", destination, 16);
}

// static struct timeb    rawInTimeMs = {0, 0, 0, 0};
long  zul_getRawInAgeMS(void)
{
    time_t          ageMS = 0;
    struct timeb    nowTime;
    struct timeb   *lastArrival = &rawInTimeMs;
    int16_t         pid;

    zul_getDevicePID(&pid);

    if ((pid == ZXY100_PRODUCT_ID) || (pid == ZXY110_PRODUCT_ID))
    {
        lastArrival = zul_zxy100RawInTime();
    }

    (void)ftime(&nowTime);
    ageMS = (nowTime.time - lastArrival->time) * 1000;
    ageMS += (nowTime.millitm - lastArrival->millitm);
    return (long) ageMS;
}

/**
 * Handler to extract the raw sensor data
 * Multitouch support only as yet. (150, 200, 300, 500)
 */
void handle_IN_rawdata_mt(uint8_t *data)
{
    uint8_t *p;
    uint8_t colIndex, rowIndex, numCells;
    uint8_t i;

    zul_log_ts(4, "RAW_MT_IN" );

    if (PROTOCOL_DEBUG)
    {
        zul_logf(1, "%s: %s\n", __FUNCTION__, zul_hex2String(data, 24));
    }

    if (*data != RAW_DATA)
    {
        return;
    }

    // if not in raw mode return!   ToDo: error message?
    if (msv_RawDataMode == 0) return;

    // validate the buffer has been set by the application
    if (msv_image == 0) return;

    (void)ftime(&rawInTimeMs);

    p = data + 1;
    colIndex = *p++;
    rowIndex = *p++;
    numCells = *p++;

    // there are invalid FF values for row and col coming from the controller!
    if ((colIndex >= msv_xWires) && (rowIndex >= msv_yWires))
    {
        memcpy(msv_rawDataStatus, data, 64);
        return;
    }

    for (i=0; i<numCells; i++)
    {
        * (uint8_t*) (msv_image + (msv_yWires*colIndex+rowIndex)) = *p++;
        rowIndex++;
        if (rowIndex == msv_yWires)
        {
            rowIndex = 0; colIndex ++;
            if (colIndex == msv_xWires) return;
        }
    }

    return;
}


/**
 * Handler to extract the heartbeat reports
 */
void handle_IN_heartbeat(uint8_t *data)
{
    zul_log_ts(3, "DEF_HBR_IN" );

    if (PROTOCOL_DEBUG)
    {
        zul_logf(1, "%s: %s\n", __FUNCTION__, zul_hex2String(data, 24));
    }

    if (*data == HEARTBEAT_REPORT)
    {
        memcpy(msv_heartBeatData, data, 64);
        return;
    }

    return;
}



/**
 * default message reply handler
 * just dump the hex of the first N bytes
 */
int default_CTRL_handler(uint8_t *data)
{
    if (PROTOCOL_DEBUG)
    {
         zul_logf(1, "%s: %s\n", __FUNCTION__, zul_hex2String(data, 24));
    }

    // dummy positive value
    return SUCCESS;
}

/**
 * general Bootloader message reply handler, this is called from within the
 * 'usb_ControlRequest()' service normally.
 */
int handle_BL_response(uint8_t *data)
{
    if (PROTOCOL_DEBUG)
    {
        zul_logf(1, "%s: %s\n", __FUNCTION__, zul_hex2String(data, 16));
    }

    switch (data[0])
    {
        case BL_RSP_Acknowledge:
            msv_fwXferResultStr = "BL_Ack";
            break;

        case BL_RSP_VersionStr:
            msv_fwXferResultStr = "VersionData";
            memcpy(msv_BL_reply, data, BL_REPLY_BUF_LEN);
            break;

        case BL_RSP_SIZE_ERROR:
            msv_fwXferResultStr = "Size error";
            break;
        case BL_RSP_BLOCK_WRITTEN:
            // The bootloader has successfully written the data packet into flash memory
            msv_fwXferResultStr = "progress";
            break;
        case BL_RSP_PROGRAMMING_COMPLETE:
            msv_fwXferResultStr = "Programming is complete";
            break;
        case BL_RSP_PING:
            msv_fwXferResultStr = "PING reply";
            break;
        /*
         * see services_sc.c
         * case BL_RSP_BL_VERSIONS: // Response to "GET_BL_VERSION"
         *   msv_fwXferResultStr = "VERSIONS reply";
         *   for(i=0; i<16; i++)
         *       msv_BL_reply[i] = data[i];
         *   break;
         */

        case BL_RSP_CRC_ERROR:
            msv_fwXferResultStr = "The program received by BL failed CRC validation";
            break;
        case BL_RSP_PROGRAMMING_FAILED:
            msv_fwXferResultStr = "Programming has failed.";
            break;

        default:
        case BL_RSP_COMMS_ERROR:
            msv_fwXferResultStr = "Unspecified error in communications.";
    }

    msv_BL_reply[0] = data[0];

    return SUCCESS;   // dummy return
}

/**
 * get reply message handler -
 * store answer in a module-global
 * ToDo - check the CRC before making the value available !!
 */
int get_response(uint8_t *data)
{
    uint16_t val = (uint16_t)data[5];
    val += (uint16_t)data[6]*0x100;

    msv_getConfigParam = val;

    if (PROTOCOL_DEBUG)
    {
        zul_logf(1, "%s: %s\n", __FUNCTION__, zul_hex2String(data, 16));
        zul_logf(1, "     Get CI %03d: %d (0x%04x)\n", (int)msv_xfrIndex, (int)val, (uint)val );
    }
    return SUCCESS;   // OK
}

/**
 * get reply message handler
 * store answer in a module-global
 * ToDo - check the CRC before making the value available !!
 */
int status_response(uint8_t *data)
{
    uint16_t val = (uint16_t)data[5];
    val += (uint16_t)data[6]*0x100;

    msv_getStatusVal = val;

    if (PROTOCOL_DEBUG)  // zul_hex2String
    {
        zul_logf(1, "%s: %s\n", __FUNCTION__, zul_hex2String(data, 16));
        zul_logf(1, "   Get SV %03d: %d (0x%04x)\n", (int)msv_xfrIndex, (int)val, (uint)val );
    }
    return SUCCESS;
}

/**
 * get reply message handler
 * store answer in a module-global
 * ToDo - check the CRC before making the value available !!
 */
int get_str_response(uint8_t *data)
{
    if (data[0] != 0x02) return FAILURE;
    if (data[1] != 0x3e) return FAILURE;
    if (data[2] != 0x6a) return FAILURE;
    if (data[3] != 0x4f) return FAILURE;

    strcpy( msv_resp_string, (char*)data + 5 );

    if (PROTOCOL_DEBUG)  // zul_hex2String
    {
        zul_logf(1, "%s:\t%s\n\t%s\n", __FUNCTION__,
                msv_resp_string, zul_hex2String(data, 16));
    }

    return SUCCESS;
}


/**
 * Dummy handler to extract the touch data from either a HID transfer or a
 * private/silent touch transfer.
 * This should cover ZXY-100, 110, 150, 200, 300, 500
 */
void handle_IN_touchdata(uint8_t *data)
{
    Contact tempC;
    uint8_t expectedCollection = (uint8_t) ( (msv_privateTouchMode) ? RAW_DATA : TOUCH_OS );

    zul_log_ts(3, "DEF_TCH_IN" );
    if (*data != expectedCollection) return;

    memcpy(msv_touchData, data, 64);

    if (zul_Get1TouchFromData (data, &tempC))
    {
        msv_lastTouchLocation = tempC;
    }

    return ;
}
