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

#include "zytypes.h"
#include "sysdata.h"

// ----------------------------------------------------------------------------

const char * getUpTime(void)
{
    static char     UpTimeResult[30];
    char            buffer[20];
    long int        upSeconds = 0;

    long int days, hours, minutes, seconds;

    FILE *pFile = fopen( "/proc/uptime","r");

    if ( pFile != NULL )
    {
        if ( NULL != fgets(buffer, 20, pFile) )
        {
           (void)sscanf(buffer, "%ld", &upSeconds );
        }
        (void)fclose(pFile);

        days = upSeconds / (24*60*60);
        upSeconds %= (24*60*60);
        hours = upSeconds / (60*60);
        upSeconds %= (60*60);
        minutes= upSeconds / (60);
        seconds = upSeconds % (60);;
        (void)snprintf( UpTimeResult, 30, "%03ld days, %02ld:%02ld:%02ld",
                                        days, hours, minutes, seconds);
    }
    return UpTimeResult;
}

// ----------------------------------------------------------------------------

const char * getMACs(void)
{
    static char result[(100+2)*8];
    char        fileBuf[100];
    FILE    *   pFile;

    const char tempFile[] = "/tmp/macAddrs.txt";
    // tr used to replace new line with tab in grep output
    // can now be printed all on one line
    int retVal = system("ifconfig -a | grep -E \"HWaddr|ether\" > /tmp/macAddrs.txt");

    if (retVal != 0) // problem with command
    {
        retVal = system("ip addr | grep ether > /tmp/macAddrs.txt");
    }

    // empty result buffer
    result[0] = '\0';

    pFile = fopen( tempFile, "r");
    if (pFile != NULL)
    {
        while ( NULL != fgets( fileBuf, 100, pFile ))
        {
            char *p1, *p2;
            fileBuf[99] = '\0';
            p1 = strstr(fileBuf, "Link");
            p2 = strstr(fileBuf, "HWaddr");

            if ((p1 != NULL) && (p2 != NULL))   // neither NULL
            {
                memmove(p1, p2+7, 3*8 + 1);
            }

            // find the first non-whitespace char
            int i=0,j=0;
            while (fileBuf[i]!='\0')
            {
                if (fileBuf[i]!=' ' && fileBuf[i]!='\t')
                {
                    break;
                }
                i++;
            }
            // shift everything to remove leading whitespace
            while (fileBuf[i]!='\0')
            {
                fileBuf[j]=fileBuf[i];
                i++;
                j++;
            }
            // replace '\n'
            fileBuf[j-1]='\0';

            strcat( result, fileBuf );
            // add a separator before adding the next MAC
            strcat( result, " | ");
        }
        // remove the last separator
        result[strlen(result)-3] = '\0';

        (void)fclose(pFile);
    }

    // clean up
    (void)unlink(tempFile);

    return result;
}

// ----------------------------------------------------------------------------

const char * getOSinfo(void)
{
    static char result[200];
    char        fileBuf[110];
    const char  tempFile[] = "/tmp/os.txt";
    FILE    *   pFile;

    (void)system("uname -srvmpi > /tmp/os.txt");

    // empty result buffer
    result[0] = '\0';

    pFile = fopen( tempFile, "r");
    if (pFile != NULL)
    {
        if ( NULL != fgets( fileBuf, 110, pFile) )
        {
            char *p = fileBuf+10;
            p = strchr(p, ' ') + 1;
            if (p != NULL)
            {
                *p = '\0';
                strcat( result, fileBuf );
                p = fileBuf+30;
                p = strrchr(p, ' ') + 1;
                if ( p != NULL)
                {
                    strcat( result, p );
                }
            }
        }
        (void)fclose(pFile);
    }

    // clean up
    (void)unlink(tempFile);

    return result;
}



// #######   /dev/input/eventXX for Touch controller CpuID   ##################
struct zyEventBinding_t {
    int     eventIndex;
    char    uniqueID[25];
};
typedef struct zyEventBinding_t ZyEventBinding;
static ZyEventBinding ControllerBindings[8];

void cacheTouchEventPaths     (void)
{
    char        fileBuf[200];
    FILE    *   pFile;
    int         index;

    const char tempFile[] = "/tmp/inputs.txt";
    (void)system("ls -l /dev/input/by-id/*Zytronic*if00 > /tmp/inputs.txt 2>/dev/null");

    // clear any previous data
    for (index=0; index<8; index++)
    {
        ControllerBindings[index].uniqueID[0] = '\0';
    }

    pFile = fopen( tempFile, "r");
    index = 0;
    if (pFile != NULL)
    {
        while ( (NULL != fgets( fileBuf, 200, pFile )) && (index < 8) )
        {
            char *p1, *p2, *p3;

            fileBuf[199] = '\0';
            p1 = strstr(fileBuf, "Controller_");
            p2 = strstr(fileBuf, " -> ");

            if ((p1 != NULL) && (p2 != NULL))   // neither NULL
            {
                p1 += 11;
                p3 = strchr(p1, '-');
                if (p3 != NULL) *p3 = '\0';     // terminate the CPU ID number

                p2 += 12;
                p3 = strchr(p2, '\n');
                if (p3 != NULL) *p3 = '\0';     // terminate the event path

                //printf("  ## Device Path '%s'(%d) for CpuID %s\n",
                //                                 p2, atoi(p2), p1 );
                ControllerBindings[index].eventIndex = atoi(p2);
                strncpy(ControllerBindings[index].uniqueID, p1, 24 );
                ControllerBindings[index].uniqueID[24] = '\0';

                //printf("DEV %d %s\n",   ControllerBindings[index].eventIndex,
                //                        ControllerBindings[index].uniqueID );

                index++;
            }
        }
        (void)fclose(pFile);
    }

    // clean up
    (void)unlink(tempFile);
}


int  getTouchEventPathIndex  (char const * uniqueID)
{
    int index;
    if (strlen(uniqueID) == 0) return -1;
    for (index=0; index<8; index++)
    {
        if (!strcmp(ControllerBindings[index].uniqueID, uniqueID))
        {
            return ControllerBindings[index].eventIndex;
        }
    }
    return -1;
}

// #######   X.Org Information ################################################

const char * getXDisplayInfo(void)
{
    static char report[1000];
    char        lineBuffer[100];
    int     nS   = getNumberOfScreens();
    Size2d  sz   = getScreenSize(0);
    int     nMon = getNumberOfMonitors(0);
    int     index;

    if (nS == 0) return "# no xrandr report";

    snprintf(report, 99,
                    "#\t%d screen(s). First screen has %d monitors "
                    "and %dx%d pixels. Primary %s\n", nS, nMon, sz.x, sz.y,
                    getPrimaryMonitor(0) );
    report[99] = '\0';     // safe termination

    for (index = 0; index < nMon; index++)
    {
        Size2d   size = getMonitorSize    (0, index);
        Location loc  = getMonitorLocation(0, index);
        snprintf( lineBuffer, 99, "#\t    Monitor %s Orientation %d Size %dx%d @ screen location %dx%d \n",
            getMonitorName(0,index), getMonitorOrientation(0,index),
                                        size.x, size.y, loc.x, loc.y  );
        lineBuffer[99] = '\0';
        strcat( report, lineBuffer );

    }
    return report;
}


/** providing storage for only four XOrg screens
 */

static uint8_t          g_numScreens = 0;
static XOrgScreenData   g_x11ScrnData[X_MAX_SCRN];

/**
 * Read the xrandr output and populate the internal data structs with the
 * parsed information.
 * Subsequent calls to query the parsed data do not re-read the system info,
 * until this function is executed again.
 * A "screen" is a collection of monitors associated with a human.
 * If zero screens are returned, the accessors should not be trusted
 */
#define RdLineLen   (200)
#define TmpFileName "/tmp/randr.txt"
int getNumberOfScreens(void)
{
    char                fileBuf[RdLineLen];
    char                mmStr[20] = "";
    const char          tempFile[] = TmpFileName;
    FILE            *   pFile;
    XOrgScreenData  *   pScrnData = NULL;
    XOrgMonitorData *   pMonitorData = NULL;
    int                 monitorIndex = 0;

    int                 w,h,x,y;
    char                op1, op2, op3;

    g_numScreens = 0;

    (void)system("xrandr > " TmpFileName);
    pFile = fopen( tempFile, "r");
    if (pFile != NULL)
    {
        while ( NULL != fgets( fileBuf, RdLineLen, pFile ) )
        {
            char *pToken, *p2;
            if (fileBuf[0] == ' ')
            {
                continue;    // skip mode lines
            }

            if (!strncmp( fileBuf, "Screen", 6 ))
            {
                g_numScreens++;

                if (g_numScreens == X_MAX_SCRN) return g_numScreens;

                pScrnData = & ( g_x11ScrnData[g_numScreens-1] );
                pScrnData->numMonitors = 0;
                monitorIndex = 0;

                pToken = strstr(fileBuf, " current ");
                if (pToken != NULL)
                {
                    int numCvt = sscanf(pToken + 8, "%d %c %d", &x, &op1, &y);
                    if (numCvt == 3)
                    {
                        pScrnData->size.x = x;
                        pScrnData->size.y = y;
                    }
                }

                continue;   // no further info needed from this line
            }

            fileBuf[RdLineLen-1] = '\0';

            pToken = strstr(fileBuf, "+");
            if (pToken == NULL) { continue; }  // ignore all connected displays which are OFF
            pToken = strstr(fileBuf, " connected");
            if (pToken == NULL) { continue; }  // ignore all disconnected displays

            // current line described a connected monitor
            pMonitorData = & ( pScrnData->monitor[monitorIndex] );
            pMonitorData->primary  = false;
            pMonitorData->orientation= xmo_Normal;

            pScrnData->numMonitors++;

            // take the location and sze data
            pToken += 10;   // length of " connected"

            p2 = strstr(pToken, "primary ");
            if (p2 != NULL)
            {
                pMonitorData->primary  = true;
                pToken += 8;
            }

            if (7 == sscanf (pToken, "%d%c%d%c%d%c%d", &w, &op1, &h,
                                              &op2,    &x, &op3, &y  ) )
            {
                // printf("-- %d%c%d%c%d%c%d", w, op1, h,   op2,   x, op3, y  ) ;

                //set size and location
                pMonitorData->size.x = w;
                pMonitorData->size.y = h;

                pMonitorData->location.x = x;
                pMonitorData->location.y = y;
            }

            // retain mm dimensions if available - not used as yet
            pToken = strstr(fileBuf, ") ");
            if (pToken != NULL)
            {
                strncpy( mmStr, pToken+2, 19 );
                mmStr[19] = '\0';
            }

            // remove text in braces (..)
            pToken = strstr(fileBuf, " (");
            if (pToken != NULL)
            {
                *pToken = '\0';
            }

            // update the assumed monitor orientation
            pToken = strstr(fileBuf, "left");
            if (pToken != NULL)
            {
                pMonitorData->orientation= xmo_Left;
            }
            pToken = strstr(fileBuf, "right");
            if (pToken != NULL)
            {
                pMonitorData->orientation= xmo_Right;
            }
            pToken = strstr(fileBuf, "inverted");
            if (pToken != NULL)
            {
                pMonitorData->orientation= xmo_Inverted;
            }

            // take the monitor name
            pToken = strchr(fileBuf, ' ');
            if (pToken != NULL)
            {
                // terminate the name
                *pToken = '\0';
                // store name
                strcpy( pMonitorData->name, fileBuf );
            }

            monitorIndex++;
            if (monitorIndex == X_MAX_MONS) return g_numScreens;
        }
        (void)fclose(pFile);
    }

    // clean up
    (void)unlink(tempFile);

    return g_numScreens;
}

/**
 * ignore disconnected monitors !
 */
int getNumberOfMonitors(int screen)
{
    if (screen < g_numScreens)
    {
        return  g_x11ScrnData[screen].numMonitors;
    }
    return -1;      // unknown
}



Size2d getScreenSize(int screen)
{
    static Size2d unknown = {-1,-1};
    if (screen < g_numScreens)
    {
        return  g_x11ScrnData[screen].size;
    }
    return unknown ;
}

const char * getPrimaryMonitor(int screen)
{
    if (screen < g_numScreens)
    {
        int index;
        int nMon = g_x11ScrnData[screen].numMonitors;
        for (index = 0; index < nMon; index++)
        {
            if (g_x11ScrnData[screen].monitor[index].primary)
            {
                return g_x11ScrnData[screen].monitor[index].name;
            }
        }
    }
    return "unKnown";
}

XMonitorOrientation getMonitorOrientation(int screen, int monitor)
{
    if (screen < g_numScreens)
    {
        if (monitor < g_x11ScrnData[screen].numMonitors)
        {
            return  g_x11ScrnData[screen].monitor[monitor].orientation;
        }
    }

    return -1;      // unknown
}

Size2d getMonitorSize(int screen, int monitor)
{
    static Size2d unknown = {-1,-1};
    if (screen < g_numScreens)
    {
        if (monitor < g_x11ScrnData[screen].numMonitors)
        {
            return  g_x11ScrnData[screen].monitor[monitor].size;
        }
    }
    return unknown ;
}

Location getMonitorLocation(int screen, int monitor)
{
    static Location unknown = {-1,-1};
    if (screen < g_numScreens)
    {
        if (monitor < g_x11ScrnData[screen].numMonitors)
        {
            return  g_x11ScrnData[screen].monitor[monitor].location;
        }
    }
    return unknown ;
}

const char * getMonitorName(int screen, int monitor)
{
    if (screen < g_numScreens)
    {
        if (monitor < g_x11ScrnData[screen].numMonitors)
        {
            return  g_x11ScrnData[screen].monitor[monitor].name;
        }
    }
    return "unknown";
}
