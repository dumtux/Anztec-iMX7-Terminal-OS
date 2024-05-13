/*
 *   Copyright (c) 2019 Zytronic Displays Limited. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  Should you need to contact Zytronic, you can do so either via the
 *  website <www.zytronic.co.uk> or by paper mail:
 *  Zytronic, Whiteley Road, Blaydon on Tyne, Tyne & Wear, NE21 5NJ, UK
 */

/**
 *   Overview
 *   ========
 *   These services allow a log file to be accumulated in RAM, and periodically
 *   sync'd to disk. Probably uses for this are to keep a record of activity
 *   through a Basic Setup or Integration Test.
 *
 *   It may also be used to keep a general record of all ZyConfig exchanges
 *   with the device.
 *
 *   NB: Initial servers only support a single file.
 */


#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>

// allow thread synchronisation: pthread_mutex_(un)lock()
#include <pthread.h>

#include "dbg2console.h"
#include "debug.h"
#include "logfile.h"

// --- Module Global Values -------------------------------

#define BUFFER_SIZE     (10000)

#define BUFF_LEN        (WritePtr - BUFFER)
#define FREE_BUFF       (BUFFER_SIZE - BUFF_LEN)

// --- Implementation -------------------------------------


void ZyLogFile::mutex__error(int e, const char *name)
{
    const char * msg = "";
    switch (e)
    {
        case EINVAL:
            msg = "The value specified by mutex does not refer to an initialized mutex object";
            break;
        case EAGAIN:
            msg = "The mutex could not be acquired because the maximum number of recursive locks for mutex has been exceeded";
            break;
        case EDEADLK:
            msg = "The current thread already owns the mutex";
            break;
        case EPERM:
            msg = "The current thread does not own the mutex";
            break;

        default:
            msg = "unknown error";
    }
    printf("%s %s\n", name, msg);
}

void ZyLogFile::mutex__Lock(void)
{
    int retval = 0;
    //retval = pthread_mutex_lock( &mutex1 );
    if (retval != 0)
    {
        mutex__error(retval, __FUNCTION__ );
    }
}
void ZyLogFile::mutexUnLock(void)
{
    int retval = 0;
    //retval = pthread_mutex_unlock( &mutex1 );
    if (retval != 0)
    {
        mutex__error(retval, __FUNCTION__ );
    }
}

/**
 * Constructor
 */
ZyLogFile::ZyLogFile(const char *fn)
{
    mutex1 = PTHREAD_MUTEX_INITIALIZER;
    FilePath = new char[400];
    time_t now;

    if (fn == NULL)
    {
        strcpy( FilePath, "/tmp/zyconfig.log");
    }
    else
    {
        strncpy( FilePath, fn, 399 );
    }
    FilePath[399] = '\0';
    TimeStamp = false;

    mutex__Lock();

    BUFFER = new char[BUFFER_SIZE];
    BUFFER[0] = '\0';
    WritePtr = BUFFER;

    mutexUnLock();

    time(&now);
    Write2Log(ctime(&now));
}

/**
 * Destructor
 */
ZyLogFile::~ZyLogFile()
{
    if (WritePtr != BUFFER)
    {
        // append buffer to file
        Sync2Disk();
    }
    delete BUFFER;
    delete FilePath;
}


/**
 */
void ZyLogFile::EnableTimeStamp(const bool enable)
{
    TimeStamp = enable;
}

bool ZyLogFile::Write2Log (const char *message)
{
    char    tsBuf[14];
    char   *ptr;

    char   *pFullRecord;
    int     entryLen = strlen(message);
    int     tsLen = 0;
    bool    retVal;

    if (TimeStamp)
    {
        // format: "%05d.%03d", seconds-of-day, milliseconds
        zul_getStringTS ( tsBuf, 12 );      // width = 9
        strcat (tsBuf, " ");
        tsLen = strlen(tsBuf);
        entryLen += tsLen;
    }

    if (entryLen > 99) entryLen = 99;

    mutex__Lock();

    if (FREE_BUFF >= entryLen)
    {
        pFullRecord = WritePtr;
        if (TimeStamp)
        {
            strcpy(WritePtr, tsBuf);
            WritePtr += tsLen;
            entryLen -= tsLen;
        }

        strncpy(WritePtr, message, entryLen);
        WritePtr[entryLen] = '\0';

        // remove any line endings
        ptr = strchr( WritePtr, '\n' );
        if (ptr != NULL) *ptr = '\0';
        ptr = strchr( WritePtr, '\r' );
        if (ptr != NULL) *ptr = '\0';

        if (LOG2STDERR)
        {
            zul_log(2, pFullRecord);
        }

        strcat ( WritePtr, "\n" );      // preferred line ending
        WritePtr += strlen (WritePtr);
        retVal =  true;
    }
    else
    {
        zul_logf (1, "%s - buffer full, sync2disk!", __FUNCTION__ );
        retVal = false;
    }

    mutexUnLock();
    return retVal;
}

bool ZyLogFile::Write2LogF (const char *fmt, ... )
{
    bool retVal;
    static char buffer[101];
    va_list ap;

    va_start(ap, fmt);
    (void)vsnprintf(buffer, 100, fmt, ap);
    buffer[99]='\0';
    retVal = Write2Log(buffer);

    va_end(ap);
    return retVal;
}

void ZyLogFile::Sync2Disk(void)
{
    const char  endMessage[] = "---\n";

    mutex__Lock();

    if (BUFF_LEN == 0) return;

    // append the buffer to the file
    FILE *fp;
    fp = fopen ( FilePath, "a" );
    if ( fp != NULL )
    {
        fwrite( BUFFER, 1, BUFF_LEN, fp );
        fwrite( endMessage, 1, sizeof(endMessage)-1, fp );
        fclose( fp );

        // clear buffer
        WritePtr = BUFFER;
    }
    else
    {
        zul_logf (1, "LogFile - fail [%s]", strerror(errno) );
    }

    mutexUnLock();
}

int ZyLogFile::GetBytesFree(void)
{
    int v;
    mutex__Lock();
    v = FREE_BUFF;
    mutexUnLock();
    return v;
}


void ZyLogFile::WipeFile (void)
{
    int retVal = unlink(FilePath);
    if ((retVal != 0) && (errno != ENOENT))
    {
        zul_logf (0, "WipeFile - fail [%s] %d", strerror(errno), errno );
    }
}

// End of Implementation


#if UNIT_TEST
// clear && g++ -DUNIT_TEST -I ../include ./logfile.cpp ./debug.c

int main()
{
    int x;
    ZyLogFile zlf = ZyLogFile("./test.txt");

    printf("Unit tests\n");
    zul_log(0, "Unit tests for LogFile services\n");
    // zlf.WipeFile();
    zlf.EnableTimeStamp ( true );
    // zlf.Sync2Disk();
    zlf.Write2Log("Hello Log ... ");
    // zlf.Sync2Disk();
    zlf.Write2Log("Hello Log ... 2");
    zlf.Write2LogF("Hello Log ... %d", 3);

    for (x=0; x < 15; x++)
        zlf.Write2LogF("Log ... entry test %d : %s ", x+4, "some string");

    zul_logf(0, "Free Buffer: %d\n", zlf.GetBytesFree() );

//    zlf.Sync2Disk();
//    zul_logf(0, "Free Buffer: %d\n", zlf.GetBytesFree() );

    return 0;
}

#endif

