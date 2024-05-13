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
 *   Provide a timestamped logfile system.
 */

#ifndef _ZYLOGFILE_H
#define _ZYLOGFILE_H


class ZyLogFile
{

  public:

    ZyLogFile(const char *fn);
    ~ZyLogFile();

    void         EnableTimeStamp     (const bool enable);

    bool         Write2Log           (const char *message);
    bool         Write2LogF          (const char *fmt, ... );

    void         Sync2Disk           (void);
    void         WipeFile            (void);

    int          GetBytesFree        (void);

  private:

    pthread_mutex_t mutex1;

    bool            TimeStamp;
    char        *   WritePtr;
    char        *   BUFFER;
    char        *   FilePath;

    void mutex__Lock(void);
    void mutexUnLock(void);
    void mutex__error(int e, const char *name);
};


#endif  // _ZYLOGFILE_H

