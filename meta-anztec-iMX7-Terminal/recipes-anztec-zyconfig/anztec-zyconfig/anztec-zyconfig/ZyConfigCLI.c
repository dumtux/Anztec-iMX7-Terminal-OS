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


/* This code is provided as a test harness, used during development.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>

#include "usb.h"
#include "protocol.h"
#include "services.h"
#include "debug.h"

#define TEMP_BUF_LEN (1000)

// INVALID_CMD marks the end of the list !
const char cmdStr[][20] = {  "get",   "set",   "status",   "reset",   "restore",   "equalize",   "save",   "load",   "firmware_update",   "list",   "INVALID"  };
enum Cmd                  { Get_cmd, Set_cmd, Status_cmd, Reset_cmd, Restore_cmd, Equalize_cmd, Save_cmd, Load_cmd, Firmware_update_cmd, List_cmd, INVALID_cmd };
const int  argsNeeded[] = {    1,       2,        1,          0,          0,           0,          0,        1,              1,            0,          0      };

typedef enum Cmd Cmd_t;

bool    g_verbose = false;
Cmd_t   g_runTest = INVALID_cmd;
int     g_testIndex = -1;
char    filename[200];
int     g_testValue = -1;
int     g_deviceIndex = -1;

char    g_testCmd[10] = "";

int16_t g_PID;


// ------------------------------------------------------------------
// ------------------------------------------------------------------
void runTestID(int testID)
{
    uint16_t y = 0;

    if (g_verbose)
    {
        printf("Device PID = %d 0x%04x\n", g_PID, g_PID);
    }

    switch (testID)
    {
        case Get_cmd:
            zul_getConfigParamByID(g_testIndex, &y);
            printf ("ConfigParam %d'd (0x%02x) = %d'd (0x%04x)\n", g_testIndex, g_testIndex, y, y);
            break;

        case Set_cmd:
            zul_setConfigParamByID(g_testIndex, g_testValue);
            zy_msleep(20);
            zul_getConfigParamByID(g_testIndex, &y);
            printf ("ConfigParam %d'd (0x%02x) = %d'd (0x%04x)\n", g_testIndex, g_testIndex, y, y);
            break;

        case Status_cmd:
            zul_getStatusByID (g_testIndex, &y);
            printf ("StatusValue %d'd (0x%02x) = %d'd (0x%04x)\n", g_testIndex, g_testIndex, y, y);
            break;

        case Reset_cmd:
            zul_resetController();
            break;

        case Restore_cmd:
            zul_restoreDefaults();
            break;

        case Equalize_cmd:
            zul_forceEqualisation();
            break;

        default:
            printf("./Unrecognised command: %d\n", testID);
            break;
    }
}


// === HELP INFO ==========================================
void help(const char * const name)
{
    printf ("Usage:  %s -h\n", name);
    printf ("        sudo  %s <command>\n", name);
    printf ("        sudo  %s -d <devIndex> <command>\n\n", name);

    printf ("  -h             display this help text\n\n");

    printf ("  -d<index>      connect to controller specified by 'index' and run command\n");
    printf ("  list           show the indexed list of Zytronic controllers connected\n\n");

    printf ("Available Commands:\n\n");

    printf ("  equalize                           force sensor equalization\n");
    printf ("  reset                              force a controller reset\n");
    printf ("  restore                            return controller to factory settings\n\n");

    printf ("  firmware_update <filename.zyf>     firmware update using the supplied ZYF file\n\n");

    printf ("  get <index>                        get a configuration parameter\n");
    printf ("  set <index> <value>                set a configuration parameter\n");
    printf ("  status <index>                     get a status value \n\n");

    printf ("  load <filename.zys>                load the supplied ZYS file\n");
    printf ("  save [filename.zys]                save a ZYS file (filename optional)\n\n");
}



// ========================================================

bool validateCommandLineOptions(int argCount, char **argStrings)
{
    char c;
    opterr = 0;
    bool CommandOK = false;
    int  argsRequired = 0;
    bool argsOK = false;
    int  nonOptArg, index;

    while ((c = getopt (argCount, argStrings, "d:hv")) != -1)
    {
        switch (c)
        {
            case 'h':
                help(argStrings[0]);
                exit (0);
                break;

            case 'd':
                g_deviceIndex = abs(atoi(optarg));
                break;

            case 'v':
                g_verbose = true;
                break;

            case '?':
                if (optopt == 'd')
                    fprintf (stderr, "Option -%c requires an numeric device index.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                        "Unknown option character `\\x%x'.\n",optopt);
                exit(1);

            default:
                abort();
        }
    }

    for (nonOptArg = 0, index = optind; index < argCount; index++, nonOptArg++)
    {
        if (nonOptArg == 0)
        {
            strcpy (g_testCmd, argStrings[index] );
        }

        if (nonOptArg == 1)
        {
            g_testIndex = abs(atoi(argStrings[index]));
            if(strlen(argStrings[index]) > 4)
            {
                // filenames longer than 195 characters will be truncated
                strncpy(filename, argStrings[index], 200);
            }
            filename[199] = '\0';

        }

        if (nonOptArg == 2)
        {
            g_testValue = abs(atoi(argStrings[index]));
        }
    }

    if (strlen(g_testCmd)==0)
    {
        // display the help text if no commands are provided
        help(argStrings[0]);
        exit (0);
    }

    // validate command string
    for (index = 0; index != INVALID_cmd; index++)
    {
        // printf ("%d - %s: ", index, cmdStr[index]);
        if (!strcmp(g_testCmd, cmdStr[index]))
        {
            g_runTest = index;
            CommandOK = true;
            argsRequired = argsNeeded[index];
        }
    }

    if ( (argsRequired == 0) )
    {
        argsOK = true;
    }
    if ( (argsRequired == 1) && (g_testIndex != -1) )
    {
        argsOK = true;
    }
    if ( (argsRequired == 2) && (g_testIndex != -1) && (g_testValue != -1) )
    {
        argsOK = true;
    }

    return CommandOK && argsOK;
}

// === MAIN ROUTINE =======================================

int main(int numArgs, char ** argv)
{
    int i;
    char tempBuffer[TEMP_BUF_LEN +1];
    char verStr[200 + 1];
    int deviceCount;

    i = zul_InitServices(); // open the comms library
    if (i!=0)
    {
        printf("zylibUSB open fail %d\n", i);
    }
    else
    {
        if (g_verbose)
        {
            zul_getVersion(verStr,200);
            verStr[199] = '\0';
            printf("%s; USD:%s\n", zul_usbLibStr(), verStr);

        }
    }

    if (validateCommandLineOptions(numArgs, argv))
    {
        if (g_verbose)
        {
            printf ("Valid command: %s", g_testCmd);
            if (g_testIndex >=0 ) printf (" %d", g_testIndex);
            if (g_testValue >=0 ) printf (" %d", g_testValue);
            puts(""); // newline
        }
    }
    else
    {
        printf ("Invalid command: %s", g_testCmd);
        if (g_testIndex >=0 ) printf (" %d", g_testIndex);
        if (g_testValue >=0 ) printf (" %d", g_testValue);
        puts(""); // newline
        exit(-1);
    }

    deviceCount = zul_getDeviceList(tempBuffer, TEMP_BUF_LEN);

    // if no device index has been selected, autoselect if only one available
    if ( g_deviceIndex < 0 )
    {
        if (deviceCount > 1)
        {
            printf("Found Zytronic touchscreen devices:\n%s", tempBuffer);
            printf("Use the -dN option to select a device from above list\n");
            exit (0);
        }
        if (deviceCount == 0 && g_runTest != List_cmd)
        {
            printf("There are no Zytronic devices connected\n");
        }
        if (deviceCount == 1)
        {
            g_deviceIndex = atoi(tempBuffer);
            printf("index %d\n", g_deviceIndex);
        }
        if (deviceCount < 0)
        {
            printf("ERROR %d\n", deviceCount);
                    exit (0);
        }
    }

    if (g_runTest == List_cmd)
    {
        if (deviceCount > 1)
        {
            printf("Found more than one Zytronic touchscreen devices:\n%s", tempBuffer);
            exit (0);
        }
        if (deviceCount == 0)
        {
            printf("There are no Zytronic devices connected\n");
        }
        if (deviceCount == 1)
        {
            g_deviceIndex = atoi(tempBuffer);
            printf("Found a single Zytronic touchscreen device, at index %d\n%s", g_deviceIndex, tempBuffer);

        }
        if (deviceCount < 0)
        {
            printf("ERROR %d\n", deviceCount);
                    exit (0);
        }

        exit(0);
    }

    // if we are not root, carp and exit
    if (0 != system("id -u | grep ^0$ > /dev/null"))
    {
        fprintf(stderr, "This application must be run as root\n");
        zul_EndServices();
        exit (EXIT_FAILURE);
    }

    if (g_deviceIndex >= 0)  /// >0 ToDo
    {
        int retVal;

        // options that don't need the CLI app to connect
        if (g_runTest == Save_cmd)
        {
            char tmp[15];
            char devIndexStr[3];
            const char* cmd;

            strcpy(tmp, "./saveZys -d");
            // pass arg through the CLI app if provided, else use the autoselected one
            sprintf(devIndexStr, "%d ", g_deviceIndex);
            strcat(tmp, devIndexStr);
            if(strlen(filename) > 4)
            {
                // add .zys to the filename if it's missing
                if(!strstr(filename, ".zys"))
                {
                    if(strlen(filename) < 200-4)
                    {
                        strcat(filename, ".zys");
                    }
                    else
                    {
                        char *p = &filename[199-4];
                        strcpy(p, ".zys");
                    }
                    filename[199] = '\0';
                }

                strcat(tmp, "-f");       // add the switch to identify the filename
                strcat(tmp, filename);  // add the name of the file to be used
            }
            cmd = tmp;

            if (g_verbose)
            {
                printf("running %s\n", cmd);
            }

            system(cmd);
        }
        else if (g_runTest == Load_cmd)
        {
            char tmp[150];
            char devIndexStr[3];
            const char* cmd;

            strcpy(tmp, "./loadZys -d");
            // pass arg through the CLI app if provided, else use the autoselected one
            sprintf(devIndexStr, "%d ", g_deviceIndex);
            strcat(tmp, devIndexStr);

            if(strlen(filename) > 4)
            {
                // add .zys to the filename if it's missing
                if(!strstr(filename, ".zys"))
                {
                    if(strlen(filename) < 200-4)
                    {
                        strcat(filename, ".zys");
                    }
                    else
                    {
                        char *p = &filename[199-4];
                        strcpy(p, ".zys");
                    }
                    filename[199] = '\0';
                }

                strcat(tmp, "-f");       // add the switch to identify the filename
                strcat(tmp, filename);  // add the name of the file to be used
            }
            cmd = tmp;

            if (g_verbose)
            {
                printf("running %s\n", cmd);
            }

            system(cmd);

        }
        else if (g_runTest == Firmware_update_cmd)
        {
            char tmp[150];
            char devIndexStr[3];
            const char* cmd;

            strcpy(tmp, "./firmwareUpdate -d");
            // pass arg through the CLI app if provided, else use the autoselected one
            sprintf(devIndexStr, "%d ", g_deviceIndex);
            strcat(tmp, devIndexStr);
            if(strlen(filename) > 4)
            {
                // add .zyf to the filename if it's missing
                if(!strstr(filename, ".zyf"))
                {
                    if(strlen(filename) < 200-4)
                    {
                        strcat(filename, ".zyf");
                    }
                    else
                    {
                        char *p = &filename[199-4];
                        strcpy(p, ".zyf");
                    }
                    filename[199] = '\0';
                }
                strcat(tmp, "-f");       // add the switch to identify the filename
                strcat(tmp, filename);  // add the name of the file to be used
            }
            cmd = tmp;

            if (g_verbose)
            {
                printf("running %s\n", cmd);
            }

            system(cmd);
        }
        else
        {
            if (g_verbose)
            {
                printf( "Open device #%d ... ", g_deviceIndex );
            }

            // pre-requirement to opening ...
            // zul_getDeviceList(tempBuffer, TEMP_BUF_LEN);

            retVal = zul_openDevice(g_deviceIndex);
            if (retVal != 0)
            {
                printf( "Error [%d] opening device index %d.\n", retVal, g_deviceIndex );
            }
            else
            {
                if (g_verbose)
                {
                    printf( "OPENED\n" );
                }
                zul_getDevicePID(&g_PID);
                zul_ResetDefaultInHandlers();

                runTestID(g_runTest);

                retVal = zul_closeDevice();
                if (retVal != 0)
                {
                    printf( "   Error [%d] closing device.\n", retVal );
                }
            }
        }
    }

    zul_EndServices();
    return 0;
}
