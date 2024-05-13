/*
 *  Copyright (c) 2019 Zytronic Displays Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact Zytronic, you can do so either via the
 * website <www.zytronic.co.uk> or by paper mail:
 * Zytronic, Whiteley Road, Blaydon on Tyne, Tyne & Wear, NE21 5NJ, UK
 */



/* Overview
   ===============
    This allows a range of console message-sets to be enabled
    to facilitate application and library debugging.
 */


#ifndef _ZY_DEBUG2CONSOLE_H
#define _ZY_DEBUG2CONSOLE_H


/**
 * Setting this token to 1 enables allows the outgoing and incoming USB 
 * messages to be viewed in the console log as hex byte sequences
 */

#define     PROTOCOL_DEBUG      (0)

/**
 * Setting this token to 1 enables the Bootloader exchanges to be viewed in hex
 */

#define     BL_DEBUG            (0)

/**
 * Setting this token to 1 enables  some timing information for 
 * time sensitive activities
 */

#define     TIMING_DEBUG        (0)


/**
 * Setting this token to 1 enables messages related to touch/calibration
 * activities to be logged to console
 */

#define     TOUCH_DEBUG         (0)



/**
 * Setting this token to 1 enables all messages being logged to a file 
 * to be echoed out to the console.
 */

#define     LOG2STDERR          (0)



#endif  // _ZY_DEBUG2CONSOLE_
