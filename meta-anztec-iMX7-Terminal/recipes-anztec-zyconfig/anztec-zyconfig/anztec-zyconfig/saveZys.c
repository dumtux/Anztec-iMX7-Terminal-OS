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


/* This code is provided as an example only.
 * It's purpose is to retrieve the internal configuration and status of a ZXY
 * touchscreen controller and save it to a ZYS file as text.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>


#include "zytypes.h"
#include "debug.h"
// #include "usb.h"
#include "protocol.h"
#include "services.h"
#include "sysdata.h"

#define TEMP_BUF_LEN (1000)

int  g_runTest = -1;
int  g_testIndex;
int  g_testValue;
int  g_numSpiDevs = 0;
int  g_deviceIndex = -1;
char g_zysFile[200+1] = "";
char g_filename[100] = "";

FILE *fp = NULL;

/**
 * default message reply handler
 * just dump the hex of the first 16 bytes
 */
int main_handle_response(uint8_t *data)
{
    int x;
    printf("DUMMY RESP HANDLER:\n\t");
    for (x=0; x<16; x++)
            printf(" 0x%02x", data[x]);

    printf("\n");

    // dummy positive value
    return 1;
}


/**
 * get reply message handler
 * just dump the index and the value in hex & decimal
 */
int main_get_response(uint8_t *data)
{
    int val = data[5];

    val += data[6]*0x100;

    printf("Get config index %d: %d (0x%04x)\n", g_testIndex, val, val );

    // dummy positive value
    return 1;
}


/**
 * get reply message handler
 * just dump the index and the value in hex & decimal
 */
int main_status_response(uint8_t *data)
{
    int val = data[5];

    val += data[6]*0x100;

    printf("Get status value %d: %d (0x%04x)\n", g_testIndex, val, val );

    // dummy positive value
    return 1;
}

static char resp_string[80];

/**
 * get reply message handler
 * store answer in a module-global
 */
int main_get_str_response(uint8_t *data)
{
    strcpy( resp_string, (char*)data + 1);

    return 1;
}

// ----------------------------------------------------------------------------

void             saveConfig100()
{

    uint8_t     index;
    uint16_t    numStatus, numConfig, tempVal;

    // printf("%s\n", __FUNCTION__);

    zul_getStatusByID(ZXY100_SI_NUM_STATUS_VALUES, &numStatus);
    zul_getStatusByID(ZXY100_SI_NUM_CONFIG_PARAMS, &numConfig);

    uint8_t     numSV8 = (uint8_t)numStatus;
    uint8_t     numCI8 = (uint8_t)numConfig;

    for (index = 0; index<numSV8; index++)
    {
        if (SUCCESS == zul_getStatusByID(index, &tempVal))
        {
            fprintf(stdout, "STATUS %02X %04X\n", index, tempVal);
            fprintf(fp,     "STATUS %02X %04X\r\n", index, tempVal);
            zul_CursorUp(1);
        }
        else
        {
            fprintf(stdout, "STATUS %02X ----\n", index);
        }
    }
    fprintf(stdout,"\n");
    for (index = 0; index<numCI8; index++)
    {
        if (SUCCESS == zul_getConfigParamByID(index, &tempVal))
        {
            fprintf(stdout, "CONFIG %02X %04X\n", index, tempVal);
            fprintf(fp,     "CONFIG %02X %04X\r\n", index, tempVal);
            zul_CursorUp(1);
        }
        else
        {
            fprintf(stdout, "CONFIG %02X ----\n", index);
        }
    }
    fprintf(stdout,"\n");
}


void             saveConfigMT()
{

    uint16_t numStatus, numConfig, tempVal;
    uint8_t index, privateBase;
    uint8_t numSV8, numCI8, spiDevIndex;

    // printf("%s\n", __FUNCTION__);

    // there are "public" and "private" values for status and config

    // public status values
    if (SUCCESS == zul_getStatusByID(ZXYMT_SI_NUM_STATUS_VALUES, &numStatus))
    {
        numSV8 = (uint8_t)numStatus;
        for (index = 0; index<numSV8; index++)
        {
            if (SUCCESS == zul_getStatusByID(index, &tempVal))
            {
                fprintf(stdout, "STATUS %02X %04X\n", index, tempVal);
                fprintf(fp,     "STATUS %02X %04X\r\n", index, tempVal);
                zul_CursorUp(1);
            }
            else
            {
                fprintf(stdout, "STATUS %02X ----\n", index);
            }
        }
    }

    // now the private status values ...
    if (SUCCESS == zul_getStatusByID(ZXYMT_SI_NUM_PRIVATE_STATUS_VALUES, &numStatus))
    {
        privateBase = (uint8_t)(256 - numStatus);
        index = privateBase;
        do {
            if (SUCCESS == zul_getStatusByID(index, &tempVal))
            {
                fprintf(stdout, "STATUS %02X %04X\n", index, tempVal);
                fprintf(fp,     "STATUS %02X %04X\r\n", index, tempVal);
                zul_CursorUp(1);
            }
            else
            {
                fprintf(stdout, "STATUS %02X ----\n", index);
            }
            index++;
        } while (index != 0);
    }
    fprintf(stdout,"\n");

    // next - any ARVALs from ZXY500s
    for (spiDevIndex=0; spiDevIndex<g_numSpiDevs; spiDevIndex++)
    {
        uint8_t reg;
        for (reg = 0; reg < 6; reg++)
        {
            uint16_t value;
            if ( SUCCESS == zul_getSpiRegister( spiDevIndex, reg, &value ))
            {
                uint16_t address = (spiDevIndex << 4) + (reg);
                fprintf(stdout, "#ARVAL %02X %04X\n", address, value );
                fprintf(fp,     "#ARVAL %02X %04X\r\n", address, value );
                zul_CursorUp(1);
            }
            else
            {
                printf("   reg error %d %d = []\n\n", spiDevIndex, reg);
            }
        }
    }
    fprintf(stdout,"\n");

    // public config values
    if (SUCCESS == zul_getStatusByID(ZXYMT_SI_NUM_CONFIG_PARAMS, &numConfig))
    {
        numCI8 = (uint8_t)numConfig;
        for (index = 0; index<numCI8; index++)
        {
            if (SUCCESS == zul_getConfigParamByID(index, &tempVal))
            {
                fprintf(stdout, "CONFIG %02X %04X\n", index, tempVal);
                fprintf(fp,     "CONFIG %02X %04X\r\n", index, tempVal);
                zul_CursorUp(1);
            }
            else
            {
                fprintf(stdout, "CONFIG %02X ----\n", index);
            }
        }
    }

    // now the private config values ...
    if (SUCCESS == zul_getStatusByID(ZXYMT_SI_NUM_PRIVATE_CONFIG_PARAMS, &numConfig))
    {
        privateBase = (uint8_t)(256 - numConfig);
        index = privateBase;
        do {
            if (SUCCESS == zul_getConfigParamByID(index, &tempVal))
            {
                fprintf(stdout, "CONFIG %02X %04X\n", index, tempVal);
                fprintf(fp,     "CONFIG %02X %04X\r\n", index, tempVal);
                zul_CursorUp(1);
            }
            else
            {
                fprintf(stdout, "CONFIG %02X ----\n", index);
            }
            index++;
        } while (index != 0);
    }
    fprintf(stdout,"\n");
}


/**
 */
void saveConfig()
{
    char hwType[8];
    char versionData[100+1];
    bool save100 = false;
    bool saveMT  = false;

    // printf("%s ", __FUNCTION__);

    time_t now = time(0);
    struct tm hms;
    char *daysOfWeek[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    char zysName[200];

    zul_Hardware(hwType, 8);
    hwType[6]='\0';

    localtime_r(&now, &hms);

    if(strlen(g_zysFile) < 4)
    {
        snprintf(zysName, 200, "%s__%04d_%02d_%02d-%02d_%02d_%02d.zys",
                 hwType,
                 hms.tm_year + 1900, hms.tm_mon + 1, hms.tm_mday,
                 hms.tm_hour, hms.tm_min, hms.tm_sec );
    }
    else
    {
        strncpy(zysName, g_zysFile, 200);
    }

    printf("to file: %s\n", zysName);

    char * day = daysOfWeek[hms.tm_wday % 7];

    fp = fopen(zysName, "w");
    if (fp != NULL)
    {
        char verStr[10+1];
        char hName[50] = "---";

        zul_getVersion(verStr, 10);
        verStr[9] = '\0';

        fprintf( fp, "# This information collected by %s (App: %s)\r\n", g_filename, verStr);

        fprintf( fp, "# Date %d/%d/%d (%s)\r\n",
                    hms.tm_mday, hms.tm_mon + 1, hms.tm_year + 1900, day);
        fprintf( fp, "# Time %02d:%02d:%02d\r\n",
                    hms.tm_hour, hms.tm_min, hms.tm_sec);
        fprintf( fp, "# System Information\r\n");

        fprintf( fp, "#\tOS Name and Version: ");
        fprintf( fp, "%s",  getOSinfo() );

        gethostname(hName, 50);
        hName[49] = '\0';
        fprintf( fp, "#\tMachine Name:        %s\r\n",  hName);

        fprintf( fp, "#\tMAC Addresses:       %s\r\n", getMACs() );

        fprintf( fp, "#\tSystem UpTime:       %s\r\n", getUpTime() );


        zul_setCommsEndurance(COM_ENDUR_MEDIUM);
        VerIndex verData;
        for (verData = STR_BL; verData<=STR_AFC; verData++)
        {
            if (SUCCESS == zul_getVersionStr(verData, versionData, 100))
            {
                fprintf(stdout, "VERSION %02d %s\n", verData, versionData);
                fprintf(fp,     "VERSION %02d %s\r\n", verData, versionData);

                // ZXY100s have the ZXYx00 string at index 2

                if (verData == STR_HW)
                {
                    if (strstr(versionData, "ZXY100") != NULL)
                    {
                        save100 = true;
                    }
                    if (strstr(versionData, "ZXY110") != NULL)
                    {
                        save100 = true;
                    }

                    if (strstr(versionData, "ZXY150") != NULL)
                    {
                        saveMT = true;
                    }
                    if (strstr(versionData, "ZXY200") != NULL)
                    {
                        saveMT = true;
                    }
                    if (strstr(versionData, "ZXY300") != NULL)
                    {
                        saveMT = true;
                    }
                    if (strstr(versionData, "ZXY500") != NULL)
                    {
                        saveMT = true;
                        g_numSpiDevs = 1;
                        if (strstr(versionData, "-128-") != NULL)
                        {
                            g_numSpiDevs = 2;
                        }
                        if (strstr(versionData, "-256-") != NULL)
                        {
                            g_numSpiDevs = 4;
                        }
                    }
                }
            }
        }

        if (save100)
        {
          saveConfig100();
        }
        if (saveMT)
        {
          saveConfigMT();
        }

        fclose(fp);
    }
}

// ----------------------------------------------------------------------------

void cleanup(void)
{
    printf("CleanUp .. \n");
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

void handleCommandLineOptions(int argCount, char **argStrings)
{
    char c;
    opterr = 0;
    bool zysFileFound = false;

    while ((c = getopt (argCount, argStrings, "hd:f:")) != -1)
    {
        switch (c)
        {
            case 'h':
                fprintf(stderr, "This console program can be used to save a set of configuration parameter settings to a \nZytronic Touchscreen controller.\n");
                fprintf(stderr, "The following options are accepted:\n");
                fprintf(stderr, "-d\ta device index\n");
                fprintf(stderr, "-l\tlist the connected devices\n");
                fprintf(stderr, "-f\tspecify the name of the ZYS file the settings will be saved to (*.ZYS)\n");
                fprintf(stderr, "Usage : %s <options>\n", argStrings[0] );

                exit(0);

            case 'd':
                g_deviceIndex = abs(atoi(optarg));
                break;

            case 'f':
                if (strlen(optarg) > 4)
                {
                    strncpy(g_zysFile, optarg, 200);
                    g_zysFile[200] = '\0';
                    zysFileFound = true;
                }
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
    if ((!zysFileFound) && (argCount>1) && (strstr(argStrings[1], ".zys")))
    {
        strncpy(g_zysFile, argStrings[1], 200);
        g_zysFile[200] = '\0';
    }
}

// ----------------------------------------------------------------------------

int main(int numArgs, char ** argv)
{
    int i;
    int numDevs;
    char tempBuffer[TEMP_BUF_LEN +1];
    char verStr[200 + 1];

    strncpy(g_filename, argv[0], 100);
    g_filename[100] = '\0';

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

    // if we are not root, return
    if (0 != system("id -u | grep ^0$ > /dev/null"))
    {
        fprintf(stderr, "This application must be run as root\n");
        zul_EndServices();
        exit (EXIT_FAILURE);
    }

    handleCommandLineOptions(numArgs, argv);

    numDevs = zul_getDeviceList(tempBuffer, TEMP_BUF_LEN);

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

     if (g_deviceIndex >= 0)
     {
        printf( "Open device #%d ... ", g_deviceIndex );
        int retVal = zul_openDevice(g_deviceIndex);
        if (retVal != 0)
        {
            printf( "Error [%d] opening device index %d.\n", retVal, g_deviceIndex );
        }
        else
        {
            printf( "OPENED\n" );

            zul_ResetDefaultInHandlers();
            saveConfig();

            retVal = zul_closeDevice();
            if (retVal != 0)
            {
                printf( "   Error [%d] closing device.\n", retVal );
            }
        }
    }

    //     zul_EndServices();   // see atexit(cleanup) !
    return 0;
}
