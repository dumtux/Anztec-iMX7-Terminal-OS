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
 *   Provide reed/write access to persistent configuration data
 *
 *   All config data is constrained to /etc/zytronic/
 *      and subdirectories are not supported!
 *   Multiple files are allowed at this location.
 */

#ifndef _ZYCONFFILE_H
#define _ZYCONFFILE_H

#include <string>
#include <map>

using namespace std;

class ZyConfFile
{

  public:

    ZyConfFile(const char *filename);
    ~ZyConfFile();

    bool        WriteFile           (void);
    bool        ReadFile            (void);
    void        Clear               (void);

    bool        GetString           (const char *key, char *value, const int len);
    bool        SetString           (const char *key, const char *value);
    bool        DeleteKey           (const char *key);
    bool        KeyExists           (const char *key);

  private:
    bool        DirExists           (void);
    bool        FileExists          (void);

    // -----------------------------------------------------------------------------

    map<string,string>      keyValues;

    static const string     BASE_PATH;
    string                  fileName;
    bool                    validFileName;

    bool                    modified;
};

#endif  // _ZYCONFFILE_H

