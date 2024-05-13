/*
 * Copyright 2018 Zytronic Displays Limited, UK.
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

/*
 *  ZXY200 USB Console Application for Firmware Upgrade
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <signal.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "zxymt.h"
// #include "usb.h"
#include "protocol.h"
#include "services.h"
#include "debug.h"

// ----------------------------------------------------------------------------

void cleanup(void)
{
    printf("CleanUp .. \n");
    zul_closeDevice();
    zul_EndServices();
    printf("Done !\n");
}

void sigHandler( int sig)
{
    printf("handling signal %d\n", sig);
    exit(0);
}

void setupHandlers(void)
{
    if (atexit(cleanup) != 0)
    {
        fprintf(stderr, "cannot set exit function\n");
        exit(-1);
    }

    if (SIG_ERR == signal( SIGHUP, sigHandler))
        printf ("Error loading signal handler SIGHUP\n");

    if (SIG_ERR == signal( SIGINT, sigHandler))
        printf ("Error loading signal handler SIGINT\n");

    if (SIG_ERR == signal( SIGQUIT, sigHandler))
        printf ("Error loading signal handler SIGQUIT\n");

    if (SIG_ERR == signal( SIGTERM, sigHandler))
        printf ("Error loading signal handler SIGTERM\n");

    if (SIG_ERR == signal( SIGTSTP, sigHandler))
        printf ("Error loading signal handler SIGTSTP\n");
}


// ----------------------------------------------------------------------------

int     g_deviceIndex = -1;
char    g_zyfFile[120+1] = "";
bool    g_listOnly;

void handleCommandLineOptions(int argCount, char **argStrings)
{
    char c;
    opterr = 0;
    bool zyfFileFound = false;

    while ((c = getopt (argCount, argStrings, "hld:f:")) != -1)
    {
        switch (c)
        {
            case 'h':
                fprintf(stderr, "This console program can be used to update the firmware on\nZytronic Touchscreen controllers.\n");
                fprintf(stderr, "The following options are accepted:\n");
                fprintf(stderr, "-d\ta device index\n");
                fprintf(stderr, "-l\tlist the connected devices\n");
                fprintf(stderr, "-f\tspecify the ZYF file holding the new firmware (*.ZYF)\n");
                fprintf(stderr, "Usage : %s <options>\n", argStrings[0] );

                exit(0);

            case 'd':
                g_deviceIndex = abs(atoi(optarg));
                break;

            case 'f':
                strncpy(g_zyfFile, optarg, 120);
                g_zyfFile[120] = '\0';
                zyfFileFound = true;
                break;

            case 'l':
                g_listOnly = 1;
                break;

            case '?':
                if (optopt == 'c')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
                exit(1);
            default:
                abort();
        }
    }

    // allow the -f to be "assumed" if 2nd arg is a 'zys' file.
    if ((!zyfFileFound) && (argCount>1) && (strstr(argStrings[1], ".zyf")))
    {
        strncpy(g_zyfFile, argStrings[1], 120);
        g_zyfFile[120] = '\0';
    }
}

// ----------------------------------------------------------------------------

#define TEMP_BUF_LEN            (1000)

int main (int argCount, char **argStrings)
{
    int             i, numDevs;
    int16_t         bootDevicePID;
    char            tempBuffer[TEMP_BUF_LEN + 1];
    char            verBuffer[60 + 1];
    char            verStr[200 + 1];
    bool            zyfOK = false;
    bool            reconnect = false;
    bool            reboot2BL = false;

    handleCommandLineOptions(argCount, argStrings);

    // prepare the library services
    i = zul_InitServices();
    if (i!=0)
    {
        printf("zylibUSB open fail %d\n", i);
    }
    else
    {
        setupHandlers();    // auto close library if interrupted
        zul_getVersion(verStr,200);
        verStr[199] = '\0';

        printf("%s :: %s\n", zul_usbLibStr(), verStr);
    }

    // check the validity of the ZYF
    if (strlen(g_zyfFile) > 0)
    {
        if (FAILURE == zul_loadAndValidateZyf(g_zyfFile))
        {
            printf("Error with Zytronic Firmware File: %s\n", g_zyfFile);
            printf("\t%s\n", zul_getZyfXferResultStr());
            exit(-1);
        }
        else
        {
            zyfOK = true;
            printf("\t%s\n", zul_getZyfXferResultStr());
        }
    }

    numDevs = zul_getDeviceList(tempBuffer, TEMP_BUF_LEN);

    if (numDevs > 0)
    {
        printf("Found Zytronic touchscreen devices:\n%s", tempBuffer);
        int autoSelectDevice = atoi(tempBuffer);
        // printf("    Autoselection index = %d\n");

        if (g_deviceIndex == -1)
        {
            g_deviceIndex = autoSelectDevice;
        }
    }

    if (numDevs == 0)
    {
        printf("No Zytronic devices found\n");
    }
    if (numDevs < 0)
    {
        printf("ERROR %d\n", numDevs);
    }

    // if we are not root, return
    if (0 != system("id -u | grep ^0$ > /dev/null"))
    {
        fprintf(stderr, "This application must be run as root\n");
        zul_EndServices();
        exit (EXIT_FAILURE);
    }

    // --- OPEN THE DEVICE ---

    if (g_deviceIndex >= 0)
    {
        printf( "Open device index %d ... \n", g_deviceIndex );
        int retVal = zul_openDevice(g_deviceIndex);
        if (retVal != 0)
        {
            printf( "Error [%d] opening device index %d.\n", retVal, g_deviceIndex );
            exit(1);
        }
    }
    else
    {
        printf("Unable to select which device to update.\n");
        exit(1);
    }

    // report the current versions, even if no ZYF is provided

    // first try the newer string based accessors

    if (zul_isBLDevice(g_deviceIndex, tempBuffer))
    {
        char const * hwID;
        zul_getDevicePID(&bootDevicePID);
        hwID = zul_getDevStrByPID(bootDevicePID);
        printf("Device is already in bootloader mode [HW:%s PID:%04X] \n\t %s\n",
                hwID, bootDevicePID, g_zyfFile);

        zul_closeDevice();
        zy_msleep(BL_RESET_DELAY_MS);

        reconnect = zul_checkZYFmatchesHW(hwID, g_zyfFile);
    }
    else
    {
        char hwID[40+1];

        reboot2BL = true;

        if (SUCCESS == zul_Hardware(verBuffer, 60))
        {
            strncpy(hwID, verBuffer, 40);
            hwID[40] = '\0';
            bootDevicePID = zul_getBLPIDByDevS(hwID);

            printf("Connected to device %s\n", verBuffer);
            zul_CpuID(verBuffer, 60);
            printf("      CpuID: %s\n", verBuffer);

            zul_Bootloader(verBuffer, 60);
            printf("      Bootloader: %s [PID:%04x]\n",
                            verBuffer, bootDevicePID);
            zul_Firmware(verBuffer, 60);
            printf("      Firmware: %s\n", verBuffer);

            if (zyfOK)
            {
                reconnect = zul_checkZYFmatchesHW(hwID, g_zyfFile);
            }
        }
        else
        {
            printf("Old protocol should be available through above API calls (2018)\n");
            /* --- Code below should no longer be required

            Zxy100VersionData zxy100VerData;
            if (SUCCESS == zul_getOldZxy100VerInfo(&zxy100VerData))
            {
                strncpy(hwID, zxy100VerData.hwVersionStr, 40);
                hwID[40] = '\0';
                bootDevicePID = zul_getBLPIDByDevS(zxy100VerData.hwVersionStr);

                printf("Connected to device %s\n", zxy100VerData.hwVersionStr);
                printf("      Bootloader  %06.2f [PID:%04X]\n",
                        zxy100VerData.blVersion, bootDevicePID);
                printf("      Application %06.2f\n", zxy100VerData.fwVersion);
                printf("      CpuID       %s\n", zxy100VerData.cpuIdStr);

                if (zyfOK)
                {
                    reconnect = zul_checkZYFmatchesHW(hwID, g_zyfFile);
                }
            } */
        }
    }

    // if there was no valid file offered, just terminate
    if (!zyfOK) exit(0);

    if (!reconnect)
    {
        printf("\n --- The supplied file is not intended for this controller. ---\n\n");
        exit(-2);
    }
    else
    {
        // file matches connected hardware.

        if (reboot2BL)
        {
            printf("Restart to BL ... \n");
            zul_StartBootLoader();
            zul_closeDevice();
            // allow the device time to reboot and re-connect to the OS
            zy_msleep(BL_RESET_DELAY_MS);
        }
        else
        {
            // we're going to re-open the device .. so close it
            zul_closeDevice();
            // ... we don't actually need to disconn/reconn ...
            // this could be improved.
        }

        // now, we should be in Bootloader Mode !   Reconnect
        int findDevLoopCount = 20;
        numDevs = zul_getDeviceList(tempBuffer, TEMP_BUF_LEN);

        while (findDevLoopCount-- > 0)
        {
            numDevs = zul_getDeviceList(tempBuffer, TEMP_BUF_LEN);

            g_deviceIndex = zul_selectPIDFromList(bootDevicePID, tempBuffer);
            if (g_deviceIndex >= 0) break;

            printf("Waiting for BL device %c\n", zul_spinner());
            zul_CursorUp(1);
            zy_msleep(BL_RESET_DELAY_MS/4);
        }
        printf("\n");

        if (numDevs > 0)
        {
            printf("Found Zytronic touchscreen devices:\n%s", tempBuffer);
            int autoSelectDevice = atoi(tempBuffer);

            if (g_deviceIndex == -1)
            {
                g_deviceIndex = autoSelectDevice;
            }
        }

        if (numDevs == 0)
        {
            printf("No Zytronic devices found\n");
        }
        if (numDevs < 0)
        {
            printf("ERROR %d\n", numDevs);
        }


        // --- OPEN THE BOOTLOADER DEVICE ---

        if (g_deviceIndex >= 0)
        {
            printf( "Open device index %d ... \n", g_deviceIndex );
            int retVal = zul_openDevice(g_deviceIndex);
            if (retVal != 0)
            {
                printf( "Error [%d] opening device index %d.\n", retVal, g_deviceIndex );
                exit(1);
            }

            if (bootDevicePID == ZXY100_BOOTLDR_ID)
            {
                zul_setCommsEndurance(COM_ENDUR_HIGH);
            }
        }
        else
        {
            printf("Unable to select which device to update.\n");
            exit(1);
        }
    }

    printf ("%d transfers of 64-byte blocks required\n", zul_getFwTransferCount());

    if (FAILURE == zul_testProgDataBlock())
    {
        printf("Device rejected this ZYF file.\n");
        zul_BL_RebootToApp();
        exit(1);
    }

    int result = zul_transferFirmware(true);   // true => output progress to console
    if (result == FAILURE)
    {
        printf("\nTransfer Failed: %s\n", zul_getZyfXferResultStr());
    }
    else
    {
        printf("Firmware Updated\n");
    }

    zul_BL_RebootToApp();

    printf("Done\n");
    return 0;
}
