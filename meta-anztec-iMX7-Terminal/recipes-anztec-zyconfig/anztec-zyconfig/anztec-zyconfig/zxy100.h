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
 *  provide specific ZXY100 values
 */

#ifndef _ZXY100_H
#define _ZXY100_H

/**
 * This software is only compatible with ZXY100 devices that are
 *  running a firmware with a version number >= this value
 */
#define ZXY100_MIN_FW_FLOAT             (501.34)

#define ZXY100_MAX_WIRES                (128)

/**
 * Record that there are no private data fields in the ZXY100
 */
#define ZXY100_PRIVATE_STATUS_EXIST             (0)
#define ZXY100_PRIVATE_CONFIGS_EXIST            (0)

#define ZXY100_THRESHOLD_DEBOUNCE               (2)

/**
 *  ZXY100 device mode consts
 */
#define ZXY100_MODE_MOUSE                       (0)
#define ZXY100_MODE_SINGLE                      (1)
#define ZXY100_MODE_DUAL                        (2)

#define ZXY100_GLASS_THIN                       (1)
#define ZXY100_GLASS_MEDIUM                     (2)
#define ZXY100_GLASS_THICK                      (3)
#define ZXY100_GLASS_EXTRA_THICK                (4)

#define ZXY100_FLEXI_LEFT                       (1)


/**
 * Locate the on-board calibration config parameters, and indicate
 * their number
 */

// #define ZXY100_ONBOARD_CAL_COUNT                (8)


/**
 * provide names for Configuration parameter Indices
 */

#define ZXY100_CI_DEVICE_MODE                   (0)
#define ZXY100_CI_COARSE_SENSITIVITY            (1)
#define ZXY100_CI_LOWER_THRESHOLD               (2)
#define ZXY100_CI_UPPER_THRESHOLD               (3)
#define ZXY100_CI_NUM_X_PIXELS                  (4)
#define ZXY100_CI_NUM_Y_PIXELS                  (5)
#define ZXY100_CI_FLEXI_POSITION                (6)     // similar to ZXYMT_CI_SWAP_XY
#define ZXY100_CI_PIXEL_AVERAGING               (7)
#define ZXY100_CI_STAB_FACTOR                   (8)
#define ZXY100_CI_MIN_TOUCH_TIME                (9)
#define ZXY100_CI_MAX_TOUCH_TIME                (10)
#define ZXY100_CI_TX_TIMER_PERIOD               (11)
#define ZXY100_CI_AUTO_EQUALIZATION_STATE       (12)
#define ZXY100_CI_TEMP_COMPENSATION_STATE       (13)
#define ZXY100_CI_USB_HEARTBEAT_STATE           (14)
#define ZXY100_CI_FLIP_X                        (15)
#define ZXY100_CI_FLIP_Y                        (16)            // 0x10
#define ZXY100_CI_DELTA_OUTPUT_STATE            (17)
#define ZXY100_CI_POINT_CLICK_STATE             (18)
#define ZXY100_CI_FRAME_AVERAGING_STATE         (19)
#define ZXY100_CI_FRAME_AVERAGING_SAMPLES       (20)
#define ZXY100_CI_PRESSURE_TRACKING_STATE       (21)
#define ZXY100_CI_PRESSURE_TRACKING_MARGIN      (22)
#define ZXY100_CI_PRESSURE_TRACKING_HISTORY     (23)
#define ZXY100_CI_NOISE_FILTER_MASK             (24)

#define ZXY100_CI_PERCENTAGE_SCANNER_ENABLED                                    (34)            // 0x22
#define ZXY100_CI_SHORT_DURATION_EQUALIZATION_TIMER                             (35)
#define ZXY100_CI_NUMBER_RESAMPLE_AVERAGING_SAMPLES                             (36)
#define ZXY100_CI_SENSE_MULTIPLICATION_FACTOR                                   (37)
#define ZXY100_CI_FIRST_TOUCH_PROTECT_MODE_TIMER                                (38)
#define ZXY100_CI_FIRST_TOUCH_PROTECT_MODE_NOISE_FILTER_MASK                    (39)
#define ZXY100_CI_FIRST_TOUCH_PROTECT_MODE_MIN_TOUCH_TIME                       (40)
#define ZXY100_CI_FIRST_TOUCH_PROTECT_MODE_NUMBER_RESAMPLE_AVERAGING_SAMPLES    (41)

#define ZXY100_CI_FIRST_TOUCH_PROTECT_MODE_AUTO_EQUALIZATION_STATE              (44)

#define ZXY100_CI_ONBOARD_CAL_BASE              (47)            //  0x2F

// 'shorthand' tokens:
#define ZXY100_CI_FTPM_TIMER                    (ZXY100_CI_FIRST_TOUCH_PROTECT_MODE_TIMER)
#define ZXY100_CI_FTPM_NOISE_FILTER_MASK        (ZXY100_CI_FIRST_TOUCH_PROTECT_MODE_NOISE_FILTER_MASK)
#define ZXY100_CI_FTPM_MIN_TOUCH_TIME           (ZXY100_CI_FIRST_TOUCH_PROTECT_MODE_MIN_TOUCH_TIME)
#define ZXY100_CI_FTPM_NUM_RESAMPLE_AVG_SMPLS   (ZXY100_CI_FIRST_TOUCH_PROTECT_MODE_NUMBER_RESAMPLE_AVERAGING_SAMPLES)
#define ZXY100_CI_FTPM_AUTO_EQ_STATE            (ZXY100_CI_FIRST_TOUCH_PROTECT_MODE_AUTO_EQUALIZATION_STATE)

/**
 * provide names for Status value Indices
 */
#define ZXY100_SI_NUM_STATUS_VALUES             (0)
#define ZXY100_SI_NUM_CONFIG_PARAMS             (1)
#define ZXY100_SI_NUM_TOUCHES_SINCE_LAST_REPORT (2)
#define ZXY100_SI_POWER_UP_TIMER_LOWER          (3)     //Lower 16-bits
#define ZXY100_SI_POWER_UP_TIMER_UPPER          (4)     //Upper 16-bits
#define ZXY100_SI_NUM_FLASH_WRITES              (5)
#define ZXY100_SI_NUM_NOISE_EVENTS_FILTER0      (6)
#define ZXY100_SI_NUM_NOISE_EVENTS_FILTER1      (7)
#define ZXY100_SI_NUM_NOISE_EVENTS_FILTER2      (8)
#define ZXY100_SI_NUM_NOISE_EVENTS_FILTER3      (9)
#define ZXY100_SI_NUM_NOISE_EVENTS_FILTER4      (10)
#define ZXY100_SI_NUM_NOISE_EVENTS_FILTER5      (11)
#define ZXY100_SI_NUM_NOISE_EVENTS_FILTER6      (12)
#define ZXY100_SI_NUM_NOISE_EVENTS_FILTER7      (13)
#define ZXY100_SI_SPARE_COMMS_TX                (14)
#define ZXY100_SI_SPARE_COMMS_RX                (15)
#define ZXY100_SI_SPARE_STACK_SIZE              (16)
#define ZXY100_SI_CONFIG_RESISTOR_MASK          (17)
#define ZXY100_SI_FPS                           (18)
#define ZXY100_SI_NUM_NOISE_RECOVERY_EVENTS     (19)
#define ZXY100_SI_NUM_EQUALIZATIONS             (20)
#define ZXY100_SI_PALM_REJECT_TIMER             (21)
#define ZXY100_SI_WIRE_VALUE_MAX                (22)
#define ZXY100_SI_WIRE_VALUE_MEAN               (23)
#define ZXY100_SI_WIRE_VALUE_STDDEV             (24)
#define ZXY100_SI_GLOBAL_FAULT                  (25)
#define ZXY100_SI_GLOBAL_FAULT_INTEG_TST_FAIL   (26)
#define ZXY100_SI_GLOBAL_FAULT_STACK_TST_FAIL   (27)
#define ZXY100_SI_SENSOR_TYPE                   (28)
#define ZXY100_SI_OPTION_BITS                   (29)


/**
 * provide counts of ConfigParam/StatusValue groups
 */
#define ZXY100_CN_ONBOARD_CAL_COUNT             (8)
// #define ZXY100_CN_ACTIVE_AREA_COUNT          (4)       NOT EXISTANT IN THE ZXY100 - TODO - FIXME


// Bit definitions for ZXY100_SI_OPTION_BITS

/* ZXYMT_OPT_BIT_ACTIVE_AREAS_OLD               0x0001U     -- Touch Active Areas - never used */
#define ZXY100_OPT_BIT_ON_BOARD_CAL             0x0002U     // ZXY100,110,MT - OnBoard Controller Calibration
/* ZXYMT_OPT_BIT_SHOW_INACTIVE_CELLS            0x0004U     -- ZXYMT Only - Show inactive cells in raw view */
#define ZXY100_OPT_BIT_DISABLE_FLASH_WRITE      0x0008U     // ZXY100,110,MT - Protocol to DisableFlashWrite
/* ZXYMT_OPT_BIT_TOUCH_ACTIVE_AREA              0x0010U     // ZXYMT -- Touch Active Area - Used!  COLLISION !! */
#define ZXY100_OPT_BIT_ONLY_REPORT_IN_CAL_AREA  0x0010U     // ZXY100 - Ignore Touches Outside Calibrated Area
/* ZXYMT_OPT_BIT_FTM_ON_PERSIST                 0x0020U     -- ZXYMT Only - FTM persistent mechanism */


// -- original ZXY100 data-structures used in the protocol
#define ZXY100_SYSRPT_NOISE_ALGOS (8)
struct zxy100_sys_report_t {
    uint32_t    uptime;
    uint16_t    flash_writes;
    uint16_t    noiseMetrics[ZXY100_SYSRPT_NOISE_ALGOS];
    uint16_t    memData_A[3];
    uint8_t     hwConfigOptions;
    uint16_t    framesPerSecond;
    uint16_t    numNoiseRecoveryEvents;
    uint16_t    numEqualizations;
};
typedef struct zxy100_sys_report_t Zxy100SysReport;

#define HW_STR_LEN      (20)
#define CPU_STR_LEN     (26)

// only required for ZXY100s
struct zxy100Version_t
{
    bool        valid;
    char        hwVersionStr[HW_STR_LEN + 1];
    char        cpuIdStr[CPU_STR_LEN+1];
    float       hwVersion, fwVersion, blVersion;
    uint8_t     controllerType, xCount, yCount;
    uint8_t     numConfigParams;
    uint8_t     numStatusValues;
};
typedef struct zxy100Version_t Zxy100VersionData;

struct zxy100_touch_data_t
{
    uint8_t     flags;
    uint8_t     contact_id;
    uint16_t    x;
    uint16_t    y;
};
typedef struct zxy100_touch_data_t Zxy100TouchReport;

struct zxy100_raw_data_t
{
    uint8_t blocksExpected, blocksReceived; // blocks of 62 bytes
    uint8_t firstYIndex, reserved1;
    uint8_t wireValue[ZXY100_MAX_WIRES];
};
typedef struct zxy100_raw_data_t Zxy100RawData;

#endif  // _ZXY100_H
