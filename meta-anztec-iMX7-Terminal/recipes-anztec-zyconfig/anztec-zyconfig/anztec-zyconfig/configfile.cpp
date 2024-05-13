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
 *      Provide read/wrte access to persistent configuration data
 */


#include <stdio.h>

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>      // supports open, with permissions

#include "configfile.h"
#include "debug.h"

// --- Module Global Values -------------------------------

#define DATA_LEN        (80)

const string ZyConfFile::BASE_PATH = "/etc/zytronic/";

// --- Implementation -------------------------------------

/**
 * Constructor
 */
ZyConfFile::ZyConfFile(const char *fn)
{
    validFileName = true;

    if (fn != NULL)
    {
        fileName = string ( fn );

        // check supplied string for validity
        if ( fileName.find('/') )
        {
            // subdirectories are not required/supported.
            validFileName = false;
        }

        if ( ! fileName.find(".conf"))
        {
            fileName.append(".conf");
        }
    }
    else
    {
        fileName = "zyconfig.conf";
    }
    keyValues.clear();
    modified = false;
}

/**
 * Destructor
 */
ZyConfFile::~ZyConfFile()
{
    if (modified && validFileName)
    {
        WriteFile();
    }
}

bool ZyConfFile::DirExists(void)
{
    struct stat sb;
    // check Zytronic directory in /etc
    if ( (stat(BASE_PATH.c_str(), &sb) == 0) && S_ISDIR(sb.st_mode) )
    {
        return true;
    }
    return false;
}

bool ZyConfFile::FileExists(void)
{
    struct stat sb;
    if (!validFileName) return false;

    // check Zytronic directory in /etc
    if (!DirExists()) return false;

    string fullPath = BASE_PATH;
    fullPath.append(fileName);
    // check for 'name'.conf
    if ( (stat(fullPath.c_str(), &sb) == 0) && S_ISREG(sb.st_mode) )
    {
        return true;
    }
    return false;
}


bool ZyConfFile::WriteFile(void)
{
    if (!validFileName) return false;

    string path = BASE_PATH;
    path.append(fileName);

    if (!DirExists())
    {
        mkdir ( BASE_PATH.c_str(), 0777 );
    }

    if (!FileExists())
    {
        int fd = creat ( path.c_str(), 0644 );
        close(fd);
    }

    ofstream fs( path );
    fs << "# ZyConfig settings file" << endl;

    {
        auto t = time(NULL);
        auto tm = *localtime(&t);
        char tm_buffer[50+1];
        snprintf (tm_buffer, 50, "%04d/%02d/%02d %02d:%02d:%02d",
                    tm.tm_year + 1900,
                    tm.tm_mon + 1,
                    tm.tm_mday,
                    tm.tm_hour,
                    tm.tm_min,
                    tm.tm_sec
                 );
        fs << "# " << tm_buffer << endl << endl;
    }

    map<string, string>::iterator it;
    for ( it = keyValues.begin(); it != keyValues.end(); it++ )
    {
        string k = it->first;
        string v = it->second;
        fs << k << "\t" << v << endl;
    }
    fs.close();
    return true;
}

bool ZyConfFile::ReadFile(void)
{
    if (!validFileName) return false;

    string path = BASE_PATH;
    path.append(fileName);
    string line;
    bool retVal = false;
    ifstream input( path );     // destructor closes

    if (!input.good()) return false;      // file doesn't exist

    Clear();
    retVal = true;
    while ( getline ( input, line ) )
    {
        if (line.length() < 3) continue;
        if ('#' == line.at(0)) continue;
        istringstream iss ( line );
        string key, value;
        if ( ! ( iss >> key >> value ) ) continue;
        SetString(key.c_str(), value.c_str());
        //cout << "Found " << key << " : " << value << endl;
    }

    return retVal;
}

void ZyConfFile::Clear(void)
{
    keyValues.clear();
    modified = true;
}

bool ZyConfFile::GetString(const char *key, char *value, const int len)
{
    map<string, string>::iterator it = keyValues.find(key);

    if (it != keyValues.end())
    {
        string v = it->second;
        int foundLen = v.length();
        if (foundLen < len)
        {
            strncpy( value, v.c_str(), len);
            value[len-1] = '\0';
            return true;
        }
    }
    return false;
}

bool ZyConfFile::SetString(const char *key, const char *value)
{
    string k = key;
    string v = value;
    int lk = k.length(), lv = v.length();

    // validate input strings
    if (lk == 0) return false;
    if (lv ==-0) return false;
    if (lk > DATA_LEN) return false;
    if (lv > DATA_LEN) return false;

    modified = true;
    keyValues[k] = v;       // overwrite any existing value
    return true;
}


// g++ -std=c++11
bool ZyConfFile::DeleteKey(const char *key)
{
    bool retVal = false;
    map<string, string>::iterator it;
    for ( it = keyValues.begin(); it != keyValues.end(); )
    {
        string k = it->first;
        if (k.compare(key) == 0)
        {
            it = keyValues.erase(it);
            retVal = true;
            modified = true;
        }
        else
        {
            it++;
        }
    }
    return retVal;
}

bool ZyConfFile::KeyExists(const char *key)
{
    map<string, string>::iterator it = keyValues.find(key);

    if (it == keyValues.end())
    {
        return false;
    }
    return true;
}


#if UNIT_TEST
// clear && g++ -std=c++11 -DUNIT_TEST -I ../include ./configfile.cpp ./debug.c

int main()
{
    int x;
    char valBuffer[DATA_LEN];
    char keyBuffer[DATA_LEN];

    ZyConfFile *zcf = new ZyConfFile(NULL);

    zul_log(0, "Unit tests for Config File services\n");

    zcf->ReadFile();
    if (zcf->GetString("zeroth", valBuffer, DATA_LEN))
    {
        zul_logf(0, "'zeroth' key found\n");
    }

    zcf->SetString("first", "one");
    zcf->SetString("second", "two");

    if (zcf->GetString("first", valBuffer, DATA_LEN))
    {
        zul_logf(0, "'first' key found\n");
    }
    zul_logf(0, "first holds '%s'\n", valBuffer);

    if (zcf->KeyExists("first"))
    {
        zul_logf(0, "'first' key found\n");
    }

    zcf->DeleteKey("first");
    if (zcf->KeyExists("first"))
    {
        zul_logf(0, "'first' key found\n");
    }
    else
    {
        zul_logf(0, "'first' key missing\n");
    }

    zcf->Clear();
    if (zcf->KeyExists("second"))
    {
        zul_logf(0, "'second' key found\n");
    }
    else
    {
        zul_logf(0, "'second' key missing\n");
    }
    zcf->SetString("third", "three");
    zcf->SetString("third", "four");
    zcf->SetString("fifth", "five");

    zcf->WriteFile();


    // ----------------
    delete zcf;
    return 0;
}

#endif

