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
 * It's purpose is to alter the configuration of a touchscreen controller to match
 * the settings taken from a ZYS text file.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>

#include "zytypes.h"
#include "debug.h"
#include "usb.h"
#include "protocol.h"
#include "services.h"

#define TEMP_BUF_LEN (1000)

int     g_testIndex;
int     g_testValue;
int     g_deviceIndex = -1;
char    g_zysFile[200+1] = "";


FILE *fp = NULL;


// ----------------------------------------------------------------------------

/**
 */
void loadConfig()
{
    char setCommand[256][10+1];
    char crcIn[10000] = "";
    bool loadData = true;
    int cmdIndex = 0;

    // fprintf(stdout,"Loading file: %s\n", g_zysFile);

    // ToDo: check file is for the connected device

    errno = 0;
    fp = fopen(g_zysFile, "r");
    if (fp != NULL)
    {
        char lineBuffer[180+1];
        char crcFromFile[5] = "", calculatedCRC[5] = "";

        while (fgets(lineBuffer, 180, fp))
        {
            if (!strncmp(lineBuffer, "# Validation", 10))
            {
                char *p = strrchr(lineBuffer, ' ');
                if (p!=NULL) strncpy(crcFromFile, p+1, 4 );
                crcFromFile[4] = '\0';
                fprintf(stdout, "CRC found in file : %s\t\t:%s\n", crcFromFile, lineBuffer);
            }

            if ( strstr(lineBuffer, "VERSION") ||
                 strstr(lineBuffer,  "STATUS") ||
                 strstr(lineBuffer,   "ARVAL") ||
                 strstr(lineBuffer,  "CONFIG") )
            {
                lineBuffer[180] = '\0';
                int len = strlen ( lineBuffer );
                lineBuffer[len-1] = '\0';                   // crop "\n"
                char c = lineBuffer[len-2];
                if ( strchr ( "\r\n", c ) )
                {
                    lineBuffer[len-2] = '\0';               // crop "\r"
                }
                strcat(crcIn,lineBuffer);
                if (NULL != strstr(lineBuffer, "CONFIG"))
                {
                    strncpy(setCommand[cmdIndex], lineBuffer+7, 10);
                    setCommand[cmdIndex][10] = '\0';
                    cmdIndex++;
                }
            }
        }

        int crc16 = zul_getCRC((unsigned char *)crcIn, strlen(crcIn));

        fprintf(stdout, "Found %d commands\n", cmdIndex);
        fprintf(stdout, "CRC is based on %zd bytes and is %04X\n", strlen(crcIn), crc16 );
        // fprintf(stdout, "CRC is based on the following string\n%s\n", crcIn );

        sprintf( calculatedCRC, "%04X", crc16 );

        fprintf(stdout, "\tCRC CHECK\t'%4s'\t'%4s'\n", crcFromFile, calculatedCRC );

        if ( crcFromFile[0] == '\0' )
        {
            fprintf(stdout,"Missing validation check in supplied file.\n");
            fprintf(stdout,"Expected to find:   '# Validation %04X'\n", crc16 );
            // if the crc is missing note it, and continue to load the file
            // loadData = false;
        }
        else
        {
            if ( strcmp(crcFromFile, calculatedCRC) )
            {

                fprintf(stdout,"Validation check failed.\n");
                fprintf(stdout,"Expected to find:   '# Validation %04X'\n", crc16 );
                loadData = false;
            }
        }


        fclose(fp);
    }
    else
    {
        fprintf(stdout,"Failed to open file: %s\n", g_zysFile);
        fprintf(stdout,"\t%s\n", strerror(errno));
        loadData = false;
    }

    if (loadData)
    {
        const int numCmds = cmdIndex;
        int x;

        // iterate through command list to program the target
        for (x = 0; x<numCmds; x++)
        {
            int percentDone = 100 * x / numCmds;
            int index, value;

            // 'CONFIG 01 0002'
            char *p = setCommand[x];    // 'CONFIG ' is not stored
            index = strtol (p, &p, 16);
            value = strtol (p, &p, 16);

            fprintf (stdout, "%3d%% Index:%03d Value:%05d (0x%04X)\n",
                        percentDone, index, value, value);
            zul_CursorUp(1);

            if (FAILURE == zul_setConfigParamByID( index, value))
                    fprintf (stdout, "xx\n");

            // zy_msleep(10);
        }
        fprintf (stdout, "100%%\n");

        // zy_msleep(200);
    }
    else
    {
        // do nothing - and exit
        zy_msleep(100);
    }
}

// ----------------------------------------------------------------------------

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
                fprintf(stderr, "This console program can be used to load a set of configuration parameter settings to a \nZytronic Touchscreen controller.\n");
                fprintf(stderr, "The following options are accepted:\n");
                fprintf(stderr, "-d\ta device index\n");
                fprintf(stderr, "-l\tlist the connected devices\n");
                fprintf(stderr, "-f\tspecify the name of the ZYS file the settings will be loaded from (*.ZYS)\n");
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

    //if (g_listOnly || (strlen(g_zysFile)==0))
    if (strlen(g_zysFile)==0)
    {
        exit(0);
    }

    // check file exists ...?

    printf("file to load: '%s'\n", g_zysFile);

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
            loadConfig();

            retVal = zul_closeDevice();
            if (retVal != 0)
            {
                printf( "   Error [%d] closing device.\n", retVal );
            }
        }
    }
    else
    {
        printf( "no device index set - exiting.\n" );
    }

    //     zul_EndServices();   // see atexit(cleanup) !
    return 0;
}
