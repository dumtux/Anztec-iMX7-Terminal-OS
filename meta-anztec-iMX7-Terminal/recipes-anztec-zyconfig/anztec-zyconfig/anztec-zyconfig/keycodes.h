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


/* Module Overview
   ===============
   This code provides programmers with a set of common keyscan codes
 */

#ifndef _ZY_KEYCODE_H
#define _ZY_KEYCODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum keyboardModifier
{
    L_CTRL      = 0x01,
    L_SHIFT     = 0x02,
    L_ALT       = 0x04,
    L_META      = 0x08,
    R_CTRL      = 0x10,
    R_SHIFT     = 0x20,
    R_ALT       = 0x40,
    R_META      = 0x80,
} KbdModifier;          // see Virtual Key service


typedef enum keyScanCode_t
{
    KEY_A           = 0x04,         // a A
    KEY_B           = 0x05,         // b B
    KEY_C           = 0x06,         // c C
    KEY_D           = 0x07,         // d D
    KEY_E           = 0x08,         // e E
    KEY_F           = 0x09,         // f F
    KEY_G           = 0x0a,         // g G
    KEY_H           = 0x0b,         // h H
    KEY_I           = 0x0c,         // i I
    KEY_J           = 0x0d,         // j J
    KEY_K           = 0x0e,         // k K
    KEY_L           = 0x0f,         // l L
    KEY_M           = 0x10,         // m M
    KEY_N           = 0x11,         // n N
    KEY_O           = 0x12,         // o O
    KEY_P           = 0x13,         // p P
    KEY_Q           = 0x14,         // q Q
    KEY_R           = 0x15,         // r R
    KEY_S           = 0x16,         // s S
    KEY_T           = 0x17,         // t T
    KEY_U           = 0x18,         // u U
    KEY_V           = 0x19,         // v V
    KEY_W           = 0x1a,         // w W
    KEY_X           = 0x1b,         // x X
    KEY_Y           = 0x1c,         // y Y
    KEY_Z           = 0x1d,         // z Z

    KEY_1           = 0x1e,         // 1 !
    KEY_2           = 0x1f,         // 2 @
    KEY_3           = 0x20,         // 3 #
    KEY_4           = 0x21,         // 4 $
    KEY_5           = 0x22,         // 5 %
    KEY_6           = 0x23,         // 6 ^
    KEY_7           = 0x24,         // 7 &
    KEY_8           = 0x25,         // 8 *
    KEY_9           = 0x26,         // 9
    KEY_0           = 0x27,         // 0

    KEY_enter       = 0x28,         // Return (ENTER)
    KEY_esc         = 0x29,         // ESCAPE
    KEY_backspace   = 0x2a,         // DELETE (Backspace)
    KEY_tab         = 0x2b,         // Tab
    KEY_space       = 0x2c,         // Spacebar
    KEY_minus       = 0x2d,         // - _
    KEY_equal       = 0x2e,         // = +
    KEY_leftbrace   = 0x2f,         // [ {
    KEY_rightbrace  = 0x30,         // ] }
    KEY_backslash   = 0x31,         // \ |
    KEY_hashtilde   = 0x32,         // Non-US # ~
    KEY_semicolon   = 0x33,         // ; :
    KEY_apostrophe  = 0x34,         // ' "
    KEY_grave       = 0x35,         // ` ~
    KEY_comma       = 0x36,         // , <
    KEY_dot         = 0x37,         // . >
    KEY_slash       = 0x38,         // / ?
    KEY_capslock    = 0x39,         // Caps Lock

    KEY_f_1         = 0x3a,         // F1
    KEY_f_2         = 0x3b,         // F2
    KEY_f_3         = 0x3c,         // F3
    KEY_f_4         = 0x3d,         // F4
    KEY_f_5         = 0x3e,         // F5
    KEY_f_6         = 0x3f,         // F6
    KEY_f_7         = 0x40,         // F7
    KEY_f_8         = 0x41,         // F8
    KEY_f_9         = 0x42,         // F9
    KEY_f10         = 0x43,         // F10
    KEY_f11         = 0x44,         // F11
    KEY_f12         = 0x45,         // F12

/*  KEY_SYSRQ       = 0x46,         // PrintScreen
    KEY_SCROLLLOCK  = 0x47,         // ScrollLock
    KEY_PAUSE       = 0x48,         // Pause
    KEY_INSERT      = 0x49,         // Insert
*/

    KEY_home        = 0x4a,         // Home
    KEY_pageup      = 0x4b,         // PageUp
    KEY_delete      = 0x4c,         // DeleteForward
    KEY_end         = 0x4d,         // End
    KEY_pagedown    = 0x4e,         // PageDown
    KEY_right       = 0x4f,         // RightArrow
    KEY_left        = 0x50,         // LeftArrow
    KEY_down        = 0x51,         // DownArrow
    KEY_up          = 0x52,         // UpArrow


/* KEYPAD Keys

    KEY_NUMLOCK     = 0x53,         // NumLock Clear
    KEY_KPSLASH     = 0x54,         // /
    KEY_KPASTERISK  = 0x55,         // *
    KEY_KPMINUS     = 0x56,         // -
    KEY_KPPLUS      = 0x57,         // +
    KEY_KPENTER     = 0x58,         // ENTER
    KEY_KP1         = 0x59,         // 1 End
    KEY_KP2         = 0x5a,         // 2 Down Arrow
    KEY_KP3         = 0x5b,         // 3 PageDn
    KEY_KP4         = 0x5c,         // 4 Left Arrow
    KEY_KP5         = 0x5d,         // 5
    KEY_KP6         = 0x5e,         // 6 Right Arrow
    KEY_KP7         = 0x5f,         // 7 Home
    KEY_KP8         = 0x60,         // 8 Up Arrow
    KEY_KP9         = 0x61,         // 9 Page Up
    KEY_KP0         = 0x62,         // 0 Insert
    KEY_KPDOT       = 0x63,         // . Delete

    KEY_102ND       = 0x64,         // Non-US \ |

    KEY_POWER       = 0x66,         // Power
    KEY_KPEQUAL     = 0x67,         // Keypad =

    KEY_OPEN        = 0x74,         // Execute
    KEY_HELP        = 0x75,         // Help
    KEY_PROPS       = 0x76,         // Menu
    KEY_FRONT       = 0x77,         // Select
    KEY_STOP        = 0x78,         // Stop
    KEY_AGAIN       = 0x79,         // Again
    KEY_UNDO        = 0x7a,         // Undo
    KEY_CUT         = 0x7b,         // Cut
    KEY_COPY        = 0x7c,         // Copy
    KEY_PASTE       = 0x7d,         // Paste
    KEY_FIND        = 0x7e,         // Find
    KEY_MUTE        = 0x7f,         // Mute
    KEY_VOLUMEUP    = 0x80,         // VolumeUp
    KEY_VOLUMEDOWN  = 0x81,         // VolumeDown

    KEY_KPCOMMA     = 0x85,         // KeypadComma

    KEY_MEDIA_PLAYPAUSE             = 0xe8,
    KEY_MEDIA_STOPCD                = 0xe9,
    KEY_MEDIA_PREVIOUSSONG          = 0xea,
    KEY_MEDIA_NEXTSONG              = 0xeb,
    KEY_MEDIA_EJECTCD               = 0xec,
    KEY_MEDIA_VOLUMEUP              = 0xed,
    KEY_MEDIA_VOLUMEDOWN            = 0xee,
    KEY_MEDIA_MUTE                  = 0xef,
    KEY_MEDIA_WWW                   = 0xf0,
    KEY_MEDIA_BACK                  = 0xf1,
    KEY_MEDIA_FORWARD               = 0xf2,
    KEY_MEDIA_STOP                  = 0xf3,
    KEY_MEDIA_FIND                  = 0xf4,
    KEY_MEDIA_SCROLLUP              = 0xf5,
    KEY_MEDIA_SCROLLDOWN            = 0xf6,
    KEY_MEDIA_EDIT                  = 0xf7,
    KEY_MEDIA_SLEEP                 = 0xf8,
    KEY_MEDIA_COFFEE                = 0xf9,
    KEY_MEDIA_REFRESH               = 0xfa,
    KEY_MEDIA_CALC                  = 0xfb

*/
} KeyScanCode;

#ifdef __cplusplus
}
#endif

#endif // _ZY_KEYCODE_H
