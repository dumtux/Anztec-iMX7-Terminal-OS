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
 *  Provide specific ZXY110 values
 *
 *  Reference, ZXY110 Protocol Doc, March 2015, Version 1.09.
 */

#ifndef _ZXY110_H
#define _ZXY110_H



/**
 * Record that there are no private data fields in the ZXY110
 */
#define ZXY110_PRIVATE_STATUS_EXIST             (0)
#define ZXY110_PRIVATE_CONFIGS_EXIST            (0)

#define ZXY110_THRESHOLD_DEBOUNCE               (6)


// NB:
//      CI      := Configuration Item
//      SI      := Status Item/Value
//      FTPM    := First Touch Protect Mode
//      TL      := Top Left
//      BR      := Bottom Right

#define ZXY110_CI_DEVICE_MODE                   ( 0)
#define ZXY110_CI_SPARE_01                      ( 1)
#define ZXY110_CI_GLASS_THICKNESS               ( 2)
#define ZXY110_CI_LOWER_THRESHOLD               ( 3)
#define ZXY110_CI_UPPER_THRESHOLD               ( 4)
#define ZXY110_CI_INVERT_X                      ( 5)
#define ZXY110_CI_INVERT_Y                      ( 6)
#define ZXY110_CI_SWAP_XY                       ( 7)
#define ZXY110_CI_SENSOR_FLIPPED                ( 8)

#define ZXY110_CI_PIXEL_AVERAGING               ( 9)
#define ZXY110_CI_STAB_FACTOR                   (10)
#define ZXY110_CI_MIN_TOUCH_TIME                (11)
#define ZXY110_CI_MAX_TOUCH_TIME                (12)
#define ZXY110_CI_TX_TIMER_PERIOD               (13)
#define ZXY110_CI_POINT_CLICK_ENABLED           (14)
#define ZXY110_CI_RAW_DATA_MODE                 (15)
#define ZXY110_CI_FREQ_HOPPING_ENABLED          (16)
#define ZXY110_CI_FINE_TUNE_ENABLE              (17)
#define ZXY110_CI_DIGITAL_POT_VALUE             (18)
#define ZXY110_CI_CANDIDATE_COLUMN_THRESHOLD    (19)

#define ZXY110_CI_DYNAMIC_EQUALISE_THRESHOLD    (20)
#define ZXY110_CI_DYNAMIC_EQUALISE_RUNNING_TIME (21)
#define ZXY110_CI_DYNAMIC_EQUALISE_SCANS        (22)

#define ZXY110_CI_FAKE_QUIET_MODE_CHECK_WIDTH   (23)
#define ZXY110_CI_FAKE_QUIET_DETECT_VALUE       (24)
#define ZXY110_CI_SPARE_25                      (25)
#define ZXY110_CI_LIFT_OFF_DELAY                (26)
#define ZXY110_CI_NUMBER_RESAMPLE_COMMON_FILTER (27)
#define ZXY110_CI_MAX_RANGE_VALUE               (28)
#define ZXY110_CI_WEIGHT_COUNT_LIMIT_NO_TOUCH   (29)
#define ZXY110_CI_WEIGHT_COUNT_LIMIT_TOUCH      (30)
#define ZXY110_CI_NUMBER_OF_NOISY_WIRES         (31)

#define ZXY110_CI_FTPM_TIMER                    (32)
#define ZXY110_CI_FTPM_MIN_TOUCH_TIMER          (33)
#define ZXY110_CI_FTPM_WGT_COUNT_LIM_NO_TOUCH   (34)
#define ZXY110_CI_FTPM_WEIGHT_COUNT_LIM_TOUCH   (35)
#define ZXY110_CI_FAKE_TOUCH_VALUE_UPPER        (36)
#define ZXY110_CI_FAKE_TOUCH_VALUE_LOWER        (37)
#define ZXY110_CI_FAKE_TOUCH_COUNTER            (38)
#define ZXY110_CI_SKIP_NOISY_WIRE_ENABLED       (39)
#define ZXY110_CI_DEF_FREQ_RECOVERY_NOISE_LEVEL (40)
#define ZXY110_CI_DEF_FREQ_RECOVERY_TIME        (41)
#define ZXY110_CI_NOISE_MEASURE_RESAMPLE        (42)
#define ZXY110_CI_SPARE_43                      (43)
#define ZXY110_CI_PALM_REJECT_THRESHOLD         (44)


#define ZXY110_CI_ONBOARD_CAL_BASE              (45)            //  0x2d

#define ZXY110_CI_TOUCHCAL_TL_TARGET_X          (45)
#define ZXY110_CI_TOUCHCAL_TL_TARGET_Y          (46)
#define ZXY110_CI_TOUCHCAL_TL_MEASURED_X        (47)
#define ZXY110_CI_TOUCHCAL_TL_MEASURED_Y        (48)
#define ZXY110_CI_TOUCHCAL_BR_TARGET_X          (49)
#define ZXY110_CI_TOUCHCAL_BR_TARGET_Y          (50)
#define ZXY110_CI_TOUCHCAL_BR_MEASURED_X        (51)
#define ZXY110_CI_TOUCHCAL_BR_MEASURED_Y        (52)


/**
 * Provide names for Status value Indices
 *    SINCE := SINCE LAST REPORT
 *    FTM   := First Touch Mode
 *    WCNL  := WEIGHT_COUNT_NOISE_LIMIT
 *    DYNEQ := DYNAMIC_EQUALIZATION
 */
#define ZXY110_SI_NUM_STATUS_VALUES             ( 0)
#define ZXY110_SI_NUM_CONFIG_PARAMS             ( 1)
#define ZXY110_SI_NUM_TOUCH_SINCE               ( 2)
#define ZXY110_SI_POWER_UP_TIMER_LOWER          ( 3)  //Lower 16-bits
#define ZXY110_SI_POWER_UP_TIMER_UPPER          ( 4)  //Upper 16-bits
#define ZXY110_SI_NUM_FLASH_WRITES              ( 5)
#define ZXY110_SI_NUM_REPEATS                   ( 6)
#define ZXY110_SI_SCAN_MODE                     ( 7)
#define ZXY110_SI_SCAN_FREQ                     ( 8)
#define ZXY110_SI_DIGITAL_POT_VALUE             ( 9)
#define ZXY110_SI_DIVIDER_SETTING               (10)
#define ZXY110_SI_FTM_STATUS                    (11)
#define ZXY110_SI_SENSITIVITY_FACTOR            (12)
#define ZXY110_SI_NUM_FAKE_QUIET_MODE_DETECTED  (13)
#define ZXY110_SI_NUM_LIVE_TOUCHES              (14)
#define ZXY110_SI_NUM_FREQ_HOPPED               (15)
#define ZXY110_SI_CONFIG_RESISTOR_MASK          (16)
#define ZXY110_SI_FPS                           (17)
#define ZXY110_SI_SPARE_0                       (18)
#define ZXY110_SI_SPARE_1                       (19)
#define ZXY110_SI_WCNL_NO_TOUCH                 (20)
#define ZXY110_SI_WCNL_TOUCH                    (21)
#define ZXY110_SI_NUM_NOISY_WIRES               (22)
#define ZXY110_SI_SPARE_2                       (23)
#define ZXY110_SI_MAX_RANGE_VALUE               (24)
#define ZXY110_SI_NUM_FTM_ACTIVATIONS           (25)
#define ZXY110_SI_FTM_TIMER                     (26)
#define ZXY110_SI_DEDICATE_WIRE_INDEX           (27)
#define ZXY110_SI_LED_STATUS                    (28)
#define ZXY110_SI_NUMBER_FAKE_TOUCH_MODE        (29)
#define ZXY110_SI_DEFAULT_FREQ_RECOVERY_TIME    (30)
#define ZXY110_SI_DYNEQ_STATUS                  (31)
#define ZXY110_SI_DYNEQ_ON_TIME                 (32)
#define ZXY110_SI_DEFAULT_FREQ_NOISE_LEVEL      (33)
#define ZXY110_SI_FORCE_FREQ_SWITCH_TIME        (34)
#define ZXY110_SI_PALM_REJECT_SIGNAL            (35)
#define ZXY110_SI_NUM_EQUALIZATIONS             (36)
#define ZXY110_SI_PROCESSOR_ID_0                (37)
#define ZXY110_SI_PROCESSOR_ID_1                (38)
#define ZXY110_SI_PROCESSOR_ID_2                (39)
#define ZXY110_SI_PROCESSOR_ID_3                (40)
#define ZXY110_SI_PROCESSOR_ID_4                (41)
#define ZXY110_SI_PROCESSOR_ID_5                (42)
#define ZXY110_SI_NUM_PALM_REJECTIONS_SINCE     (43)
#define ZXY110_SI_OPTION_BITS                   (44)
#define ZXY110_SI_PRESSURE_SIGNAL               (45)


/**
 * provide counts of ConfigParam/StatusValue groups
 */

#define ZXY110_CN_ONBOARD_CAL_COUNT             (8)

#define ZXY110_CN_ACTIVE_AREA_COUNT             (0)     // NOT EXISTANT IN THE ZXY110 - TBC - FIXME

#define ZXY110_SI_NUM_PRIVATE_CONFIG_PARAMS     (0)     // NOT EXISTANT IN THE ZXY110
#define ZXY110_SI_NUM_PRIVATE_STATUS_PARAMS     (0)     // NOT EXISTANT IN THE ZXY110


// Bit definitions for ZXY110_SI_OPTION_BITS

/* ZXYMT_OPT_BIT_ACTIVE_AREAS_OLD               0x0001U     -- Touch Active Areas - never used */
#define ZXY110_OPT_BIT_ON_BOARD_CAL             0x0002U     // ZXY100,110,MT - OnBoard Controller Calibration
/* ZXYMT_OPT_BIT_SHOW_INACTIVE_CELLS            0x0004U     -- ZXYMT Only - Show inactive cells in raw view */
#define ZXY100_OPT_BIT_DISABLE_FLASH_WRITE      0x0008U     // ZXY100,110,MT - Protocol to DisableFlashWrite
/* ZXYMT_OPT_BIT_TOUCH_ACTIVE_AREA              0x0010U     // ZXYMT -- Touch Active Area - Used!  COLLISION !! */
#define ZXY100_OPT_BIT_ONLY_REPORT_IN_CAL_AREA  0x0010U     // ZXY100 - Ignore Touches Outside Calibrated Area
/* ZXYMT_OPT_BIT_FTM_ON_PERSIST                 0x0020U     -- ZXYMT Only - FTM persistent mechanism */


struct zxy110_sys_report_t {
    uint32_t    uptime;             // seconds
    uint16_t    zxy100DevMode;      // mouse, single or dual touch
    uint16_t    ftpmTimeout;
    uint16_t    scannerMode;
    uint16_t    ftmStatus;
    uint16_t    framesPerSecond;
    uint16_t    numNoiseFixEvents;
    uint16_t    numEqualizations;   //
    uint16_t    numTouchSinceLast;  //
    uint16_t    numFtmActivations;  //
    uint16_t    numFreqHopped;      //
    uint16_t    numNoisyWires;
    uint16_t    scanFreqKHz;
    uint16_t    digitalPotValue;
    uint16_t    cfg_DynEqScans;     //
    uint16_t    cfg_FtpmTimer;      //
};
typedef struct zxy110_sys_report_t Zxy110SysState;

#endif  // _ZXY110_H
