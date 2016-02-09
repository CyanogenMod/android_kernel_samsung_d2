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

#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <media/msm_camera.h>
#include <media/v4l2-subdev.h>
#include <mach/gpio.h>
#include <mach/camera.h>

#include <asm/mach-types.h>
#include <mach/vreg.h>
#include <linux/io.h>

#include "../msm.h"
#include "s5c73m3.h"
#include "../csi/msm_ispif.h"
#include <linux/vmalloc.h>
#include <linux/firmware.h>

#define SENSOR_NAME "s5c73m3"
#define PLATFORM_DRIVER_NAME "msm_camera_s5c73m3"

#define S5C73M3_FW_PATH		"/mnt/sdcard/SlimISP.bin"
#define S5C73M3_FW_REQUEST_PATH	"/system/cameradata/"
#define S5C73M3_FW_REQUEST_SECOND_PATH	"/data/"

#define MOVIEMODE_FLASH	17
#define FLASHMODE_FLASH	18

#define ERROR 1

//#define CONFIG_S5C73M3

#define IN_AUTO_MODE 1
#define IN_MACRO_MODE 2

#undef QC_TEST

/*#define YUV_PREVIEW*/
#define SPI_DMA_MODE

static uint32_t op_pixel_clk1;
const int camcorder_30fps = 30;

struct s5c73m3_work {
	struct work_struct work;
};

static struct  s5c73m3_work *s5c73m3_sensorw;
static struct  i2c_client *s5c73m3_client;

struct s5c73m3_ctrl {
	const struct msm_camera_sensor_info *sensordata;
	struct v4l2_subdev *sensor_dev;

	int op_mode;
	int dtp_mode;
	int app_mode;
	int vtcall_mode;
	int started;
	int isCapture;
	int flash_mode;
	int hdr_mode;
	int low_light_mode;
	int low_light_mode_size;
	int jpeg_size;
	int preview_size;
	int preview_size_width;
	int preview_size_height;
	int wide_preview;
	int i2c_write_check;
	int wb;
	int scene;
	int fps;
	bool isAeLock;
	bool isAwbLock;
	bool camcorder_mode;
	bool vdis_mode;
	bool anti_shake_mode;
	int record_size_width;
	int record_size_height;
	int fw_index;
	int zoomPreValue;

	u8 sensor_fw[10];
	u8 phone_fw[10];
	u32 crc_table[256];
	u32 sensor_size;
};

static unsigned int config_csi2;
static struct s5c73m3_ctrl *s5c73m3_ctrl;

struct s5c73m3_format {
	enum v4l2_mbus_pixelcode code;
	enum v4l2_colorspace colorspace;
	u16 fmt;
	u16 order;
};
static int32_t s5c73m3_sensor_setting(int update_type, int rt);
static int s5c73m3_set_touch_auto_focus(void);
static int s5c73m3_wait_ISP_status(void);
static int s5c73m3_set_fps(int fps);
static int s5c73m3_set_af_mode(int val);
static int s5c73m3_open_firmware_file(const char *filename,
	u8 *buf, u16 offset, u16 size);

void power_on_flash(void);

static DECLARE_WAIT_QUEUE_HEAD(s5c73m3_wait_queue);


static char *Fbuf; /*static QCTK*/
static char FW_buf[409600] = {0}; /*static QCTK 400KB*/

#define CHECK_ERR(x)	if ((x) < 0) { \
				cam_err("i2c failed, err %d\n", x); \
				return x; \
			}

static char fw_path[40] = {0};
static char fw_path_in_data[40] = {0};
struct s5c73m3_fw_version camfw_info[S5C73M3_PATH_MAX];

static int s5c73m3_i2c_write(unsigned short addr, unsigned short data)
{
	unsigned char buf[4];
	int i, err;
	int retry_count = 5;
	struct i2c_msg msg = {s5c73m3_client->addr, 0, sizeof(buf), buf};
	if (!s5c73m3_client->adapter) {
		cam_err("s5c73m3_client->adapter is not!!\n");
		return -EIO;
	}

	buf[0] = addr >> 8;
	buf[1] = addr & 0xff;
	buf[2] = data >> 8;
	buf[3] = data & 0xff;

	cam_i2c_dbg("addr %#x, data %#x\n", addr, data);

	for (i = retry_count; i; i--) {
		err = i2c_transfer(s5c73m3_client->adapter, &msg, 1);
		if (err == 1)
			break;

		msleep(20);
	}

	if (err != 1) {
		cam_err("fail i2c_transfer!!\n");
		err = -EIO;
		return err;
	}

	return err;
}

static void s5c73m3_i2c_write_check(void)
{
	int index = 0;
	do {
		if (s5c73m3_ctrl->i2c_write_check == 1) {
			cam_err("i2c is writing now!!, index : %d\n", index);
			index++;
			usleep(1*1000);
		} else
			break;

	} while (index < 20);

	if (index == 20) {
		cam_err("error index : %d\n", index);
		s5c73m3_ctrl->i2c_write_check = 0;
	}
}

static int s5c73m3_i2c_write_block(const u32 regs[], int size)
{
	int i, err = 0;

	s5c73m3_i2c_write_check();
	s5c73m3_ctrl->i2c_write_check = 1;
	for (i = 0; i < size; i++) {
		err = s5c73m3_i2c_write((regs[i]>>16), regs[i]);
		if (err < 0) {
			s5c73m3_ctrl->i2c_write_check = 0;
			cam_err("fail s5c73m3_i2c_write!!\n");
			break;
		}
	}
	s5c73m3_ctrl->i2c_write_check = 0;
	return err;
}

static int s5c73m3_i2c_read(unsigned short addr, unsigned short *data)
{
	unsigned char buf[2];
	int i, err;
	int retry_count = 5;
	struct i2c_msg msg = {s5c73m3_client->addr, 0, sizeof(buf), buf};

	if (!s5c73m3_client->adapter) {
		cam_err("s5c73m3_client->adapter is not!!\n");
		return -ENODEV;
	}

	buf[0] = addr >> 8;
	buf[1] = addr & 0xff;

	for (i = retry_count; i; i--) {
		err = i2c_transfer(s5c73m3_client->adapter, &msg, 1);
		if (err == 1)
			break;

		msleep(20);
	}

	if (err != 1) {
		cam_err("fail i2c_transfer!!\n");
		err = -EIO;
		return err;
	}

	msg.flags = I2C_M_RD;

	for (i = retry_count; i; i--) {
		err = i2c_transfer(s5c73m3_client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	if (err != 1) {
		cam_err("fail i2c_transfer!!\n");
		err = -EIO;
		return err;
	}

	*data = ((buf[0] << 8) | buf[1]);

	return err;
}

static int s5c73m3_writeb(unsigned short addr, unsigned short data)
{
	int err;
	/* check whether ISP can be used */
	err = s5c73m3_wait_ISP_status();
	if (err < 0) {
		cam_err("failed s5c73m3_wait_ISP_status\n");
			return -EIO;
	}

	s5c73m3_i2c_write_check();
	s5c73m3_ctrl->i2c_write_check = 1;

	err = s5c73m3_i2c_write(0x0050, 0x0009);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0054, 0x5000);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0F14, addr);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0F14, data);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0054, 0x5080);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0F14, 0x0001);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");

	s5c73m3_ctrl->i2c_write_check = 0;

	return err;
}

static int s5c73m3_write(unsigned short addr1,
			 unsigned short addr2, unsigned short data)
{
	int err;

	s5c73m3_i2c_write_check();
	s5c73m3_ctrl->i2c_write_check = 1;
	err = s5c73m3_i2c_write(0x0050, addr1);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0054, addr2);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0F14, data);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");

	s5c73m3_ctrl->i2c_write_check = 0;

	return err;
}

static int s5c73m3_read(unsigned short addr1,
			unsigned short addr2, unsigned short *data)
{
	int err;

	s5c73m3_i2c_write_check();
	s5c73m3_ctrl->i2c_write_check = 1;
	err = s5c73m3_i2c_write(0xfcfc, 0x3310);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0058, addr1);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x005C, addr2);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_read(0x0F14, data);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_read!!\n");

	s5c73m3_ctrl->i2c_write_check = 0;

	return err;
}

static int s5c73m3_set_timing_register_for_vdd(void)
{
	int err = 0;

	err = s5c73m3_write(0x3010, 0x0018, 0x0618);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3010, 0x001C, 0x10C1);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3010, 0x0020, 0x249E);
	CHECK_ERR(err);

	return err;
}

static int s5c73m3_i2c_check_status_with_CRC(void)
{
	int err = 0;
	int index = 0;
	u16 status = 0;
	u16 i2c_status = 0;
	u16 i2c_seq_status = 0;
	do {
		err = s5c73m3_read(0x0009, S5C73M3_STATUS, &status);
		err = s5c73m3_read(0x0009,
			S5C73M3_I2C_ERR_STATUS, &i2c_status);
		if (i2c_status & ERROR_STATUS_CHECK_BIN_CRC) {
			CAM_DBG_M("failed to check CRC value of ISP Ram\n");
			err = -1;
			break;
		}

		if (status == 0xffff)
			break;

		index++;
		udelay(500);
	} while (index < 2000);	/* 1 sec */

	if (index >= 2000) {
		err = s5c73m3_read(0x0009,
			S5C73M3_I2C_ERR_STATUS, &i2c_status);
		err = s5c73m3_read(0x0009,
			S5C73M3_I2C_SEQ_STATUS, &i2c_seq_status);
		CAM_DBG_M("TimeOut!! index:%d,status:%#x\n",
			index,
			status);
		CAM_DBG_M("i2c_stauts:%#x,i2c_seq_status:%#x\n",
			i2c_status,
			i2c_seq_status);

		err = -1;
	}

	return err;
}

void s5c73m3_make_CRC_table(u32 *table, u32 id)
{
	u32 i, j, k;

	for (i = 0; i < 256; ++i) {
		k = i;
		for (j = 0; j < 8; ++j) {
			if (k & 1)
				k = (k >> 1) ^ id;
			else
				k >>= 1;
		}
		table[i] = k;
	}
}

static int s5c73m3_reset_module(bool powerReset)
{
	int err = 0;

	CAM_DBG_M("E\n");
	if (powerReset) {
		s5c73m3_ctrl->sensordata->sensor_platform_info \
			->sensor_power_off(0);
		s5c73m3_ctrl->sensordata->sensor_platform_info \
			->sensor_power_on(0, 0);
		s5c73m3_ctrl->sensordata->sensor_platform_info \
			->sensor_power_on(0, 1);
	} else {
		s5c73m3_ctrl->sensordata->sensor_platform_info \
		->sensor_isp_reset();
	}

	err = s5c73m3_set_timing_register_for_vdd();
	CHECK_ERR(err);

	CAM_DBG_M("X\n");

	return err;
}

void s5c73m3_sensor_reset(void)
{
	s5c73m3_ctrl->sensordata->sensor_platform_info->sensor_isp_reset();
}

static int s5c73m3_wait_ISP_status(void)
{
	int err = 0;
	u16 stream_status = 0;
	int index = 0;

	CAM_DBG_H("Entered\n");
	/*Waiting until ISP will be prepared*/
	do {
		err = s5c73m3_read(0x0009, 0x5080, &stream_status);
		if (err < 0) {
			cam_err("failed s5c73m3_read!!\n");
			return -EIO;
		}
		CAM_DBG_H("stream_status1 = 0x%#x\n", stream_status);

		if (stream_status == 0xffff)
			break;

		++index;
		CAM_DBG_H("Waiting until to finish handling a command"
			"in ISP =====>> %d\n", index);

	} while (index < 400);

	if (index == 400) {
		cam_err("FAIL : ISP has been not prepared!! : 0x%#x\n",
			stream_status);

		err = s5c73m3_read(0x0009, 0x599E, &stream_status);
		if (err < 0) {
			cam_err("failed s5c73m3_read!!\n");
			return -EIO;
		}
		cam_err("0009_599E = 0x%#x\n", stream_status);

		err = s5c73m3_read(0x0009, 0x59A6, &stream_status);
		if (err < 0) {
			cam_err("failed s5c73m3_read!!\n");
			return -EIO;
		}
		cam_err("0009_59A6 = 0x%#x\n", stream_status);

		return -EIO;
	}

	return err;
}

void s5c73m3_jpeg_update(void)
{

	CAM_DBG_H("Entered\n");

}
#ifndef CONFIG_S5C73M3
static int s5c73m3_sensor_af_status(void)
{
	CAM_DBG_H("Entered\n");
	return 0;
}

static int s5c73m3_sensor_af_result(void)
{
	CAM_DBG_H("Entered\n");
	return 0;
}
#endif
static int s5c73m3_set_antibanding(int val)
{
	int err = 0;
	int antibanding_mode = 0;
	CAM_DBG_M("E, value %d\n", val);

	switch (val) {
	case ANTI_BANDING_OFF:
		antibanding_mode = S5C73M3_FLICKER_NONE;
		break;
	case ANTI_BANDING_50HZ:
		antibanding_mode = S5C73M3_FLICKER_AUTO_50HZ;
		break;
	case ANTI_BANDING_60HZ:
		antibanding_mode = S5C73M3_FLICKER_AUTO_60HZ;
		break;
	case ANTI_BANDING_AUTO:
	default:
		antibanding_mode = S5C73M3_FLICKER_AUTO;
		break;

	}

	err = s5c73m3_writeb(S5C73M3_FLICKER_MODE, antibanding_mode);
	CHECK_ERR(err);

	return err;
}

static int s5c73m3_set_flash(int val)
{
	int err;
	printk("E, value %d\n", val);

retry:
	switch  (val) {
	case MAIN_CAMERA_FLASH_OFF:
		err = s5c73m3_writeb(S5C73M3_FLASH_MODE,
			S5C73M3_FLASH_MODE_OFF);
		CHECK_ERR(err);
		err = s5c73m3_writeb(S5C73M3_FLASH_TORCH,
			S5C73M3_FLASH_TORCH_OFF);
		CHECK_ERR(err);
		break;

	case MAIN_CAMERA_FLASH_AUTO:
		power_on_flash();
		err = s5c73m3_writeb(S5C73M3_FLASH_TORCH,
			S5C73M3_FLASH_TORCH_OFF);
		CHECK_ERR(err);
		err = s5c73m3_writeb(S5C73M3_FLASH_MODE,
			S5C73M3_FLASH_MODE_AUTO);
		CHECK_ERR(err);
		break;

	case MAIN_CAMERA_FLASH_ON:
		power_on_flash();
		err = s5c73m3_writeb(S5C73M3_FLASH_TORCH,
			S5C73M3_FLASH_TORCH_OFF);
		CHECK_ERR(err);
		err = s5c73m3_writeb(S5C73M3_FLASH_MODE,
			S5C73M3_FLASH_MODE_ON);
		CHECK_ERR(err);
		break;

	case MAIN_CAMERA_FLASH_TORCH:
		power_on_flash();
		err = s5c73m3_writeb(S5C73M3_FLASH_MODE,
			S5C73M3_FLASH_MODE_OFF);
		CHECK_ERR(err);
		err = s5c73m3_writeb(S5C73M3_FLASH_TORCH,
			S5C73M3_FLASH_TORCH_ON);
		CHECK_ERR(err);
	break;

	default:
		cam_err("invalid value, %d\n", val);
		val = CAMERA_FLASH_OFF;
		goto retry;
	}
	flash_mode = val;

	CAM_DBG_H("X\n");
	return 0;
}

static int s5c73m3_set_preview(void)
{
	CAM_DBG_H("Entered\n");
	return 0;
}

static int s5c73m3_set_capture(void)
{
	int32_t	rc = 0;
	CAM_DBG_H("Entered\n");
	/*frame capture*/
	s5c73m3_ctrl->isCapture = 1;
	if (s5c73m3_sensor_setting(UPDATE_PERIODIC, RES_CAPTURE) < 0) {
		cam_err("fail s5c73m3_sensor_setting fullsize!!\n");
		return rc;
}

	return rc;
}

bool s5c73m3_CAF_enabled(void)
{
	bool val = false;

	CAM_DBG_H("Entered\n");
	if (camera_focus.mode >=
		FOCUS_MODE_CONTINOUS &&
		camera_focus.mode <=
		FOCUS_MODE_CONTINOUS_VIDEO) {
		val = true;
	}

	return val;
}

static int s5c73m3_s_stream_preview(int enable, int rt)
{
	int err = 0;

	CAM_DBG_M("Entered, enable %d\n", enable);
	if (enable) {
		err = s5c73m3_wait_ISP_status();
		if (err < 0) {
			cam_err("failed s5c73m3_wait_ISP_status\n");
			return -EIO;
		}
#if defined(YUV_PREVIEW)
		CAM_DBG_M("YUV_PREVIEW\n");
		err = s5c73m3_i2c_write_block(S5C73M3_YUV_PREVIEW,
			sizeof(S5C73M3_YUV_PREVIEW)/
			sizeof(S5C73M3_YUV_PREVIEW[0]));
		if (err < 0) {
			cam_err("failed s5c73m3_write_block!!\n");
			return -EIO;
		}
#else
		switch (rt) { /*nishu rhint*/
		case RES_PREVIEW:
			printk("Camcorder Interleaved Preview\n");
			if ((s5c73m3_ctrl->record_size_width >= 1920) &&
				(s5c73m3_ctrl->record_size_height >= 1080)) {
				if (s5c73m3_ctrl->vdis_mode == true) {
					printk("FHD VDIS preview\n");
					err = s5c73m3_i2c_write_block(
						S5C73M3_FHD_VDIS,
						sizeof(S5C73M3_FHD_VDIS)/
						sizeof(S5C73M3_FHD_VDIS[0]));
				} else {
					printk("FHD Normal preview\n");
					err = s5c73m3_i2c_write_block(
						S5C73M3_FHD,
						sizeof(S5C73M3_FHD)/
						sizeof(S5C73M3_FHD[0]));
				}
			} else if ((s5c73m3_ctrl->record_size_width >= 1280) ||
				(s5c73m3_ctrl->record_size_height >= 720)) {
				if (s5c73m3_ctrl->vdis_mode == true) {
					printk("HD VDIS preview\n");
					err = s5c73m3_i2c_write_block(
						S5C73M3_HD_VDIS,
						sizeof(S5C73M3_HD_VDIS)/
						sizeof(S5C73M3_HD_VDIS[0]));
				} else {
					printk("HD Normal preview\n");
					err = s5c73m3_i2c_write_block(
						S5C73M3_HD,
						sizeof(S5C73M3_HD)/
						sizeof(S5C73M3_HD[0]));
				}
			} else {
				if (s5c73m3_ctrl->wide_preview) {
					printk("WVGA preview\n");
					err = s5c73m3_i2c_write_block(
						S5C73M3_WVGA,
						sizeof(S5C73M3_WVGA)/
						sizeof(S5C73M3_WVGA[0]));
				} else {
					printk("VGA preview\n");
					err = s5c73m3_i2c_write_block(
						S5C73M3_VGA,
						sizeof(S5C73M3_VGA)/
						sizeof(S5C73M3_VGA[0]));
				}
			}

			err = s5c73m3_writeb(S5C73M3_AF_MODE,
				S5C73M3_AF_MODE_MOVIE_CAF_START);

			s5c73m3_set_fps(camcorder_30fps);
			/* Set VFE to turbo mode */
			op_pixel_clk1 = 320000000;
			v4l2_subdev_notify(s5c73m3_ctrl->sensor_dev,
				NOTIFY_PCLK_CHANGE, &op_pixel_clk1);
			break;

		case RES_CAPTURE:
			printk("Camera Interleaved Preview\n");

			if (s5c73m3_ctrl->hdr_mode == 1) {
				printk("Start HDR\n");
				err = s5c73m3_i2c_write_block(
					S5C73M3_HDR,
					sizeof(S5C73M3_HDR)/
					sizeof(S5C73M3_HDR[0])
							      );
				if (err < 0) {
					cam_err("failed s5c73m3_write_block!!\n");
					return -EIO;
				}

				/* check whether ISP can be used */
				err = s5c73m3_wait_ISP_status();
				if (err < 0) {
					cam_err("failed s5c73m3_wait_ISP_status\n");
					return -EIO;
				}
			} else if (s5c73m3_ctrl->low_light_mode_size == 1) {
				printk("Start Low Light Shot\n");
				err = s5c73m3_i2c_write_block(
					S5C73M3_LLS,
					sizeof(S5C73M3_LLS)/
					sizeof(S5C73M3_LLS[0])
							      );
				if (err < 0) {
					cam_err("failed s5c73m3_write_block!!\n");
					return -EIO;
				}

				/* check whether ISP can be used */
				err = s5c73m3_wait_ISP_status();
				if (err < 0) {
					cam_err("failed s5c73m3_wait_ISP_status\n");
					return -EIO;
				}
			} else {
				printk("preview_size : %x\n",
					s5c73m3_ctrl->preview_size);
				printk("jpeg_size : %x\n",
					s5c73m3_ctrl->jpeg_size);
					err = s5c73m3_i2c_write_block(
				S5C73M3_PREVIEW,
				sizeof(S5C73M3_PREVIEW)/
				sizeof(S5C73M3_PREVIEW[0]));

				err = s5c73m3_writeb(S5C73M3_CHG_MODE,
					S5C73M3_YUV_MODE |
					s5c73m3_ctrl->preview_size |
					s5c73m3_ctrl->jpeg_size);
			}

			err = s5c73m3_writeb(S5C73M3_SENSOR_STREAMING,
				S5C73M3_SENSOR_STREAMING_ON);

			/* check whether ISP can be used */
			err = s5c73m3_wait_ISP_status();
			if (err < 0) {
				cam_err("failed s5c73m3_wait_ISP_status\n");
				return -EIO;
			}

			/* Set VFE to turbo mode */
			op_pixel_clk1 = 320000000;
			v4l2_subdev_notify(s5c73m3_ctrl->sensor_dev,
			     NOTIFY_PCLK_CHANGE, &op_pixel_clk1);
			break;

		default:
			err = -1;
			break;

		};
		if (err < 0) {
			cam_err("failed s5c73m3_write_block!!\n");
			return -EIO;
		}
#endif
		/* check whether ISP can be used */
		err = s5c73m3_wait_ISP_status();
		if (err < 0) {
			cam_err("failed s5c73m3_wait_ISP_status\n");
				return -EIO;
			}
	}

	return err;
}


static int s5c73m3_sensor_setting(int update_type, int rt)
{
	int32_t rc = 0;
	struct msm_camera_csid_params s5c73m3_csid_params;
	struct msm_camera_csiphy_params s5c73m3_csiphy_params;

	CAM_DBG_M("Entered\n");

	switch (update_type) {
	case REG_INIT:
		break;

	case UPDATE_PERIODIC:
		if (rt == RES_PREVIEW || rt == RES_CAPTURE) {
			CAM_DBG_H("UPDATE_PERIODIC\n");
#if defined(YUV_PREVIEW)
			v4l2_subdev_notify(s5c73m3_ctrl->sensor_dev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX0, ISPIF_OFF_IMMEDIATELY));
#endif

#ifdef QC_TEST
			do {
				rc = s5c73m3_read(
					0x0009, 0x5804, &stream_status);
				if (rc < 0) {
					cam_err("failed s5c73m3_read!!\n");
					return -EIO;
				}
				CAM_DBG_M("#####1stream_status : %x\n",
					stream_status);
				index++;
				msleep(30);
			} while (index < 10);
#endif

			if (config_csi2 == 0) {
				struct msm_camera_csid_vc_cfg
					s5c73m3_vccfg[] = {
					{0, 0x1E, CSI_DECODE_8BIT},
					{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
#if !defined(YUV_PREVIEW)
					{2, 0x30, CSI_DECODE_8BIT},
#endif
				};
				s5c73m3_csid_params.lane_cnt = 4;
				s5c73m3_csid_params.lane_assign = 0xe4;
				s5c73m3_csid_params.lut_params.num_cid =
					ARRAY_SIZE(s5c73m3_vccfg);
				s5c73m3_csid_params.lut_params.vc_cfg =
					&s5c73m3_vccfg[0];
				s5c73m3_csiphy_params.lane_cnt = 4;
				s5c73m3_csiphy_params.settle_cnt = 0x27;
				v4l2_subdev_notify(s5c73m3_ctrl->sensor_dev,
						   NOTIFY_CSID_CFG,
						   &s5c73m3_csid_params);
				v4l2_subdev_notify(s5c73m3_ctrl->sensor_dev,
						   NOTIFY_CID_CHANGE, NULL);
				mb();
				v4l2_subdev_notify(s5c73m3_ctrl->sensor_dev,
						   NOTIFY_CSIPHY_CFG,
						   &s5c73m3_csiphy_params);
				mb();
				/*s5c73m3_delay_msecs_stdby*/
				/*msleep(20);*/
				config_csi2 = 1;
			}

			rc = s5c73m3_s_stream_preview(1, rt);
			if (rc < 0) {
				cam_err("failed s5c73m3_s_stream_preview!!\n");
				return -EIO;
			}

#if defined(YUV_PREVIEW)
			v4l2_subdev_notify(s5c73m3_ctrl->sensor_dev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX0, ISPIF_ON_FRAME_BOUNDARY));
#endif

#ifdef QC_TEST
			do {
				rc = s5c73m3_read(0x0009, 0x5804,
					&stream_status);
				if (rc < 0) {
					cam_err("failed s5c73m3_read!!\n");
					return -EIO;
				}
				CAM_DBG_M("#####2stream_status : %x\n",
					stream_status);
				index++;
				msleep(30);
			} while (index < 20);

			rc = s5c73m3_read(0x0009, 0x583C, &stream_status);
			if (rc < 0) {
				cam_err("failed s5c73m3_read!!\n");
				return -EIO;
			}
			CAM_DBG_M("#####0x583C : %x\n",
				stream_status);

			s5c73m3_read(0x2A00, 0x0060, &temp);
			CAM_DBG_M("### 1 value : %#x\n", temp);
			s5c73m3_read(0x2A00, 0x0064, &temp);
			CAM_DBG_M("### 2 value : %#x\n", temp);
			s5c73m3_read(0x2A00, 0x008C, &temp);
			CAM_DBG_M("### 3 value : %#x\n", temp);
			s5c73m3_read(0x2A00, 0x0090, &temp);
			CAM_DBG_M("### 4 value : %#x\n", temp);
			s5c73m3_read(0x2A00, 0x01CC, &temp);
			CAM_DBG_M("### 5 value : %#x\n", temp);
			s5c73m3_read(0x2A00, 0x002C, &temp1);
			s5c73m3_read(0x2A00, 0x0030, &temp2);
			s5c73m3_read(0x2A00, 0x0034, &temp3);
			temp = (temp1<<16)|(temp2<<8)|temp3;
			CAM_DBG_M("### jpeg size : %#x\n", temp);

			s5c73m3_read(0x2A00, 0x0100, &temp);
			CAM_DBG_M("### preview buffer : %#x\n", temp);

			s5c73m3_read(0x2A00, 0x0104, &temp);
			CAM_DBG_M("### jpeg buffer : %#x\n", temp);

#endif
			if (s5c73m3_CAF_enabled())
				s5c73m3_set_af_mode(camera_focus.mode);

		}
		break;
	default:
		cam_err("fail!!\n");
		rc = -EINVAL;
		break;
	}

	CAM_DBG_M("sensor setting is done!!\n");
	return rc;
}

static int s5c73m3_video_config(int mode)
{
	int32_t	rc = 0;
	printk("Entered, mode %d\n", mode);
	if (s5c73m3_sensor_setting(UPDATE_PERIODIC, RES_PREVIEW) < 0) {
		cam_err("fail s5c73m3_sensor_setting!!\n");
		return rc;
	}

	return rc;
}

static int s5c73m3_set_sensor_mode(int mode)
{
	int err = 0;
	printk("Entered, mode %d\n", mode);
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
	case SENSOR_VIDEO_MODE:
		s5c73m3_ctrl->camcorder_mode = true;
		if (s5c73m3_ctrl->isCapture == 1) {
			s5c73m3_ctrl->isCapture = 0;
			err = s5c73m3_set_preview();
			if (err < 0) {
				cam_err("fail s5c73m3_set_preview!!\n");
				return err;
			}
		}

		err = s5c73m3_video_config(mode);
		if (err < 0) {
			cam_err("fail s5c73m3_video_config!!\n");
			return err;
		}
		break;

	case SENSOR_SNAPSHOT_MODE:
	case SENSOR_RAW_SNAPSHOT_MODE:
		s5c73m3_ctrl->camcorder_mode = false;
		err = s5c73m3_set_capture();
		if (err < 0) {
			cam_err("fail s5c73m3_set_capture!!\n");
			return err;
		}
		break;

	default:
		return 0;
	}
	return 0;
}

static int s5c73m3_set_effect(int effect)
{
	int32_t rc = 0;

	printk("Entered, effect %d\n", effect);
	switch (effect) {
	case CAMERA_EFFECT_OFF:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_NONE);
		break;

	case CAMERA_EFFECT_MONO:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_MONO);
		break;

	case CAMERA_EFFECT_NEGATIVE:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_NEGATIVE);
		break;

	case CAMERA_EFFECT_SEPIA:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_SEPIA);
		break;

	case CAMERA_EFFECT_WASHED:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_WASHED);
		break;

	case CAMERA_EFFECT_VINTAGE_WARM:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_VINTAGE_WARM);
		break;

	case CAMERA_EFFECT_VINTAGE_COLD:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_VINTAGE_COLD);
		break;

	case CAMERA_EFFECT_SOLARIZE:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_SOLARIZE);
		break;

	case CAMERA_EFFECT_POSTERIZE:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_POSTERIZE);
		break;

	case CAMERA_EFFECT_POINT_COLOR_1:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_POINT_COLOR_1);
		break;

	case CAMERA_EFFECT_POINT_COLOR_2:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_POINT_COLOR_2);
		break;

	case CAMERA_EFFECT_POINT_COLOR_3:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_POINT_COLOR_3);
		break;

	case CAMERA_EFFECT_POINT_COLOR_4:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_POINT_COLOR_4);
		break;

	case CAMERA_EFFECT_CARTOONIZE:
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
				    S5C73M3_IMAGE_EFFECT_CARTOONIZE);
		break;

	default:
		CAM_DBG_M("default effect\n");
		rc = s5c73m3_writeb(S5C73M3_IMAGE_EFFECT,
			S5C73M3_IMAGE_EFFECT_NONE);
	return 0;
}
	return rc;
}

static int s5c73m3_set_whitebalance(int wb)
{
	int32_t rc = 0;

	printk("Entered, %d\n", wb);
	switch (wb) {
	case CAMERA_WHITE_BALANCE_AUTO:
		rc = s5c73m3_writeb(S5C73M3_AWB_MODE,
			S5C73M3_AWB_MODE_AUTO);
		break;

	case CAMERA_WHITE_BALANCE_INCANDESCENT:
		rc = s5c73m3_writeb(S5C73M3_AWB_MODE,
			S5C73M3_AWB_MODE_INCANDESCENT);
		break;

	case CAMERA_WHITE_BALANCE_FLUORESCENT:
		rc = s5c73m3_writeb(S5C73M3_AWB_MODE,
			S5C73M3_AWB_MODE_FLUORESCENT1);
		break;

	case CAMERA_WHITE_BALANCE_DAYLIGHT:
		rc = s5c73m3_writeb(S5C73M3_AWB_MODE,
			S5C73M3_AWB_MODE_DAYLIGHT);
		break;

	case CAMERA_WHITE_BALANCE_CLOUDY_DAYLIGHT:
		rc = s5c73m3_writeb(S5C73M3_AWB_MODE,
			S5C73M3_AWB_MODE_CLOUDY);
		break;

	default:
		CAM_DBG_M("default WB mode\n");
		rc = s5c73m3_writeb(S5C73M3_AWB_MODE,
			S5C73M3_AWB_MODE_AUTO);
		break;
	}
	s5c73m3_ctrl->wb = wb;

	return rc;
}

static int s5c73m3_set_ev(int ev)
{
	int32_t rc = 0;
	printk("Entered, ev %d\n", ev);

	switch (ev) {
	case CAMERA_EV_M4:
		rc = s5c73m3_writeb(S5C73M3_EV,
			S5C73M3_EV_M20);
		break;

	case CAMERA_EV_M3:
		rc = s5c73m3_writeb(S5C73M3_EV,
			S5C73M3_EV_M15);
		break;

	case CAMERA_EV_M2:
		rc = s5c73m3_writeb(S5C73M3_EV,
			S5C73M3_EV_M10);
		break;

	case CAMERA_EV_M1:
		rc = s5c73m3_writeb(S5C73M3_EV,
			S5C73M3_EV_M05);
		break;

	case CAMERA_EV_DEFAULT:
		rc = s5c73m3_writeb(S5C73M3_EV,
			S5C73M3_EV_ZERO);
		break;

	case CAMERA_EV_P1:
		rc = s5c73m3_writeb(S5C73M3_EV,
			S5C73M3_EV_P05);
		break;

	case CAMERA_EV_P2:
		rc = s5c73m3_writeb(S5C73M3_EV,
			S5C73M3_EV_P10);
		break;

	case CAMERA_EV_P3:
		rc = s5c73m3_writeb(S5C73M3_EV,
			S5C73M3_EV_P15);
		break;

	case CAMERA_EV_P4:
		rc = s5c73m3_writeb(S5C73M3_EV,
			S5C73M3_EV_P20);
		break;

	default:
		CAM_DBG_M("default ev mode\n");
		rc = s5c73m3_writeb(S5C73M3_EV,
			S5C73M3_EV_ZERO);
		break;
	}
	return rc;
}

static int s5c73m3_set_scene_mode(int mode)
{
	int32_t rc = 0;

	printk("Entered, mode %d\n", mode);
	switch (mode) {
	case CAMERA_SCENE_AUTO:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_NONE);
		break;

	case CAMERA_SCENE_LANDSCAPE:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_LANDSCAPE);
		break;

	case CAMERA_SCENE_DAWN:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_DAWN);
		break;

	case CAMERA_SCENE_BEACH:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_BEACH);
		break;

	case CAMERA_SCENE_SUNSET:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_SUNSET);
		break;

	case CAMERA_SCENE_NIGHT:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_NIGHT);
		break;

	case CAMERA_SCENE_PORTRAIT:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_PORTRAIT);
		break;

	case CAMERA_SCENE_AGAINST_LIGHT:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_AGAINSTLIGHT);
		break;

	case CAMERA_SCENE_SPORT:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_SPORTS);
		break;

	case CAMERA_SCENE_FALL:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_FALL);
		break;

	case CAMERA_SCENE_TEXT:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_TEXT);
		break;

	case CAMERA_SCENE_CANDLE:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_CANDLE);
		break;

	case CAMERA_SCENE_FIRE:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_FIRE);
		break;

	case CAMERA_SCENE_PARTY:
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_INDOOR);
		break;

	default:
		CAM_DBG_M("default scene mode\n");
		rc = s5c73m3_writeb(S5C73M3_SCENE_MODE,
			S5C73M3_SCENE_MODE_NONE);
		break;
	}
	s5c73m3_ctrl->scene = mode;

	return rc;
}

static int s5c73m3_set_iso(int iso)
{
	int32_t rc = 0;

	printk("Entered, iso %d\n", iso);
	switch (iso) {
	case CAMERA_ISO_MODE_AUTO:
		rc = s5c73m3_writeb(S5C73M3_ISO,
			S5C73M3_ISO_AUTO);
		break;

	case CAMERA_ISO_MODE_100:
		rc = s5c73m3_writeb(S5C73M3_ISO,
			S5C73M3_ISO_100);
		break;

	case CAMERA_ISO_MODE_200:
		rc = s5c73m3_writeb(S5C73M3_ISO,
			S5C73M3_ISO_200);
		break;

	case CAMERA_ISO_MODE_400:
		rc = s5c73m3_writeb(S5C73M3_ISO,
			S5C73M3_ISO_400);
		break;

	case CAMERA_ISO_MODE_800:
		rc = s5c73m3_writeb(S5C73M3_ISO,
			S5C73M3_ISO_800);
		break;

	default:
		CAM_DBG_M("default iso mode\n");
		rc = s5c73m3_writeb(S5C73M3_ISO,
			S5C73M3_ISO_AUTO);
		break;
	}
	return rc;
}

static int s5c73m3_set_metering(int mode)
{
	int32_t rc = 0;

	printk("Entered, mode %d\n", mode);
	switch (mode) {
	case CAMERA_CENTER_WEIGHT:
		rc = s5c73m3_writeb(S5C73M3_METER,
			S5C73M3_METER_CENTER);
		break;

	case CAMERA_AVERAGE:
		rc = s5c73m3_writeb(S5C73M3_METER,
			S5C73M3_METER_AVERAGE);
		break;

	case CAMERA_SPOT:
		rc = s5c73m3_writeb(S5C73M3_METER,
			S5C73M3_METER_SPOT);
		break;

	default:
		CAM_DBG_M("default metering mode\n");
		rc = s5c73m3_writeb(S5C73M3_METER,
			S5C73M3_METER_CENTER);
		break;
	}
	return rc;
}

static int s5c73m3_set_zoom(int level)
{
	int err;
	int diff = 0;

	printk("Entered, zoom %d\n", level);

retry:
	if (level < 0 || level > 30) {
		cam_err("invalid value, %d\n", level);
		level = 0;
		s5c73m3_ctrl->zoomPreValue = 0;
		goto retry;
	}

	if (s5c73m3_ctrl->zoomPreValue > level)
		diff = s5c73m3_ctrl->zoomPreValue - level;
	else
		diff = level - s5c73m3_ctrl->zoomPreValue;

	if (diff > 2) {
		err = s5c73m3_wait_ISP_status();
		if (err < 0) {
			cam_err("failed s5c73m3_wait_ISP_status\n");
				return -EIO;
		}
	}

	s5c73m3_ctrl->zoomPreValue = level;

	err = s5c73m3_i2c_write(0x0050, 0x0009);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0054, 0x5000);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0F14, S5C73M3_ZOOM_STEP);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0F14, level);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0054, 0x5080);
	if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n");
	err = s5c73m3_i2c_write(0x0F14, 0x0001);
	/* if (err < 0)
		cam_err("fail s5c73m3_i2c_write!!\n"); */


	if (err < 0) {
		cam_err("fail s5c73m3_i2c_write!!\n");
		cam_err("fail s5c73m3_zoom!!\n");
		return err;
	}

	return err;
}

static int s5c73m3_set_jpeg_quality(int quality)
{
	int32_t rc = 0;
	CAM_DBG_H("Entered, quality %d\n", quality);

	if (quality <= 80)		/* LOW for camcorder */
		rc = s5c73m3_writeb(S5C73M3_IMAGE_QUALITY,
			S5C73M3_IMAGE_QUALITY_LOW);
	else if (quality <= 90)		/* Normal */
		rc = s5c73m3_writeb(S5C73M3_IMAGE_QUALITY,
			S5C73M3_IMAGE_QUALITY_NORMAL);
	else if (quality <= 95)	/* Fine */
		rc = s5c73m3_writeb(S5C73M3_IMAGE_QUALITY,
			S5C73M3_IMAGE_QUALITY_FINE);
	else			/* Superfine */
		rc = s5c73m3_writeb(S5C73M3_IMAGE_QUALITY,
			S5C73M3_IMAGE_QUALITY_SUPERFINE);

	if (rc < 0) {
		cam_err("fail s5c73m3_quality!!\n");
		return rc;
	}

	return rc;
}

static int s5c73m3_set_face_detection(int val)
{
	int32_t rc = 0;
	printk("Entered, Face detection %d\n", val);

retry:
	switch (val) {
	case FACE_DETECTION_ON:
		rc = s5c73m3_writeb(S5C73M3_FACE_DET,
			S5C73M3_FACE_DET_ON);
		break;

	case FACE_DETECTION_OFF:
		rc = s5c73m3_writeb(S5C73M3_FACE_DET,
			S5C73M3_FACE_DET_OFF);
		break;

	default:
		cam_err("invalid value, %d\n", val);
		val = FACE_DETECTION_OFF;
		goto retry;
	}

	return rc;
}

static int s5c73m3_aeawb_lock_unlock(int32_t ae_lock, int32_t awb_lock)
{
	int err = 0;

	printk("Entered, wb :%d\n", s5c73m3_ctrl->wb);

	if (ae_lock) {
		CAM_DBG_M("ae lock");
		err = s5c73m3_writeb(S5C73M3_AE_CON, S5C73M3_AE_STOP);
		CHECK_ERR(err);
		s5c73m3_ctrl->isAeLock = true;
	} else {
		CAM_DBG_M("ae unlock");
		err = s5c73m3_writeb(S5C73M3_AE_CON, S5C73M3_AE_START);
		CHECK_ERR(err);
		s5c73m3_ctrl->isAeLock = false;
	}

	if (s5c73m3_ctrl->wb == CAMERA_WHITE_BALANCE_AUTO &&
		s5c73m3_ctrl->scene != CAMERA_SCENE_SUNSET &&
		s5c73m3_ctrl->scene != CAMERA_SCENE_DAWN &&
		s5c73m3_ctrl->scene != CAMERA_SCENE_CANDLE) {
		if (awb_lock) {
			CAM_DBG_M("awb lock");
			err = s5c73m3_writeb(S5C73M3_AWB_CON,
				S5C73M3_AWB_STOP);
			CHECK_ERR(err);
			s5c73m3_ctrl->isAwbLock = true;
		} else {
			CAM_DBG_M("awb unlock");
			err = s5c73m3_writeb(S5C73M3_AWB_CON,
				S5C73M3_AWB_START);
			CHECK_ERR(err);
			s5c73m3_ctrl->isAwbLock = false;
		}
	}

	return 0;
}

static int s5c73m3_set_focus(int val)
{
	int err = 0;

	CAM_DBG_M("%s, mode %#x\n",
		val ? "start" : "stop", camera_focus.mode);

	camera_focus.status = 0;

	if (val) {
		isflash = S5C73M3_ISNEED_FLASH_ON;

		if (!s5c73m3_ctrl->camcorder_mode)
			s5c73m3_aeawb_lock_unlock(1, 1);
		if (camera_focus.mode == FOCUS_MODE_TOUCH)
			err = s5c73m3_set_touch_auto_focus();
		else {
			err = s5c73m3_writeb(S5C73M3_AF_CON,
				S5C73M3_AF_CON_START);
		}
	} else {
		s5c73m3_aeawb_lock_unlock(0, 0);
		err = s5c73m3_writeb(S5C73M3_STILL_MAIN_FLASH
				, S5C73M3_STILL_MAIN_FLASH_CANCEL);
		err = s5c73m3_writeb(S5C73M3_AF_CON, S5C73M3_AF_CON_STOP);
		isflash = S5C73M3_ISNEED_FLASH_UNDEFINED;
	}

	CHECK_ERR(err);

	CAM_DBG_H("X\n");
	return err;
}

static int s5c73m3_set_caf_focus(int val)
{
	int err = 0;

	CAM_DBG_M("%s, mode %#x\n",
		val ? "start" : "stop", camera_focus.mode);

	if (val) {
		if (camera_focus.mode == FOCUS_MODE_CONTINOUS_VIDEO) {
			CAM_DBG_M("Movie CAF\n");
			err = s5c73m3_writeb(S5C73M3_AF_MODE,
				S5C73M3_AF_MODE_MOVIE_CAF_START);
		} else {
			CAM_DBG_M("Preview CAF\n");
			err = s5c73m3_writeb(S5C73M3_AF_MODE,
				S5C73M3_AF_MODE_PREVIEW_CAF_START);
		}
	} else {
		err = s5c73m3_writeb(S5C73M3_AF_CON, S5C73M3_AF_CON_STOP);
	}

	CHECK_ERR(err);

	CAM_DBG_H("X\n");
	return err;
}

static int s5c73m3_get_pre_flash(int val)
{
	int err = 0;
	u16 pre_flash = false;

	err = s5c73m3_read(0x0009,
			S5C73M3_STILL_PRE_FLASH | 0x5000, &pre_flash);
	isPreflashFired |= pre_flash;

	return err;
}

static int s5c73m3_get_af_result(void)
{
	int ret = 0;
	u16 af_status = S5C73M3_AF_STATUS_UNFOCUSED;
	/*u16 temp_status = 0;*/

	CAM_DBG_M("Entered\n");

	ret = s5c73m3_read(0x0009, S5C73M3_AF_STATUS, &af_status);

	switch (af_status) {
	case S5C73M3_AF_STATUS_FOCUSING:
	case S5C73M3_CAF_STATUS_FOCUSING:
	case S5C73M3_CAF_STATUS_FIND_SEARCHING_DIR:
	case S5C73M3_CAF_STATUS_INITIALIZE:
	case S5C73M3_AF_STATUS_INVALID:
		ret = MSM_V4L2_AF_STATUS_IN_PROGRESS;
		break;

	case S5C73M3_AF_STATUS_FOCUSED:
	case S5C73M3_CAF_STATUS_FOCUSED:
		ret = MSM_V4L2_AF_STATUS_SUCCESS;
		break;

	case S5C73M3_AF_STATUS_UNFOCUSED:
	default:
		ret = MSM_V4L2_AF_STATUS_FAIL;
		break;
	}
	camera_focus.status = af_status;

	return ret;
}

static int s5c73m3_set_af_mode(int val)
{
	int err;
	CAM_DBG_M("Entered, new mode %d, old mode %d\n",
		val, camera_focus.mode);

retry:
	switch (val) {
	case FOCUS_MODE_AUTO:
	case FOCUS_MODE_INFINITY:
		if (camera_focus.mode >=
			FOCUS_MODE_CONTINOUS_PICTURE) {
			err = s5c73m3_writeb(S5C73M3_AF_CON,
				S5C73M3_AF_CON_STOP);
		}

		if (camera_focus.mode !=
			FOCUS_MODE_CONTINOUS_PICTURE) {
			err = s5c73m3_writeb(S5C73M3_AF_MODE,
				S5C73M3_AF_MODE_NORMAL);
			CHECK_ERR(err);
		}
		camera_focus.mode = val;
		camera_focus.caf_mode = S5C73M3_AF_MODE_NORMAL;
		break;

	case FOCUS_MODE_MACRO:
		if (camera_focus.mode !=
			FOCUS_MODE_CONTINOUS_PICTURE_MACRO) {
			err = s5c73m3_writeb(S5C73M3_AF_MODE,
				S5C73M3_AF_MODE_MACRO);
			CHECK_ERR(err);
		}
		camera_focus.mode = val;
		camera_focus.caf_mode = S5C73M3_AF_MODE_MACRO;
		break;

	case FOCUS_MODE_CONTINOUS_PICTURE:
		if (val != camera_focus.mode &&
			camera_focus.caf_mode != S5C73M3_AF_MODE_NORMAL) {
			camera_focus.mode = val;
			err = s5c73m3_writeb(S5C73M3_AF_MODE,
				S5C73M3_AF_MODE_NORMAL);
			CHECK_ERR(err);
			camera_focus.caf_mode = S5C73M3_AF_MODE_NORMAL;
		}

		err = s5c73m3_writeb(S5C73M3_AF_MODE,
			S5C73M3_AF_MODE_PREVIEW_CAF_START);
		CHECK_ERR(err);
		break;

	case FOCUS_MODE_CONTINOUS_PICTURE_MACRO:
		if (val != camera_focus.mode &&
			camera_focus.caf_mode != S5C73M3_AF_MODE_MACRO) {
			camera_focus.mode = val;
			err = s5c73m3_writeb(S5C73M3_AF_MODE,
				S5C73M3_AF_MODE_MACRO);
			CHECK_ERR(err);
			camera_focus.caf_mode = S5C73M3_AF_MODE_MACRO;
		}

		err = s5c73m3_writeb(S5C73M3_AF_MODE,
			S5C73M3_AF_MODE_PREVIEW_CAF_START);
		CHECK_ERR(err);
		break;

	case FOCUS_MODE_CONTINOUS_VIDEO:
		camera_focus.mode = val;

		err = s5c73m3_writeb(S5C73M3_AF_MODE,
			S5C73M3_AF_MODE_MOVIE_CAF_START);
		CHECK_ERR(err);
		break;

	case FOCUS_MODE_FACEDETECT:
		camera_focus.mode = val;
		break;

	case FOCUS_MODE_TOUCH:
		camera_focus.mode = val;
		break;

	default:
		cam_err("invalid value, %d\n", val);
		val = FOCUS_MODE_AUTO;
		goto retry;
	}

	camera_focus.mode = val;

	CAM_DBG_H("X\n");
	return 0;
}

static int s5c73m3_set_touch_auto_focus()
{
	int err;

	CAM_DBG_M("s5c73m3_set_touch_auto_focus\n");
	CAM_DBG_H("Touch Position(%d,%d)\n",
		camera_focus.pos_x, camera_focus.pos_y);

	s5c73m3_i2c_write_check();
	s5c73m3_ctrl->i2c_write_check = 1;

	err = s5c73m3_i2c_write(0xfcfc, 0x3310);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0050, 0x0009);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0054, S5C73M3_AF_TOUCH_POSITION);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, camera_focus.pos_x);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, camera_focus.pos_y);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, s5c73m3_ctrl->preview_size_width);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, s5c73m3_ctrl->preview_size_height);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0050, 0x0009);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0054, 0x5000);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, 0x0E0A);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, 0x0000);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0054, 0x5080);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, 0x0001);
	CHECK_ERR(err);

	s5c73m3_ctrl->i2c_write_check = 0;

	return 0;
}

static int s5c73m3_capture_firework(void)
{
	int err = 0;
	CAM_DBG_H("E\n");
	err = s5c73m3_writeb(S5C73M3_FIREWORK_CAPTURE, 0x0001);
	CHECK_ERR(err);

	return err;
}

static int s5c73m3_capture_nightshot(void)
{
	int err = 0;
	CAM_DBG_H("E\n");
	err = s5c73m3_writeb(S5C73M3_NIGHTSHOT_CAPTURE, 0x0001);
	CHECK_ERR(err);

	return err;
}

static int s5c73m3_start_capture(int val)
{
	int err = 0;
	u16 isneed_flash = false;
	u16 pre_flash = false;

	isPreflashFired = false;
	err = s5c73m3_read(0x0009,
			S5C73M3_STILL_PRE_FLASH | 0x5000, &pre_flash);

	if (flash_mode == MAIN_CAMERA_FLASH_ON) {
		if (!pre_flash) {
			err = s5c73m3_writeb(S5C73M3_STILL_PRE_FLASH
					, S5C73M3_STILL_PRE_FLASH_FIRE);
			isPreflashFired = true;
			msleep(100);
		}
		err = s5c73m3_writeb(S5C73M3_STILL_MAIN_FLASH
				, S5C73M3_STILL_MAIN_FLASH_FIRE);
		CAM_DBG_H("full flash!!!\n");
	} else if (flash_mode == MAIN_CAMERA_FLASH_AUTO) {
		if (pre_flash) {
			err = s5c73m3_writeb(S5C73M3_STILL_MAIN_FLASH
					, S5C73M3_STILL_MAIN_FLASH_FIRE);
		} else if (isflash != S5C73M3_ISNEED_FLASH_ON) {
			err = s5c73m3_read(0x0009,
				S5C73M3_AE_ISNEEDFLASH | 0x5000, &isneed_flash);
			if (isneed_flash) {
				err = s5c73m3_writeb(S5C73M3_STILL_PRE_FLASH
					, S5C73M3_STILL_PRE_FLASH_FIRE);
				isPreflashFired = true;
				msleep(100);
				err = s5c73m3_writeb(S5C73M3_STILL_MAIN_FLASH
					, S5C73M3_STILL_MAIN_FLASH_FIRE);
				CAM_DBG_H("full flash with CAF!!!\n");
			}
		}
	}

	isflash = S5C73M3_ISNEED_FLASH_UNDEFINED;

	return 0;
}


static int s5c73m3_set_wdr(int val)
{
	int err;
	CAM_DBG_H("E, value %d\n", val);
retry:
	switch (val) {
	case WDR_OFF:
		err = s5c73m3_writeb(S5C73M3_WDR,
			S5C73M3_WDR_OFF);
		CHECK_ERR(err);
		break;

	case WDR_ON:
		err = s5c73m3_writeb(S5C73M3_WDR,
			S5C73M3_WDR_ON);
		CHECK_ERR(err);
		break;

	default:
		cam_err("invalid value, %d\n", val);
		val = WDR_OFF;
		goto retry;
	}

	CAM_DBG_H("X\n");
	return 0;
}

static int s5c73m3_set_HDR(int val)
{
	int err = 0;
	CAM_DBG_H("E, value %d\n", val);
	s5c73m3_ctrl->hdr_mode = val;
#if defined(TEMP_REMOVE)
	/* this case for JPEG HDR */
	if (val) {
		err = s5c73m3_writeb(S5C73M3_AE_AUTO_BRAKET,
				S5C73M3_AE_AUTO_BRAKET_EV20);
		CHECK_ERR(err);
	}

	/* this case for interealved JPEG HDR(full yuv : 420) */
	if (val) {
		err = s5c73m3_i2c_write_block(S5C73M3_HDR,
			sizeof(S5C73M3_HDR)/
			sizeof(S5C73M3_HDR[0]));
		if (err < 0) {
			cam_err("failed s5c73m3_write_block!!\n");
			return -EIO;
		}
		/* check whether ISP can be used */
		err = s5c73m3_wait_ISP_status();
		if (err < 0) {
			cam_err("failed s5c73m3_wait_ISP_status\n");
				return -EIO;
		}
	}
#endif

	CAM_DBG_H("X\n");
	return err;
}

static int s5c73m3_start_HDR(int val)
{
	int err = 0;
	printk("E, value %d\n", val);

	if (s5c73m3_ctrl->hdr_mode & val) {
		CAM_DBG_H("SET AE_BRACKET\n");
		err = s5c73m3_writeb(S5C73M3_AE_AUTO_BRAKET,
				     S5C73M3_AE_AUTO_BRAKET_EV20);
		CHECK_ERR(err);
	}
	/* check whether ISP can be used */
	err = s5c73m3_wait_ISP_status();
	if (err < 0) {
		cam_err("failed s5c73m3_wait_ISP_status\n");
		return -EIO;
	}

	CAM_DBG_H("X\n");
	return err;
}

static int s5c73m3_set_low_light(int val)
{
	int err = 0;
	CAM_DBG_H("E, value %d\n", val);

	s5c73m3_ctrl->low_light_mode = val;

	if (s5c73m3_ctrl->low_light_mode) {
		CAM_DBG_H("LLS mode : 0N\n");
		err = s5c73m3_writeb(S5C73M3_LLS_MODE,
				     S5C73M3_LLS_MODE_ON);
		CHECK_ERR(err);
	} else {
		CAM_DBG_H("LLS mode : OFF\n");
		err = s5c73m3_writeb(S5C73M3_LLS_MODE,
				     S5C73M3_LLS_MODE_OFF);
		CHECK_ERR(err);
	}
	/* check whether ISP can be used */
	err = s5c73m3_wait_ISP_status();
	if (err < 0) {
		cam_err("failed s5c73m3_wait_ISP_status\n");
		return -EIO;
	}

	CAM_DBG_H("X\n");
	return err;
}

static int s5c73m3_set_antishake(int val)
{
	

	int err = 0;
	CAM_DBG_H("Entered, %d\n", val);
	if (val) {
		err = s5c73m3_writeb(S5C73M3_AE_MODE,
			S5C73M3_ANTI_SHAKE_ON);
		CHECK_ERR(err);
		s5c73m3_ctrl->anti_shake_mode = true;
	} else {
		if (s5c73m3_ctrl->anti_shake_mode) {
			err = s5c73m3_writeb(S5C73M3_AE_MODE,
				S5C73M3_AUTO_MODE_AE_SET);
			CHECK_ERR(err);
			s5c73m3_ctrl->anti_shake_mode = false;
		}
	}
	return err;
}

static int s5c73m3_set_jpeg_size(int width, int height)
{
	int32_t rc = 0;
	CAM_DBG_M("Entered, width : %d, height : %d\n", width, height);

	if (width == 3264 && height == 2448)
		s5c73m3_ctrl->jpeg_size = 0x00F0;
	else if (width == 3264 && height == 1836)
		s5c73m3_ctrl->jpeg_size = 0x00E0;
	else if (width == 1024 && height == 768)
		s5c73m3_ctrl->jpeg_size = 0x00D0;
	else if (width == 3264 && height == 2176)
		s5c73m3_ctrl->jpeg_size = 0x00C0;
	else if (width == 2560 && height == 1920)
		s5c73m3_ctrl->jpeg_size = 0x00B0;
	else if (width == 2560 && height == 1440)
		s5c73m3_ctrl->jpeg_size = 0x00A0;
	else if (width == 2048 && height == 1536)
		s5c73m3_ctrl->jpeg_size = 0x0090;
	else if (width == 2048 && height == 1152)
		s5c73m3_ctrl->jpeg_size = 0x0080;
	else if (width == 1600 && height == 1200)
		s5c73m3_ctrl->jpeg_size = 0x0070;
	else if (width == 1600 && height == 900)
		s5c73m3_ctrl->jpeg_size = 0x0060;
	else if (width == 1280 && height == 960)
		s5c73m3_ctrl->jpeg_size = 0x0050;
	else if (width == 1280 && height == 720)
		s5c73m3_ctrl->jpeg_size = 0x0040;
	else if (width == 960 && height == 720)
		s5c73m3_ctrl->jpeg_size = 0x0030;
	else if (width == 960 && height == 540)
		s5c73m3_ctrl->jpeg_size = 0x0020;
	else if (width == 640 && height == 480)
		s5c73m3_ctrl->jpeg_size = 0x0010;
	else
		s5c73m3_ctrl->jpeg_size = 0x00F0;

	return rc;
}

static int s5c73m3_set_record_size(int width, int height)
{
	int32_t rc = 0;
	printk("Entered, width : %d, height : %d\n", width, height);
	s5c73m3_ctrl->record_size_width = width;
	s5c73m3_ctrl->record_size_height = height;

	return rc;
}

static int s5c73m3_set_face_beauty(int val)
{
	CAM_DBG_H("Entered, %d\n", val);
	return 0;
}

static int s5c73m3_set_fps(int fps)
{
	int err;

	printk("Entered, %s mode fps %d\n",
		s5c73m3_ctrl->camcorder_mode ? "camcorder" : "camera",
		fps);

	/* It can't support camera mode */
	if (s5c73m3_ctrl->camcorder_mode == 0) {
		printk("set default fps\n");
		err = s5c73m3_writeb(S5C73M3_AE_MODE,
			S5C73M3_AUTO_MODE_AE_SET);
		CHECK_ERR(err);
	} else {
		switch (fps) {
		case 30:
			printk("set 30fps\n");
			err = s5c73m3_writeb(S5C73M3_AE_MODE,
				S5C73M3_FIXED_30FPS);
			CHECK_ERR(err);
			break;

		case 20:
			printk("set 20fps\n");
			err = s5c73m3_writeb(S5C73M3_AE_MODE,
				S5C73M3_FIXED_20FPS);
			CHECK_ERR(err);
			break;

		case 15:
			printk("set 15fps\n");
			err = s5c73m3_writeb(S5C73M3_AE_MODE,
				S5C73M3_FIXED_15FPS);
			CHECK_ERR(err);
			break;

		case 7:
			printk("set 7fps\n");
			err = s5c73m3_writeb(S5C73M3_AE_MODE,
				S5C73M3_FIXED_7FPS);
			CHECK_ERR(err);
		break;

		default:
			cam_err("invalid value, %d\n", fps);
		}
	}

	s5c73m3_ctrl->fps = fps;

	return 0;
}

static int s5c73m3_set_face_zoom(int val)
{
	int err;

	CAM_DBG_M("s5c73m3_set_face_zoom, %d\n", val);
	CAM_DBG_H("Touch Position(%d,%d)\n",
		camera_focus.pos_x, camera_focus.pos_y);
	CAM_DBG_H("Preview Size(%d,%d)\n",
		s5c73m3_ctrl->preview_size_width,
		s5c73m3_ctrl->preview_size_height);

	err = s5c73m3_writeb(S5C73M3_AF_CON,
		S5C73M3_AF_CON_STOP);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0xfcfc, 0x3310);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0050, 0x0009);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0054, S5C73M3_AF_TOUCH_POSITION);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, camera_focus.pos_x);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, camera_focus.pos_y);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, s5c73m3_ctrl->preview_size_width);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, s5c73m3_ctrl->preview_size_height);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0050, 0x0009);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0054, 0x5000);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, S5C73M3_AF_FACE_ZOOM);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, val); /*0:reset, 1:Start*/
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0054, 0x5080);
	CHECK_ERR(err);

	err = s5c73m3_i2c_write(0x0F14, 0x0001);
	CHECK_ERR(err);

	udelay(400);
	err = s5c73m3_writeb(S5C73M3_AF_MODE,
		S5C73M3_AF_MODE_PREVIEW_CAF_START);
	CHECK_ERR(err);

	return 0;
}

static int s5c73m3_set_vdis(int onoff)
{
	int32_t rc = 0;
	CAM_DBG_H("Entered, VDIS onoff %d\n", onoff);

	switch (onoff) {
	case 0:
		s5c73m3_ctrl->vdis_mode = false;
		break;

	case 1:
		s5c73m3_ctrl->vdis_mode = true;
		break;

	default:
		CAM_DBG_M("unexpected ev mode\n");
		break;
	}
	return rc;
}

static int s5c73m3_get_lux(void)
{
	int err = 0;
	unsigned short int lux_val = 0;

	err = s5c73m3_read(0x0009, 0x5C88, &lux_val);
	if (err < 0) {
		cam_err("failed s5c73m3_read!!\n");
		return -EIO;
	}

	return lux_val;
}

static int s5c73m3_set_preview_size(int32_t width, int32_t height)
{
	printk("Entered, width %d, height %d\n",
			 width, height);
	s5c73m3_ctrl->preview_size_width = width;
	s5c73m3_ctrl->preview_size_height = height;

	if ((width == 1008 && height == 672) ||
		(width == 720 && height == 480))
		s5c73m3_ctrl->preview_size = 0x000F;
	else if (width == 352 && height == 288)
		s5c73m3_ctrl->preview_size = 0x000E;
	else if (width == 3264 && height == 2448)
		s5c73m3_ctrl->preview_size = 0x000D;
	else if (width == 2304 && height == 1296)
		s5c73m3_ctrl->preview_size = 0x000C;
	else if (width == 1920 && height == 1080)
		s5c73m3_ctrl->preview_size = 0x000A;
	else if (width == 800 && height == 600)
		s5c73m3_ctrl->preview_size = 0x0009;
	else if (width == 1600 && height == 1200)
		s5c73m3_ctrl->preview_size = 0x0008;
	else if (width == 1536 && height == 864)
		s5c73m3_ctrl->preview_size = 0x0007;
	else if (width == 1280 && height == 720)
		s5c73m3_ctrl->preview_size = 0x0006;
	else if (width == 1184 && height == 666)
		s5c73m3_ctrl->preview_size = 0x0005;
	else if (width == 960 && height == 720)
		s5c73m3_ctrl->preview_size = 0x0004;
	else if (width == 880 && height == 720)
		s5c73m3_ctrl->preview_size = 0x0003;
	else if (width == 640 && height == 480)
		s5c73m3_ctrl->preview_size = 0x0002;
	else if (width == 320 && height == 240)
		s5c73m3_ctrl->preview_size = 0x0001;
	else
		s5c73m3_ctrl->preview_size = 0x0004;

	if ((width == 1920 && height == 1080) ||
		(width == 1280 && height == 720) ||
		(width == 720 && height == 480))
		s5c73m3_ctrl->wide_preview = 1;
	else
		s5c73m3_ctrl->wide_preview = 0;

	return 0;
}

static int s5c73m3_load_fw(void)
{
#if defined(SPI_DMA_MODE)
	#define FW_WRITE_SIZE 65536
#endif

	int err = 0, txSize;

	struct file *fp = NULL;
	mm_segment_t old_fs;
	long fsize, nread;

	CAM_DBG_M("Entered\n");
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (s5c73m3_ctrl->fw_index == S5C73M3_SD_CARD ||
		s5c73m3_ctrl->fw_index == S5C73M3_IN_DATA) {

		if (s5c73m3_ctrl->fw_index == S5C73M3_SD_CARD)
			fp = filp_open(S5C73M3_FW_PATH, O_RDONLY, 0);
		else
			fp = filp_open(fw_path_in_data, O_RDONLY, 0);
		if (IS_ERR(fp)) {
			cam_err("failed to filp_open\n");
			goto out;
		}
	} else {
		fp = filp_open(fw_path, O_RDONLY, 0);
		if (IS_ERR(fp)) {
			cam_err("failed to filp_open\n");
			goto out;
		}
	}

	fsize = fp->f_path.dentry->d_inode->i_size;

	printk("index %d is opened\n",
		s5c73m3_ctrl->fw_index);
	printk("fsize is %ld\n", fsize);

	Fbuf = (char *)roundup((unsigned int)FW_buf, 64); /*ALRAN 64*/
	nread = vfs_read(fp, (char __user *)Fbuf,
		fsize, &fp->f_pos);
	if (nread != fsize) {
		cam_err("failed to read firmware file, %ld Bytes\n", nread);
		err = -EIO;
		goto out;
	}
	set_fs(old_fs);

#if defined(SPI_DMA_MODE)
	txSize = FW_WRITE_SIZE; /*QCTK*/
#else
	txSize = 8;
#endif
	err = s5c73m3_spi_write(Fbuf, fsize, txSize);

	if (err < 0) {
		cam_err("s5c73m3_spi_write falied\n");
		goto out;
	}

	CAM_DBG_M("Exit\n");

out:
	filp_close(fp, current->files);
	set_fs(old_fs);

	return err;
}

static int s5c73m3_SPI_booting(void)
{
	u16 read_val;
	int i, err = 0;
	
	CAM_DBG_M("Entered\n");

	/*ARM go*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFF);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}

	udelay(400);

	/*Check boot done*/
	for (i = 0; i < 3; i++) {
		err = s5c73m3_read(0x3010, 0x0010, &read_val);
		if (err < 0) {
			cam_err("failed s5c73m3_read!!\n");
			return -EIO;
		}
		if (read_val == 0x0C)
			break;

		udelay(100);
	}

	if (read_val != 0x0C) {
		cam_err("boot fail, read_val %#x\n", read_val);
		return -EIO;
	}

       /*P,M,S and Boot Mode*/
	err = s5c73m3_write(0x3010, 0x0014, 0x2146);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}
	err = s5c73m3_write(0x3010, 0x0010, 0x210C);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}

	udelay(200);

	/*Check SPI ready*/
	for (i = 0; i < 3; i++) {
		err = s5c73m3_read(0x3010, 0x0010, &read_val);
		if (err < 0) {
			cam_err("failed s5c73m3_read!!\n");
			return -EIO;
		}

		if (read_val == 0x210D)
			break;

		udelay(100);
	}

	if (read_val != 0x210D) {
		cam_err("SPI not ready, read_val %#x\n", read_val);
		return -EIO;
	}

	/*download fw by SPI*/
	err = s5c73m3_load_fw();
	if (err < 0) {
		cam_err("failed s5c73m3_load_fw!!\n");
		return -EIO;
	}

	/*ARM reset*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFD);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}

	/*remap*/
	err = s5c73m3_write(0x3010, 0x00A4, 0x0183);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}

#if defined(DEBUG_LEVEL_HIGH)
	/* check FW version name */
	printk("FW version is : ");
	for (i = 0; i < 19; i++) {
		err = s5c73m3_read(0x0000, 0x0060+i, &read_val);
		if (err < 0) {
			cam_err("failed s5c73m3_read!!\n");
			return -EIO;
		}
		CAM_DBG_H("%c%c", read_val&0xff,
		       (read_val>>8)&0xff);
		i++;
	}
#endif

	/*ARM go again*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFF);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}

	return 0;
}

static int s5c73m3_dump_fw(void)
{
	return 0;
}

static int s5c73m3_get_sensor_fw_binary(void)
{
	u16 read_val;
	int i, rxSize;
	int err = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long ret = 0;
	char l_fw_path[40] = {0};
	u8 mem0 = 0, mem1 = 0;
	u32 CRC = 0;
	u32 DataCRC = 0;
	u32 IntOriginalCRC = 0;
	u32 crc_index = 0;
	int retryCnt = 2;
	CAM_DBG_M("Entered\n");

	if (s5c73m3_ctrl->sensor_fw[0] == 'O') {
		sprintf(l_fw_path, "%sSlimISP_G%c.bin",
			S5C73M3_FW_REQUEST_SECOND_PATH,
			s5c73m3_ctrl->sensor_fw[1]);
	} else if (s5c73m3_ctrl->sensor_fw[0] == 'S') {
		sprintf(l_fw_path, "%sSlimISP_Z%c.bin",
			S5C73M3_FW_REQUEST_SECOND_PATH,
			s5c73m3_ctrl->sensor_fw[1]);
	} else {
		sprintf(l_fw_path, "%sSlimISP_%c%c.bin",
			S5C73M3_FW_REQUEST_SECOND_PATH,
			s5c73m3_ctrl->sensor_fw[0],
			s5c73m3_ctrl->sensor_fw[1]);
	}

	/* Make CRC Table */
	s5c73m3_make_CRC_table((u32 *)&s5c73m3_ctrl->crc_table, 0xEDB88320);

	/*ARM go*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFF);
	CHECK_ERR(err);

	udelay(400);

	/*Check boot done*/
	for (i = 0; i < 3; i++) {
		err = s5c73m3_read(0x3010, 0x0010, &read_val);
		CHECK_ERR(err);

		if (read_val == 0x0C)
			break;

		udelay(100);
	}

	if (read_val != 0x0C) {
		cam_err("boot fail, read_val %#x\n", read_val);
		return -EINVAL;
	}

	/* Change I/O Driver Current in order to read from F-ROM */
	err = s5c73m3_write(0x3010, 0x0120, 0x0820);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3010, 0x0124, 0x0820);
	CHECK_ERR(err);

	/*P,M,S and Boot Mode*/
	err = s5c73m3_write(0x3010, 0x0014, 0x2146);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3010, 0x0010, 0x230C);
	CHECK_ERR(err);

	udelay(200);

	/*Check SPI ready*/
	for (i = 0; i < 300; i++) {
		err = s5c73m3_read(0x3010, 0x0010, &read_val);
		CHECK_ERR(err);

		if (read_val == 0x230E)
			break;

		udelay(100);
	}

	if (read_val != 0x230E) {
		cam_err("SPI not ready, read_val %#x\n", read_val);
		return -EINVAL;
	}

	/*ARM reset*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFD);
	CHECK_ERR(err);

	/*remap*/
	err = s5c73m3_write(0x3010, 0x00A4, 0x0183);
	CHECK_ERR(err);

	/*ARM go again*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFF);
	CHECK_ERR(err);

	mdelay(200);

retry:
	mem0 = 0, mem1 = 0;
	CRC = 0;
	DataCRC = 0;
	IntOriginalCRC = 0;
	crc_index = 0;

	/* SPI Copy mode ready I2C CMD */
	err = s5c73m3_writeb(0x0924, 0x0000);
	CHECK_ERR(err);
	CAM_DBG_M("sent SPI ready CMD\n");

	rxSize = 64*1024;
	mdelay(10);

	/* check whether ISP can be used */
        err = s5c73m3_wait_ISP_status();
        if (err < 0) {
                cam_err("failed s5c73m3_wait_ISP_status\n");
                return -EIO;
        }

	Fbuf = (char *)roundup((unsigned int)FW_buf, 64); /*ALRAN 64*/
	err = s5c73m3_spi_read(Fbuf,
		s5c73m3_ctrl->sensor_size, rxSize);
	CHECK_ERR(err);

	CRC = ~CRC;
	for (crc_index = 0; crc_index < (s5c73m3_ctrl->sensor_size-4)/2
		; crc_index++) {
		/*low byte*/
		mem0 = (unsigned char)(Fbuf[crc_index*2] & 0x00ff);
		/*high byte*/
		mem1 = (unsigned char)(Fbuf[crc_index*2+1] & 0x00ff);
		CRC = s5c73m3_ctrl->crc_table[(CRC ^ (mem0)) & 0xFF] \
			^ (CRC >> 8);
		CRC = s5c73m3_ctrl->crc_table[(CRC ^ (mem1)) & 0xFF] \
			^ (CRC >> 8);
	}
	CRC = ~CRC;

	DataCRC = (CRC&0x000000ff)<<24;
	DataCRC += (CRC&0x0000ff00)<<8;
	DataCRC += (CRC&0x00ff0000)>>8;
	DataCRC += (CRC&0xff000000)>>24;
	CAM_DBG_M("made CSC value by S/W = 0x%x\n", DataCRC);

	IntOriginalCRC = (Fbuf[s5c73m3_ctrl->sensor_size-4]&0x00ff)<<24;
	IntOriginalCRC += (Fbuf[s5c73m3_ctrl->sensor_size-3]&0x00ff)<<16;
	IntOriginalCRC += (Fbuf[s5c73m3_ctrl->sensor_size-2]&0x00ff)<<8;
	IntOriginalCRC += (Fbuf[s5c73m3_ctrl->sensor_size-1]&0x00ff);
	CAM_DBG_M("Original CRC Int = 0x%x\n", IntOriginalCRC);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (IntOriginalCRC == DataCRC) {
		fp = filp_open(l_fw_path, O_WRONLY|O_CREAT, 0644);
		if (IS_ERR(fp) || fp == NULL) {
			cam_err("failed to open %s, err %ld\n",
				l_fw_path, PTR_ERR(fp));
			err = -EINVAL;
			goto out;
		}

		ret = vfs_write(fp, (char __user *)Fbuf,
			s5c73m3_ctrl->sensor_size, &fp->f_pos);

		memcpy(s5c73m3_ctrl->phone_fw,
			s5c73m3_ctrl->sensor_fw,
			S5C73M3_FW_VER_LEN);
		s5c73m3_ctrl->phone_fw[S5C73M3_FW_VER_LEN+1] = ' ';

		CAM_DBG_M("Changed to Phone_version = %s\n",
			s5c73m3_ctrl->phone_fw);
	} else {
		if (retryCnt > 0) {
			set_fs(old_fs);
			retryCnt--;
			goto retry;
		}
	}

	if (fp != NULL)
		filp_close(fp, current->files);

out:
	set_fs(old_fs);

	CAM_DBG_M("Exit\n");

	return err;
}

static int s5c73m3_get_sensor_fw_version(void)
{
	u16 read_val;
	int i, err;
	u16 temp_buf;

	CAM_DBG_H("Entered\n");
	/*ARM go*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFF);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}

	udelay(400);

	/*Check boot done*/
	for (i = 0; i < 3; i++) {
		err = s5c73m3_read(0x3010, 0x0010, &read_val);
		if (err < 0) {
			cam_err("failed s5c73m3_read!!\n");
			return -EIO;
		}
		if (read_val == 0x0C)
			break;

		udelay(100);
	}

	if (read_val != 0x0C) {
		cam_err("boot fail, read_val %#x\n", read_val);
		return -EIO;
	}

	/* Change I/O Driver Current in order to read from F-ROM */
	err = s5c73m3_write(0x3010, 0x0120, 0x0820);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}
	err = s5c73m3_write(0x3010, 0x0124, 0x0820);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}

	/* Offset Setting */
	err = s5c73m3_write(0x0001, 0x0418, 0x0008);
	CHECK_ERR(err);

	   /*P,M,S and Boot Mode*/
	err = s5c73m3_write(0x3010, 0x0014, 0x2146);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}
	err = s5c73m3_write(0x3010, 0x0010, 0x230C);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}

	udelay(200);

	/*Check SPI ready*/
	for (i = 0; i < 300; i++) {
		err = s5c73m3_read(0x3010, 0x0010, &read_val);
		if (err < 0) {
			cam_err("failed s5c73m3_read!!\n");
			return -EIO;
		}

		if (read_val == 0x230E)
			break;

		udelay(100);
	}

	if (read_val != 0x230E) {
		cam_err("SPI not ready, read_val %#x\n", read_val);
		return -EIO;
	}

	/*ARM reset*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFD);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}

	/*remap*/
	err = s5c73m3_write(0x3010, 0x00A4, 0x0183);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}

#if defined(TEMP_REMOVE)
	/*ARM go again*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFF);
	if (err < 0) {
		cam_err("failed s5c73m3_write!!\n");
		return -EIO;
	}

	/* check FW version name */
	for (i = 0; i < 3; i++) {
		err = s5c73m3_read(0x0000, 0x0060+i*2, &read_val);
		if (err < 0) {
			cam_err("failed s5c73m3_read!!\n");
			return -EIO;
		}
		s5c73m3_ctrl->sensor_fw[i*2] = read_val&0x00ff;
		s5c73m3_ctrl->sensor_fw[i*2+1] = (read_val&0xff00)>>8;
	}
	s5c73m3_ctrl->sensor_fw[i*2+2] = ' ';
#else
	for (i = 0; i < 3; i++) {
		err = s5c73m3_read(0x0000, i*2, &read_val);
		if (err < 0) {
			cam_err("failed s5c73m3_read!!\n");
			return -EIO;
		}
		s5c73m3_ctrl->sensor_fw[i*2] = read_val&0x00ff;
		s5c73m3_ctrl->sensor_fw[i*2+1] = (read_val&0xff00)>>8;
	}
	s5c73m3_ctrl->sensor_fw[i*2+2] = ' ';
#endif

	s5c73m3_ctrl->sensor_size = 0;
	for (i = 0; i < 2; i++) {
		err = s5c73m3_read(0x0000, 0x0014+i*2, &temp_buf);
		s5c73m3_ctrl->sensor_size += temp_buf<<(i*16);
		CHECK_ERR(err);
	}

	if ((s5c73m3_ctrl->sensor_fw[0] >= 'A')
		&& s5c73m3_ctrl->sensor_fw[0] <= 'Z') {
		cam_err("sensor_fw = %s\n",
			s5c73m3_ctrl->sensor_fw);
		return 0;
	} else {
		cam_err("Sensor is not connected!!\n");
		return -EIO;
	}
}

static int s5c73m3_open_firmware_file(const char *filename,
	u8 *buf, u16 offset, u16 size) {
	struct file *fp;
	int err = 0;
	mm_segment_t old_fs;
	long nread;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		err = -ENOENT;
		goto out;
	} else {
		CAM_DBG_H("%s is opened\n", filename);
	}

	err = vfs_llseek(fp, offset, SEEK_SET);
	if (err < 0) {
		cam_err("failed to fseek, %d\n", err);
		goto out;
	}

	nread = vfs_read(fp, (char __user *)buf, size, &fp->f_pos);

	if (nread != size) {
		cam_err("failed to read firmware file, %ld Bytes\n", nread);
		err = -EIO;
		goto out;
	}
out:
	if (!IS_ERR(fp))
		filp_close(fp, current->files);

	set_fs(old_fs);

	return err;
}

static int s5c73m3_compare_date(char *camfw_info_ver1,
	char *camfw_info_ver2)
{
	u8 date1[5] = {0,};
	u8 date2[5] = {0,};

	strncpy((char *)&date1, camfw_info_ver1, 4);
	strncpy((char *)&date2, camfw_info_ver2, 4);
	CAM_DBG_M("date1 = %s, date2 = %s\n, compare result = %d",
		date1,
		date2,
		strcmp((char *)&date1, (char *)&date2));

	return strcmp((char *)&date1, (char *)&date2);
}

static int s5c73m3_get_phone_fw_version(void)
{
#if defined(SPI_DMA_MODE)
#define FW_WRITE_SIZE 65536
#endif

	static char *buf; /*static*/
	int err = 0;
	int retVal = 0;
	int fw_requested = 1;

	CAM_DBG_M("Entered\n");

	buf = vmalloc(S5C73M3_FW_VER_LEN+1);
	if (!buf) {
		cam_err("failed to allocate memory\n");
		err = -ENOMEM;
		goto out;
	}

	if (s5c73m3_ctrl->sensor_fw[0] == 'O') {
		sprintf(fw_path, "%sSlimISP_G%c.bin",
			S5C73M3_FW_REQUEST_PATH,
			s5c73m3_ctrl->sensor_fw[1]);
		sprintf(fw_path_in_data, "%sSlimISP_G%c.bin",
			S5C73M3_FW_REQUEST_SECOND_PATH,
			s5c73m3_ctrl->sensor_fw[1]);
	} else if (s5c73m3_ctrl->sensor_fw[0] == 'S') {
		sprintf(fw_path, "%sSlimISP_Z%c.bin",
			S5C73M3_FW_REQUEST_PATH,
			s5c73m3_ctrl->sensor_fw[1]);
		sprintf(fw_path_in_data, "%sSlimISP_G%c.bin",
			S5C73M3_FW_REQUEST_SECOND_PATH,
			s5c73m3_ctrl->sensor_fw[1]);
	} else {
		sprintf(fw_path, "%sSlimISP_%c%c.bin",
			S5C73M3_FW_REQUEST_PATH,
			s5c73m3_ctrl->sensor_fw[0],
			s5c73m3_ctrl->sensor_fw[1]);
		sprintf(fw_path_in_data, "%sSlimISP_%c%c.bin",
			S5C73M3_FW_REQUEST_SECOND_PATH,
			s5c73m3_ctrl->sensor_fw[0],
			s5c73m3_ctrl->sensor_fw[1]);
	}

	retVal = s5c73m3_open_firmware_file(S5C73M3_FW_PATH,
		buf,
		S5C73M3_FW_VER_FILE_CUR,
		S5C73M3_FW_VER_LEN);
	if (retVal >= 0) {
		camfw_info[S5C73M3_SD_CARD].opened = 1;
		memcpy(camfw_info[S5C73M3_SD_CARD].ver,
			buf,
			S5C73M3_FW_VER_LEN);
		camfw_info[S5C73M3_SD_CARD]
			.ver[S5C73M3_FW_VER_LEN+1] = '\0';
		s5c73m3_ctrl->fw_index = S5C73M3_SD_CARD;
		fw_requested = 0;
	}

request_fw:
	if (fw_requested) {
		/* check fw in data folder */
		retVal = s5c73m3_open_firmware_file(fw_path_in_data,
			buf,
			S5C73M3_FW_VER_FILE_CUR,
			S5C73M3_FW_VER_LEN);
		if (retVal >= 0) {
			camfw_info[S5C73M3_IN_DATA].opened = 1;
			memcpy(camfw_info[S5C73M3_IN_DATA].ver,
				buf,
				S5C73M3_FW_VER_LEN);
			camfw_info[S5C73M3_IN_DATA]
				.ver[S5C73M3_FW_VER_LEN+1] = '\0';
		}

		/* check fw in system folder */
		retVal = s5c73m3_open_firmware_file(fw_path,
			buf,
			S5C73M3_FW_VER_FILE_CUR,
			S5C73M3_FW_VER_LEN);
		if (retVal >= 0) {
			camfw_info[S5C73M3_IN_SYSTEM].opened = 1;
			memcpy(camfw_info[S5C73M3_IN_SYSTEM].ver,
				buf,
				S5C73M3_FW_VER_LEN);
			camfw_info[S5C73M3_IN_SYSTEM]
				.ver[S5C73M3_FW_VER_LEN+1] = '\0';
		}

		/* compare */
		if (camfw_info[S5C73M3_IN_DATA].opened == 0 &&
			camfw_info[S5C73M3_IN_SYSTEM].opened == 1)  {
			s5c73m3_ctrl->fw_index = S5C73M3_IN_SYSTEM;
		} else if (camfw_info[S5C73M3_IN_DATA].opened == 1 &&
			camfw_info[S5C73M3_IN_SYSTEM].opened == 0) {
			s5c73m3_ctrl->fw_index = S5C73M3_IN_DATA;
		} else if (camfw_info[S5C73M3_IN_DATA].opened == 1 &&
			camfw_info[S5C73M3_IN_SYSTEM].opened == 1) {
			int i = 1;
			retVal = s5c73m3_compare_date(
			(char *)(&camfw_info[S5C73M3_IN_DATA].ver[2]),
			(char *)(&camfw_info[S5C73M3_IN_SYSTEM].ver[2]));

			while (1) {
				if (camfw_info[S5C73M3_IN_DATA].ver[i] !=
					camfw_info[S5C73M3_IN_SYSTEM].ver[i]) {
					cam_err("FW is diff!\n");
					retVal = 1;
					break;
				}
				if (--i < 0) {
					CAM_DBG_M("FW is same!!\n");
					break;
				}
			}
			if (retVal <= 0) {
				/*unlink(&fw_path_in_data);*/
				s5c73m3_ctrl->fw_index = S5C73M3_IN_SYSTEM;
			} else {
				s5c73m3_ctrl->fw_index = S5C73M3_IN_DATA;
			}
		} else {
			CAM_DBG_M("can't open %s. download from F-ROM\n",
				s5c73m3_ctrl->sensor_fw);

			s5c73m3_reset_module(true);

			retVal = s5c73m3_get_sensor_fw_binary();
			CHECK_ERR(retVal);
			goto request_fw;
		}
	}

	memcpy(s5c73m3_ctrl->phone_fw,
		camfw_info[s5c73m3_ctrl->fw_index].ver,
		S5C73M3_FW_VER_LEN);
	s5c73m3_ctrl->phone_fw[S5C73M3_FW_VER_LEN+1] = '\0';
	CAM_DBG_M("Phone_version = %s(index=%d)\n",
		s5c73m3_ctrl->phone_fw, s5c73m3_ctrl->fw_index);

out:
	if (buf != NULL)
		vfree(buf);

	CAM_DBG_M("Exit\n");
	return err;
}

static int s5c73m3_update_camerafw_to_FROM(void)
{
	int err;
	int index = 0;
	u16 status = 0;

	do {
		/* stauts 0 : not ready ISP */
		if (status == 0) {
			err = s5c73m3_writeb(0x0906, 0x0000);
			CHECK_ERR(err);
		}

		err = s5c73m3_read(0x0009, 0x5906, &status);
		/* Success : 0x05, Fail : 0x07 , Progressing : 0xFFFF*/
		if (status == 0x0005 ||
			status == 0x0007)
			break;

		index++;
		msleep(20);
	} while (index < 500);	/* 10 sec */


	if (status == 0x0007)
		return -EIO;
	else
		return 0;
}
#ifndef CONFIG_S5C73M3
static int s5c73m3_SPI_booting_by_ISP(void)
{
	u16 read_val;
	int i;
	int err = 0;
	/*ARM go*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFF);
	CHECK_ERR(err);

	udelay(400);

	/*Check boot done*/
	for (i = 0; i < 3; i++) {
		err = s5c73m3_read(0x3010, 0x0010, &read_val);
		CHECK_ERR(err);

		if (read_val == 0x0C)
			break;

		udelay(100);
	}

	if (read_val != 0x0C) {
		cam_err("boot fail, read_val %#x\n", read_val);
		return -1;
	}

	/*P,M,S and Boot Mode*/
	err = s5c73m3_write(0x3010, 0x0014, 0x2146);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3010, 0x0010, 0x230C);
	CHECK_ERR(err);

	udelay(200);

	/*Check SPI ready*/
	for (i = 0; i < 300; i++) {
		err = s5c73m3_read(0x3010, 0x0010, &read_val);
		CHECK_ERR(err);

		if (read_val == 0x230E)
			break;

		udelay(100);
	}

	if (read_val != 0x230E) {
		cam_err("SPI not ready, read_val %#x\n", read_val);
		return -1;
	}

	/*ARM reset*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFD);
	CHECK_ERR(err);

	/*remap*/
	err = s5c73m3_write(0x3010, 0x00A4, 0x0183);
	CHECK_ERR(err);

	/*ARM go*/
	err = s5c73m3_write(0x3000, 0x0004, 0xFFFF);
	CHECK_ERR(err);

	return err;
}
#endif
static int s5c73m3_check_fw_date(void)
{
	u8 sensor_date[5] = {0,};
	u8 phone_date[5] = {0,};

	int i = 1;
	while (1) {
		if (s5c73m3_ctrl->sensor_fw[i] !=
			s5c73m3_ctrl->phone_fw[i]) {
			cam_err("Sensor is diff!\n");
			cam_err("Sensor_date = %s, "
				"Phone_date = %s\n",
				s5c73m3_ctrl->sensor_fw,
				s5c73m3_ctrl->phone_fw);
			return 1;
		}
		if (--i < 0) {
			CAM_DBG_M("FW is same!!\n");
			break;
		}
	}

	strncpy((char *)&sensor_date,
		&s5c73m3_ctrl->sensor_fw[2], 4);
	strncpy((char *)&phone_date,
		(const char *)&s5c73m3_ctrl->phone_fw[2], 4);
	CAM_DBG_M("Sensor_date = %s, Phone_date = %s\n, "
		"compare result = %d",
		sensor_date,
		phone_date,
		strcmp((char *)&sensor_date, (char *)&phone_date));

	return strcmp((char *)&sensor_date, (char *)&phone_date);
}

static int s5c73m3_check_fw(const struct msm_camera_sensor_info *data,
					bool download)
{
	int err = 0;
	int retVal;
	int i = 0;
	CAM_DBG_M("Enter\n");

	if (!download) {
		for (i = 0; i < S5C73M3_PATH_MAX; i++)
			camfw_info[i].opened = 0;

		err = s5c73m3_get_sensor_fw_version();
		if (err < 0) {
			cam_err("failed s5c73m3_get_sensor_fw_version!!\n");
			return -EIO;
		}
		err = s5c73m3_get_phone_fw_version();
		if (err < 0) {
			cam_err("failed s5c73m3_get_phone_fw_version!!\n");
			return -EIO;
		}
	}

	data->sensor_platform_info->sensor_get_fw(s5c73m3_ctrl->sensor_fw,
		s5c73m3_ctrl->phone_fw);

	retVal = s5c73m3_check_fw_date();

	/* retVal = 0 : Same Version
	    retVal < 0 : Phone Version is latest Version than sensorFW.
	    retVal > 0 : Sensor Version is latest version than phoenFW. */
	if (1) {
		if (s5c73m3_ctrl->fw_index == 0)
			CAM_DBG_M("Loading From PhoneFW forced......\n");
		else
			CAM_DBG_M("Loading From PhoneFW......\n");

		if ((s5c73m3_ctrl->phone_fw[0] >= 'A')
			&& s5c73m3_ctrl->phone_fw[0] <= 'Z') {
			s5c73m3_reset_module(false);
			err = s5c73m3_SPI_booting();
			if (err < 0) {
				cam_err("failed s5c73m3_SPI_booting!!\n");
				return -EIO;
			}
			if (download) {
				err = s5c73m3_update_camerafw_to_FROM();
				if (err < 0) {
					cam_err("failed s5c73m3_update_camerafw_to_FROM!!\n");
					return -EIO;
				}
			}
		} else {
			cam_err("phone FW is wrong!!\n");
			return -EIO;
		}
	} else {
		CAM_DBG_M("Loading From SensorFW......\n");
		s5c73m3_reset_module(true);
		err = s5c73m3_get_sensor_fw_binary();
		if (err < 0) {
			cam_err("failed s5c73m3_get_sensor_fw_binary!!\n");
			return -EIO;
		}
		/* check fw in data folder */
		{
		static char *buf; /*static*/
		buf = vmalloc(S5C73M3_FW_VER_LEN+1);
		if (buf == NULL) {
			cam_err("Mem allocation failed\n");
			return -EIO;
		}
		retVal = s5c73m3_open_firmware_file(fw_path_in_data,
			buf,
			S5C73M3_FW_VER_FILE_CUR,
			S5C73M3_FW_VER_LEN);
		if (retVal >= 0) {
			camfw_info[S5C73M3_IN_DATA].opened = 1;
			memcpy(camfw_info[S5C73M3_IN_DATA].ver,
				buf,
				S5C73M3_FW_VER_LEN);
			camfw_info[S5C73M3_IN_DATA]
				.ver[S5C73M3_FW_VER_LEN+1] = '\0';
			s5c73m3_ctrl->fw_index = S5C73M3_IN_DATA;
			memcpy(s5c73m3_ctrl->phone_fw,
				camfw_info[s5c73m3_ctrl->fw_index].ver,
				S5C73M3_FW_VER_LEN);
			s5c73m3_ctrl->phone_fw[S5C73M3_FW_VER_LEN+1] = '\0';
			CAM_DBG_M("FW is %s!!\n", &s5c73m3_ctrl->phone_fw[0]);
			data->sensor_platform_info->sensor_get_fw(
				s5c73m3_ctrl->sensor_fw,
				s5c73m3_ctrl->phone_fw);
		} else
			cam_err("Warnning!! can't check FW!\n");
		if (buf)
			vfree(buf);
		}
	}
#if defined(TEMP_REMOVE)
	data->sensor_platform_info->sensor_get_fw(&s5c73m3_ctrl->sensor_fw,
		&s5c73m3_ctrl->phone_fw);

	if ((s5c73m3_ctrl->phone_fw[0] >= 'A')
		&& s5c73m3_ctrl->phone_fw[0] <= 'Z') {
		s5c73m3_sensor_reset();

		err = s5c73m3_SPI_booting();
		if (err < 0) {
			cam_err("failed s5c73m3_SPI_booting!!\n");
			return -EIO;
		}
	}
#endif

	CAM_DBG_M("Exit\n");
	return 0;
}
#ifndef CONFIG_S5C73M3
static int s5c73m3_init_param(void)
{
	int err = 0;

	CAM_DBG_H("Entered\n");
	err = s5c73m3_i2c_write_block(S5C73M3_INIT,
		sizeof(S5C73M3_INIT)/sizeof(S5C73M3_INIT[0]));

	if (err < 0) {
		cam_err("failed s5c73m3_write_block!!\n");
		return -EIO;
	}

	return err;
}
#endif
static int s5c73m3_read_vdd_core(void)
{
	u16 read_val;
	int err;

	CAM_DBG_M("Entered\n");

	/*Initialize OTP Controller*/
	err = s5c73m3_write(0x3800, 0xA004, 0x0000);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA000, 0x0004);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA0D8, 0x0000);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA0DC, 0x0004);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA0C4, 0x4000);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA0D4, 0x0015);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA000, 0x0001);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA0B4, 0x9F90);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA09C, 0x9A95);
	CHECK_ERR(err);

	/*Page Select*/
	err = s5c73m3_write(0x3800, 0xA0C4, 0x4800);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA0C4, 0x4400);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA0C4, 0x4200);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA004, 0x00C0);
	CHECK_ERR(err);
	err = s5c73m3_write(0x3800, 0xA000, 0x0001);
	CHECK_ERR(err);

	/*Read Data*/
	err = s5c73m3_read(0x3800, 0xA034, &read_val);
	CHECK_ERR(err);
	CAM_DBG_M("vdd_core_info %#x\n", read_val);

	err = s5c73m3_read(0x3800, 0xA040, &read_val);
	CHECK_ERR(err);
	CAM_DBG_M("chip info#1 %#x\n", read_val);

	err = s5c73m3_read(0x3800, 0xA044, &read_val);
	CHECK_ERR(err);
	CAM_DBG_M("chip info#2 %#x\n", read_val);

	err = s5c73m3_read(0x3800, 0xA048, &read_val);
	CHECK_ERR(err);
	CAM_DBG_M("chip info#3 %#x\n", read_val);

	/*Read Data End*/
	err = s5c73m3_write(0x3800, 0xA000, 0x0000);
	CHECK_ERR(err);

	if (read_val & 0x200)
		s5c73m3_ctrl->sensordata->sensor_platform_info
		->sensor_set_isp_core(1150000);
	else if (read_val & 0x800)
		s5c73m3_ctrl->sensordata->sensor_platform_info
		->sensor_set_isp_core(1100000);
	else if (read_val & 0x2000)
		s5c73m3_ctrl->sensordata->sensor_platform_info
		->sensor_set_isp_core(1050000);
	else if (read_val & 0x8000)
		s5c73m3_ctrl->sensordata->sensor_platform_info
		->sensor_set_isp_core(1000000);
	else
#if defined(CONFIG_MACH_M2_DCM)
		s5c73m3_ctrl->sensordata->sensor_platform_info
		->sensor_set_isp_core(1230000);
#else
		s5c73m3_ctrl->sensordata->sensor_platform_info
		->sensor_set_isp_core(1150000);
#endif

	CAM_DBG_H("X\n");

	return 0;
}

static int s5c73m3_set_af_softlanding(void)
{
	int err = 0;
	CAM_DBG_M("Entered\n");

	err = s5c73m3_writeb(S5C73M3_AF_SOFTLANDING,
		S5C73M3_AF_SOFTLANDING_ON);
	CHECK_ERR(err);

	return 0;
}

static int s5c73m3_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	int retVal = 0;
	CAM_DBG_M("Entered\n");

	if (!data->sensor_platform_info->sensor_is_vdd_core_set()) {
		rc = s5c73m3_read_vdd_core();
		if (rc < 0) {
			cam_err("failed s5c73m3_read_vdd_core!!\n");
			return -EIO;
		}
	}
	rc = s5c73m3_set_timing_register_for_vdd();
	CHECK_ERR(rc);

	rc = s5c73m3_check_fw(data, 0);
	/*rc = s5c73m3_SPI_booting();*/
	if (rc < 0) {
		cam_err("failed s5c73m3_check_fw!!\n");
		return -EIO;
	}

	rc = s5c73m3_i2c_check_status_with_CRC();
	if (rc < 0) {
		cam_err("ISP is not ready. retry loading fw!!\n");
		/* retry */
		retVal = s5c73m3_check_fw_date();

		/* retVal = 0 : Same Version
		retVal < 0 : Phone Version is latest Version than sensorFW.
		retVal > 0 : Sensor Version is latest version than phoenFW. */
		if (1) {
			cam_err("Loading From PhoneFW......\n");
			s5c73m3_reset_module(false);
			rc = s5c73m3_SPI_booting();
			CHECK_ERR(rc);
		} else {
			cam_err("Loading From SensorFW......\n");
			s5c73m3_reset_module(true);
			rc = s5c73m3_get_sensor_fw_binary();
			CHECK_ERR(rc);
		}
	}

	return rc;
}


int s5c73m3_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	CAM_DBG_M("Entered\n");
	if (!s5c73m3_ctrl) {
		cam_err("s5c73m3_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		s5c73m3_ctrl->sensordata = data;

	s5c73m3_ctrl->i2c_write_check = 0;
	s5c73m3_ctrl->fps = 0;
	s5c73m3_ctrl->low_light_mode_size = 0;

	config_csi2 = 0;
	if(data)
	{
		rc = s5c73m3_sensor_init_probe(data);
		if (rc < 0)
			cam_err("s5c73m3_sensor_init failed!\n");
	}

init_done:
	return rc;
}

static int s5c73m3_init_client(struct i2c_client *client)
{
	CAM_DBG_M("Entered\n");
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&s5c73m3_wait_queue);
	return 0;
}

void sensor_native_control(void __user *arg)
{
	struct ioctl_native_cmd ctrl_info;
	int err = 0;

	if (copy_from_user((void *)&ctrl_info,
		(const void *)arg, sizeof(ctrl_info)))
		CAM_DBG_M("fail copy_from_user!\n");


	switch (ctrl_info.mode) {

	case EXT_CAM_EV:
		s5c73m3_set_ev(ctrl_info.value_1);
		break;

	case EXT_CAM_EFFECT:
		s5c73m3_set_effect(ctrl_info.value_1);
		break;

	case EXT_CAM_SCENE_MODE:
		s5c73m3_set_scene_mode(ctrl_info.value_1);
		break;

	case EXT_CAM_ISO:
		s5c73m3_set_iso(ctrl_info.value_1);
		break;

	case EXT_CAM_METERING:
		s5c73m3_set_metering(ctrl_info.value_1);
		break;

	case EXT_CAM_WB:
		s5c73m3_set_whitebalance(ctrl_info.value_1);
		break;

	case EXT_CAM_QUALITY:
		s5c73m3_set_jpeg_quality(ctrl_info.value_1);
		break;

	case EXT_CAM_ZOOM:
		s5c73m3_set_zoom(ctrl_info.value_1);
		break;

	case EXT_CAM_FD_MODE:
		s5c73m3_set_face_detection(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_WDR:
		s5c73m3_set_wdr(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_HDR:
		s5c73m3_set_HDR(ctrl_info.value_1);
		break;

	case EXT_CAM_START_HDR:
		s5c73m3_start_HDR(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_LOW_LIGHT_MODE:
		s5c73m3_set_low_light(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_LOW_LIGHT_SIZE:
		s5c73m3_ctrl->low_light_mode_size = ctrl_info.value_1;
		break;

	case EXT_CAM_SET_ANTI_SHAKE:
		s5c73m3_set_antishake(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_BEAUTY_SHOT:
		s5c73m3_set_face_beauty(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_FPS:
		s5c73m3_set_fps(ctrl_info.value_1);
		break;

	case EXT_CAM_AF:
		CAM_DBG_M("Entered %s mode %d\n",
				  "EXT_CAM_AF", ctrl_info.address);

		/* check whether ISP can be used */
		err = s5c73m3_wait_ISP_status();
		if (err < 0) {
			cam_err("failed s5c73m3_wait_ISP_status\n");
			return;
		}

		if (ctrl_info.address == MSM_V4L2_AF_SET_AUTO_FOCUS) {
			err = s5c73m3_set_focus(ctrl_info.value_1);
		} else if (ctrl_info.address ==
			MSM_V4L2_AF_SET_AUTO_FOCUS_MODE) {
			err = s5c73m3_set_af_mode(ctrl_info.value_1);
		} else if (ctrl_info.address ==
			MSM_V4L2_AF_GET_AUTO_FOCUS) {
			ctrl_info.value_1 = camera_focus.mode;
		} else if (ctrl_info.address ==
			MSM_V4L2_AF_GET_AUTO_FOCUS_RESULT) {
			ctrl_info.value_1 = s5c73m3_get_af_result();
		} else if (ctrl_info.address ==
			MSM_V4L2_AF_SET_AUTO_FOCUS_DEFAULT_POSITION) {
			err = s5c73m3_set_af_mode(ctrl_info.value_1);
		} else if (ctrl_info.address ==
			MSM_V4L2_AF_CANCEL_AUTO_FOCUS) {
			err = s5c73m3_set_focus(ctrl_info.value_1);
		} else if (ctrl_info.address ==
			MSM_V4L2_CAF_FOCUS) {
			err = s5c73m3_set_caf_focus(ctrl_info.value_1);
		} else{ /* MSM_V4L2_AF_COMMAND_MAX */
			cam_err("%s can't support %d\n",
				"EXT_CAM_AF", ctrl_info.address);
		}
		break;

	case EXT_CAM_SET_TOUCHAF_POS:
		CAM_DBG_H("Entered %s\n",
				  "EXT_CAM_SET_TOUCHAF_POS");
		CAM_DBG_H("w %d, h %d\n",
				  ctrl_info.value_1, ctrl_info.value_2);
		camera_focus.pos_x = ctrl_info.value_1;
		camera_focus.pos_y = ctrl_info.value_2;
		break;

	case EXT_CAM_FLASH_MODE:
		s5c73m3_set_flash(ctrl_info.value_1);
		break;

	case EXT_CAM_START_CAPTURE:
		/* check whether ISP can be used */
		err = s5c73m3_wait_ISP_status();
		if (err < 0) {
			cam_err("failed s5c73m3_wait_ISP_status\n");
			return;
		}

		err = s5c73m3_start_capture(ctrl_info.value_1);

		if (s5c73m3_ctrl->scene == CAMERA_SCENE_FIRE)
			err = s5c73m3_capture_firework();
		else if (s5c73m3_ctrl->scene == CAMERA_SCENE_NIGHT)
			err = s5c73m3_capture_nightshot();
		break;

	case EXT_CAM_SET_JPEG_SIZE:
		err = s5c73m3_set_jpeg_size(ctrl_info.value_1,
					    ctrl_info.value_2);
		break;

	case EXT_CAM_SET_RECORD_SIZE:
		err = s5c73m3_set_record_size(ctrl_info.value_1,
					    ctrl_info.value_2);
		break;

	case EXT_CAM_SET_PREVIEW_SIZE:
		err = s5c73m3_set_preview_size(ctrl_info.value_1,
					       ctrl_info.value_2);
		break;

	case EXT_CAM_GET_FLASH_STATUS:
		/* check whether ISP can be used */
		err = s5c73m3_wait_ISP_status();
		if (err < 0) {
			cam_err("failed s5c73m3_wait_ISP_status\n");
			return;
		}

		err = s5c73m3_get_pre_flash(ctrl_info.value_1);
		ctrl_info.value_1 = isPreflashFired;
		break;

	case EXT_CAM_START_AE_AWB_LOCK:
		err = s5c73m3_aeawb_lock_unlock(ctrl_info.value_1,
					       ctrl_info.value_2);
		break;

	case EXT_CAM_GET_AE_AWB_LOCK:
		ctrl_info.value_1 = s5c73m3_ctrl->isAeLock;
		ctrl_info.value_2 = s5c73m3_ctrl->isAwbLock;
		break;

	case EXT_CAM_SET_VDIS:
		s5c73m3_set_vdis(ctrl_info.value_1);
		break;

	case EXT_CAM_GET_LUX:
		/* check whether ISP can be used */
		err = s5c73m3_wait_ISP_status();
		if (err < 0) {
			cam_err("failed s5c73m3_wait_ISP_status\n");
			return;
		}

		ctrl_info.value_1 = s5c73m3_get_lux();
		break;

	case EXT_CAM_SET_FACE_ZOOM:
		/* check whether ISP can be used */
		err = s5c73m3_wait_ISP_status();
		if (err < 0) {
			cam_err("failed s5c73m3_wait_ISP_status\n");
			return;
		}

		err = s5c73m3_set_face_zoom(ctrl_info.value_1);
		break;

	case EXT_CAM_UPDATE_FW:
		/* check whether ISP can be used */
		err = s5c73m3_wait_ISP_status();
		if (err < 0) {
			cam_err("failed s5c73m3_wait_ISP_status\n");
			return;
		}

		if (ctrl_info.value_1 == CAM_FW_MODE_DUMP)
			err = s5c73m3_dump_fw();
		else if (ctrl_info.value_1 == CAM_FW_MODE_UPDATE)
			err = s5c73m3_check_fw(s5c73m3_ctrl->sensordata, 1);
		else
			err = 0;
		break;

	case EXT_CAM_ANTI_BANDING:
		err = s5c73m3_set_antibanding(ctrl_info.value_1);
		break;

	default:
		CAM_DBG_M("default mode\n");
		break;
	}

	if (err < 0)
		cam_err("failed sensor_native_control handle "
			"ctrl_info.mode %d\n", ctrl_info.mode);

	if (copy_to_user((void *)arg,
		(const void *)&ctrl_info, sizeof(ctrl_info)))
		CAM_DBG_M("fail copy_to_user!\n");

}

int s5c73m3_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	CAM_DBG_M("Entered\n");

	if (copy_from_user(&cfg_data,
			(void *)argp,
			sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	cam_err("s5c73m3_ioctl, cfgtype = %d, mode = %d\n",
		cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = s5c73m3_set_sensor_mode(
					cfg_data.mode);
		break;

	case CFG_GET_AF_MAX_STEPS:
	default:
		rc = 0;
		cam_err("Invalid cfgtype\n");
		break;
	}

	return rc;
}

int s5c73m3_sensor_release(void)
{
	CAM_DBG_M("Entered\n");
	s5c73m3_set_af_softlanding();
	return 0;
}

static int s5c73m3_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int ret;
	CAM_DBG_M("Entered\n");

	ret = s5c73m3_spi_init();
	if (ret)
		cam_err("failed to register s5c73mc fw - %x\n", ret);

#if !defined(CONFIG_S5C73M3) && !defined(CONFIG_S5K6A3YX)
	msm_camio_clk_rate_set(24000000);
#endif

	s->s_init = s5c73m3_sensor_init;
	s->s_release = s5c73m3_sensor_release;
	s->s_config  = s5c73m3_sensor_config;
	s->s_camera_type = BACK_CAMERA_2D;
	s->s_mount_angle = 90;

	cam_err("Probe_done!!\n");
	return ret;
}


static struct s5c73m3_format s5c73m3_subdev_info[] = {
	{
#if defined(YUV_PREVIEW)
	.code   = V4L2_MBUS_FMT_YUYV8_2X8,
#else
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
#endif
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static int s5c73m3_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	CAM_DBG_H("Entered, index %d\n", index);
	if ((unsigned int)index >= ARRAY_SIZE(s5c73m3_subdev_info))
		return -EINVAL;

	*code = s5c73m3_subdev_info[index].code;
	return 0;
}

static struct v4l2_subdev_core_ops s5c73m3_subdev_core_ops;
static struct v4l2_subdev_video_ops s5c73m3_subdev_video_ops = {
	.enum_mbus_fmt = s5c73m3_enum_fmt,
};

static struct v4l2_subdev_ops s5c73m3_subdev_ops = {
	.core = &s5c73m3_subdev_core_ops,
	.video  = &s5c73m3_subdev_video_ops,
};

static int s5c73m3_sensor_probe_cb(const struct msm_camera_sensor_info *info,
	struct v4l2_subdev *sdev, struct msm_sensor_ctrl *s)
{
	int rc = 0;
	CAM_DBG_M("Entered\n");

	rc = s5c73m3_sensor_probe(info, s);
	if (rc < 0)
		return rc;

	s5c73m3_ctrl = kzalloc(sizeof(struct s5c73m3_ctrl), GFP_KERNEL);
	if (!s5c73m3_ctrl) {
		cam_err("s5c73m3_sensor_probe failed!\n");
		return -ENOMEM;
	}

	/* probe is successful, init a v4l2 subdevice */
	if (sdev) {
		v4l2_i2c_subdev_init(sdev, s5c73m3_client,
						&s5c73m3_subdev_ops);
		s5c73m3_ctrl->sensor_dev = sdev;
	} else {
		cam_err("sdev is null in probe_cb\n");
	}
	return rc;
}

static int __s5c73m3_probe(struct platform_device *pdev)
{
	CAM_DBG_M("S5C73M3 probe\n");

	return msm_sensor_register(pdev, s5c73m3_sensor_probe_cb);
}

static struct platform_driver msm_camera_driver = {
	.probe = __s5c73m3_probe,
	.driver = {
		.name = PLATFORM_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int s5c73m3_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	CAM_DBG_M("Entered\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	s5c73m3_sensorw =
		kzalloc(sizeof(struct s5c73m3_work), GFP_KERNEL);

	if (!s5c73m3_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, s5c73m3_sensorw);
	s5c73m3_init_client(client);
	s5c73m3_client = client;

	CAM_DBG_M("Exit\n");

	return platform_driver_register(&msm_camera_driver);

probe_failure:
	kfree(s5c73m3_sensorw);
	s5c73m3_sensorw = NULL;
	cam_err("s5c73m3_probe failed!\n");
	return rc;
}

static const struct i2c_device_id s5c73m3_i2c_id[] = {
	{ SENSOR_NAME, 0},
	{ },
};

static struct i2c_driver s5c73m3_i2c_driver = {
	.id_table = s5c73m3_i2c_id,
	.probe  = s5c73m3_i2c_probe,
	.remove = __exit_p(s5c73m3_i2c_remove),
	.driver = {
		.name = SENSOR_NAME,
	},
};

static int __init s5c73m3_init(void)
{
	return i2c_add_driver(&s5c73m3_i2c_driver);
}

module_init(s5c73m3_init);
MODULE_DESCRIPTION("Samsung 8 MP camera driver");
MODULE_LICENSE("GPL v2");
