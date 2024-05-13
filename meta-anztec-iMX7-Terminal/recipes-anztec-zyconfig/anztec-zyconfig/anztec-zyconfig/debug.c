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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <syslog.h>

#include "zytypes.h"
#include "debug.h"


// === Implementation =========================================================


/**
 *   Verbosity and destination control module static variables (msv)
 *      0       - OFF - only critical errors written to console
 *      1..2    Monitoring levels, only significant events logged
 *      3       Verbose - many normal events logged
 *      4       Very verbose, all transfers logged
 */
static int                      msv_log_level         = 2;

static bool                     msv_use_syslog        = false;

/**
 * return a spinning character
 */
char zul_spinner(void)
{
    static int count = 0;
    char symbols[] = "|/-\\";
    return symbols[(count ++) % 4];
}


/**
 * move back UP 0-5 lines on the console
 */
void zul_CursorUp(int lines)
{
    // esc seq to move back up a line
    if (lines > 5) lines = 5;           // sanity check
    fprintf(stderr, "\r%c[%dA", 0x1B, lines);
}


/**
 * Modify the threshold at which messages are logged
 *   higher numbers here means more log entries will be generated
 */
void zul_setLogLevel(int newLevel)
{
    msv_log_level = newLevel;
}


/**
 * dump a string to the log
 */
void zul_log(int level, const char *string)
{
    if (level > msv_log_level) return;

    if (!msv_use_syslog)
    {
        (void)puts(string);   // to stdout
        return;
    }

    // on Ubuntu, read reports in /var/log/syslog

    // level 4 and above intentionally omitted -- too verbose.
    switch (level)
    {
        case 0:
            syslog(LOG_DAEMON|LOG_CRIT, "%s", string);
            break;

        case 1:
            syslog(LOG_DAEMON|LOG_ERR, "%s", string);
            break;

        case 2:
            syslog(LOG_DAEMON|LOG_NOTICE, "%s", string);
            break;

        case 3:
            syslog(LOG_DAEMON|LOG_DEBUG, "%s", string);
            break;
    }
}

/*
 * get a timestamp as a long int
 */
long int zul_getLongTS(void)
{
    struct timeb    nowTtimeMs;

    (void)ftime(&nowTtimeMs);

    return (long int)
              ( 1000L * (nowTtimeMs.time) % (24 * 60 * 60) +
                               nowTtimeMs.millitm );
}

/*
 * get a timestamp as a string
 */
void zul_getStringTS(char *string, size_t length)
{
    struct timeb    nowTtimeMs;

    (void)ftime(&nowTtimeMs);

    (void)snprintf(string, length-1, "%05d.%03d",
             (int)(nowTtimeMs.time) % (24 * 60 * 60),
             (int)nowTtimeMs.millitm);
             string[length-1] = '\0';
}

/*
 * dump a const string to the console, with a timestamp
 */
void zul_log_ts(int level, const char *string)
{
    char            timeStamp[120+1];
    struct timeb    nowTtimeMs;

    if (level > msv_log_level) return;

    (void)ftime(&nowTtimeMs);

    (void)snprintf(timeStamp, 120, "%05d.%03d %s",
             (int)(nowTtimeMs.time) % (24 * 60 * 60),
             (int)nowTtimeMs.millitm,
             string);
             timeStamp[120] = '\0';
    zul_log(level, timeStamp);
}


/**
 * hex dump a byte array to the log -- ToDo - see protocol service!
 */
void zul_log_hex(int level, const char *header, uint8_t *d, int len)
{
    const int   bytes_per_line = 16;
    int         i;
    char        buffer[128+1];

    if (level > msv_log_level) return;

    for (i = 0; i < len; i += bytes_per_line)
    {
        unsigned char *b = (unsigned char *)(d+i);
        (void)snprintf(buffer, 128, "%s [%02d..%02d] = "
            "%02x %02x %02x %02x  %02x %02x %02x %02x  "
            "%02x %02x %02x %02x  %02x %02x %02x %02x",
            header, i, i + bytes_per_line - 1,
            *(b+0), *(b+1), *(b+2), *(b+3),
            *(b+4), *(b+5), *(b+6), *(b+7),

            *(b+8),  *(b+9),  *(b+10), *(b+11),
            *(b+12), *(b+13), *(b+14), *(b+15)
        );
        buffer[128] = '\0';
        zul_log(level, buffer);
    }
}

/**
 * dump a formatted string to the log
 */
void zul_logf(int level, const char *format, ...)
{
    static char buffer[201];
    va_list ap;

    va_start(ap, format);

    if (level > msv_log_level) return;
    (void)vsnprintf(buffer, 200, format, ap);
    buffer[200]='\0';
    zul_log(level, buffer);

    va_end(ap);
}


/**
 * Print a time stamp with the supplied string to the console
 */
void zul_printTimeStamped(const char * msg)
{
    zul_log_ts(1, msg);
}

/**
 * Make byteStrings readable - Complex version
bool  zul_hex2Str (uint8_t *buffer,  int bufLen, uint8_t *data, int len,
                  int bytesPerLine, int bytesPerGroup)
{
    snprintf((char*)buffer, (size_t)bufLen, "Not Implemented as yet ... ");
    buffer[bufLen-1] = (uint8_t)'\0';
    return false;
} */


/**
 * Make byteStrings readable - Simple version
 */
char const * zul_hex2String(uint8_t *d, int len)
{
    const int   bytes_per_line = 16;
    int         i;
    char        buffer[101];
    static char retBuf[400];

    retBuf[0]='\0';

    // prevent overrun
    if (len > 71) len=71;

    for (i = 0; i < len; i += bytes_per_line)
    {
        unsigned char *b = (unsigned char *)(d+i);
        (void)snprintf(buffer, 100, "[%02d..%02d] = "
            "%02x %02x %02x %02x  %02x %02x %02x %02x  "
            "%02x %02x %02x %02x  %02x %02x %02x %02x\n",

            i, i + bytes_per_line - 1,

            *(b+00), *(b+01), *(b+02), *(b+03), *(b+04), *(b+05), *(b+06), *(b+07),
            *(b+ 8), *(b+ 9), *(b+10), *(b+11), *(b+12), *(b+13), *(b+14), *(b+15)
        );
        buffer[100] = '\0';
        strcat(retBuf, buffer);
    }
    return retBuf;
}
