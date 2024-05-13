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


#ifdef __linux__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "dbg2console.h"
#include "usb.h"
#include "debug.h"

#ifdef __linux__
#include <libusb.h>
#endif

//
// --- Module Global Variables ---
//

static char                     msv_usbLibStr[40 + 1] = "unopened";

/*@null@*/
static libusb_context          *msv_libusb_ctx        = NULL;
static int                      msv_device_index      = -1;

// format "bb_pp"; i.e. bus 3, address 2:  "03_02"  - values in HEX
static char                     msv_last_device_addr[7] = "";

/*@null@*/
static libusb_device_handle    *msv_dev_handle        = NULL;
static int16_t                  msv_device_pid        = -1;
static uint8_t                  msv_activeInterface   = 0xff;

/**
 * Boolean to indicate device connected is a BOOTLOADER */
static bool                     msv_bootloader        = false;

/**
 * Boolean to indicate device was dettached from kernel    */
static bool                     msv_reattach          = false;

/**
 * Boolean to indicate various states                      */
static bool                     msv_dev_claimed       = false;


/**
 * Holder for user supplied IN data handlers */
static interrupt_handler_t      msv_IN_handler[MAX_REPORT_ID];

/**
 * Set this to terminate the IN handler thread during device closure */
static bool                     msv_closeInThread     = false;


//
// --- Private Prototypes ---
//

static bool dev_match                   (libusb_device *dev,
                                         const uint16_t vendor,
                                         const uint16_t product);

static int  usb_claimInterface          (uint8_t iface);
static int  usb_releaseInterface        (uint8_t iface);


// test if data holds non-zero bytes
static bool nonZeroData                 (uint8_t *data, int len);

int         usb_BeginInterruptTransfer  (int timeoutMs);

void        usb_stopInXfrService(void);

void        *interruptXfrWorker         (void *arg);


// --- Default interrupt data handlers --

/* These are provided so that expected interrupt transfers
 * do not stall the device and trigger a reset.
 * It is expected that users will replace these with more
 * useful versions.
 */

void        default_IN_handler          (uint8_t *data);


// ============================================================================
// --- Public Implementation ---
// ============================================================================



/**
 * Open the library, initialising all internal states
 * return zero on success, else a negative error code
 */
int usb_openLib(void)
{
    int retVal = 0, i;
    const struct libusb_version *v;
    v = libusb_get_version();

    (void)snprintf ( msv_usbLibStr, 40, "libUSB Version: %d.%d.%d.%d",
                    (int)v->major, (int)v->minor, (int)v->micro, (int)v->nano);
    msv_usbLibStr[40] = '\0';     // force null termination

    if (msv_libusb_ctx == NULL)
        retVal = libusb_init(&msv_libusb_ctx); //initialize a library session

    if(retVal < 0)
    {
        zul_logf (0, "Init Error %d\n", retVal);
        return -1;  // Failure
    }

    /*
        LIBUSB_LOG_LEVEL_NONE (0) : no messages ever printed by the library (default)
        LIBUSB_LOG_LEVEL_ERROR (1) : error messages are printed to stderr
        LIBUSB_LOG_LEVEL_WARNING (2) : warning and error messages are printed to stderr
        LIBUSB_LOG_LEVEL_INFO (3) : informational messages are printed to stdout, warning and error messages are printed to stderr
        LIBUSB_LOG_LEVEL_DEBUG (4) : debug and informational messages are printed to stdout, warnings and errors to stderr
    */


    libusb_set_debug(msv_libusb_ctx, LIBUSB_LOG_LEVEL_ERROR );

    for (i = 0; i < MAX_REPORT_ID; i++)
    {
        msv_IN_handler[i] = NULL;
    }

    return retVal;
}


/**
 * provide access to the underlying libUSB version
 */
char *      usb_usbLibStr(void)
{
    return msv_usbLibStr;
}

/**
 * Close library and free any resources
 */
void usb_closeLib(void)
{
    if (msv_libusb_ctx==NULL) return;

    if (msv_dev_handle != NULL)
    {
        usb_closeDevice();
    }

    libusb_exit(msv_libusb_ctx); //close the session
    msv_libusb_ctx = NULL;
}

/**
 * Provide access to a string listing the connected Zytronic Touchscreens,
 *       one device per line
 * Return value is the number of devices available
 * Negative return values indicate a fault
 */
int usb_getDeviceList(char *buf, int len)
{
    char            result[2002] = "";
    int             numFound = 0;
    int             i, cnt;       // list handling

    libusb_device   **list;       // pointer to hold list of devices

    if (msv_libusb_ctx==NULL) return -11;

    cnt = (int) libusb_get_device_list(msv_libusb_ctx, &list);

    if(cnt < 0)
    {
        zul_log(0, "Get Device Error");
        return -12;
    }

    if(cnt == 0)
    {
        zul_log(0, "No Devices");
        return 0;
    }

    zul_logf ( 3, "%d Devices in list", cnt);

    for (i = 0; i < cnt; i++)
    {
        if (dev_match(list[i], ZYTRONIC_VENDOR_ID, 0))
        {
            char newDevice[120];
            uint16_t idProduct;
            int ok = -1;
            uint8_t     busNo  = libusb_get_bus_number ( list[i] );
            uint8_t     addr = libusb_get_device_address( list[i] );

            zul_logf(3, " >> Instance:%d. Zytronic! Addr=%02X_%02X", i, busNo, addr );

            struct libusb_device_descriptor desc;
            idProduct = 0;
            ok =  libusb_get_device_descriptor( list[i], &desc );
            if (ok == 0)
            {
                idProduct = desc.idProduct;
            }

            /* below we leave room at end of strings for device name string, and APP/BL marker
             * which is filled in by zul_getDeviceList() */
            (void)snprintf(newDevice, 120,
                         "  %d. VID:%04X PID:%04X Addr=%02X_%02X NNNNNN MMM\n",
                            i, ZYTRONIC_VENDOR_ID, (uint)idProduct, (uint)busNo, (uint)addr
                          );
            strncat(result, newDevice, 2000);
            numFound ++;
        }
    }

    zul_logf ( 3, " >> Found %d Zytronic devices\n", numFound);

    libusb_free_device_list( list, 1); //free the list, unref the devices in it

    strncpy(buf, result, (size_t)len);
    buf[len] = '\0';
    return numFound;
}

/**
 * return true if the supplied Product ID is a bootloader device
 */
bool usb_isBLDevicePID(int16_t pid)
{
    switch (pid)
    {
        case ZXY100_BOOTLDR_ID:
        case ZXY110_BOOTLDR_ID:
        case ZXY150_BOOTLDR_ID:
        case ZXY200_BOOTLDR_ID:
        case ZXY300_BOOTLDR_ID:
        case ZXY500_BOOTLDR_ID:
            return true;
        default:
            return false;
    }
}


/**
 * Re-Open the last device closed, by index provided by zul_getDeviceList()
 */
int usb_reOpenLastDevice(void)
{
    int retVal = -1;
    // format "bb_pp"; i.e. bus 3, address 26:  "03_26"
    if (strlen(msv_last_device_addr) == 5)
    {
        retVal = usb_openDeviceByAddr(msv_last_device_addr);
    }

    return retVal;
}

/**
 * Copy the address string to the supplied buffer if available.
 * If supplied, return zero, else return a negative error number.
 */
int usb_getAddrStr(char * addrStr)
{
    if (msv_device_index == -1)
    {
        return -1;                      // busy !! one connection at a time
    }
    if (msv_dev_handle == NULL)
    {
        return -2;                      // busy !! error ?
    }
    if (strlen(msv_last_device_addr) != 5)
    {
        return -3;
    }

    strcpy( addrStr, msv_last_device_addr );
    return SUCCESS;
}

/**
 * get the interface number for the supplied PID
 */
uint8_t usb_getManagementIface(int16_t idProduct)
{
    switch (idProduct)
    {
        case ZXY500_PRODUCT_ID:
        case ZXY500_PRODUCT_ID_ALT1:
            return 1;
        default:
            return 0;
    }
}

// ####################################################################

/**
 * Open a particular device for interactions, based on the usb bus address
 * string provided.  Return 0 to indicate success, else a negative error code
 */
int usb_openDeviceByAddr(char *addrStr)
{
    ssize_t         cnt;                // list handling
    libusb_device   **list;             // pointer to hold list of devices
    char            newDevice[7] = "";  // device USB Address String

    int16_t         idProduct;
    int             ok = -1;
    struct libusb_device_descriptor desc;

    if (msv_device_index != -1)
    {
        return -1;                      // busy !! one connection at a time
    }
    if (msv_dev_handle != NULL)
    {
        return -2;                      // busy !! error ?
    }

    if (msv_libusb_ctx==NULL)
    {
        return -11;
    }

    cnt = libusb_get_device_list(msv_libusb_ctx, &list);

    if (cnt < 0)
    {
        zul_log(0, "Get Device Error");
        return -12;
    }

    if (cnt == 0)
    {
        zul_log(0, "No Devices");
        return -13;
    }

    zul_logf( 4, "OPENING, %d Devices Available.\n", cnt);

    // search device list (of Zytronic devices) for supplied address
    int index = -1,i;
    for (i = 0; (i < cnt) && (index < 0); i++)
    {
        if (dev_match(list[i], ZYTRONIC_VENDOR_ID, 0))
        {
            uint8_t     busNo = libusb_get_bus_number ( list[i] );
            uint8_t     addr  = libusb_get_device_address ( list[i] );
            snprintf(newDevice, 7, "%02X_%02X", (uint)busNo, (uint)addr );
            zul_logf(3, " >> Instance:%03d. Zytronic Device Addr=%s",
                            i, newDevice );
            if ( 0 == strcmp (newDevice, addrStr))
            {
                index = i;
                strcpy ( msv_last_device_addr, newDevice );
            }
        }
    }

    idProduct = 0;
    ok =  libusb_get_device_descriptor( list[index], &desc );
    if (ok == 0)
    {
        idProduct = desc.idProduct;
        zul_logf ( 3, " >> Instance:%d. Zytronic PID %d!\n",
                        index, (int)idProduct);
    }

    if (idProduct <= USB32C_PRODUCT_ID)
    {
        zul_logf ( 0, " Device is too old for this library!\n");
        return -5;
    }

    ok = libusb_open( list[index], &msv_dev_handle);
    zul_logf( 3, " >> libusb_open: %d\n", ok );
    if (ok == 0)
    {
        zul_log_ts ( 3, "Device Opened" );
        msv_device_index = index;
        msv_device_pid = idProduct;

        // set Bootloader Mode for BL devices
        msv_bootloader = usb_isBLDevicePID(msv_device_pid);
        // get the preferred interface
        msv_activeInterface = usb_getManagementIface(idProduct);

        zul_logf(3, "Device Handle : %p PID:%04x BL:%d IF:%d\n",
                 msv_dev_handle, msv_device_pid,
                 msv_bootloader, msv_activeInterface );

        // prepare device
        ok = usb_claimInterface(msv_activeInterface);
        if ( ok != 0 )  // failed to prepare device
        {
            zul_log(0, "failed to prepare touchcontroller device");
            (void)usb_closeDevice();
            return -3;
        }
    }
    else
    {
        msv_dev_handle = NULL;
    }

    //free the list, unref the devices in it
    libusb_free_device_list( list, 1);

    if (msv_dev_handle == NULL)
        return -4;

    return 0;
}



/**
 * Open a particular device for interactions, based on the indices
 * provided by zul_getDeviceList().
 * Return 0 to indicate success, else a negative error code.
 */
int usb_openDevice(int index)
{
    ssize_t         cnt;            // list handling
    libusb_device   **list;         // pointer to hold list of devices

    if (msv_device_index != -1)
        return -1;                  // busy !! one connection at a time
    if (msv_dev_handle != NULL)
        return -2;                  // busy !! error ?

    // setup ?
    if (msv_libusb_ctx==NULL)
        return -11;

    cnt = libusb_get_device_list(msv_libusb_ctx, &list);

    if(cnt < 0)
    {
        zul_log(0, "Get Device Error");
        return -12;
    }

    if(cnt == 0)
    {
        zul_log(0, "No Devices");
        return -13;
    }

    zul_logf( 4, "OPENING, %d Devices Available.\n", cnt);

    if (dev_match(list[index], ZYTRONIC_VENDOR_ID, 0))
    {
        int16_t                         idProduct;
        int                             ok = -1;
        uint8_t                         busNo, addr;
        struct libusb_device_descriptor desc;

        idProduct = 0;
        ok =  libusb_get_device_descriptor( list[index], &desc );
        if (ok == 0)
        {
            idProduct = desc.idProduct;
            zul_logf ( 3, " >> Instance:%d. Zytronic PID %d!\n",
                index, (int)idProduct);
        }

        if (idProduct <= USB32C_PRODUCT_ID)
        {
            zul_logf ( 0, " Device is too old for this library!\n");
            return -5;
        }

        ok = libusb_open( list[index], &msv_dev_handle);

        busNo   = libusb_get_bus_number ( list[index] );
        addr = libusb_get_device_address( list[index] );
        snprintf(msv_last_device_addr, 7, "%02X_%02X",
                                    (uint)busNo, (uint)addr );

        zul_logf( 3, " >> libusb_open: %d\n", ok );
        if (ok == 0)
        {
            zul_log_ts ( 3, "Device Opened" );
            msv_device_index = index;
            msv_device_pid = idProduct;

            // set Bootloader Mode for BL devices
            msv_bootloader = usb_isBLDevicePID(msv_device_pid);
            // get the preferred interface
            msv_activeInterface = usb_getManagementIface(idProduct);

            zul_logf(3, "Device Handle : %p PID:%04x BL:%d IF:%d\n",
                     msv_dev_handle, msv_device_pid,
                     msv_bootloader, msv_activeInterface);

            // prepare device
            ok = usb_claimInterface(msv_activeInterface);
            if ( ok != 0 )  // failed to prepare device
            {
                zul_log(0, "failed to prepare touchcontroller device");
                (void)usb_closeDevice();
                return -3;
            }
        }
        else
        {
            msv_dev_handle = NULL;
        }
    }

    //free the list, unref the devices in it
    libusb_free_device_list( list, 1);

    if (msv_dev_handle == NULL)
        return -4;

    return 0;
}

/**
 * If a device is open, set the supplied pid and return true
 * else, return false
 */
bool usb_getDevicePID(int16_t *pid)
{
    if (msv_device_pid < 0) return false;

    *pid = msv_device_pid;
    return true;
}

/* All pre-2018 devices use interface 0 which also carries the touch
 * interrupt transfers. This is problematic as the kernel needs the
 * interrupt tansfers so that the app can be driven by touch. If we
 * do not grab it, we can't manage the device.
 *
 * From Jan 2018, some ZXY500 devices have a controller interface
 * available on a NEW INTERFACE (#1, a second interface).
 * This should allow an application to claim the 2nd interface, and
 * leave the touch events run free to kernel over interface "#0"
 * (the initial/first interface)
 */


/**
 * Close the current interface, and attempt to open the requested interface.
 * Return true if requested interface was available.
 * If requested interface can't be opened, reconnect to the previous interface
 *  and return false.
 */
bool usb_switchIFace (uint8_t iface)
{
    zul_logf(3, "%s to %d\n", __FUNCTION__, iface );
    usb_releaseInterface(msv_activeInterface);

    if (0 == usb_claimInterface(iface))
    {
        msv_activeInterface = iface;
        return true;
    }

    // restore initial interface
    usb_claimInterface(msv_activeInterface);
    return false;
}


/**
 * Close an open device
 * modified to release interface #1, rather than #0 -- CFM Jan 2018
 */
int usb_closeDevice(void)
{
    if (msv_libusb_ctx==NULL)
        return -11;

    if (msv_dev_handle == NULL)
        return -2;      // error - not open!

    usb_releaseInterface(msv_activeInterface);

    libusb_close(msv_dev_handle);

    msv_dev_handle          = NULL;

    msv_device_index        = -1;
    msv_device_pid          = -1;

    return 0;
}


// ----------------------------------------------------------------------------
// --- Control of Communications Parameters ---
// ----------------------------------------------------------------------------


/**
 * Typically the defaults work well - providing for fast get/set/status
 * transfers.  However, more persistent efforts are required to handle certain
 * operations:
 *    - ZXY100 devices
 *    - handling NACKs (should we ever need to handle a NACK?)
 *    - Firmware Upgrade (APP -> BL & BL -> APP ..?)
 */
#define     DEF_CTRL_DELAY      (5)
static int                      msv_CtrlDelay           = DEF_CTRL_DELAY;

#define     DEF_CTRL_RETRY      (10)
static int                      msv_CtrlRetry           = DEF_CTRL_RETRY;

#define     DEF_CTRL_TIMEOUT    (1000)
static unsigned int             msv_CtrlTimeout         = DEF_CTRL_TIMEOUT;

void usb_setCtrlDelay(int delay)
{
    msv_CtrlDelay = delay;
    zul_logf (4, "%s - TX-RX Delay %d (ms)", __FUNCTION__, msv_CtrlDelay );
}

void usb_defaultCtrlDelay(void)
{
    usb_setCtrlDelay ( DEF_CTRL_DELAY );
}

void usb_setCtrlRetry(int retries)
{
    msv_CtrlRetry = retries;
    zul_logf (4, "%s %d Retries ", __FUNCTION__, msv_CtrlRetry );
}

void usb_defaultCtrlRetry(void)
{
    usb_setCtrlRetry ( DEF_CTRL_RETRY );
}

void usb_setCtrlTimeout(int delay)
{
    msv_CtrlTimeout = delay;
    zul_logf (4, "%s  %d(ms)", __FUNCTION__, msv_CtrlDelay );
}

void usb_defaultCtrlTimeout(void)
{
    usb_setCtrlTimeout ( DEF_CTRL_TIMEOUT );
}

#define BUF_LEN                     (64)
/**
 * The request of reqLen bytes is sent to the Zytronic USB device, and if
 * the handle_reply pointer-to-function is not null, the supplied function is
 * called to handle the response.
 */
int usb_ControlRequest(uint8_t *request, uint16_t reqLen,
                                            response_handler_t handle_reply)
{
    const uint8_t   txRmReqType     = 0x21;     // 00=>ENDPOINT_OUT;  20 => CLASS; 01 => INTERFACE
    const uint8_t   txBReq          = 0x09;     // HID_SET_REPORT

    const uint8_t   rxRmReqType     = 0xA1;     // 80=>ENDPOINT_IN; 20 => CLASS; 01 => INTERFACE
    const uint8_t   rxBReq          = 0x01;     // HID_GET_REPORT

    const uint16_t  wIndex          = msv_activeInterface; // see USB Complete Ed.3 P331.
    const uint16_t  wLength         = BUF_LEN;

    // msv_CtrlTimeout: BL Program DataBlock takes > 2500 mS

    uint8_t data[BUF_LEN+1];
    int res = 0;

    // bootloader or application comms
    uint16_t wValue;
    if(msv_bootloader)
    {
        wValue = 0x0300;    // 03 = ReportType
    }
    else
    {
        wValue = 0x0305;    // 03 = ReportType; 05 = ReportID
    }

    zul_log_hex(4, "  CTRL req : ", request, (int)reqLen);

    if (msv_dev_handle == NULL)
    {
        zul_logf (0, "%s - no device", __FUNCTION__);
        return LIBUSB_ERROR_NO_DEVICE;
    }

    if (request == NULL)
    {
        return -20;
    }

    if (reqLen == 0)
    {
        return -21;
    }

    // always send 64 byte usb packets
    uint8_t txBuffer[64];
    // copy the usb message into a 64 byte buffer
    memcpy(txBuffer, request, reqLen);

    int i;
    // make sure the rest of the packet is zero
    for( i = reqLen; i < 64; i++)
    {
        txBuffer[i] = 0;
    }

    zul_log_hex(4, "  CTRL req (padded) : ", txBuffer, USB_PACKET_LEN);


    res = libusb_control_transfer ( msv_dev_handle,  txRmReqType, txBReq,
            wValue, wIndex,       (unsigned char*)txBuffer, USB_PACKET_LEN, msv_CtrlTimeout);

    if (res < 0)
    {
        switch (res)
        {
        case LIBUSB_ERROR_TIMEOUT:
            zul_log(2, "\n\nControl TX timeout");
            break;
        case LIBUSB_ERROR_PIPE:
            zul_log(1, "Control TX pipe error");
            break;
        case LIBUSB_ERROR_NO_DEVICE:
            zul_log(1, "Control TX No Device");
            break;
        case LIBUSB_ERROR_BUSY:
            zul_log(1, "Control TX Busy");
            break;
        case LIBUSB_ERROR_INVALID_PARAM:
            zul_log(1, "Control TX Invalid parameter");
            break;
        default:
            zul_logf(1, "Control TX unknown error %d", res);
        }
        zul_log_hex (3, "TXReq:", request, 8 );
        return res;     // can't trust any received message
    }

    // when no reply handler provided, return result
    if (NULL == handle_reply)
    {
      return res;
    }

    zul_log(5, "Reply expected");

    bool validResp = false;
    int rxTransferAttempts = msv_CtrlRetry;

    while ((!validResp) && (rxTransferAttempts-- > 0))
    {
        (void)usleep(msv_CtrlDelay * 1000);

        res =  libusb_control_transfer ( msv_dev_handle,
                                            rxRmReqType, rxBReq,
                                            wValue, wIndex,
                                            data, wLength, msv_CtrlTimeout );

        if (res > 0)
        {
            zul_log_ts (4, "REPLIED");
            zul_logf   (4, "       attempt %d/%d",
                          msv_CtrlRetry-rxTransferAttempts, msv_CtrlRetry);
            zul_log_hex(4, "  CTRL resp: ", data, res);
            if (nonZeroData(data, res))
            {
                (void)handle_reply(data);
                validResp = true;
            }
        }

        if (res < 0)
        {
            switch (res)
            {
            case LIBUSB_ERROR_TIMEOUT:
                zul_logf(2, "\n\nControl RX timeout\n");
                break;
            case LIBUSB_ERROR_PIPE:
                zul_logf(1, "Control RX pipe error");
                rxTransferAttempts = 0;     // no point in continuing
                break;
            case LIBUSB_ERROR_NO_DEVICE:
                zul_logf(1, "Control RX No Device");
                rxTransferAttempts = 0;     // no point in continuing
                break;
            case LIBUSB_ERROR_BUSY:
                zul_logf(1, "Control RX Busy");
                break;
            case LIBUSB_ERROR_INVALID_PARAM:
                zul_logf(1, "Control RX Invalid parameter");
                break;
            default:
                rxTransferAttempts = 0;     // no point in continuing
                zul_logf(1, "Control RX unknown error %d", res);
            }
            zul_log_hex (2, "TXReq:", request, 8 );
        }
    }
    if (rxTransferAttempts == 0)
    {
        zul_logf(1, "\n\nControl RX retries failed\n");
    }

    return res;
}

/*
 * The request of reqLen bytes is sent to the Zytronic USB device, and
 * multiple replies are expected!  [see ZXY100 get single raw data]
 *
 * This is not used, typically!
 *
 * If the handle_reply pointer-to-function is not null, the supplied function is
 * called to handle the response.
 */
int usb_ControlRequestMR(uint8_t *request, uint16_t reqLen,
                                response_handler_t handle_reply, int replies)
{
    // general result var
    int res = -1;

    if (replies < 1) return -1;

    res = usb_ControlRequest(request, reqLen, handle_reply);

    if (replies == 1) return res;

    while (--replies>0)
    {
        zul_logf(4, "Multi-Reply expected [%d]", replies);

        {
            bool validResp = false;
            int rxTransferAttempts = 2;

            // buffer to hold possible replies
            uint8_t data[BUF_LEN];
            memset (data, 0, BUF_LEN);

            while ((!validResp) && (rxTransferAttempts-- > 0))
            {
                data[0] = 0x05;
                (void)usleep(msv_CtrlDelay * 1000);

                zul_log(4, "  CTRL M-RX attempt...");

                const uint8_t   rxRmReqType     = 0xA1;
                const uint8_t   rxBReq          = 0x01;
                const uint16_t  wIndex          = 0x0000;
                const uint16_t  wLength         = BUF_LEN;
                // const unsigned  timeout         = msv_CtrlTimeout;

                uint16_t        wValue          =
                    (uint16_t)((msv_bootloader) ? 0x0300 : 0x0305);

                res =  libusb_control_transfer ( msv_dev_handle,
                        rxRmReqType, rxBReq,
                        wValue, wIndex, data, wLength, msv_CtrlTimeout );

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
                    // no point in continuing
                    rxTransferAttempts = 0;
                    replies = 0;
                }
            }
            if (rxTransferAttempts == 0)
            {
                zul_logf(1, "\n\nControl RX retries failed\n");
            }
        }
    }

    return res;
}

// ============================================================================
// --- Private Implementation ---
// ============================================================================

/**
 * return false if device fails to match the VendorID/ProductID supplied.
 * a productID of 0 indicates any product for that vendor
 */
static bool dev_match(libusb_device *dev,
                                const uint16_t vendor, const uint16_t product)
{
    struct libusb_device_descriptor desc;

    int r = libusb_get_device_descriptor(dev, &desc);

    if (r < 0)
    {
        zul_log( 1, "failed to get device descriptor\n");
        return false;
    }

    if (desc.idVendor != vendor)  return false;

    if (product != 0)
    {
        if (desc.idProduct != product) return false;
    }

    return true;
}


/**
 * Take ownership of the interface from the kernel
 * Return 0 on success
 * Return a negative int on failure
 */
static int usb_claimInterface(uint8_t iface)
{
    int     retVal;
    int     r;

    if (msv_dev_handle == NULL)
    {
        // zul_log(0, "can't prepare a null device");
        return -20;
    }

    zul_logf( 4, "Check Kernel isn't driving interface %d", iface);
    if ( libusb_kernel_driver_active(msv_dev_handle, iface) == 1 )
    {
        zul_logf( 3, "Attempt to detach interface %d from kernel", iface);
        if ( libusb_detach_kernel_driver(msv_dev_handle, iface) == 0 )
        {
            // extra space to overwrite previous line
            zul_log( 3, "    ... detach done           " );
            msv_reattach = true;     // NB: module global
            retVal = 0;
        }
        else
        {
            zul_logf( 0, "Failed to detach interface %d from kernel driver", iface);
            retVal = -21;
            goto exit;
        }
    }

    zul_logf( 3, "Claim interface %d", iface);
    r = libusb_claim_interface(msv_dev_handle, iface);
    if (r != 0) // returns zero on success
    {
        zul_logf (0, "Claimed interface %d retval %d", iface, r);
        switch (r)
        {
        case LIBUSB_ERROR_NOT_FOUND:
            zul_log(0, "the requested interface does not exist");
            retVal = -22;
            break;
        case LIBUSB_ERROR_BUSY:
            zul_log(0, "another program or driver has claimed the interface");
            retVal = -23;
            break;
        case LIBUSB_ERROR_NO_DEVICE:
            zul_log(0, "the device has been disconnected");
            retVal = -24;
            break;
        default:
            zul_log(0, "other failure");
            retVal = -25;
        }
        goto exit;
    }

    // start the interrupt transfer managment service
    if ( (! msv_bootloader) && (iface == 0))
    {
        msv_closeInThread = false;
        pthread_t myThread;
        errno = 0;
        errno = pthread_create(&myThread, NULL, interruptXfrWorker, NULL);
        if (errno)
        {
            zul_logf(1, "ERROR: from pthread_create() is %s\n", strerror(errno));
            perror("Create Interrupt Rx Thread");
            exit(-1);
        }
        else
        {
            zul_log_ts ( 3, "interruptXfrWorker is running");
        }
    }
    zul_logf( 3, "Interface %d Claimed\n", iface);

    msv_dev_claimed = true;
    retVal = 0;

exit:
    return retVal;
}



/**
 * Release a claimed interface
 * NB: the device is NOT closed here - so that another interface may be opened/claimed
 */
static int usb_releaseInterface(uint8_t iface)
{
    if (msv_libusb_ctx==NULL)
        return -11;

    if (msv_dev_handle == NULL)
        return -2;      // error - not open!

    if (msv_dev_claimed)
    {
        zul_log( 3, "Terminate the interrupt transfer monitor" );
        usb_stopInXfrService();

        zul_log( 3, "Release interface" );
        (void)libusb_release_interface(msv_dev_handle, iface);
        msv_dev_claimed = false;
    }

    if (msv_reattach)
    {
        zul_log_ts( 3, "Attempt to re-attach to kernel" );
        (void)libusb_attach_kernel_driver(msv_dev_handle, iface);
        msv_reattach = false;
    }

    return SUCCESS;
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

// ============================================================================
// --- Private asynchronous USB INterrupt Transfer Support ---
// ============================================================================

#define IN_BUF_SZ (64)

static struct libusb_transfer * msv_pIntXfr = NULL;
//static bool                   msv_intXfrRunning = false;
static unsigned char            msv_IntXfrBuffer[IN_BUF_SZ];
static long int                 msv_lastIntCallbackTS = 0;
static int                      msv_INXfrTimeout = 200;     // milliseconds


/**
 * By default, interrupt transfers received are printed. This keeps the user
 * in the loop, provides a view of the transfer data and encourages the user
 * to supply a handler.  The handler need not process the data, it can just
 * discard it silently.
 */
void usb_RegisterHandler (UsbReportID_t ReportID, interrupt_handler_t handler)
{
    if (ReportID < MAX_REPORT_ID)
    {
        msv_IN_handler[ReportID] = handler;
    }
}

/**
 *  Assure that suitable handlers are in place for any
 *  interrupt transfers from the controller.
 *  TODO - call this at openDevice - and remove application calls to it!
 */
void usb_ResetDefaultInHandlers (void)
{
    int i;
    for (i = 0; i < MAX_REPORT_ID; i++)
    {
        usb_RegisterHandler( (UsbReportID_t)i, NULL );
    }

    // as of 2017, IN data is only expected on the following ReportIDs
    usb_RegisterHandler( TOUCH_OS,          default_IN_handler );
    usb_RegisterHandler( RAW_DATA,          default_IN_handler );
    usb_RegisterHandler( HEARTBEAT_REPORT,  default_IN_handler );
}

/**
 *  https://computing.llnl.gov/tutorials/pthreads/
 */
void *interruptXfrWorker(void *arg)
{
    (void)(arg);     // unused var - pthread signature
    (void)usleep(87711);
    usb_BeginInterruptTransfer(200);        // timeout in ms

    while (!msv_closeInThread)
    {
        int retVal;

        // 1 packet per ms is the limit for IN packets from ZXYdd0 devices (2017)
        (void)usleep(877);
        retVal = libusb_handle_events(msv_libusb_ctx);
        if (retVal != 0) zul_logf (0, "ERROR @ %s %d", __FUNCTION__, __LINE__ );
    }
    zul_log(3, "Interrupt Transfer Worker - Terminating");

    pthread_exit(NULL);
}

/**
 * myIntCallBack - handle all interrupt transfers from device:
 *  { Touches, RawData, HBR, ... }
 */
void myIntCallBack(struct libusb_transfer *transfer)
{
    bool handleNewData = false;

    int reportID = transfer->buffer[0];
    zul_log(4, __FUNCTION__ );
    msv_lastIntCallbackTS = zul_getLongTS();

    if (reportID >= MAX_REPORT_ID)
    {
        zul_logf(0, "Interrupt Transfer - Bad Report ID %d", reportID );
        return;
    }

    switch (transfer->status)
    {
        case LIBUSB_TRANSFER_COMPLETED:
            zul_logf(4, "IN Xfr Complete [ID:%02d]",reportID);
            handleNewData = true;
        break;

        case LIBUSB_TRANSFER_ERROR:
            zul_logf(3, "Interrupt Transfer Error %d", errno);
            //zul_log(0, strerror( errno ));
            //zul_log(0, libusb_strerror( errno ));
        break;
        case LIBUSB_TRANSFER_TIMED_OUT:
            zul_logf(4, "Interrupt Transfer - TimeOut");
        break;
        case LIBUSB_TRANSFER_CANCELLED:
            zul_logf(3, "Interrupt Transfer Cancelled");
            libusb_free_transfer(msv_pIntXfr);
            msv_pIntXfr = NULL;
        break;
        case LIBUSB_TRANSFER_STALL:
            zul_logf(1, "Interrupt Transfer Stalled");
            libusb_free_transfer(msv_pIntXfr);
            msv_pIntXfr = NULL;
        break;
        case LIBUSB_TRANSFER_NO_DEVICE:
            zul_logf(1, "Interrupt NoDevice");
            libusb_free_transfer(msv_pIntXfr);
            msv_pIntXfr = NULL;
        break;
        case LIBUSB_TRANSFER_OVERFLOW:
            zul_logf(1, "Interrupt Too Much Data");
        break;
        default :
            zul_logf(0, "Unknown Inturrupt Transfer Status");
    }

    if (handleNewData)
    {
        interrupt_handler_t handler = msv_IN_handler[reportID];

        if (handler == NULL)
        {
            zul_logf(3, "Size: %d.  TS: %ld", transfer->actual_length,
                                                    msv_lastIntCallbackTS);
            zul_log_hex(3, "IntXfr",transfer->buffer, 16);
        }

        if (handler != NULL)
        {
            handler(transfer->buffer);
        }
    }

    // if exiting, stop now
    if (msv_closeInThread)
    {
        int retVal = libusb_handle_events_completed (msv_libusb_ctx, NULL);
        if (retVal != 0) zul_logf (0, "ERROR @ %s %d", __FUNCTION__, __LINE__ );
        zul_logf( 3 , "%s -- don't restart IN transfer \n", __FUNCTION__ );
        return;
    }

    // otherwise, set up next interrupt transfer.
    if (msv_pIntXfr != NULL)
    {
        usb_BeginInterruptTransfer(msv_INXfrTimeout);
    }

    return;
}


/**
 * Start a thread to handle ANY interrupt transfer from controller
 *   This prevents the controller resetting, when a touch or other interrupt
 *   transfer is initiated by the controller, as otherwise it would be
 *   uncompleted, and timeout => reset.
 */
int usb_BeginInterruptTransfer(int timeoutMs)
{
    void *  user_data = NULL;
    int     retVal, errnoSave;

    if (msv_closeInThread) return 0;

    if (msv_pIntXfr == NULL)
    {
        msv_pIntXfr = libusb_alloc_transfer(0);
        zul_logf (3, "BUF_ALLOC @ %s %d", __FUNCTION__, __LINE__ );
    }
    msv_INXfrTimeout = timeoutMs;
    (void)libusb_fill_interrupt_transfer( msv_pIntXfr, msv_dev_handle, 0x81,
                                    msv_IntXfrBuffer, IN_BUF_SZ,
                                    myIntCallBack, user_data, timeoutMs );

    retVal = libusb_submit_transfer(msv_pIntXfr);
    errnoSave = errno;
    switch (retVal)
    {
        default:
            zul_logf (0, "ERROR @ %s %d %s\n\t%s", __FUNCTION__, __LINE__,
                    libusb_error_name(retVal), strerror(errnoSave) );
            retVal = FAILURE;
            break;
        case     0:
            retVal = SUCCESS;
            // success
            break;
        case LIBUSB_ERROR_NO_DEVICE:
            zul_logf( 0, "%s device has been disconnected", __FUNCTION__ );
            retVal = FAILURE;
            break;
        case LIBUSB_ERROR_BUSY:
            zul_logf( 0, "%s transfer has already been submitted", __FUNCTION__ );
            retVal = FAILURE;
            break;
        case LIBUSB_ERROR_NOT_SUPPORTED:
            zul_logf( 0, "%s transfer flags are not supported by OS", __FUNCTION__ );
            retVal = FAILURE;
            break;
    }
    return retVal;
}


/**
 * If an interrupt transfer is in flight, cancel it and await
 * for the transfer to be freed, which occurs once the cancel action
 * is processed by libusb_handle_events.
 */
void usb_stopInXfrService(void)
{
    int retVal;
    zul_log_ts( 3 , __FUNCTION__ );
    if (msv_closeInThread) return;

    if (msv_pIntXfr != NULL)
    {
        retVal = libusb_cancel_transfer(msv_pIntXfr);
        if (retVal != 0)
        {
            zul_logf (3, "ERROR @ %s %d %s", __FUNCTION__, __LINE__, libusb_error_name(retVal) );
        }
        // above WILL call myIntCallBack() IF the transfer is active
    }

    int countdown = 30;
    // supervised shutdown - handle cancelled transfer
    while ((msv_pIntXfr != NULL) && (countdown-- > 0))
    {
        (void)usleep(900);
        // in thread calls libusb_handle_events(msv_libusb_ctx)
        // which NULLs msv_pIntXfr
    }
    msv_closeInThread = true;

    (void)usleep(msv_INXfrTimeout * 1100);
        // * 1100 => a little more than the INXfrTimeout in microseconds
}


// ============================================================================
// --- Dummy Interrupt Data Handlers ---
//     Users will overwrite the handlers to access the data transferred
// ============================================================================

/**
 * Dummy handler to prevent IN transfers from stalling the USB comms and
 * triggering a device reset.
 */
void default_IN_handler(uint8_t *data)
{
    if (PROTOCOL_DEBUG)
    {
        zul_logf(1, "%s: %s\n", __FUNCTION__, zul_hex2String(data, 24));
    }

    // do nothing
}

#endif // ifdef __linux__
