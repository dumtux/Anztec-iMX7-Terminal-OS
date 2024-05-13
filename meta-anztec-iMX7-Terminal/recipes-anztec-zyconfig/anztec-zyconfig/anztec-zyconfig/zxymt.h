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

/**
 *  Overview
 *  ========
 *  provide specific values consistent in the Zytronic Multitouch device range:
 *    { ZXY150, ZXY200, ZXY300 }
 */

#ifndef _ZXYMT_H
#define _ZXYMT_H

/**
 * This software is only compatible with Zytronic MT controllers that are
 * running a firmware which supports on-board calibration.
 */

/**
 * Record that there are no private data fields in the ZXY100
 */
#define ZXYMT_PRIVATE_STATUS_EXIST              (  1)
#define ZXYMT_PRIVATE_CONFIGS_EXIST             (  1)

/**
 * provide names for Configuration parameter Indices
 * FTM = First Touch Mode
 */
#define ZXYMT_CI_LOWER_THRESHOLD                (  0)
#define ZXYMT_CI_UPPER_THRESHOLD                (  1)
#define ZXYMT_CI_INVERT_X                       (  2)
#define ZXYMT_CI_INVERT_Y                       (  3)
#define ZXYMT_CI_SWAP_XY                        (  4)
#define ZXYMT_CI_PALM_REJECT_WEIGHT_LIMIT       (  5)
#define ZXYMT_CI_PIXEL_AVERAGING                (  6)
#define ZXYMT_CI_MIN_TOUCH_TIME_MS              (  7)
#define ZXYMT_CI_MAX_TOUCHES                    (  8)
#define ZXYMT_CI_FTM_SELECTION                  (  9)
#define ZXYMT_CI_PALM_REJECT_FLOOD_RADIUS       ( 10)

#define ZXYMT_CI_ONBOARD_CAL_BASE               ( 11)

#define ZXYMT_CI_ACTIVE_AREA_BASE               ( 19)

#define ZXYMT_CI_RESAMPLE_THRESHOLD_MAX         (220)
#define ZXYMT_CI_INTERPOLATOR_BIAS              (230)
#define ZXYMT_CI_FTM_OFF_THRESHOLD              (237)
#define ZXYMT_CI_FTM_ON_THRESHOLD               (238)
#define ZXYMT_CI_MAX_TRACK_DISTANCE             (252)



/**
 * provide names for Status value Indices
 */
#define ZXYMT_SI_CURRENT_SIGNAL_LEVEL           (  0)
#define ZXYMT_SI_PALM_REJECT_WEIGHT             (  1)
#define ZXYMT_SI_NUM_LIVE_TOUCHES               (  2)
#define ZXYMT_SI_NUM_RESAMPLED_CELLS            (  3)
#define ZXYMT_SI_NUM_LIVE_ROWS                  (  4)
#define ZXYMT_SI_NUM_LIVE_COLUMNS               (  5)
#define ZXYMT_SI_FRAME_RATE                     (  6)
#define ZXYMT_SI_NUM_ACTIVE_CELLS               (  7)
#define ZXYMT_SI_CONNECTED_WIRE_X_FIRST         (  8)
#define ZXYMT_SI_CONNECTED_WIRE_X_LAST          (  9)
#define ZXYMT_SI_CONNECTED_WIRE_Y_FIRST         ( 10)
#define ZXYMT_SI_CONNECTED_WIRE_Y_LAST          ( 11)
#define ZXYMT_SI_STATUS_ACTIVE                  ( 12)
#define ZXYMT_SI_STATUS_OR_SINCE_PREVIOUS       ( 13)
#define ZXYMT_SI_STATUS_AND_SINCE_PREVIOUS      ( 14)

#define ZXYMT_SI_NUM_CONFIG_PARAMS              ( 15)
#define ZXYMT_SI_NUM_STATUS_VALUES              ( 16)

#define ZXYMT_SI_PROCESSOR_ID_BASE              ( 17)
#define ZXYMT_SI_FAILSAFE_REASON                ( 34)

#define ZXYMT_SI_SHIELD_WIRE                    (207)

#define ZXYMT_SI_NUM_DEAD_Y_WIRES               (208)
#define ZXYMT_SI_NUM_DEAD_X_WIRES               (209)
#define ZXYMT_SI_NEXT_DEAD_Y_WIRE               (210)
#define ZXYMT_SI_NEXT_DEAD_X_WIRE               (211)

#define ZXYMT_SI_CORE_ACTIVE_CELLS              (212)

#define ZXYMT_SI_OPTION_BITS                    (215)
#define ZXYMT_SI_NUM_Y_WIRES                    (217)
#define ZXYMT_SI_NUM_X_WIRES                    (218)

#define ZXYMT_SI_STATUS_LEDS                    (223)
#define ZXYMT_SI_CURR_SIG_MIN                   (224)
#define ZXYMT_SI_CURR_SIG_MAX                   (225)
#define ZXYMT_SI_CURR_SIG_Y                     (226)
#define ZXYMT_SI_CURR_SIG_X                     (227)

#define ZXYMT_SI_NUM_ZEROS_WARNING_MAX          (228)

#define ZXYMT_SI_AVG_SIG_LEVEL                  (231)

#define ZXYMT_SI_NUM_PRIVATE_CONFIG_PARAMS      (236)
#define ZXYMT_SI_NUM_PRIVATE_STATUS_VALUES      (237)

#define ZXYMT_SI_TOUCH_COUNT_STAT_ID            (242)
#define ZXYMT_SI_MEAN_ABS_DIFF_SAMPLE_REPEATS   (249)

// shorter tokens:
#define ZXYMT_CI_PR_WGT_LIM                     (ZXYMT_CI_PALM_REJECT_WEIGHT_LIMIT)
#define ZXYMT_CI_PR_FLOOD_RAD                   (ZXYMT_CI_PALM_REJECT_FLOOD_RADIUS)
#define ZXYMT_CI_FTM_SEL                        (ZXYMT_CI_FIRST_TOUCH_MODE_SELECTION)


// provide counts of ConfigParam/StatusValue groups

#define ZXYMT_CN_ONBOARD_CAL_COUNT              (  8)
#define ZXYMT_CN_ACTIVE_AREA_COUNT              (  4)

#define ZXYMT_SN_PROCESSOR_ID_COUNT             (  6)

#define ZXYMT_MAX_TOUCH                         ( 40)
#define ZXY500_MAX_TOUCH                        (100)


// Bit definitions for ZXYMT_SI_STATUS_ACTIVE

#define STATUS_PALM_REJECT_ACTIVE               0x0001U
#define STATUS_INACTIVE                         0x0002U
#define STATUS_FIRST_TOUCH_MODE                 0x0004U
#define STATUS_INHIBIT                          0x0008U
#define STATUS_TOUCH                            0x0010U
#define STATUS_EQUALISING                       0x0020U
// fault flags
#define STATUS_NUM_RESAMPLED_CELLS_FAULT        0x0400U
#define STATUS_NUM_LIVE_ROWS_FAULT              0x0800U
#define STATUS_NUM_LIVE_COLUMNS_FAULT           0x1000U



// Bit definitions for ZXYMT_SI_OPTION_BITS

/* ZXYMT_OPT_BIT_ACTIVE_AREAS_OLD               0x0001U     -- Touch Active Areas - never used */
#define ZXYMT_OPT_BIT_ON_BOARD_CAL              0x0002U     // ZXY100,110,MT - OnBoard Controller Calibration
#define ZXYMT_OPT_BIT_SHOW_INACTIVE_CELLS       0x0004U     // ZXYMT - Show inactive cells in raw view
#define ZXYMT_OPT_BIT_DISABLE_FLASH_WRITE       0x0008U     // ZXY100,110,MT - Protocol to DisableFlashWrite
#define ZXYMT_OPT_BIT_TOUCH_ACTIVE_AREA         0x0010U     // Touch Active Area - Used!
#define ZXYMT_OPT_BIT_FTM_ON_PERSIST            0x0020U     // FTM persistent mechanism


// Bit definitions for ZXYMT_SI_FAILSAFE_REASON

#define FAILSAFE_ASIC0                          0x0001U
#define FAILSAFE_ASIC1                          0x0002U
#define FAILSAFE_ASIC2                          0x0004U
#define FAILSAFE_ASIC3                          0x0008U
#define FAILSAFE_ALL_WIRES_SHORTED              0x0010U
#define FAILSAFE_UNKNOWN_BOARD                  0x0020U
#define FAILSAFE_DFU_CORRUPT                    0x0040U
#define FAILSAFE_HVCONC_FAULT                   0x0080U
#define FAILSAFE_USER_REQUESTED                 0x0100U
#define FAILSAFE_SENSOR_MISSING                 0x0200U

#endif  // _ZXYMT_H
