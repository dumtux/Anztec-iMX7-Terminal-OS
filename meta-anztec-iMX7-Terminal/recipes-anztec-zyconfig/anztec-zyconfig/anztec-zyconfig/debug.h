/* Module Overview
   ===============
   This code should provide command-line debug services to the library.
   to:
    - print hex strings
    - print timestamped events
    - log to files
 */

#ifndef _ZY_DEBUG_H
#define _ZY_DEBUG_H


#ifdef __cplusplus
extern "C" {
#endif

#include "zytypes.h"

/**
 * simple console funcs
 */
char            zul_spinner             (void);

void            zul_CursorUp            (int lines);


/**
 * timestamp accessors
 */
long int        zul_getLongTS           (void);

void            zul_getStringTS         (char *string, size_t length);


/**
 * General logging services
 *  - level 1 - always reported
 *  - level 2, 3, 4 are higher verbosity settings
 *      for less important, or more freqent events
 *      or move volumous reports.
 */


// Change the default logging level (default is typically 2)
void            zul_setLogLevel         (int newLevel);



// conditionally print a simple string
void            zul_log                 (int level, const char *string);

// conditionally print a simple string with a timestamp
void            zul_log_ts              (int level, const char *string);

// conditionally print a user formatted string
void            zul_logf                (int level, const char *format, ...);

// conditionally print hex data with user supplied header
void            zul_log_hex             (int level, const char *header,
                                                    uint8_t *d, int len);


/**
 * print a timestamp to console with HDR message
 */
void            zul_printTimeStamped    (const char * HDR); // deprecated ToDo


/**
 * debug assistance, make byteStrings readable
 * Simple hex to string
 */
char const *    zul_hex2String          (uint8_t *d, int len);
/**
 * debug assistance, make byteStrings readable
 */
bool            zul_hex2Str             (uint8_t *buffer, int bufLen,
                                         uint8_t *data, int len,
                                         int bytesPerLine,
                                         int bytesPerGroup);

#ifdef __cplusplus
}
#endif


#endif // _ZY_DEBUG_H

