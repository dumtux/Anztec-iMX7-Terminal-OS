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


#ifdef __APPLE__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include "comms.h"
#include "debug.h"

#define DEV_ENTRY_SZ        (140)
#define DEV_LIST_SZ         (20*DEV_ENTRY_SZ)

//
// --- Module Static Variables (msv_) ---
//

static char                     msv_usbLibStr[40 + 1]   = "unopened";
static bool                     msv_libInitialised      = false;


/*@null@*/
static struct hid_device_info * msv_device_list         = NULL;      // pointer to hold list of devices


/* persistent list of Zytronic devices. This list should be consistent for a
 * given population of USB devices.  The list is defined and updated during
 * a call to zul_getDeviceList()                                            */

static char                     msv_DevStrArray [DEV_LIST_SZ];


/* This variable holds the entry in msv_DevStrArray which is currently
 * connected                                                                */
static int                      msv_device_index        = -1;


/* This variable holds the PID of the device which is currently connected   */
static int16_t                  msv_device_pid          = -1;


/* This variable is the opaque handle to the connected HID device           */
/*@null@*/
static hid_device              *msv_dev_handle          = NULL;


/**
 * Boolean to indicate device connected is a BOOTLOADER */
static bool                     msv_bootloader          = false;



//
// === Private Prototypes ========================
//



/* given an index in the msv_DevStrArray, set the provided path variable
 * and return true.
 * If the index is missing, return false.
 */
static
bool                getDevPathByIndex   (int index, char *path, int len);

static struct
hid_device_info    *getDevInfoByPath    (char *path);

// Test if data holds non-zero bytes
static
bool                nonZeroData         (uint8_t *data, int len);


// ============================================================================
// --- Public Implementation ---
// ============================================================================


/**
 * given a USB Product ID, return a device name string :
 *      ZXY100, ZXY110, ZXY150, ZXY200, ZXY300
 */
char const * zul_getDevStrByPID  (int16_t pid)
{
    switch (pid)
    {
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
        case ZXY200_BOOTLDR_ID:
            return "ZXY200";

        case ZXY300_PRODUCT_ID:
        case ZXY300_BOOTLDR_ID:
            return "ZXY300";

        default:
            return "UNKNOWN";
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
    }

    return retVal;
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
    }

    return retVal;
}

/**
 * Open the library, initialising all internal states
 * return zero on success, else a negative error code
 */
int zul_openLib(void)
{
    int retVal = 0;

    if (msv_libInitialised)
    {
        zul_log(0, "Already Initialised\n");
        return -1;
    }

    memset(msv_DevStrArray, 0, 2000);

    (void)snprintf ( msv_usbLibStr, 40, "HID-API Version Unknown");
    msv_usbLibStr[40] = '\0';     // force null termination

    retVal = hid_init(); //initialize a hid-api session
    if(retVal < 0)
    {
        zul_logf (0, "Init Error %d\n", retVal);
        return -1;  // Failure
    }

    msv_libInitialised = true;

    return retVal;
}


/**
 * provide access to the underlying HID-API version
 */
char *      zul_usbLibStr(void)
{
    return msv_usbLibStr;
}

/**
 * Close library and free any resources
 */
void zul_closeLib(void)
{
    if (!msv_libInitialised) return;

    if (msv_device_list != NULL)
    {
        hid_free_enumeration(msv_device_list);
        msv_device_list = NULL;
    }

    // if a device is open - close it
    if (msv_dev_handle != NULL)
    {
        zul_closeDevice();
    }

    hid_exit(); //close the session

    msv_libInitialised = false;
}

int myComparitor(const void *s1, const void *s2)
{
    return strcmp((const char *)s1, (const char *)s2);
}

/**
 * return a string listing the connected Zytronic Touchscreens, one per line
 * return count is the number of devices
 * negative return values indicate a fault
 *
 * The string is sorted so that the indexing is reliable when devices are
 * neither disconnected nor connected.
 */
int zul_getDeviceList(char *buf, int len)
{
    int                     numFound = 0;
    int                     i = 0;
    char                    zyDevStrList[DEV_LIST_SZ+1];

    struct hid_device_info *cur_dev;

    memset(msv_DevStrArray, 0, DEV_LIST_SZ);

    zyDevStrList[0] = '\0';

    if (!msv_libInitialised)
    {
        zul_logf(0, "Call zul_openLib before %s", __FUNCTION__);
        return -11;
    }

    // list all connected devices with a Zytronic Vendor ID
    //ZYTRONIC_VENDOR_ID
    msv_device_list = hid_enumerate(0x0, 0x0);

    if(msv_device_list == NULL)
    {
        zul_log(0, "Get Device Error, or no devices");
        return -12;
    }

    /*  NB: for each call to hid_enumerate, even with no USB conections
     *      or disconnections, a particular device can move about in
     *      the list. Therefore, we sort them by PID and SerialNumber
     *      before returning a list to the users */

    cur_dev = msv_device_list;
    while (cur_dev && (i < 20))
    {
        i++;

        {
            char newDevice[DEV_ENTRY_SZ+1];

            zul_logf(4, " >> Instance:%d. ProductID:%04X SerialNo:%ls",
                            i, cur_dev->product_id, cur_dev->serial_number );

            if (ZYTRONIC_VENDOR_ID == cur_dev->vendor_id)
            {
                (void)snprintf(newDevice, DEV_ENTRY_SZ,
                            "VID:%04X PID:%04X SN:%-24ls Path:%s",
                            cur_dev->vendor_id, cur_dev->product_id,
                            cur_dev->serial_number, cur_dev->path );

                newDevice[DEV_ENTRY_SZ] = '\0';

                /* example entry from Apple:
                "VID:14C8 PID:0005 SN:49FF74064983545624430987 Path:USB_14c8_0005_0x7fe56bd0a4a0"
                */

                memcpy(msv_DevStrArray+100*numFound, newDevice, DEV_ENTRY_SZ);
                numFound ++;
            }
        }

        cur_dev = cur_dev->next;
    }

    if (i == 20) zul_log( 1, "Too many devices to list");

    zul_logf( 3, "%d Devices in list", numFound);
    qsort(msv_DevStrArray, numFound, DEV_ENTRY_SZ, myComparitor);
    for (i=0; i<numFound; i++)
    {
        char deviceEntry[DEV_ENTRY_SZ+1];
        snprintf(deviceEntry, DEV_ENTRY_SZ, "%d) %s\n",
                    i+1, msv_DevStrArray+DEV_ENTRY_SZ*i);
        zul_log(4, deviceEntry);
        strncat(zyDevStrList, deviceEntry, (size_t)DEV_LIST_SZ);
    }

    strncpy(buf, zyDevStrList, (size_t)len);
    buf[len] = '\0';

    return numFound;
}

/**
 * return true if the supplied Product ID is a bootloader device
 */
bool zul_isBLDevicePID(int16_t pid)
{
    switch (pid)
    {
        case ZXY100_BOOTLDR_ID:
        case ZXY110_BOOTLDR_ID:
        case ZXY150_BOOTLDR_ID:
        case ZXY200_BOOTLDR_ID:
        case ZXY300_BOOTLDR_ID:
            return true;
        default:
            return false;
    }
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

/**
 * Open a particular device for interactions, based on the indices
 * provided by zul_getDeviceList().
 * Return 0 on success, else a negative error code.
 */
int zul_openDevice(int index)
{
    char    iPath[100+1];

    if (msv_device_list == NULL) return -1;

    if (!getDevPathByIndex(index, iPath, 100)) return -2;
    zul_logf(3, "Dev Path: %s", iPath);
    // ### write test func for the above service  !!  ###

    msv_dev_handle = hid_open_path(iPath);

    if (msv_dev_handle != NULL)
    {
        wchar_t reply[255];
        struct  hid_device_info * iDev;

        msv_device_index = index;

        iDev = getDevInfoByPath(iPath);
        if (iDev != NULL)
        {
            msv_device_pid = iDev->product_id;
            msv_bootloader = zul_isBLDevicePID(msv_device_pid);

            if (0)
            {
                zul_logf(4, "Device Handle : %p\n", msv_dev_handle);

                hid_get_manufacturer_string(msv_dev_handle, reply, 255);
                zul_logf(3, "Device Manufacturer: %ls", reply);

                hid_get_product_string(msv_dev_handle, reply, 255);
                zul_logf(3, "Device Product: %ls", reply);

                hid_get_serial_number_string(msv_dev_handle, reply, 255);
                zul_logf(3, "Device SerialNo: %ls", reply);
            }

            hid_set_nonblocking(msv_dev_handle, 1);

            return 0;
        }

        hid_close(msv_dev_handle);
        msv_dev_handle = NULL;
        msv_device_index = -1;

        return -3;
    }
    return -4;
}

/**
 * If a device is open, set the supplied pid and return 1;
 * Else, return 0
 */
bool zul_getDevicePID(int16_t *pid)
{
    if (msv_device_pid < 0) return false;

    *pid = msv_device_pid;
    return true;
}

/**
 * Close an open device
 */
int zul_closeDevice(void)
{
    if (msv_device_index == -1) return -1;
    if (msv_dev_handle == NULL) return -2;

    hid_close(msv_dev_handle);

    msv_device_index = -1;
    msv_device_pid = -1;

    return 0;
}

/*
 * The request of reqLen bytes is sent to the Zytronic USB device, and if
 * the handle_reply pointer-to-function is not null, the supplied function is
 * called to handle the response.
 */
#define BUF_LEN                     (64)
int usb_ControlRequest(uint8_t *request, uint16_t reqLen,
                                            response_handler_t handle_reply)
{
    /*  libUSB data :
    const uint8_t   txRmReqType     = 0x21;
    const uint8_t   txBReq          = 0x09;

    const uint8_t   rxRmReqType     = 0xA1;
    const uint8_t   rxBReq          = 0x01;

    const uint16_t  wIndex          = 0x0000;
    const uint16_t  wLength         = BUF_LEN;
    */


    // general result var
    int             res             = 0;

    // bootloader or application comms?
    // uint16_t        wValue          =
    //            (uint16_t)((msv_bootloader) ? 0x0300 : 0x0305);

    // msecs - BL Program DataBlock takes  ~ 2500 mS !
    // const unsigned  timeout         = 3000;


    zul_logf(3, "Device Handle : %p\n", msv_dev_handle);
    zul_logf(4, "  CTRL req %d bytes", (int)reqLen);

    if (msv_dev_handle == NULL)
    {
        zul_logf (0, "%s - no device", __FUNCTION__);
        return -1;
    }

    if (request == NULL)
        return -20;

    if (reqLen == 0)
        return -21;


    zul_log_hex(4, "  CTRL req : ", request, (int)reqLen);

    //res = libusb_control_transfer ( msv_dev_handle,  txRmReqType, txBReq,
    //        wValue, wIndex,       (unsigned char*)request, reqLen, timeout );
    res = hid_send_feature_report(msv_dev_handle, request, (size_t)reqLen);

    if (res < 0)
    {
        zul_logf(1, "CTRL-TX unknown error %d", res);
    }


    if (NULL == handle_reply) return res;

    zul_log(3, "Reply expected");

    {
        bool validResp = false;
        int rxTransferAttempts = 100;        // ############# 100 for FW update

        // buffer to hold possible replies
        uint8_t data[BUF_LEN];
        memset (data, 0, BUF_LEN);

        while ((!validResp) && (rxTransferAttempts-- > 0))
        {
            data[0] = 0x05;
            (void)usleep(50000);      // ##### TBC - see NACK issue ####

            /* ToDo - there exists a high likelihood that we can accelerate
             * the comms by better handling the above delay.  CFM April 2016
             */

            zul_log(4, "  CTRL RX attempt...");
            res = hid_get_feature_report(msv_dev_handle, data, BUF_LEN);

            if (res > 0)
            {
                zul_log_hex(4, "  CTRL resp: ", data, res);
                if (nonZeroData(data, res))
                {
                    (void)handle_reply(data);
                    validResp = true;
                }
            }

            if (res < 0)
            {
                zul_logf(1, "CTRL-RX unknown error %d", res);
                // return res;

                /* ToDo consider - wrt normal usage, and FW usage as we do not
                 * (yet) suport FW transfers with hid-api, should we remove all
                 * this support to simplify the code?
                 */
            }

            // if (res == 0) ... ?

        }
        if (rxTransferAttempts == 0) zul_logf(1, "\n\nControl RX retries failed\n");
    }

    return res;
}

int usb_ControlRequestMR(uint8_t *request, uint16_t reqLen,
                                response_handler_t handle_reply, int replies)
{
    // general result var
    int res;

    if (replies < 1) return -1;

    res = usb_ControlRequest(request, reqLen, handle_reply);

    if (replies == 1) return res;

    while (--replies>0)
    {
        zul_logf(3, "Multi-Reply expected [%d]", replies);

        {
            bool validResp = false;
            int rxTransferAttempts = 2;

            // buffer to hold possible replies
            uint8_t data[BUF_LEN];
            memset (data, 0, BUF_LEN);

            while ((!validResp) && (rxTransferAttempts-- > 0))
            {
                data[0] = 0x05;
                (void)usleep(50000);      // ##### TBC - see NACK issue ####

                zul_log(4, "  CTRL M-RX attempt...");
                res = hid_get_feature_report(msv_dev_handle, data, BUF_LEN);

                if (res > 0)
                {
                    zul_log_hex(4, "    CTRL resp: ", data, res);
                    if (nonZeroData(data, res))
                    {
                        (void)handle_reply(data);
                        validResp = true;
                    }
                }

                if (res < 0)
                {
                    zul_logf(1, "CTRL-RX unknown error %d", res);
                    // return res;
                }
            }
            if (rxTransferAttempts == 0) zul_logf(1, "\n\nControl RX retries failed\n");
        }

    }
    return res;

}


/**
 * Attempt a USB Interrupt transfer to get touch information
 */
int zul_InterruptCheck(interrupt_handler_t data_handler)
{
    static uint8_t          data[64];
    static int              x = 0;
    int                     res = -1;

    if (msv_dev_handle == NULL)
    {
        zul_logf (0, "%s - no device", __FUNCTION__);
        return -1;
    }

    // using a 10 ms timeout
    res = hid_read_timeout(msv_dev_handle, data, 64, 10);

    if (res > 0)
    {
        zul_logf(4, "%s Iteration %06x, transferred %d ", __FUNCTION__, x, res);
        x++;
        zul_log_hex(4,  "| ", data, res);

        if (data_handler != NULL)
            data_handler(data);

        return res;
    }

    if (res < 0)
    {
        zul_logf(1, "%s unknown error %d", __FUNCTION__, res);
    }

    if (res == 0)
    {
        zul_logf(4, "%s NO DATA", __FUNCTION__);
    }


    return res;
}




// ============================================================================
// --- Private Implementation ---
// ============================================================================


/**
 * Given an index in the msv_DevStrArray, set the provided path variable
 * and return true.
 * If the index is missing, return false.
 */
bool getDevPathByIndex (int index, char *path, int len)
{
    int listLocation = 1;
    char *p = msv_DevStrArray;

    while (p[0] != '\0')
    {
        if (listLocation == index)
        {
            char *loc = strstr(p, "Path:");
            if (loc != NULL)
            {
                strncpy(path, loc+strlen("Path:"), len);
                path[len-1] = '\0';
                return true;
            }
            else
            {
                return false;
            }
        }

        p += DEV_ENTRY_SZ;
        listLocation ++;
    }

    return false;
}


struct hid_device_info *getDevInfoByPath(char *path)
{
    struct hid_device_info *returnDevice = msv_device_list;

    while (returnDevice)
    {
        if (!strcmp(returnDevice->path, path)) return returnDevice;

        returnDevice = returnDevice->next;
    }

    return NULL;
}


/**
 * return true if some of the array is != 0
 */
bool nonZeroData(uint8_t *data, int len)
{
    int i;
    for (i=0; i<len; i++)
    {
        if (data[i] != 0x00) return true;
    }
    return false;
}

#endif // ifdef __APPLE__
