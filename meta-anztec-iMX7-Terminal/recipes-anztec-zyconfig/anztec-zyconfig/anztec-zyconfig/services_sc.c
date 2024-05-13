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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/timeb.h>

#include "dbg2console.h"
#include "zytypes.h"
#include "protocol.h"
#include "version.h"
#include "zxy100.h"
#include "zxy110.h"
#include "usb.h"
#include "services.h"
#include "services_sc.h"
#include "debug.h"


// === Useful static data holders =============================================

static uint8_t              msv_oldVerInfo[64];
static uint8_t              msv_BL100_reply[BL_REPLY_BUF_LEN];

static Zxy100SysReport      msv_zxy100SystemReport;
static Zxy100TouchReport    msv_zxy100TouchReport;
static Zxy100RawData        msv_zxy100RawData;
static Zxy100VersionData    msv_zxy100VersionData;
static uint16_t             msv_xWires100 = 0, msv_yWires100 = 0;

static int                  msv_RawDataMode100 = 0;
//static bool                 msv_privateTouchMode = false;

// data buffers for interrupt data storage

 /*@null@*/
static void             *   msv_image100 = 0;
//static uint8_t              msv_touchData100[64];
//static Contact              msv_lastTouchLoc100;

// === Private prototypes =====================================================

int             handle_oldVerResponse           (uint8_t *data);    // deprecated ?

int             handle_sysReportResponse        (uint8_t *data);
int             handle_sysTouchReport           (uint8_t *data);
int             handle_singleRawData            (uint8_t *data);

int             handle_BL100_response           (uint8_t *data);

void            parseOldAppVersionInfo          (Zxy100VersionData *d);

int             zul_getOldZxy100VerInfo         (Zxy100VersionData *d);

// ============================================================================
// --- Implementation ---
// ============================================================================


void zul_InitServSelfCap(void)
{
    zul_logf(3, "%s", __FUNCTION__);
    msv_oldVerInfo [0] = 0x00;
    msv_BL100_reply[0] = 0x00;

    msv_zxy100VersionData.valid = false;
    msv_zxy100VersionData.numStatusValues = 0;
    msv_zxy100VersionData.numConfigParams = 0;
}

void zul_ResetSelfCapData(void)
{
    zul_InitServSelfCap();
}

/**
 * ZXY100 Bootloader service to retrieve (float) version number
 */
bool zul_old_BLgetVersion(void)
{
    bool    ok;
    uint8_t msgBuf[2];
    int     retVal;

    bzero(msgBuf, 2);
    ok = zul_encode_BL_VersionF(msgBuf, 2);

    if (!ok) return false;

    retVal = usb_ControlRequest(msgBuf, 2, handle_BL100_response);
    return retVal > 0;
}

/**
 * special case for zxy100 Bootloader message reply handler,
 */
int handle_BL100_response(uint8_t *data)
{
    if (PROTOCOL_DEBUG)
    {
        zul_logf(0, "%s: %s\n", __FUNCTION__, zul_hex2String(data, 16));
    }

    switch (data[0])
    {
        /*
         * the services.c implementation is used for these cases:
         * case BL_RSP_Acknowledge:
         * case BL_RSP_VersionStr:
         * case BL_RSP_SIZE_ERROR:
         * case BL_RSP_BLOCK_WRITTEN:
         * case BL_RSP_PROGRAMMING_COMPLETE:
         * case BL_RSP_PING:
         * case BL_RSP_CRC_ERROR:
         * case BL_RSP_PROGRAMMING_FAILED:
         */

        case BL_RSP_BL_VERSIONS: // Response to CMD_GET_BL_VERSIONS for OLD firmware
            //msv_fwXferResultStr = "VERSIONS reply";
            memcpy( msv_BL100_reply, data, BL_REPLY_BUF_LEN );
            break;

        default:
        case BL_RSP_COMMS_ERROR:
            // msv_fwXferResultStr = "Unspecified error in communications.";
            zul_log(0, "OLD BL - comms error");
    }

    msv_BL100_reply[0] = data[0];

    return SUCCESS;   // dummy return
}


/**
 * This method provides a string holding the bootloader's floating point
 * version value, if the controller correctly replied to a 'BLGetVersionData'
 * request. This should only be required for ZXY100 bootloader devices.
 */
bool zul_BLgetVersionFromResponse(char *VerStr, int len)
{
    float f = 0.0;
    void *pf = &f;

    if (len < 6) return false;
    if (msv_BL100_reply[0] != BLGetVersionData) return false;

    *((uint8_t*)pf + 0) = msv_BL100_reply[1];
    *((uint8_t*)pf + 1) = msv_BL100_reply[2];
    *((uint8_t*)pf + 2) = msv_BL100_reply[3];
    *((uint8_t*)pf + 3) = msv_BL100_reply[4];

    if (BL_DEBUG)
    {
        int z;
        printf("BL Version :: ");   // ToDo use zul_log()
        for (z = 0; z < 6; z++)     // ToDo use zul_hex2String()
        {
            printf("%02x ", (uint)msv_BL100_reply[z]);
            if (z % 4 == 0) printf(" ");
        }
        // printf("\nFloat Size %zd. Version:%f.\n", sizeof(float), f);
    }

    if ((f > 0.0) && (f < 1000.0))
    {
        (void)snprintf(VerStr, (size_t)len-1, "%06.2f", f);
        VerStr[len-1] = '\0';
        return true;
    }

    return false;
}

/**
 * This method provides a processor ID string, if the controller correctly
 * replied to a 'BLGetVersionData' request.
 * This should only be required for ZXY100 bootloader devices.
 */
bool zul_BLgetUniqIDFromResponse(char *IDStr, int len)
{
    if (msv_BL100_reply[0] != BLGetVersionData) return false;

    if (BL_DEBUG)
    {
        int z;
        printf("BL UniqID :: ");
        for (z = 0; z < 20; z++)
        {
            printf("%02x ", (uint)msv_BL100_reply[z]);
            if (z % 4 == 0) printf(" ");
        }
        printf("\n");
    }

    if (len >= 24)
    {
        int z;
        for (z = 0; z < 12; z++)
        {
            char *p = IDStr + z * 2;
            (void)snprintf(p, 3, "%02x", (uint)msv_BL100_reply[z + 4 + 1]);
        }
    }

    return true;
}

/**
 * Early ZXY100s did not have the option to use zul_getVersionStr()
 * so this function provides the data when needed for APP devices.
 */
int zul_getZxy100VersionStr(VerIndex verType, char *v, int len)
{
    zul_logf(3, "%s %d", __FUNCTION__, verType);

    // if verion data cache is empty
    if (! msv_zxy100VersionData.valid)
    {
        // read from controller, and parse
        if (FAILURE == zul_getOldZxy100VerInfo( &msv_zxy100VersionData ) )
        {
            // failures are rare, but have been seen
            zul_log (1, "=== ZXY100 version read fail ===");
            zul_getOldZxy100VerInfo( &msv_zxy100VersionData );
        }
    }

    if (! msv_zxy100VersionData.valid)
    {
        zul_log(0, "zul_getZxy100VersionStr error");
        strncpy (v, "read error", (size_t)len);
        v[len-1] = '\0';    // force string termination
        return FAILURE;
    }

    zul_logf(4, "  SC - FW: %.2f", msv_zxy100VersionData.fwVersion );

    if (msv_zxy100VersionData.fwVersion < 402.00 )
    {
        switch (verType)
        {
             case STR_FW:
                snprintf(v, len, "%.2f", msv_zxy100VersionData.fwVersion );
                break;

             case STR_HW:
                strncpy (v, msv_zxy100VersionData.hwVersionStr, (size_t)len);
                v[len-1] = '\0';    // force string termination
                break;

             case STR_BL:
                snprintf(v, len, "%.2f", msv_zxy100VersionData.blVersion );
                break;

             case STR_AFC:
                snprintf(v, len, "-na-" );
                break;

             case STR_CPUID:
                snprintf(v, len, "%s",  msv_zxy100VersionData.cpuIdStr);
                break;
        }
        return SUCCESS;
    }

    if (verType == STR_CPUID)
    {
        snprintf(v, len, "%s",  msv_zxy100VersionData.cpuIdStr);
        return SUCCESS;
    }

    return FAILURE;
}

int handle_oldVerResponse(uint8_t *data)
{
    memcpy(msv_oldVerInfo, data, 64);
    if (PROTOCOL_DEBUG)
    {
        printf("%s:\n%s\n", __FUNCTION__, zul_hex2String(data, 24));
    }
    return SUCCESS;    // dummy positive value
}
/**
 * query the device for version data
 */
int zul_getOldZxy100VerInfo(Zxy100VersionData *d)
{
    bool        ok;
    uint8_t     msgBuf[SINGLE_BYTE_MSG_LEN];

    zul_logf(3, "%s", __FUNCTION__);

    bzero(msgBuf, SINGLE_BYTE_MSG_LEN);
    ok = zul_encodeOldVersionReq(msgBuf, SINGLE_BYTE_MSG_LEN);

    if (ok)
    {
        msv_oldVerInfo[0] = (uint8_t)'\0';
        (void)usb_ControlRequest(msgBuf, SINGLE_BYTE_MSG_LEN, handle_oldVerResponse);

        if (msv_oldVerInfo[0] != '\0')
        {
            if (d != NULL)
            {
                parseOldAppVersionInfo(d);
            }
            else
            {
                parseOldAppVersionInfo(&msv_zxy100VersionData);
            }
            return SUCCESS;
        }
    }

    return FAILURE;
}


/**
 * Determine XWire and YWire counts for Self Capacitive controllers
 * Multi-touch devices have standard status values to provide this info
 */
int zul_getOldZxy100WireCnt(uint16_t *xWC, uint16_t *yWC)
{
    int                 retVal = FAILURE;
    int16_t             PID;
    char                hardVers[60] = "";
    Zxy100VersionData   d;

    *xWC = *yWC = 299;         // obviously invalid value
    if (usb_getDevicePID(&PID))
    {
        switch (PID)
        {
            case ZXY100_PRODUCT_ID:
                msv_xWires100 = msv_yWires100 = 0;
                if (SUCCESS == zul_getOldZxy100VerInfo(&d))
                {
                    *xWC = msv_xWires100 = d.xCount;
                    *yWC = msv_yWires100 = d.yCount;
                    retVal = SUCCESS;
                }
                break;

            case ZXY110_PRODUCT_ID:
                // Why are these not available as normal Status Values?
                retVal = SUCCESS;
                *xWC = *yWC = 16;
                if(SUCCESS == zul_Hardware(hardVers, 60))
                {
                    if (strstr(hardVers, "-OFF-64-") != NULL)
                    {
                        *xWC = *yWC = 32;
                    }
                    if (strstr(hardVers, "-OFF-128-") != NULL)
                    {
                        // DOESN'T EXIST YET -- 24/11/2018
                        *xWC = *yWC = 64;
                    }
                }
                break;
        }
    }
    return retVal;
}


/**
 *  This reads a single frame of raw data from a ZXY100
 */
int zul_getSingleRawData (Zxy100RawData *d)
{
    bool                ok;
    uint8_t             msgBuf[SINGLE_BYTE_MSG_LEN];
    Zxy100VersionData   vd;
    int                 transfersRequired;

    bzero( msgBuf, SINGLE_BYTE_MSG_LEN );
    bzero( &msv_zxy100RawData, sizeof(msv_zxy100RawData) );

    // assure that the internal wire-counts are OK
    if (msv_xWires100 == 0)
    {
        (void) zul_getOldZxy100VerInfo(&vd);
    }

    zul_logf(3, "%s, X:%d 0x%x , Y:%d 0x%x  \n", __FUNCTION__,
                msv_xWires100, msv_xWires100,
                msv_yWires100, msv_yWires100);

    if (msv_xWires100 == 0) return FAILURE;

    transfersRequired  = 1;
    if (msv_xWires100>16)
    {
        transfersRequired ++;
    }
    if (msv_xWires100>32)
    {
        transfersRequired ++;
    }

    msv_zxy100RawData.firstYIndex = msv_xWires100;
    msv_zxy100RawData.blocksExpected = (uint8_t)transfersRequired;
    msv_zxy100RawData.blocksReceived = 0;

    zul_logf(3, "%s transfers %d\n", __FUNCTION__, transfersRequired);

    while ( transfersRequired != 0)
    {
        transfersRequired--;
        ok = zul_encodeGetSingleRawData(msgBuf, SINGLE_BYTE_MSG_LEN);
        if (ok)
        {
            (void)usb_ControlRequestMR(msgBuf, SINGLE_BYTE_MSG_LEN,
                                            handle_singleRawData, 2);
        }

        // rough check that some data was fetched - ToDo
        zul_logf(3, "### %d %d [%d]",   msv_zxy100RawData.blocksReceived,
                                        msv_zxy100RawData.blocksExpected,
                                        transfersRequired);

        if ( msv_zxy100RawData.blocksReceived == msv_zxy100RawData.blocksExpected  )
        {
            if (d) memcpy(d, &msv_zxy100RawData, sizeof(Zxy100RawData));
            zul_log_hex(4, "   100RawData:", (void*)&msv_zxy100RawData, 132);
            return SUCCESS;
        }
    }

    return FAILURE;
}


/**
 *  This reads a set of system state values, in a single transfer
 */
int zul_getOldSysReport (Zxy100SysReport *d)
{
    bool        ok;
    uint8_t     msgBuf[SINGLE_BYTE_MSG_LEN];

    bzero(msgBuf, SINGLE_BYTE_MSG_LEN);
    ok = zul_encodeOldSysReportReq(msgBuf, SINGLE_BYTE_MSG_LEN);

    if (ok)
    {
        msv_zxy100SystemReport.uptime = 0;
        (void)usb_ControlRequest(msgBuf, SINGLE_BYTE_MSG_LEN, handle_sysReportResponse);

        // rough check that some data was fetched
        if (msv_zxy100SystemReport.uptime != '\0')
        {
            if (d) memcpy(d, &msv_zxy100SystemReport, sizeof(Zxy100SysReport));
            return SUCCESS;
        }
    }

    return FAILURE;
}

/**
 *  Return the noise metric delta
 *    ToDo - move to the Zxy100Data Qt class ??
 */
int zul_getNoiseAlgoMetric (Zxy100SysReport *d)
{
    static int  storedTotal = 0;
    int         difference = 0;

    if (d)
    {
        int     newTotal = 0, i;

        for (i=0; i<ZXY100_SYSRPT_NOISE_ALGOS; i++)
        {
            newTotal += d->noiseMetrics[i];
        }
        difference = newTotal - storedTotal;
        storedTotal = newTotal;
    }

    return difference;
}

/**
 *  This reads a single touch report from ZXY100s
 *  It may be used to tell if there is a touch NOW.
 */
int zul_getOldTouchReport (Zxy100TouchReport *tr)
{
    bool        ok;
    uint8_t     msgBuf[SINGLE_BYTE_MSG_LEN];

    bzero(msgBuf, SINGLE_BYTE_MSG_LEN);
    bzero(&msv_zxy100TouchReport, sizeof(Zxy100TouchReport));

    ok = zul_encodeGetSingleTouchData(msgBuf, SINGLE_BYTE_MSG_LEN);

    if (ok)
    {
        msv_zxy100SystemReport.uptime = 0;
        (void)usb_ControlRequest(msgBuf, SINGLE_BYTE_MSG_LEN, handle_sysTouchReport);

        // rough check that some data was fetched
        if (msv_zxy100TouchReport.x != 0)
        {
            if (tr) memcpy(tr, &msv_zxy100TouchReport, sizeof(Zxy100TouchReport));
            return SUCCESS;
        }
    }

    return FAILURE;
}

/**
 * a function to read the OLD versioning protocol (pre 2014)
 * i.e. ZXY100s before firmware version 500.00
 */
void parseOldAppVersionInfo(Zxy100VersionData *d)
{
    int         wires;
    char        ct;
    // uint8_t     cpuid[CPU_STR_LEN/2];

    zul_logf (3, "%s %p",  __FUNCTION__, d);


    d->hwVersion = *(float *)(msv_oldVerInfo + 4);
    d->fwVersion = *(float *)(msv_oldVerInfo + 8);
    d->blVersion = *(float *)(msv_oldVerInfo + 12);
    d->controllerType = msv_oldVerInfo[16];

    msv_xWires100 = d->xCount = msv_oldVerInfo[17];
    msv_yWires100 = d->yCount = msv_oldVerInfo[18];

    wires = (int)(d->xCount + d->yCount);
    ct = d->controllerType == 1 ? 'S' : 'U';
    (void)snprintf(d->hwVersionStr, HW_STR_LEN,
            "ZXY%03.0f-%c-OFF-%d", d->hwVersion, ct, wires);
    d->hwVersionStr[HW_STR_LEN]= '\0';

    //memcpy(cpuid, msv_oldVerInfo + 20, CPU_STR_LEN/2-1);
    //cpuid[CPU_STR_LEN/2-1] = 0;

    // get the CPU Unique ID into the struct
    {
        char *p = d->cpuIdStr;
        int i;
        for (i = 0; i < 12; i++, p+=2)
        {
            (void)snprintf(p, 3, "%02X", (uint)msv_oldVerInfo[i + 20]);
        }
        *p = '\0';
    }

    // for old FW, derive number of status and config items available
    d->numConfigParams=0;
    d->numStatusValues=0;
    if (d->fwVersion > 401.8)
    {
        d->numConfigParams=25;
        d->numStatusValues=0;
    }
    if (d->fwVersion > 402.3)
    {
        d->numConfigParams=45;
        d->numStatusValues=0;
    }
    if (d->fwVersion > 402.4)
    {
        d->numConfigParams=46;
        d->numStatusValues=0;
    }
    // if (d->fwVewrsion > 501.3) { }; NOT NEEDED - read the status values 0 and 1 !

    zul_logf(3, "CPU.. %s", d->cpuIdStr );
    zul_logf(3, "FW .. %.2f", d->fwVersion );
    zul_logf(3, "HW .. %s [#CI:%d]", d->hwVersionStr, d->numConfigParams );

    d->valid = true;
}

uint16_t zul_getZxy100StatusCount(void)
{
    uint16_t num;
    if (msv_zxy100VersionData.fwVersion < 500.0)
    {
        num = msv_zxy100VersionData.numStatusValues;
    }
    else
    {
        (void)zul_getStatusByID( ZXY100_SI_NUM_STATUS_VALUES, &num );
    }
    return num;
}

uint16_t zul_getZxy100ConfigCount(void)
{
    uint16_t num;
    if (msv_zxy100VersionData.fwVersion < 500.0)
    {
        num = msv_zxy100VersionData.numConfigParams;
    }
    else
    {
        (void)zul_getStatusByID( ZXY100_SI_NUM_CONFIG_PARAMS, &num );
    }
    return num;
}

/**
 * There could be a byte order thing here - this was OK on OSX, but linux may be different!
 */
int handle_sysReportResponse(uint8_t *data)
{
    const size_t szPart1 = 4 + 2 * (1+ZXY100_SYSRPT_NOISE_ALGOS+3);
    const size_t szPart2 = 2 * 3;

    uint8_t *pSrc = data+4;

    memcpy( &msv_zxy100SystemReport, pSrc, szPart1 );
    pSrc += szPart1;
    msv_zxy100SystemReport.hwConfigOptions = *pSrc;
    pSrc ++;
    memcpy( &(msv_zxy100SystemReport.framesPerSecond), pSrc, szPart2 );

    if (PROTOCOL_DEBUG>1)
    {
        printf("%s:\n%s\n", __FUNCTION__, zul_hex2String(data, 48));
        printf("%s:\n%s\n", __FUNCTION__, zul_hex2String((void*)&msv_zxy100SystemReport, 48));
    }

    // dummy positive value
    return SUCCESS;
}

/**
 *  The ZXY100 style fetch touch state reply handler
 */
int handle_sysTouchReport (uint8_t *data)
{
    uint8_t *pSrc = data+4;
    msv_zxy100TouchReport.x = msv_zxy100TouchReport.y = 0;

    msv_zxy100TouchReport.flags = *pSrc++;

    memcpy( &msv_zxy100TouchReport.x, pSrc, 4 );
    pSrc +=4;
    msv_zxy100TouchReport.contact_id = *pSrc++;

    if (PROTOCOL_DEBUG>1)
    {
        printf("%s:\n%s\n", __FUNCTION__, zul_hex2String(data, 16));
        printf("%s:\n%s\n", __FUNCTION__, zul_hex2String((void*)&msv_zxy100TouchReport, 16));
    }

    return (int)msv_zxy100TouchReport.x ;
}


/**
 *  The ZXY100 style fetch touch state reply handler
 */
#define ZXY100_RAW_DATA_LEN                     (62)

int handle_singleRawData (uint8_t *data)
{
    uint8_t      *  pSrc;
    uint8_t      *  pDest;
    int             headOffset;
    int             numBytesRxd;
    unsigned int    bytesStored;
    uint8_t         blockIndex;

    if (msv_xWires100 == 0) return FAILURE;

    // is this the head or tail packet of the 62 payload-byte transfer
    if (data[2] == 0x6a)    // this will NOT happen on a tail section [SlaveResponse]
    {
        // head - take data
        numBytesRxd = 59;
        pSrc        = data + 5;
        blockIndex  = data[4];
        headOffset  = 0;
    }
    else
    {
        // tail - take some data
        numBytesRxd = 3;
        pSrc        = data + 0;
        blockIndex  = msv_zxy100RawData.blocksReceived;
        headOffset  = 59;
    }

    if (blockIndex > 2)
    {
        zul_logf(1, "BAD RAW DATA OFFSET [%d]", blockIndex);
        zul_log_hex(1, "Rxd:   ", data, 64);
        return FAILURE;
    }

    msv_zxy100RawData.blocksReceived = (uint8_t)blockIndex;

    bytesStored = (unsigned int) blockIndex * ZXY100_RAW_DATA_LEN;
    if ( (bytesStored + numBytesRxd) > (unsigned int) msv_xWires100 * 2)
    {
        numBytesRxd = (int) (msv_xWires100 * 2 - bytesStored);
    }
    if (numBytesRxd<0) numBytesRxd = 0;

    pDest = &(msv_zxy100RawData.wireValue[0]);
    pDest += blockIndex * ZXY100_RAW_DATA_LEN + headOffset;

    memcpy( pDest, pSrc, (size_t)numBytesRxd );

    if (data[2] != 0x6a)    // if the tail has been received, bump the received count
    {
        msv_zxy100RawData.blocksReceived ++;
    }

    if (PROTOCOL_DEBUG)
    {
        zul_logf(1, "Root: %p", &(msv_zxy100RawData));
        zul_logf(1, "Dest: %p (offset:%d)", pDest, blockIndex);
        zul_logf(1, "Size: %d", numBytesRxd);

        zul_log_hex(2, "GRD Rxd:   ", data, 64);
        printf("offset %d\n", (int) blockIndex);
    }

    return (1);
}

// =====================================


static struct timeb    rawInTimeMs100 = {0, 0, 0, 0};

void zul_SetRawDataBuffer100(void *buffer)
{
    msv_image100 = buffer;
    ZXY110_rawImage *ri = (ZXY110_rawImage*)buffer;
    zul_logf(3, "SC Raw Buffer setup %d %d\n", ri->sensorSz.xWires, ri->sensorSz.yWires );
    (void)ftime(&rawInTimeMs100);
}

void zul_setRawMode100(int mode)
{
    msv_RawDataMode100 = mode;
}

struct timeb *zul_zxy100RawInTime(void)
{
    return  &rawInTimeMs100;
}

/**
 * Handler to extract the ZXY100 raw sensor data
 * ZXY100 Only - one data byte per wire - different to ZXY110
 */
void handle_IN_rawdata_100(uint8_t *data)
{
    uint8_t             *p;
    uint8_t             *wire;
    ZXY100_rawImage     *rawImg;

    int                 wiresInPacket;
    int                 i;

    zul_log_ts(5, "\tRAW_100_IN 1" );

    if (PROTOCOL_DEBUG)
    {
        zul_logf(1, "%s:\n%s\n", __FUNCTION__, zul_hex2String(data, 24));
    }

    if (*data != RAW_DATA)
    {
        return;
    }

    zul_log_ts(5, "\tRAW_100_IN 2" );
    // if not in raw mode return!   ToDo: error message?
    if (msv_RawDataMode100 == 0) return;
    zul_log_ts(5, "\tRAW_100_IN 3" );

    // validate the buffer has been set by the application
    if (msv_image100 == 0) return;

    rawImg = (ZXY100_rawImage *)msv_image100;
    zul_logf(3, "\tRAW_100_IN 4: %d", rawImg->sensorSz.xWires );

    (void)ftime(&rawInTimeMs100);

    p = data + 1;   // packet offset byte

    switch (*p)
    {
        case 0:
            wire = &(rawImg->wireSig[0]);
            wiresInPacket = 62;
            if (rawImg->sensorSz.xWires == 16)
            {
                rawImg->allValid = true; // no missing wire values
            }
            break;
        case 1:
            wire = &(rawImg->wireSig[62]);
            wiresInPacket = 62;
            if (rawImg->sensorSz.xWires == 32)
            {
                rawImg->allValid = true; // no missing wire values
            }
            break;
        case 2:
            wire = &(rawImg->wireSig[124]);
            wiresInPacket = 4;
            if (rawImg->sensorSz.xWires == 64)
            {
                rawImg->allValid = true; // no missing wire values
            }
            break;
        default:
            // error case - take no data
            wire = &(rawImg->wireSig[0]);
            wiresInPacket = 0;
            break;
    }

    p++;            // advance to wire data

    // copy the wire data into the ZXY110 raw data struct
    // NB: memcpy option here
    for (i=0; i<wiresInPacket; i++)
    {
        *wire++ = *p++;
    }

    return;
}

/**
 * Handler to extract the ZXY110 raw sensor data
 * ZXY110 Only -- 2 bytes of data per wire !
 *  Review raw data format from "ZXY110 Protocol V1.08(Draft).docx"
 */
void handle_IN_rawdata_110(uint8_t *data)
{
    uint8_t             *p;
    uint16_t            *wire;
    ZXY110_rawImage     *rawImg;

    int                 wiresInPacket;
    int                 i;

    zul_log_ts(4, "RAW_110_IN" );

    if (PROTOCOL_DEBUG)
    {
        zul_logf(1, "%s:\n%s\n", __FUNCTION__, zul_hex2String(data, 24));
    }

    if (*data != RAW_DATA)
    {
        return;
    }

    // if not in raw mode return!   ToDo: error message?
    if (msv_RawDataMode100 == 0) return;

    // validate the buffer has been set by the application
    if (msv_image100 == 0) return;

    rawImg = (ZXY110_rawImage *)msv_image100;

    (void)ftime(&rawInTimeMs100);

    p = data + 1;   // packet offset byte

    switch (*p)
    {
        case 0:
            wire = &(rawImg->wireSig[0]);
            wiresInPacket = 31;
            break;
        case 1:
            wire = &(rawImg->wireSig[31]);
            wiresInPacket = 31;
            if (rawImg->sensorSz.xWires == 16)
            {
                rawImg->allValid = true; // no missing wire values
            }
            break;
        case 2:
            wire = &(rawImg->wireSig[62]);
            wiresInPacket = 2;
            if (rawImg->sensorSz.xWires == 32)
            {
                rawImg->allValid = true; // no missing wire values
            }
            break;
        default:
            // error case - take no data - (128 wire ZXY110 non-existant 2018)
            wire = &(rawImg->wireSig[0]);
            wiresInPacket = 0;
            break;
    }

    p++;            // advance to wire data

    // copy the wire data into the ZXY110 raw data struct
    for (i=0; i<wiresInPacket; i++)
    {
        // byte order important - on some systems memcpy would be OK
        uint16_t v = (*p++);    // first byte of value is LSB
        v += (0x100 * (*p++));
        if (v > 100)    // sanity check
        {
            zul_logf(0, "ODD WIRE-VALUE %05d %04d\n", v, v);
        }
        *wire++ = v;
    }

    return;
}


/**
 * Handler to extract the ZXY110 raw sensor data - into zxy100 data store with
 * only 8-bit wire values. This allows the same Integration Test code to be run
 * for ZXY110 devices as for ZXY100.
 *
 * ToDo - too much code duplication with handle_IN_rawdata_110 - refactor.
 */
void handle_IN_rawdata_110_Clipped(uint8_t *data)
{
    uint8_t             *p;
    uint8_t             *wire;
    ZXY100_rawImage     *rawImg;

    int                 wiresInPacket;
    int                 i;

    zul_log_ts(4, "RAW_110_IN_CLIP" );

    if (PROTOCOL_DEBUG)
    {
        zul_logf(1, "%s:\n%s\n", __FUNCTION__, zul_hex2String(data, 24));
    }

    if (*data != RAW_DATA)
    {
        return;
    }

    // if not in raw mode return!   ToDo: error message?
    if (msv_RawDataMode100 == 0) return;

    // validate the buffer has been set by the application
    if (msv_image100 == 0) return;


    // NB: storing the ZXY110 u16 values as u8  !!

    rawImg = (ZXY100_rawImage *)msv_image100;           // was rawImg = (ZXY110_rawImage *)msv_image100;

    (void)ftime(&rawInTimeMs100);

    p = data + 1;   // packet offset byte

    switch (*p)
    {
        case 0:
            wire = &(rawImg->wireSig[0]);
            wiresInPacket = 31;
            break;
        case 1:
            wire = &(rawImg->wireSig[31]);
            wiresInPacket = 31;
            if (rawImg->sensorSz.xWires == 16)
            {
                rawImg->allValid = true; // no missing wire values
            }
            break;
        case 2:
            wire = &(rawImg->wireSig[62]);
            wiresInPacket = 2;
            if (rawImg->sensorSz.xWires == 32)
            {
                rawImg->allValid = true; // no missing wire values
            }
            break;
        default:
            // error case - take no data - (128 wire ZXY110 non-existant 2018)
            wire = &(rawImg->wireSig[0]);
            wiresInPacket = 0;
            break;
    }

    p++;            // advance to wire data

    // copy the wire data into the ZXY110 raw data struct
    for (i=0; i<wiresInPacket; i++)
    {
        uint16_t v = (*p++);            // first byte of value is LSB
        v += (0x100 * (*p++));
        if (v > 100)    // sanity check
        {
            zul_logf(0, "ODD WIRE-VALUE %05d %04d\n", v, v);
        }

        *wire++ = (v > 255) ? 255 : v;    // CLIPPING
    }

    return;
}


