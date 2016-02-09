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

#include <linux/module.h>
#include "msm_sensor.h"
#define SENSOR_NAME "s5k6a3yx"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k6a3yx"

static DEFINE_MUTEX(s5k6a3yx_mut);
static struct msm_sensor_ctrl_t s5k6a3yx_s_ctrl;

static struct msm_camera_i2c_reg_conf s5k6a3yx_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_mode0_settings[] = {
	{0x0344, 0x00}, /* x_addr_start MSB */
	{0x0345, 0x00}, /* x_addr_start LSB */
	{0x0346, 0x00}, /* y_addr_start MSB */
	{0x0347, 0x00}, /* y_addr_start LSB */
	{0x0348, 0x05}, /* x_addr_end MSB */
	{0x0349, 0x83}, /* x_addr_end LSB */
	{0x034A, 0x05}, /* y_addr_end MSB */
	{0x034B, 0x83}, /* y_addr_end LSB */
	{0x034C, 0x05}, /* x_output_size */
	{0x034D, 0x84}, /* x_output_size */
	{0x034E, 0x05}, /* y_output_size */
	{0x034F, 0x84}, /* y_output_size */
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_mode1_settings[] = {
	{0x0344, 0x00}, /* x_addr_start MSB */
	{0x0345, 0x00}, /* x_addr_start LSB */
	{0x0346, 0x00}, /* y_addr_start MSB */
	{0x0347, 0x00}, /* y_addr_start LSB */
	{0x0348, 0x05}, /* x_addr_end MSB */
	{0x0349, 0x83}, /* x_addr_end LSB */
	{0x034A, 0x05}, /* y_addr_end MSB */
	{0x034B, 0x83}, /* y_addr_end LSB */
	{0x034C, 0x05}, /* x_output_size */
	{0x034D, 0x80}, /* x_output_size */
	{0x034E, 0x05}, /* y_output_size */
	{0x034F, 0x84}, /* y_output_size */
};

static struct msm_camera_i2c_reg_conf s5k6a3yx_recommend_settings[] = {
	{0x0100, 0x00}, /* Streaming off */
	{0x3061, 0x55},
	{0x3062, 0x54},
	{0x5703, 0x07},
	{0x5704, 0x07},
	{0x305E, 0x0D},
	{0x305F, 0x2E},
	{0x3052, 0x01},
	{0x300B, 0x28},
	{0x300C, 0x2E},
	{0x3004, 0x0A},
	{0x5700, 0x08},
	{0x3005, 0x3D},
	{0x3008, 0x1E},
	{0x3025, 0x40},
	{0x3023, 0x20},
	{0x3029, 0xFF},
	{0x302A, 0xFF},
	{0x3505, 0x41},
	{0x3506, 0x00},
	{0x3521, 0x01},
	{0x3522, 0x01},
	{0x3D20, 0x63},
	{0x3095, 0x15},
	{0x3110, 0x01},
	{0x3111, 0x62},
	{0x3112, 0x0E},
	{0x3113, 0xBC},
	{0x311D, 0x30},
	{0x311F, 0x40},
	{0x3009, 0x1E},
	{0x0138, 0x00},
	/* MIPI CLK 720Mbps */
	{0x0305, 0x06}, /* pre_pll_clk_div */
	{0x0306, 0x00}, /* pll_multiplier MSB */
	{0x0307, 0xB4}, /* pll_multiplier LSB */
	{0x0820, 0x02}, /* requested_link_bit_rate_mbps MSB MSB */
	{0x0821, 0xD0}, /* requested_link_bit_rate_mbps MSB LSB */
	{0x0822, 0x00}, /* requested_link_bit_rate_mbps LSB MSB */
	{0x0823, 0x00}, /* requested_link_bit_rate_mbps LSB LSB */
	{0x0101, 0x00}, /* image_orientation */
	{0x0111, 0x02}, /* CSI_signaling_mode */
	{0x0112, 0x0A}, /* CSI_data_format MSB */
	{0x0113, 0x0A}, /* CSI_data_format LSB */
	{0x0136, 0x18}, /* extclk_frequency_mhz MSB */
	{0x0137, 0x00}, /* extclk_frequency_mhz LSB */
	{0x0200, 0x01}, /* fine_integration_time MSB */
	{0x0201, 0xD3}, /* fine_integration_time LSB */
	{0x0202, 0x05}, /* coarse_integration_time MSB */
	{0x0203, 0xA6}, /* coarse_integration_time LSB */
	{0x0204, 0x00}, /* analogue_gain_code_global MSB */
	{0x0205, 0x20}, /* analogue_gain_code_global LSB */
	{0x0340, 0x05}, /* frame_length_lines MSB */
	{0x0341, 0xAA}, /* frame_length_lines LSB */
	{0x0342, 0x06}, /* line_length_pck MSB */
	{0x0343, 0x42}, /* line_length_pck LSB */
	{0x0381, 0x01}, /* x_even_inc */
	{0x0383, 0x01}, /* x_odd_inc */
	{0x0385, 0x01}, /* y_even_inc */
	{0x0387, 0x01}, /* y_odd_inc */
	{0x0408, 0x00}, /* digital_crop_x_offset MSB */
	{0x0409, 0x00}, /* digital_crop_x_offset LSB */
	{0x040A, 0x00}, /* digital_crop_y_offset MSB */
	{0x040B, 0x00}, /* digital_crop_y_offset LSB */
	{0x040C, 0x05}, /* digital_crop_image_width MSB */
	{0x040D, 0x84}, /* digital_crop_image_width LSB */
	{0x040E, 0x05}, /* digital_crop_image_height MSB */
	{0x040F, 0x84}, /* digital_crop_image_height LSB */
	{0x0900, 0x00}, /* binning_mode */
	{0x0105, 0x01}, /* frame mask */
	};

static struct v4l2_subdev_info s5k6a3yx_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array s5k6a3yx_init_conf[] = {
	{&s5k6a3yx_recommend_settings[0],
	ARRAY_SIZE(s5k6a3yx_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array s5k6a3yx_confs[] = {
	{&s5k6a3yx_mode0_settings[0],
	ARRAY_SIZE(s5k6a3yx_mode0_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k6a3yx_mode1_settings[0],
	ARRAY_SIZE(s5k6a3yx_mode1_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t s5k6a3yx_dimensions[] = {
	/* mode 0 */
	{
		.x_output = 0x0584, /* 1412 */
		.y_output = 0x0584, /* 1412 */
		.line_length_pclk = 0x0642, /* 1602 */
		.frame_length_lines = 0x05AA, /* 1450*/
		.vt_pixel_clk =  72000000, /*76800000*/
		.op_pixel_clk = 320000000, /*76800000*/
		.binning_factor = 1,
	},
	/* mode 1 */
	{
		.x_output = 0x0580, /* 1408 */
		.y_output = 0x0584, /* 1412 */
		.line_length_pclk = 0x0642, /* 1602 */
		.frame_length_lines = 0x05AA, /* 1450*/
		.vt_pixel_clk =  72000000, /*76800000*/
		.op_pixel_clk = 320000000, /*76800000*/
		.binning_factor = 1,
	},
};

static struct msm_camera_csid_vc_cfg s5k6a3yx_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
};

static struct msm_camera_csi2_params s5k6a3yx_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 1,
		.lut_params = {
			.num_cid = ARRAY_SIZE(s5k6a3yx_cid_cfg),
			.vc_cfg = s5k6a3yx_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 1,
		.settle_cnt = 0x20, /* MIPI CLK 720Mbps */
	},
};

static struct msm_camera_csi2_params *s5k6a3yx_csi_params_array[] = {
	&s5k6a3yx_csi_params,
	&s5k6a3yx_csi_params,
};

static struct msm_sensor_output_reg_addr_t s5k6a3yx_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t s5k6a3yx_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x0000,
};

static struct msm_sensor_exp_gain_info_t s5k6a3yx_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 8,
};

#if !defined(CONFIG_S5C73M3)
void sensor_native_control(void __user *arg)
{
	printk(KERN_DEBUG "%s Entered\n", __func__);
}
#endif

static int s5k6a3yx_sensor_config(void __user *argp)
{
	return msm_sensor_config(&s5k6a3yx_s_ctrl, argp);
}

static int s5k6a3yx_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	return msm_sensor_open_init(&s5k6a3yx_s_ctrl, data);
}

static int s5k6a3yx_sensor_release(void)
{
	return msm_sensor_release(&s5k6a3yx_s_ctrl);
}

static const struct i2c_device_id s5k6a3yx_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k6a3yx_s_ctrl},
	{ }
};

static struct i2c_driver s5k6a3yx_i2c_driver = {
	.id_table = s5k6a3yx_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client s5k6a3yx_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int s5k6a3yx_sensor_v4l2_probe(const struct msm_camera_sensor_info *info,
	struct v4l2_subdev *sdev, struct msm_sensor_ctrl *s)
{
	printk(KERN_DEBUG "#######s5k6a3yx_sensor_v4l2_probe #########\n");
	return msm_sensor_v4l2_probe(&s5k6a3yx_s_ctrl, info, sdev, s);
}

static int s5k6a3yx_probe(struct platform_device *pdev)
{
	printk(KERN_DEBUG "############# s5k6a3yx_probe ##############\n");
	return msm_sensor_register(pdev, s5k6a3yx_sensor_v4l2_probe);
}

struct platform_driver s5k6a3yx_driver = {
	.probe = s5k6a3yx_probe,
	.driver = {
		.name = PLATFORM_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_sensor_init_module(void)
{
	return platform_driver_register(&s5k6a3yx_driver);
}

static struct v4l2_subdev_core_ops s5k6a3yx_subdev_core_ops;
static struct v4l2_subdev_video_ops s5k6a3yx_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k6a3yx_subdev_ops = {
	.core = &s5k6a3yx_subdev_core_ops,
	.video = &s5k6a3yx_subdev_video_ops,
};

static void s5k6a3yx_write_exp_params(
	struct msm_sensor_ctrl_t *s_ctrl,
	uint32_t gain,
	uint32_t fl_lines,
	uint32_t line)
{

uint8_t msb_fl_lines, lsb_fl_lines;
uint8_t msb_line, lsb_line;
uint8_t msb_gain, lsb_gain;

	CDBG("%s gain %d fl %d line %d\n",
		__func__,
		gain,
		fl_lines,
		line);
	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);


	msb_fl_lines = (uint8_t)((fl_lines >> 8) & 0xFF);
	lsb_fl_lines = (uint8_t)(fl_lines & 0xFF);

	msb_line = (uint8_t)((line >> 8) & 0xFF);
	lsb_line = (uint8_t)(line & 0xFF);

	msb_gain = (uint8_t)((gain >> 8) & 0xFF);
	lsb_gain = (uint8_t)(gain & 0xFF);

	CDBG("%s : %d %d %d %d %d %d\n",
		   __func__, msb_fl_lines,  lsb_fl_lines, msb_line
		   , lsb_line, msb_gain, lsb_gain);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines,
		msb_fl_lines, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines+1,
		lsb_fl_lines, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, msb_line,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr+1, lsb_line,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, msb_gain,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr+1, lsb_gain,
		MSM_CAMERA_I2C_BYTE_DATA);

	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
}
#ifndef CONFIG_S5C73M3
static void s5k6a3yx_write_fl_lines(
	struct msm_sensor_ctrl_t *s_ctrl,
	uint32_t gain,
	uint32_t fl_lines,
	uint32_t line)
{
	uint8_t msb_fl_lines, lsb_fl_lines;
	uint8_t msb_line, lsb_line;

	CDBG("%s gain %d fl %d line %d",
		   __func__,
		   gain,
		   fl_lines,
		   line);
	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);


	msb_fl_lines = (uint8_t)((fl_lines >> 8) &  0xFF);
	lsb_fl_lines = (uint8_t)(fl_lines & 0xFF);

	msb_line = (uint8_t)((line >> 8) &  0xFF);
	lsb_line = (uint8_t)(line & 0xFF);

		CDBG("%s : %d %d %d %d",
		   __func__,  msb_fl_lines,  lsb_fl_lines,  msb_line, lsb_line);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines,
		msb_fl_lines, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
		lsb_fl_lines, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, msb_line,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr+1, lsb_line,
		MSM_CAMERA_I2C_BYTE_DATA);


	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
}

static void s5k6a3yx_write_gain(
	struct msm_sensor_ctrl_t *s_ctrl,
	uint32_t gain,
	uint32_t fl_lines,
	uint32_t line)
{
	uint8_t msb_gain, lsb_gain;

	CDBG("%s gain %d fl %d line %d\n",
		   __func__,
		   gain,
		   fl_lines,
		   line);
	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);

	msb_gain = (uint8_t)((gain >> 8) &  0xFF);
	lsb_gain = (uint8_t)(gain & 0xFF);

		CDBG("%s : %d %d",
		   __func__,  msb_gain, lsb_gain);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, msb_gain,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr+1, lsb_gain,
		MSM_CAMERA_I2C_BYTE_DATA);

	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
}
#endif
static int32_t s5k6a3yx_write_exp_gain(
	struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t gain,
	uint32_t line)
{
	int rc = 0;
	uint32_t fl_lines = 0;
	uint8_t offset;
	static uint32_t old_gain;
	static uint32_t old_line;
	static uint32_t old_fl_lines;

	CDBG("%s E gain %d line %d\n", __func__, gain, line);
	CDBG("%s E old_gain %d old_fl %d old_line %d\n",
		   __func__,
		   old_gain,
		   old_fl_lines,
		   old_line);

		if ((gain == old_gain) &&  (line == old_line)) {
			CDBG("%s XXX\n", __func__);
			return rc;
		}

		fl_lines = s_ctrl->curr_frame_length_lines;
		fl_lines = (fl_lines * s_ctrl->fps_divider) / Q10;
		offset = s_ctrl->sensor_exp_gain_info->vert_offset;

		if (line > (fl_lines - offset))
			fl_lines = line + offset;

			s5k6a3yx_write_exp_params(s_ctrl, gain, fl_lines, line);

		old_gain = gain;
		old_line = line;
		old_fl_lines = fl_lines;

	CDBG("%s X old_gain %d old_line %d\n", __func__, old_gain, old_line);

	return rc;
}


static int32_t s5k6a3yx_write_snapshot_exp_gain(
	struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t gain,
	uint32_t line)
{
	int rc = 0;
	uint32_t fl_lines = 0;
	uint8_t offset;

	CDBG("%s E gain %d line %d\n", __func__, gain, line);

	fl_lines = s_ctrl->curr_frame_length_lines;
	fl_lines = (fl_lines * s_ctrl->fps_divider) / Q10;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;

	if (line > (fl_lines - offset))
		fl_lines = line + offset;

	s5k6a3yx_write_exp_params(s_ctrl, gain, fl_lines, line);

	return rc;
}

static struct msm_sensor_fn_t s5k6a3yx_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,

	.sensor_write_exp_gain = s5k6a3yx_write_exp_gain,
	.sensor_write_snapshot_exp_gain = s5k6a3yx_write_snapshot_exp_gain,

	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = s5k6a3yx_sensor_config,
	.sensor_open_init = s5k6a3yx_sensor_open_init,
	.sensor_release = s5k6a3yx_sensor_release,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_probe = msm_sensor_probe,
};

static struct msm_sensor_reg_t s5k6a3yx_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = s5k6a3yx_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(s5k6a3yx_start_settings),
	.stop_stream_conf = s5k6a3yx_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(s5k6a3yx_stop_settings),
	.group_hold_on_conf = s5k6a3yx_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(s5k6a3yx_groupon_settings),
	.group_hold_off_conf = s5k6a3yx_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(s5k6a3yx_groupoff_settings),
	.init_settings = &s5k6a3yx_init_conf[0],
	.init_size = ARRAY_SIZE(s5k6a3yx_init_conf),
	.mode_settings = &s5k6a3yx_confs[0],
	.output_settings = &s5k6a3yx_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k6a3yx_confs),
};

static struct msm_sensor_ctrl_t s5k6a3yx_s_ctrl = {
	.msm_sensor_reg = &s5k6a3yx_regs,
	.sensor_i2c_client = &s5k6a3yx_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.sensor_output_reg_addr = &s5k6a3yx_reg_addr,
	.sensor_id_info = &s5k6a3yx_id_info,
	.sensor_exp_gain_info = &s5k6a3yx_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &s5k6a3yx_csi_params_array[0],
	.msm_sensor_mutex = &s5k6a3yx_mut,
	.sensor_i2c_driver = &s5k6a3yx_i2c_driver,
	.sensor_v4l2_subdev_info = s5k6a3yx_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k6a3yx_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k6a3yx_subdev_ops,
	.func_tbl = &s5k6a3yx_func_tbl,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Samsung 2 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
