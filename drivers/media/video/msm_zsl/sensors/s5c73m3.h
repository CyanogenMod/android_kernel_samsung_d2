/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __S5C73M3_H__
#define __S5C73M3_H__

//#define CONFIG_S5C73M3

#include <linux/types.h>
#include <mach/board.h>

/*#define DEBUG_LEVEL_HIGH */
//#define DEBUG_LEVEL_MID

/* #define DEBUG_CAM_I2C */

#if defined(DEBUG_LEVEL_HIGH)
#define CAM_DBG_H(fmt, arg...)\
	do {					\
		printk(KERN_ERR "[%s:%d] " fmt,	\
			__func__, __LINE__, ##arg);		\
	}							\
	while (0)
#define CAM_DBG_M(fmt, arg...)\
	do {					\
		printk(KERN_ERR "[%s:%d] " fmt,	\
			__func__, __LINE__, ##arg);		\
	}							\
	while (0)
#elif defined(DEBUG_LEVEL_MID)
#define CAM_DBG_H(fmt, arg...)
#define CAM_DBG_M(fmt, arg...)\
	do {					\
		printk(KERN_ERR "[%s:%d] " fmt,	\
			__func__, __LINE__, ##arg);		\
	}							\
	while (0)
#else
#define CAM_DBG_H(fmt, arg...)
#define CAM_DBG_M(fmt, arg...)
#endif

#if defined(DEBUG_CAM_I2C)
#define cam_i2c_dbg(fmt, arg...)	\
	do {					\
		printk(KERN_ERR "[%s : % d] " fmt,	\
			__func__, __LINE__, ##arg);		\
	}							\
	while (0)
#else
#define cam_i2c_dbg(fmt, arg...)
#endif

#define cam_err(fmt, arg...)	\
	do {					\
		printk(KERN_ERR "[%s : % d] " fmt,	\
			__func__, __LINE__, ##arg);		\
	}							\
	while (0)

#define CAPTURE_FLASH	1
#define MOVIE_FLASH		0

#define S5C73M3_FW_VER_LEN		6
#define S5C73M3_FW_VER_FILE_CUR		0x60
/* level at or below which we need to enable flash when in auto mode */
#define LOW_LIGHT_LEVEL		0x20


/* DTP */
#define DTP_OFF		0
#define DTP_ON		1
#define DTP_OFF_ACK		2
#define DTP_ON_ACK		3

struct s5c73m3_userset {
	unsigned int focus_mode;
	unsigned int focus_status;
	unsigned int continuous_af;

	unsigned int	metering;
	unsigned int	exposure;
	unsigned int		wb;
	unsigned int		iso;
	int	contrast;
	int	saturation;
	int	sharpness;
	int	brightness;
	int	scene;
	unsigned int zoom;
	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int scenemode;
	unsigned int detectmode;
	unsigned int antishake;
	unsigned int fps;
	unsigned int flash_mode;
	unsigned int flash_state;

	unsigned int stabilize;	/* IS */

	unsigned int strobe;
	unsigned int jpeg_quality;
	/*unsigned int preview_size;*/
	/*struct m5mo_preview_size preview_size;*/
	unsigned int preview_size_idx;
	unsigned int capture_size;
	unsigned int thumbnail_size;


};


/*extern struct s5c73m3_reg s5c73m3_regs;*/
struct reg_struct_init {
	/* PLL setting */
	uint8_t pre_pll_clk_div; /* 0x0305 */
	uint8_t plstatim; /* 0x302b */
	uint8_t reg_3024; /*ox3024*/
	uint8_t image_orientation;  /* 0x0101*/
	uint8_t vndmy_ablmgshlmt; /*0x300a*/
	uint8_t y_opbaddr_start_di; /*0x3014*/
	uint8_t reg_0x3015; /*0x3015*/
	uint8_t reg_0x301c; /*0x301c*/
	uint8_t reg_0x302c; /*0x302c*/
	uint8_t reg_0x3031; /*0x3031*/
	uint8_t reg_0x3041; /* 0x3041 */
	uint8_t reg_0x3051; /* 0x3051 */
	uint8_t reg_0x3053; /* 0x3053 */
	uint8_t reg_0x3057; /* 0x3057 */
	uint8_t reg_0x305c; /* 0x305c */
	uint8_t reg_0x305d; /* 0x305d */
	uint8_t reg_0x3060; /* 0x3060 */
	uint8_t reg_0x3065; /* 0x3065 */
	uint8_t reg_0x30aa; /* 0x30aa */
	uint8_t reg_0x30ab;
	uint8_t reg_0x30b0;
	uint8_t reg_0x30b2;
	uint8_t reg_0x30d3;
	uint8_t reg_0x3106;
	uint8_t reg_0x310c;
	uint8_t reg_0x3304;
	uint8_t reg_0x3305;
	uint8_t reg_0x3306;
	uint8_t reg_0x3307;
	uint8_t reg_0x3308;
	uint8_t reg_0x3309;
	uint8_t reg_0x330a;
	uint8_t reg_0x330b;
	uint8_t reg_0x330c;
	uint8_t reg_0x330d;
	uint8_t reg_0x330f;
	uint8_t reg_0x3381;
};

struct reg_struct {
	uint8_t pll_multiplier; /* 0x0307 */
	uint8_t frame_length_lines_hi; /* 0x0340*/
	uint8_t frame_length_lines_lo; /* 0x0341*/
	uint8_t y_addr_start;  /* 0x347 */
	uint8_t y_add_end;  /* 0x034b */
	uint8_t x_output_size_msb;  /* 0x034c */
	uint8_t x_output_size_lsb;  /* 0x034d */
	uint8_t y_output_size_msb; /* 0x034e */
	uint8_t y_output_size_lsb; /* 0x034f */
	uint8_t x_even_inc;  /* 0x0381 */
	uint8_t x_odd_inc; /* 0x0383 */
	uint8_t y_even_inc;  /* 0x0385 */
	uint8_t y_odd_inc; /* 0x0387 */
	uint8_t hmodeadd;   /* 0x3001 */
	uint8_t vmodeadd;   /* 0x3016 */
	uint8_t vapplinepos_start;/*ox3069*/
	uint8_t vapplinepos_end;/*306b*/
	uint8_t shutter;	/* 0x3086 */
	uint8_t haddave;	/* 0x30e8 */
	uint8_t lanesel;    /* 0x3301 */
};

struct s5c73m3_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
};

enum s5c73m3_test_mode_t {
	TEST_OFF,
	TEST_1,
	TEST_2,
	TEST_3
};

struct s5c73m3_focus {
	unsigned int mode;
	unsigned int caf_mode;
	unsigned int lock;
	unsigned int status;
	unsigned int touch;
	unsigned int pos_x;
	unsigned int pos_y;
};

struct s5c73m3_fw_version {
	unsigned int index;
	unsigned int opened;
	char path[25];
	char ver[10];
};

enum s5c73m_flash_mode {
	MAIN_CAMERA_FLASH_OFF,
	MAIN_CAMERA_FLASH_ON,
	MAIN_CAMERA_FLASH_AUTO,
	MAIN_CAMERA_FLASH_TORCH,
	MAIN_CAMERA_FLASH_MAX
};

enum s5c73m_fd_mode {
	FACE_DETECTION_OFF,
	FACE_DETECTION_ON,
	FACE_DETECTION_MAX
};

enum s5c73m_jpeg_size {
	JPEG_3264_2448,
	JPEG_3264_1836,
	JPEG_2048_1536,
	JPEG_2048_1152,
	JPEG_1600_1200,
	JPEG_1280_720,
	JPEG_640_480
};

enum s5c73m3_isneed_flash_tristate {
	S5C73M3_ISNEED_FLASH_OFF = 0x00,
	S5C73M3_ISNEED_FLASH_ON = 0x01,
	S5C73M3_ISNEED_FLASH_UNDEFINED = 0x02,
};

enum s5c73m3_wdr_mode {
	WDR_OFF,
	WDR_ON,
	WDR_MAX,
};

struct s5c73m3_focus camera_focus;
enum s5c73m_flash_mode flash_mode;
char isflash;
unsigned int isPreflashFired;

enum s5c73m3_resolution_t {
	QTR_SIZE,
	FULL_SIZE,
	INVALID_SIZE
};
enum s5c73m3_setting {
	RES_PREVIEW,
	RES_CAPTURE
};
enum mt9p012_reg_update {
	/* Sensor egisters that need to be updated during initialization */
	REG_INIT,
	/* Sensor egisters that needs periodic I2C writes */
	UPDATE_PERIODIC,
	/* All the sensor Registers will be updated */
	UPDATE_ALL,
	/* Not valid update */
	UPDATE_INVALID
};

enum s5c73m3_fw_path {
	S5C73M3_SD_CARD,
	S5C73M3_IN_DATA,
	S5C73M3_IN_SYSTEM,
	S5C73M3_PATH_MAX,
};


#define S5C73M3_IMG_OUTPUT	0x0902
#define S5C73M3_HDR_OUTPUT	0x0008
#define S5C73M3_YUV_OUTPUT	0x0009
#define S5C73M3_INTERLEAVED_OUTPUT	0x000D
#define S5C73M3_HYBRID_OUTPUT	0x0016

#define S5C73M3_STILL_PRE_FLASH			0x0A00
#define S5C73M3_STILL_PRE_FLASH_FIRE		0x0000
#define S5C73M3_STILL_PRE_FLASH_NON_FIRED	0x0000
#define S5C73M3_STILL_PRE_FLASH_FIRED		0x0001

#define S5C73M3_STILL_MAIN_FLASH		0x0A02
#define S5C73M3_STILL_MAIN_FLASH_CANCEL		0x0001
#define S5C73M3_STILL_MAIN_FLASH_FIRE		0x0002


#define S5C73M3_ZOOM_STEP	0x0B00


#define S5C73M3_IMAGE_EFFECT		0x0B0A
#define S5C73M3_IMAGE_EFFECT_NONE	0x0001
#define S5C73M3_IMAGE_EFFECT_NEGATIVE	0x0002
#define S5C73M3_IMAGE_EFFECT_AQUA	0x0003
#define S5C73M3_IMAGE_EFFECT_SEPIA	0x0004
#define S5C73M3_IMAGE_EFFECT_MONO	0x0005
#define S5C73M3_IMAGE_EFFECT_SKETCH	0x0006
#define S5C73M3_IMAGE_EFFECT_WASHED	0x0007
#define S5C73M3_IMAGE_EFFECT_VINTAGE_WARM	0x0008
#define S5C73M3_IMAGE_EFFECT_VINTAGE_COLD	0x0009
#define S5C73M3_IMAGE_EFFECT_SOLARIZE	0x000A
#define S5C73M3_IMAGE_EFFECT_POSTERIZE	0x000B
#define S5C73M3_IMAGE_EFFECT_POINT_COLOR_1	0x000C
#define S5C73M3_IMAGE_EFFECT_POINT_COLOR_2	0x000D
#define S5C73M3_IMAGE_EFFECT_POINT_COLOR_3	0x000E
#define S5C73M3_IMAGE_EFFECT_POINT_COLOR_4	0x000F
#define S5C73M3_IMAGE_EFFECT_CARTOONIZE	0x001A

#define S5C73M3_IMAGE_QUALITY		0x0B0C
#define S5C73M3_IMAGE_QUALITY_SUPERFINE	0x0000
#define S5C73M3_IMAGE_QUALITY_FINE	0x0001
#define S5C73M3_IMAGE_QUALITY_NORMAL	0x0002
#define S5C73M3_IMAGE_QUALITY_LOW	0x0003

#define S5C73M3_FLASH_MODE		0x0B0E
#define S5C73M3_FLASH_MODE_OFF		0x0000
#define S5C73M3_FLASH_MODE_ON		0x0001
#define S5C73M3_FLASH_MODE_AUTO		0x0002

#define S5C73M3_FLASH_STATUS		0x0B80
#define S5C73M3_FLASH_STATUS_OFF	0x0001
#define S5C73M3_FLASH_STATUS_ON		0x0002
#define S5C73M3_FLASH_STATUS_AUTO	0x0003

#define S5C73M3_FLASH_TORCH		0x0B12
#define S5C73M3_FLASH_TORCH_OFF		0x0000

#if defined(CONFIG_MACH_M2_DCM)
#define S5C73M3_FLASH_TORCH_ON		0x0004
#else
#define S5C73M3_FLASH_TORCH_ON		0x0001
#endif

#define S5C73M3_AE_ISNEEDFLASH		0x0CBA
#define S5C73M3_AE_ISNEEDFLASH_OFF	0x0000
#define S5C73M3_AE_ISNEEDFLASH_ON	0x0001


#define S5C73M3_CHG_MODE		0x0B10
#define S5C73M3_YUV_MODE		0x8000
#define S5C73M3_INTERLEAVED_MODE	0x8000
#define S5C73M3_CHG_MODE_YUV_320_240	0x8001
#define S5C73M3_CHG_MODE_YUV_640_480		0x8002
#define S5C73M3_CHG_MODE_YUV_880_720		0x8003
#define S5C73M3_CHG_MODE_YUV_960_720		0x8004
#define S5C73M3_CHG_MODE_YUV_1184_666		0x8005
#define S5C73M3_CHG_MODE_YUV_1280_720	0x8006
#define S5C73M3_CHG_MODE_YUV_1280_960	0x8007
#define S5C73M3_CHG_MODE_YUV_1600_1200	0x8008
#define S5C73M3_CHG_MODE_YUV_1632_1224	0x8009
#define S5C73M3_CHG_MODE_YUV_1920_1080	0x800A
#define S5C73M3_CHG_MODE_YUV_1920_1440	0x800B
#define S5C73M3_CHG_MODE_YUV_2304_1296	0x800C
#define S5C73M3_CHG_MODE_YUV_2304_1728	0x800D
#define S5C73M3_CHG_MODE_JPEG_640_480	0x0010
#define S5C73M3_CHG_MODE_JPEG_800_450	0x0020
#define S5C73M3_CHG_MODE_JPEG_800_600	0x0030
#define S5C73M3_CHG_MODE_JPEG_1280_720		0x0040
#define S5C73M3_CHG_MODE_JPEG_1280_960		0x0050
#define S5C73M3_CHG_MODE_JPEG_1600_960		0x0060
#define S5C73M3_CHG_MODE_JPEG_1600_1200	0x0070
#define S5C73M3_CHG_MODE_JPEG_2048_1152	0x0080
#define S5C73M3_CHG_MODE_JPEG_2048_1536	0x0090
#define S5C73M3_CHG_MODE_JPEG_2560_1440	0x00A0
#define S5C73M3_CHG_MODE_JPEG_2560_1920	0x00B0
#define S5C73M3_CHG_MODE_JPEG_3072_1728	0x00C0
#define S5C73M3_CHG_MODE_JPEG_3264_2304	0x00D0
#define S5C73M3_CHG_MODE_JPEG_3264_1836	0x00E0
#define S5C73M3_CHG_MODE_JPEG_3264_2448	0x00F0


#define S5C73M3_AF_CON			0x0E00
#define S5C73M3_AF_CON_STOP		0x0000
#define S5C73M3_AF_CON_SCAN		0x0001/*AF_SCAN:Full Search*/
#define S5C73M3_AF_CON_START	0x0002/*AF_START:Fast Search*/

#define S5C73M3_AF_STATUS		0x5E80

#define S5C73M3_AF_TOUCH_AF		0x0E0A

#define S5C73M3_AF_CAL			0x0E06

#define S5C73M3_CAF_STATUS_FIND_SEARCHING_DIR	0x0001
#define S5C73M3_CAF_STATUS_FOCUSING	0x0002
#define S5C73M3_CAF_STATUS_FOCUSED	0x0003
#define S5C73M3_CAF_STATUS_INITIALIZE	0x0004

#define S5C73M3_AF_STATUS_INVALID	0x0010
#define S5C73M3_AF_STATUS_FOCUSING	0x0020
#define S5C73M3_AF_STATUS_FOCUSED	0x0030/*SUCCESS*/
#define S5C73M3_AF_STATUS_UNFOCUSED	0x0040/*FAIL*/

#define S5C73M3_AF_TOUCH_POSITION	0x5E8E

#define S5C73M3_AF_FACE_ZOOM	0x0E10

#define S5C73M3_AF_MODE			0x0E02
#define S5C73M3_AF_MODE_NORMAL		0x0000
#define S5C73M3_AF_MODE_MACRO		0x0001
#define S5C73M3_AF_MODE_MOVIE_CAF_START	0x0002
#define S5C73M3_AF_MODE_MOVIE_CAF_STOP		0x0003
#define S5C73M3_AF_MODE_PREVIEW_CAF_START	0x0004
#define S5C73M3_AF_MODE_PREVIEW_CAF_STOP	0x0005

#define S5C73M3_AF_SOFTLANDING		0x0E16
#define S5C73M3_AF_SOFTLANDING_ON	0x0000

#define S5C73M3_FACE_DET		0x0E0C
#define S5C73M3_FACE_DET_OFF		0x0000
#define S5C73M3_FACE_DET_ON		0x0001

#define S5C73M3_FACE_DET_OSD		0x0E0E
#define S5C73M3_FACE_DET_OSD_OFF	0x0000
#define S5C73M3_FACE_DET_OSD_ON		0x0001

#define S5C73M3_AE_CON		0x0C00
#define S5C73M3_AE_STOP		0x0000/*LOCK*/
#define S5C73M3_AE_START	0x0001/*UNLOCK*/

#define S5C73M3_ISO		0x0C02
#define S5C73M3_ISO_AUTO	0x0000
#define S5C73M3_ISO_100		0x0001
#define S5C73M3_ISO_200		0x0002
#define S5C73M3_ISO_400		0x0003
#define S5C73M3_ISO_800		0x0004
#define S5C73M3_ISO_SPORTS	0x0005
#define S5C73M3_ISO_NIGHT	0x0006
#define S5C73M3_ISO_INDOOR	0x0007

#define S5C73M3_EV		0x0C04
#define S5C73M3_EV_M20		0x0000
#define S5C73M3_EV_M15		0x0001
#define S5C73M3_EV_M10		0x0002
#define S5C73M3_EV_M05		0x0003
#define S5C73M3_EV_ZERO		0x0004
#define S5C73M3_EV_P05		0x0005
#define S5C73M3_EV_P10		0x0006
#define S5C73M3_EV_P15		0x0007
#define S5C73M3_EV_P20		0x0008

#define S5C73M3_METER		0x0C06
#define S5C73M3_METER_CENTER	0x0000
#define S5C73M3_METER_SPOT	0x0001
#define S5C73M3_METER_AVERAGE	0x0002
#define S5C73M3_METER_SMART	0x0003

#define S5C73M3_WDR		0x0C08
#define S5C73M3_WDR_OFF		0x0000
#define S5C73M3_WDR_ON		0x0001

#define S5C73M3_FLICKER_MODE	0x0C12
#define S5C73M3_FLICKER_NONE	0x0000
#define S5C73M3_FLICKER_MANUAL_50HZ	0x0001
#define S5C73M3_FLICKER_MANUAL_60HZ	0x0002
#define S5C73M3_FLICKER_AUTO	0x0003
#define S5C73M3_FLICKER_AUTO_50HZ	0x0004
#define S5C73M3_FLICKER_AUTO_60HZ	0x0005

#define S5C73M3_AE_MODE	0x0C1E
#define S5C73M3_AUTO_MODE_AE_SET	0x0000
#define S5C73M3_FIXED_30FPS	0x0002
#define S5C73M3_FIXED_20FPS	0x0003
#define S5C73M3_FIXED_15FPS	0x0004
#define S5C73M3_FIXED_7FPS	0x0009
#define S5C73M3_ANTI_SHAKE_ON	0x0013

#define S5C73M3_SHARPNESS	0x0C14
#define S5C73M3_SHARPNESS_0	0x0000
#define S5C73M3_SHARPNESS_1	0x0001
#define S5C73M3_SHARPNESS_2	0x0002
#define S5C73M3_SHARPNESS_M1	0x0003
#define S5C73M3_SHARPNESS_M2	0x0004

#define S5C73M3_SATURATION	0x0C16
#define S5C73M3_SATURATION_0	0x0000
#define S5C73M3_SATURATION_1	0x0001
#define S5C73M3_SATURATION_2	0x0002
#define S5C73M3_SATURATION_M1	0x0003
#define S5C73M3_SATURATION_M2	0x0004

#define S5C73M3_CONTRAST	0x0C18
#define S5C73M3_CONTRAST_0	0x0000
#define S5C73M3_CONTRAST_1	0x0001
#define S5C73M3_CONTRAST_2	0x0002
#define S5C73M3_CONTRAST_M1	0x0003
#define S5C73M3_CONTRAST_M2	0x0004

#define S5C73M3_SCENE_MODE		0x0C1A
#define S5C73M3_SCENE_MODE_NONE		0x0000
#define S5C73M3_SCENE_MODE_PORTRAIT	0x0001
#define S5C73M3_SCENE_MODE_LANDSCAPE	0x0002
#define S5C73M3_SCENE_MODE_SPORTS	0x0003
#define S5C73M3_SCENE_MODE_INDOOR	0x0004
#define S5C73M3_SCENE_MODE_BEACH	0x0005
#define S5C73M3_SCENE_MODE_SUNSET	0x0006
#define S5C73M3_SCENE_MODE_DAWN		0x0007
#define S5C73M3_SCENE_MODE_FALL		0x0008
#define S5C73M3_SCENE_MODE_NIGHT	0x0009
#define S5C73M3_SCENE_MODE_AGAINSTLIGHT	0x000A
#define S5C73M3_SCENE_MODE_FIRE		0x000B
#define S5C73M3_SCENE_MODE_TEXT		0x000C
#define S5C73M3_SCENE_MODE_CANDLE	0x000D

#define S5C73M3_FIREWORK_CAPTURE	0x0C20
#define S5C73M3_NIGHTSHOT_CAPTURE	0x0C22

#define S5C73M3_AE_AUTO_BRAKET		0x0B14
#define S5C73M3_AE_AUTO_BRAKET_EV05	0x0080
#define S5C73M3_AE_AUTO_BRAKET_EV10	0xC100
#define S5C73M3_AE_AUTO_BRAKET_EV15	0x0180
#define S5C73M3_AE_AUTO_BRAKET_EV20	0x8200

#define S5C73M3_LLS_MODE                0x0C2C
#define S5C73M3_LLS_MODE_ON             0x0001
#define S5C73M3_LLS_MODE_OFF            0x0000

#define S5C73M3_SENSOR_STREAMING	0x090A
#define S5C73M3_SENSOR_STREAMING_OFF	0x0000
#define S5C73M3_SENSOR_STREAMING_ON	0x0001

#define S5C73M3_AWB_MODE		0x0D02
#define S5C73M3_AWB_MODE_INCANDESCENT	0x0000
#define S5C73M3_AWB_MODE_FLUORESCENT1	0x0001
#define S5C73M3_AWB_MODE_FLUORESCENT2	0x0002
#define S5C73M3_AWB_MODE_DAYLIGHT	0x0003
#define S5C73M3_AWB_MODE_CLOUDY		0x0004
#define S5C73M3_AWB_MODE_AUTO		0x0005

#define S5C73M3_AWB_CON			0x0D00
#define S5C73M3_AWB_STOP		0x0000/*LOCK*/
#define S5C73M3_AWB_START		0x0001/*UNLOCK*/

#define S5C73M3_HYBRID_CAPTURE	0x0996

/* S5C73M3 Sensor Mode */
#define S5C73M3_SYSINIT_MODE	0x0
#define S5C73M3_PARMSET_MODE	0x1
#define S5C73M3_MONITOR_MODE	0x2
#define S5C73M3_STILLCAP_MODE	0x3

#define S5C73M3_STATUS			0x5080
#define S5C73M3_I2C_ERR_STATUS	0x599E
#define S5C73M3_I2C_SEQ_STATUS	0x59A6
#define ERROR_STATUS_CHECK_BIN_CRC    (1<<0x8)

static const u32 S5C73M3_INIT[] = {
0x00500009,
0x00545000,
0x0F140B08,
0x0F140000,
0x0F140900,
0x0F140403, /*640MHz*/
0x00545080,
0x0F140002
};

/*
MIPI_BIT_RATE_360MHz=0,
MIPI_BIT_RATE_450MHz=1,
MIPI_BIT_RATE_540MHz=2,
MIPI_BIT_RATE_640MHz=3,
MIPI_BIT_RATE_720MHz=4,
MIPI_BIT_RATE_750MHz=5,
*/
#ifndef CONFIG_S5C73M3
static u32 S5C73M3_YUV_PREVIEW[] = {
	0x00500009,
	0x00545000,
	0x0F140902,
	0x0F140009,
	0x0F140900,		/* MIPI OUTPUT CLK */
	0x0F140401,		/* MIPI_BIT_RATE_450MHz=1 */
	0x0F140B10,
	0x0F148004,
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140004,
};
#endif
static u32 S5C73M3_HDR[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /* MiPi 0xetting */
	0x0F140403, /* Lane:4 , DataRate:3(640Mbp0x) */
	0x0F140902, /* Change Out interface */
	0x0F140014, /* Image Out Mode :D(interleave)0x14VC */
	0x0F140B10,
	0x0F14801D, /* (1:640x480 JPEG  D:3264 2448 YUV) */
	0x00545080,
	0x0F140003,
};

static u32 S5C73M3_LLS[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /* MiPi 0xetting */
	0x0F140403, /* Lane:4 , DataRate:3(640Mbp0x) */
	0x0F140902, /* Change Out interface */
	0x0F140014, /* Image Out Mode :D(interleave)0x14VC */
	0x0F140B10,
	0x0F14801D, /* (1:640x480 JPEG  D:3264 2448 YUV) */
	0x00545080,
	0x0F140003,
};

static u32 S5C73M3_FHD_VDIS[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140003, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F14811C,  /*E:3264X1836 JPEG, C:2304 x 1296 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 S5C73M3_FHD[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F1481EA,  /*E:3264X1836 JPEG, A:1920 x 1080 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 S5C73M3_HD_VDIS[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F148117,  /*E:3264X1836 JPEG, 7: YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 S5C73M3_HD[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F1481E6,  /*E:3264X1836 JPEG, 6:1280X720 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 S5C73M3_WVGA[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F1481CF,  /*C:3264X2176 JPEG, F:1008X672 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 S5C73M3_VGA[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F1481F2,  /*F:3264X2448 JPEG, 2:640 x 480 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 S5C73M3_PREVIEW[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x00545080,
	0x0F140003,
};

/* Below Not used settings will be removed later */
#ifndef CONFIG_S5C73M3
static u32 S5C73M3_INTERLEAVED_PREVIEW[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140401, /*450MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140003, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F1480F4,  /*F:3264X2448 JPEG, 4:960X720 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 CAM_IL_PREVIEW_8M[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F1480F4,  /*F:3264X2448 JPEG, 4:960X720 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 CAM_IL_PREVIEW_6M[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F1480E6,  /*E:3264X1836 JPEG, 6:1280X720 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 CAM_IL_PREVIEW_3_2M[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F148094,  /*9:2048X1536 JPEG, 4:960X720 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 CAM_IL_PREVIEW_2_4M[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F148086,  /*8:2048X1156 JPEG, 6:1280X720 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 CAM_IL_PREVIEW_2M[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F148074,  /*7:1600X1200 JPEG, 4:960X720 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 CAM_IL_PREVIEW_1M[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F148046,  /*4:1280X720 JPEG, 6:1280X720 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 CAM_IL_PREVIEW_VGA[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F148014,  /*1:640X480 JPEG, 4:960X720 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 S5C73M3_INTERLEAVED_PREVIEW_HD[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F1480F6,  /*F:3264X2448 JPEG, 6:1280X720 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 CAM_IL_PREVIEW_STND[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F1481FA,  /*F:3264X2448 JPEG, A:1920 x 1080 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 CAM_IL_PREVIEW_WIDE[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140403, /*640MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F1481EA,  /*E:3264X1836 JPEG, A:1920 x 1080 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

static u32 S5C73M3_INTERLEAVED_PREVIEW_30[] = {
	0x00500009,
	0x00545000,
	0x0F140900, /*MIPI Setting*/
	0x0F140402, /*540MHz*/
	0x0F140902, /*Change out interleave*/
	0x0F140014, /*Image out mode*/
	0x0F14091A, /*ISP PCLK SET*/
	0x0F140004, /*0:27, 1:129.6, 2:140.4, 3:194.4, 4:264*/
	0x0F140B10,
	0x0F148074,  /*F:1600X1200 JPEG, 4:960X720 YUV*/
	0x0F14090A,
	0x0F140001,
	0x00545080,
	0x0F140005,
};

/* MIPI 4lane(540Mhz),30fps, YUV 1920x1080(FHD), JPEG : 1600x1200(2M) */
static u32 S5C73M3_INTERLEAVED_CAMCORDER_1[] = {
	0x00500009,
	0x00545000,
	0x0F140900,	/* MIPI Setting */
	0x0F140402,	/* Mipi Lane 4:4Lane,
	DataRate(1:450Mbps, 2:540Mbps 3:640Mbps, 4:720Mbps) */
	0x0F140902,	/* Change Out interface */
	0x0F140014,	/* Image Out Mode :14
	(For Qualcomm VC Interleaved) */
	0x0F14091A,	/* ISP PCLK Set */
	0x0F140004,	/* Mhz 0:27   1:129.6
	2:140   3:194.4   4:264Mhz */
	0x0F140B10,	/* Mode Change */
	0x0F14807A,	/* 7:1600x1200 JPEG   A:1920x1080 YUV */
	0x0F140C1E,	/* Frame rate set */
	0x0F140002,	/* 0 : 30~15f/s  1:24~30f/s 2:30f/s
	3:20f/s   4:15f/s fixed  5:24f/s  6:27f/s */
	0x0F14090A,	/* Sensor Stream control */
	0x0F140001,	/* 0 : Stop  1:Start */
	0x00545080,	/* Run I2C Function */
	0x0F140006,
};

/* MIPI 4lane(540Mhz),30fps, YUV 1920x1080(FHD), JPEG : 2560x1920 */
static u32 S5C73M3_INTERLEAVED_CAMCORDER_2[] = {
	0x00500009,
	0x00545000,
	0x0F140900,	/* MIPI Setting */
	0x0F140402,	/* Mipi Lane 4:4Lane,
	DataRate(1:450Mbps, 2:540Mbps 3:640Mbps, 4:720Mbps) */
	0x0F140902,	/* Change Out interface */
	0x0F140014,	/* Image Out Mode :14 (For Qualcomm VC Interleaved) */
	0x0F14091A,	/* ISP PCLK Set */
	0x0F140004,	/* Mhz 0:27   1:129.6
	2:140   3:194.4   4:264Mhz */
	0x0F140B10,	/* Mode Change */
	0x0F1480BA,	/* B:2560x1920 JPEG   A:1920x1080 YUV */
	0x0F140C1E,	/* Frame rate set */
	0x0F140002,	/* 0 : 30~15f/s  1:24~30f/s 2:30f/s
	3:20f/s   4:15f/s fixed  5:24f/s  6:27f/s */
	0x0F14090A,	/* Sensor Stream control */
	0x0F140001,	/* 0 : Stop  1:Start */
	0x00545080,	/* Run I2C Function */
	0x0F140006,
};

/* MIPI 4lane(540Mhz),30fps, YUV 1920x1080(FHD), JPEG : 3264x1836(6M wide) */
static u32 S5C73M3_INTERLEAVED_CAMCORDER_3[] = {
	0x00500009,
	0x00545000,
	0x0F140900,	/* MIPI Setting */
	0x0F140402,	/* Mipi Lane 4:4Lane,
	DataRate(1:450Mbps, 2:540Mbps 3:640Mbps, 4:720Mbps) */
	0x0F140902,	/* Change Out interface */
	0x0F140014,	/* Image Out Mode :14 (For Qualcomm VC Interleaved) */
	0x0F14091A,	/* ISP PCLK Set */
	0x0F140004,	/* Mhz 0:27   1:129.6
	2:140   3:194.4   4:264Mhz */
	0x0F140B10,	/* Mode Change */
	0x0F1480EA,	/* E:3264x1836 JPEG   A:1920x1080 YUV */
	0x0F140C1E,	/* Frame rate set */
	0x0F140002,	/* 0 : 30~15f/s  1:24~30f/s 2:30f/s
	3:20f/s   4:15f/s fixed  5:24f/s  6:27f/s */
	0x0F14090A,	/* Sensor Stream control */
	0x0F140001,	/* 0 : Stop  1:Start */
	0x00545080,	/* Run I2C Function */
	0x0F140006,
};

/* MIPI 4lane(540Mhz),30fps, YUV 1920x1080(FHD), JPEG : 3264x2448(8M wide) */
static u32 S5C73M3_INTERLEAVED_CAMCORDER_4[] = {
	0x00500009,
	0x00545000,
	0x0F140900,	/* MIPI Setting */
	0x0F140402,	/* Mipi Lane 4:4Lane,
	DataRate(1:450Mbps, 2:540Mbps 3:640Mbps, 4:720Mbps) */
	0x0F140902,	/* Change Out interface */
	0x0F140014,	/* Image Out Mode :14 (For Qualcomm VC Interleaved) */
	0x0F14091A,	/* ISP PCLK Set */
	0x0F140004,	/* Mhz 0:27   1:129.6
	2:140   3:194.4   4:264Mhz */
	0x0F140B10,	/* Mode Change */
	0x0F1480FA,	/* F:3264x2448 JPEG   A:1920x1080 YUV */
	0x0F140C1E,	/* Frame rate set */
	0x0F140002,	/* 0 : 30~15f/s  1:24~30f/s 2:30f/s
	3:20f/s   4:15f/s fixed  5:24f/s  6:27f/s */
	0x0F14090A,	/* Sensor Stream control */
	0x0F140001,	/* 0 : Stop  1:Start */
	0x00545080,	/* Run I2C Function */
	0x0F140006,
};

static u32 S5C73M3_OTP_CONTROL[] = {
0xFCFC3310,
0x00503800,
0x0054A004,
0x0F140000,
0x0054A000,
0x0F140004,
0x0054A0D8,
0x0F140000,
0x0054A0DC,
0x0F140004,
0x0054A0C4,
0x0F144000,
0x0054A0D4,
0x0F140015,
0x0054A000,
0x0F140001,
0x0054A0B4,
0x0F149F90,
0x0054A09C,
0x0F149A95,
};

static u32 S5C73M3_OTP_PAGE[] = {
0x0054A0C4,
0x0F144800,
0x0054A0C4,
0x0F144400,
0x0054A0C4,
0x0F144200,
0x0054A004,
0x0F1400C0,
0x0054A000,
0x0F140001,
};
#endif
extern int s5c73m3_spi_read(u8 *buf, size_t len, const int rxSize);
extern int s5c73m3_spi_write(const u8 *addr, const int len, const int txSize);
extern int s5c73m3_spi_init(void);
extern void print_ldos(void);
#endif /* __S5C73M3_H__ */
