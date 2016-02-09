/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#include <asm/mach-types.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <mach/msm_bus_board.h>
#include <mach/gpiomux.h>
#include "devices.h"
#include "board-8960.h"
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/ctype.h>
#include <linux/regulator/consumer.h>
#include <mach/board-msm8960-camera.h>
#include <mach/gpio.h>
#include <mach/msm8960-gpio.h>
#include <mach/msm_iomap.h>
#include <mach/camera.h>
#include <linux/spi/spi.h>
#ifdef CONFIG_LEDS_AAT1290A
#include <linux/leds-aat1290a.h>
#endif

extern unsigned int system_rev;

static struct gpiomux_setting cam_settings[] = {
	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 1*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 2*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 3*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_5, /*active 4*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_6, /*active 5*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_2, /*active 6*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_3, /*active 7*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*i2c suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},
	{
		.func = GPIOMUX_FUNC_1, /* drive strength for D2*/
#if defined(CONFIG_MACH_M2) || defined(CONFIG_MACH_M2_DCM)
		.drv = GPIOMUX_DRV_2MA,
#else
		.drv = GPIOMUX_DRV_4MA,
#endif
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_4, /* sub camera of D2 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_4, /*active 11 : sub camera of ApexQ*/
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_NONE,
	},
};

static struct msm_gpiomux_config msm8960_cam_common_configs[] = {
#ifdef CONFIG_S5C73M3
	{
		.gpio = GPIO_MSM_FLASH_NOW,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = GPIO_CAM_MCLK0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[9],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#if !defined(CONFIG_MACH_M2_DCM) && !defined(CONFIG_MACH_K2_KDI)
	{
		.gpio = CAM2_RST_N,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#endif
	{
		.gpio = ISP_RESET,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#else
#if !defined(CONFIG_MACH_STRETTO)
	{
		.gpio = GPIO_MSM_FLASH_CNTL_EN,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#endif
	{
		.gpio = GPIO_MSM_FLASH_NOW,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#if !(defined(CONFIG_MACH_AEGIS2) || defined(CONFIG_MACH_JASPER))
	{
		.gpio = GPIO_MAIN_CAM_STBY,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#endif
	{
		.gpio = GPIO_CAM_MCLK,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#if !defined(CONFIG_MACH_STRETTO)
	{
		.gpio = GPIO_VT_STBY,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#endif
	{
		.gpio = GPIO_CAM2_RST_N,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = GPIO_CAM1_RST_N,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#endif
};

#ifdef CONFIG_MSM_CAMERA

#if defined(CONFIG_S5C73M3) && defined(CONFIG_S5K6A3YX)
static struct msm_gpiomux_config msm8960_cam_2d_configs[] = {
	{
		.gpio = 2,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 20,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 21,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
};

static struct msm_gpiomux_config msm8960_cam_2d_configs_v2[] = {
	{
		.gpio = 2,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[10],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 20,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 21,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
};

static uint16_t msm_cam_gpio_2d_tbl[] = {
	5, /*CAMIF_MCLK*/
	20, /*CAMIF_I2C_DATA*/
	21, /*CAMIF_I2C_CLK*/
};

static uint16_t msm_cam_gpio_2d_tbl_v2[] = {
	2, /*CAMIF_MCLK*/
	20, /*CAMIF_I2C_DATA*/
	21, /*CAMIF_I2C_CLK*/
};

static struct msm_camera_gpio_conf rear_gpio_conf = {
	.cam_gpiomux_conf_tbl = msm8960_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(msm8960_cam_2d_configs),
	.cam_gpio_tbl = msm_cam_gpio_2d_tbl,
	.cam_gpio_tbl_size = ARRAY_SIZE(msm_cam_gpio_2d_tbl),
};

static struct msm_camera_gpio_conf front_gpio_conf = {
	.cam_gpiomux_conf_tbl = msm8960_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(msm8960_cam_2d_configs),
	.cam_gpio_tbl = msm_cam_gpio_2d_tbl,
	.cam_gpio_tbl_size = ARRAY_SIZE(msm_cam_gpio_2d_tbl),
};

static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 27648000,
		.ib  = 2656000000UL,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
};


static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 600000000,
		.ib  = 2656000000UL,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 206807040,
		.ib  = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 800000000,
		.ib  = 4264000000UL,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 600000000,
		.ib  = 2656000000UL,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 1350000000,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
};

static struct msm_bus_vectors cam_video_ls_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 348192000,
		.ib  = 617103360,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 206807040,
		.ib  = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_dual_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 600000000,
		.ib  = 2656000000UL,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 206807040,
		.ib  = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
};

static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_preview_vectors),
		cam_preview_vectors,
	},
	{
		ARRAY_SIZE(cam_video_vectors),
		cam_video_vectors,
	},
	{
		ARRAY_SIZE(cam_snapshot_vectors),
		cam_snapshot_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
	{
		ARRAY_SIZE(cam_video_ls_vectors),
		cam_video_ls_vectors,
	},
	{
		ARRAY_SIZE(cam_dual_vectors),
		cam_dual_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};

#if defined(CONFIG_S5C73M3) && defined(CONFIG_S5K6A3YX)
static struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 1,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
};
#endif

static struct regulator *l29, *l28;
#if !defined(CONFIG_MACH_M2_DCM) && !defined(CONFIG_MACH_K2_KDI)
static struct regulator *isp_core;
#endif
/* CAM power
	CAM_SENSOR_A_2.8		:  GPIO_CAM_A_EN(GPIO 46)
	CAM_SENSOR_IO_1.8		: VREG_L29		: l29
	CAM_AF_2.8				: VREG_L11		: l11
	CAM_SENSOR_CORE1.2		: VREG_L12		: l12
	CAM_ISP_CORE_1.2		: CAM_CORE_EN(GPIO 6)

	CAM_DVDD_1.5		: VREG_L18		: l18
*/

static int vddCore = 1150000;
static bool isVddCoreSet;
static u8 gpio_cam_flash_sw;
static u8 pmic_gpio_msm_flash_cntl_en;
static bool isFlashCntlEn;

static void cam_set_isp_core(int level)
{
	if (level == 1050000) {
		pr_err("Change core voltage\n");
		vddCore = 1100000;
	} else
		vddCore = level;

	isVddCoreSet = true;
	pr_err("ISP CORE = %d\n", vddCore);
}

static bool cam_is_vdd_core_set(void)
{
	return isVddCoreSet;
}

void power_on_flash(void)
{
	int temp = 0;
	int ret = 0;

	if(!isFlashCntlEn) {
		temp = gpio_get_value(GPIO_MSM_FLASH_NOW);
		if (isFlashCntlEn == 0 && temp == 0) {
			/* FLASH_LED_UNLOCK*/
			gpio_set_value_cansleep(PM8921_MPP_PM_TO_SYS
				(PMIC_MPP_FLASH_LED_UNLOCK), 1);
			/* GPIO_CAM_FLASH_SW : tourch */
			gpio_set_value_cansleep(gpio_cam_flash_sw, 1);
			temp = gpio_get_value(gpio_cam_flash_sw);
			printk("[s5c73m3] check GPIO_CAM_FLASH_SW : %d\n", temp);
			usleep(1*1000);
		}

		/* flash power 1.8V */
		l28 = regulator_get(NULL, "8921_lvs4");
		ret = regulator_enable(l28);

		if (ret)
			printk("error enabling regulator\n");

		usleep(1*1000);
		isFlashCntlEn = true;
	}
}

static void cam_ldo_power_on(int mode, int num)
{
	int ret = 0;
	int temp = 0;

	printk(KERN_DEBUG "[%s : %d] %s CAMERA POWER ON!!\n",
	       __func__, __LINE__, mode ? "FRONT" : "REAR");

	if (mode) {		/* front camera */
		if (num == 0) {
			/* ISP CORE 1.2V */
			/* delete for unnecessary power */
			/* 8M AVDD 2.8V */
			gpio_set_value_cansleep(GPIO_CAM_A_EN, 1);
			temp = gpio_get_value(GPIO_CAM_A_EN);
			printk(KERN_DEBUG "[s5k6a3yx] check GPIO_CAM_A_EN : %d\n",
				temp);
			usleep(1*1000);
		}	else	{
			/* VT_RESET */
		#if defined(CONFIG_MACH_M2_DCM) || defined(CONFIG_MACH_K2_KDI)
			gpio_set_value_cansleep(gpio_rev(CAM2_RST_N), 1);
			temp = gpio_get_value(gpio_rev(CAM2_RST_N));
		#else
			gpio_set_value_cansleep(CAM2_RST_N, 1);
			temp = gpio_get_value(CAM2_RST_N);
		#endif
			printk(KERN_DEBUG "[s5k6a3yx] check CAM2_RST_N : %d\n",
				temp);
			usleep(1*1000);

			/* ISP 8M HOST 1.8V */
			l29 = regulator_get(NULL, "8921_lvs5");
			ret = regulator_enable(l29);
			if (ret)
				cam_err("error enabling regulator\n");
			usleep(1*1000);

			/* ISP 8M MIPI 1.2V */
			/* delete for unnecessary power */
		}
	} else {		/* rear camera */
		if (num == 0) {
			printk(KERN_DEBUG "[s5c73m3] rear camera on 1\n");

			temp = gpio_get_value(GPIO_MSM_FLASH_NOW);

			if (isFlashCntlEn == 0 && temp == 0) {
				/* FLASH_LED_UNLOCK*/
				gpio_set_value_cansleep(PM8921_MPP_PM_TO_SYS
					(PMIC_MPP_FLASH_LED_UNLOCK), 1);
				/* GPIO_CAM_FLASH_SW : tourch */
				gpio_set_value_cansleep(gpio_cam_flash_sw, 1);
				temp = gpio_get_value(gpio_cam_flash_sw);
				printk(KERN_DEBUG "[s5c73m3] check"
					" GPIO_CAM_FLASH_SW : %d\n", temp);
				usleep(1*1000);
			}

			/* flash power 1.8V */
			l28 = regulator_get(NULL, "8921_lvs4");
			ret = regulator_enable(l28);
			if (ret)
				cam_err("error enabling regulator\n");
			usleep(1*1000);

			/* ISP CORE 1.2V */
#if defined(CONFIG_MACH_M2)
			if (system_rev >= BOARD_REV08) {
				printk(KERN_DEBUG "[s5c73m3] m2 check vddCore : %d\n",
					vddCore);

				isp_core = regulator_get(NULL, "cam_isp_core");
				ret = regulator_set_voltage(isp_core,
					vddCore, vddCore);
				if (ret)
					cam_err("error setting voltage\n");

				ret = regulator_enable(isp_core);
				if (ret)
					cam_err("error enabling regulator.");
			} else
				gpio_set_value_cansleep(CAM_CORE_EN, 1);
#elif defined(CONFIG_MACH_M2_DCM) || defined(CONFIG_MACH_K2_KDI)
			gpio_set_value_cansleep(gpio_rev(CAM_CORE_EN), 1);
#else
			gpio_set_value_cansleep(CAM_CORE_EN, 1);
#endif
			usleep(1200);

			/* 8M AVDD 2.8V */
			gpio_set_value_cansleep(GPIO_CAM_A_EN, 1);
			temp = gpio_get_value(GPIO_CAM_A_EN);
			printk(KERN_DEBUG "[s5c73m3] check GPIO_CAM_A_EN : %d\n",
				temp);
			usleep(1*1000);

			/* 8M DVDD 1.2V */
			gpio_set_value_cansleep(GPIO_CAM_SENSOR_EN, 1);
			temp = gpio_get_value(GPIO_CAM_SENSOR_EN);
			printk(KERN_DEBUG "[s5c73m3] check GPIO_CAM_SENSOR_EN : %d\n",
			       temp);
			usleep(1*1000);
		} else {
			printk(KERN_DEBUG "[s5c73m3] rear camera on 2\n");
			/* AF 2.8V */
			gpio_set_value_cansleep(gpio_rev(CAM_AF_EN), 1);
			temp = gpio_get_value(gpio_rev(CAM_AF_EN));
			printk(KERN_DEBUG "[s5c73m3] check CAM_AF_EN : %d\n",
			       temp);
			usleep(3*1000);

			/* ISP 8M HOST 1.8V */
			l29 = regulator_get(NULL, "8921_lvs5");
			ret = regulator_enable(l29);
			if (ret)
				cam_err("error enabling regulator\n");

			/* ISP 8M MIPI 1.2V */
			gpio_set_value_cansleep(CAM_MIPI_EN, 1);
			temp = gpio_get_value(CAM_MIPI_EN);
			printk(KERN_DEBUG "[s5c73m3] check CAM_MIPI_EN : %d\n",
				temp);
			usleep(1*1000);

			/* ISP_STANDBY */
			gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(24), 1);
			usleep(1*1000);

			/* ISP_RESET */
			gpio_set_value_cansleep(ISP_RESET, 1);
			temp = gpio_get_value(ISP_RESET);
			printk(KERN_DEBUG "[s5c73m3] check ISP_RESET : %d\n",
				temp);
			usleep(1*1000);
		}
	}

}

static void cam_ldo_power_off(int mode)
{
	int ret = 0;
	int temp = 0;

	printk(KERN_DEBUG "[%s : %d] %s CAMERA POWER OFF!!\n",
	       __func__, __LINE__, mode ? "FRONT" : "REAR");

	if (mode) {		/* front camera */
		/* VT_RESET */
#if defined(CONFIG_MACH_M2_DCM) || defined(CONFIG_MACH_K2_KDI)
		gpio_set_value_cansleep(gpio_rev(CAM2_RST_N), 0);
		usleep(1*1000);
#else
		gpio_set_value_cansleep(CAM2_RST_N, 0);
		usleep(1*1000);
#endif
		/* ISP 8M MIPI 1.2V */
	  /* delete for unnecessary power */


		/* ISP 8M HOST 1.8V */
		if (l29) {
			ret = regulator_disable(l29);
			if (ret)
				cam_err("error disabling regulator\n");
		}
		usleep(1*1000);

		/* MCLK 24MHz*/

		/* 8M AVDD 2.8V */
		gpio_set_value_cansleep(GPIO_CAM_A_EN, 0);
		usleep(1*1000);
		/* ISP CORE 1.2V */
	  /* delete for unnecessary power */
	} else {		/* rear camera */
		/* ISP_STANDBY */
		gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(24), 0);
		usleep(1*1000);

		/* ISP_RESET */
		gpio_set_value_cansleep(ISP_RESET, 0);
		usleep(1*1000);

		/* AF 2.8V */
		gpio_set_value_cansleep(gpio_rev(CAM_AF_EN), 0);
		usleep(1*1000);

		/* ISP 8M MIPI 1.2V */
		gpio_set_value_cansleep(CAM_MIPI_EN, 0);
		usleep(1*1000);

		/* ISP 8M HOST 1.8V */
		if (l29) {
			ret = regulator_disable(l29);
			if (ret)
				cam_err("error disabling regulator\n");
		}
		usleep(1*1000);

		/* 8M DVDD 1.2V */
		gpio_set_value_cansleep(GPIO_CAM_SENSOR_EN, 0);
		usleep(1*1000);

		/* 8M AVDD 2.8V */
		gpio_set_value_cansleep(GPIO_CAM_A_EN, 0);
		usleep(1*1000);

		/* ISP CORE 1.2V */
#if defined(CONFIG_MACH_M2)
		if (system_rev >= BOARD_REV08)
			ret = regulator_disable(isp_core);
		if (ret)
			cam_err("error disabling regulator");
		regulator_put(isp_core);
#endif
#elif defined(CONFIG_MACH_M2_DCM) || defined(CONFIG_MACH_K2_KDI)
		gpio_set_value_cansleep(gpio_rev(CAM_CORE_EN), 0);
#else
		gpio_set_value_cansleep(CAM_CORE_EN, 0);
#endif
		usleep(1*1000);

		/* flash power 1.8V */
		if (l28) {
			ret = regulator_disable(l28);
			if (ret)
				cam_err("error disabling regulator\n");
		}
		usleep(1*1000);

		/* GPIO_CAM_FLASH_SW : tourch */
		temp = gpio_get_value(GPIO_MSM_FLASH_NOW);

		if (isFlashCntlEn == 0 && temp == 0) {
			/* GPIO_CAM_FLASH_SW : tourch */
			gpio_set_value_cansleep(gpio_cam_flash_sw, 0);
			temp = gpio_get_value(gpio_cam_flash_sw);
			printk(KERN_DEBUG "[s5c73m3] check"
				" GPIO_CAM_FLASH_SW : %d\n", temp);
			/* FLASH_LED_LOCK*/
			gpio_set_value_cansleep(PM8921_MPP_PM_TO_SYS
				(PMIC_MPP_FLASH_LED_UNLOCK), 0);
			usleep(1*1000);
		}
	}
}

static void cam_isp_reset(void)
{
	int temp = 0;

	/* ISP_RESET */
	gpio_set_value_cansleep(ISP_RESET, 0);
	temp = gpio_get_value(ISP_RESET);
	printk(KERN_DEBUG "[s5c73m3] check ISP_RESET : %d\n", temp);
	usleep(1*1000);
	/* ISP_RESET */
	gpio_set_value_cansleep(ISP_RESET, 1);
	temp = gpio_get_value(ISP_RESET);
	printk(KERN_DEBUG "[s5c73m3] check ISP_RESET : %d\n", temp);
	usleep(1*1000);
}

static u8 *rear_sensor_fw;
static u8 *rear_phone_fw;
static void cam_get_fw(u8 *isp_fw, u8 *phone_fw)
{
	rear_sensor_fw = isp_fw;
	rear_phone_fw = phone_fw;
	pr_debug("sensor_fw = %s\n", rear_sensor_fw);
	pr_debug("phone_fw = %s\n", rear_phone_fw);
}

#ifdef CONFIG_S5C73M3
static struct msm_camera_sensor_flash_data flash_s5c73m3 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
};

static struct msm_camera_sensor_platform_info sensor_board_info_s5c73m3 = {
	.mount_angle	= 90,
	.sensor_reset	= ISP_RESET,
	.flash_en	= GPIO_MSM_FLASH_NOW,
	.flash_set	= GPIO_MSM_FLASH_CNTL_EN,
	.mclk	= GPIO_CAM_MCLK0,
	.sensor_pwd	= CAM_CORE_EN,
	.vcm_pwd	= 0,
	.vcm_enable	= 1,
	.sensor_power_on = cam_ldo_power_on,
	.sensor_power_off = cam_ldo_power_off,
	.sensor_isp_reset = cam_isp_reset,
	.sensor_get_fw = cam_get_fw,
	.sensor_set_isp_core = cam_set_isp_core,
	.sensor_is_vdd_core_set = cam_is_vdd_core_set,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5c73m3_data = {
	.sensor_name	= "s5c73m3",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_s5c73m3,
	.sensor_platform_info = &sensor_board_info_s5c73m3,
	.gpio_conf = &rear_gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
};

struct platform_device msm8960_camera_sensor_s5c73m3 = {
	.name	= "msm_camera_s5c73m3",
	.dev	= {
		.platform_data = &msm_camera_sensor_s5c73m3_data,
	},
};
#endif

#ifdef CONFIG_S5K6A3YX
static struct msm_camera_sensor_flash_data flash_s5k6a3yx = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_platform_info sensor_board_info_s5k6a3yx = {
	.mount_angle	= 270,
	.mclk	= GPIO_CAM_MCLK0,
	.sensor_pwd	= CAM_CORE_EN,
	.vcm_pwd	= 0,
	.vcm_enable	= 1,
	.sensor_power_on = cam_ldo_power_on,
	.sensor_power_off = cam_ldo_power_off,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k6a3yx_data = {
	.sensor_name	= "s5k6a3yx",
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_s5k6a3yx,
	.sensor_platform_info = &sensor_board_info_s5k6a3yx,
	.gpio_conf = &front_gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
};

struct platform_device msm8960_camera_sensor_s5k6a3yx = {
	.name	= "msm_camera_s5k6a3yx",
	.dev	= {
		.platform_data = &msm_camera_sensor_s5k6a3yx_data,
	},
};
#endif

#ifdef CONFIG_LEDS_AAT1290A
static int aat1290a_setGpio(void)
{
	int ret;
	int temp = 0;

	printk(KERN_DEBUG "[%s : %d]!!\n", __func__, __LINE__);

#if defined(CONFIG_S5C73M3) && defined(CONFIG_S5K6A3YX) /* D2 */
	/* FLASH_LED_UNLOCK*/
	gpio_set_value_cansleep(PM8921_MPP_PM_TO_SYS
		(PMIC_MPP_FLASH_LED_UNLOCK), 1);
#endif

	/* GPIO_CAM_FLASH_SW : tourch */
	gpio_set_value_cansleep(gpio_cam_flash_sw, 0);
	temp = gpio_get_value(gpio_cam_flash_sw);
	printk(KERN_DEBUG "[s5c73m3] check GPIO_CAM_FLASH_SW : %d\n", temp);
	usleep(1*1000);

	/* flash power 1.8V */
	l28 = regulator_get(NULL, "8921_lvs4");
	ret = regulator_enable(l28);
	if (ret)
		cam_err("error enabling regulator\n");
	usleep(1*1000);

	if (pmic_gpio_msm_flash_cntl_en) {
		gpio_set_value_cansleep(pmic_gpio_msm_flash_cntl_en, 1);
	} else {
		gpio_set_value_cansleep(GPIO_MSM_FLASH_CNTL_EN, 1);
		temp = gpio_get_value(GPIO_MSM_FLASH_CNTL_EN);
		printk(KERN_DEBUG "[s5c73m3] check Flash set GPIO : %d\n",
			temp);
	}
	isFlashCntlEn = true;
	usleep(1*1000);

	gpio_set_value_cansleep(GPIO_MSM_FLASH_NOW, 1);
	temp = gpio_get_value(GPIO_MSM_FLASH_NOW);
	printk(KERN_DEBUG "[s5c73m3] check Flash enable GPIO : %d\n", temp);
	usleep(1*1000);

	return 0;
}

static int aat1290a_freeGpio(void)
{
	int ret;

	printk(KERN_DEBUG "[%s : %d]!!\n", __func__, __LINE__);

	if (pmic_gpio_msm_flash_cntl_en)
		gpio_set_value_cansleep(pmic_gpio_msm_flash_cntl_en, 0);
	else
		gpio_set_value_cansleep(GPIO_MSM_FLASH_CNTL_EN, 0);
	isFlashCntlEn = false;
	usleep(1*1000);
	gpio_set_value_cansleep(GPIO_MSM_FLASH_NOW, 0);
	usleep(1*1000);

	/* flash power 1.8V */
	if (l28) {
		ret = regulator_disable(l28);
		if (ret)
			cam_err("error disabling regulator\n");
	}
	usleep(1*1000);

	/* GPIO_CAM_FLASH_SW : tourch */
	gpio_set_value_cansleep(gpio_cam_flash_sw, 0);
	usleep(1*1000);

#if defined(CONFIG_S5C73M3) && defined(CONFIG_S5K6A3YX) /* D2 */
	/* FLASH_LED_LOCK*/
	gpio_set_value_cansleep(PM8921_MPP_PM_TO_SYS
		(PMIC_MPP_FLASH_LED_UNLOCK), 0);
#endif
	return 0;
}

static void aat1290a_torch_en(int onoff)
{
	int temp = 0;

	printk(KERN_DEBUG "[%s : %d] %s!!\n",
	       __func__, __LINE__, onoff ? "enabled" : "disabled");

	gpio_set_value_cansleep(GPIO_MSM_FLASH_NOW, onoff);
	temp = gpio_get_value(GPIO_MSM_FLASH_NOW);
	printk(KERN_DEBUG "[s5c73m3] check Flash enable GPIO : %d\n", temp);
	usleep(1*1000);
}

static void aat1290a_torch_set(int onoff)
{
	int temp = 0;

	printk(KERN_DEBUG "[%s : %d] %s!!\n",
	       __func__, __LINE__, onoff ? "enabled" : "disabled");

	if (pmic_gpio_msm_flash_cntl_en) {
		gpio_set_value_cansleep(pmic_gpio_msm_flash_cntl_en, onoff);
	} else {
		gpio_set_value_cansleep(GPIO_MSM_FLASH_CNTL_EN, onoff);
		temp = gpio_get_value(GPIO_MSM_FLASH_CNTL_EN);
		printk(KERN_DEBUG "[s5c73m3] check Flash set GPIO : %d\n",
			temp);
	}
	usleep(1*1000);
}

static struct aat1290a_led_platform_data aat1290a_led_data = {
	.brightness = TORCH_BRIGHTNESS_50,
	.status	= STATUS_UNAVAILABLE,
	.setGpio = aat1290a_setGpio,
	.freeGpio = aat1290a_freeGpio,
	.torch_en = aat1290a_torch_en,
	.torch_set = aat1290a_torch_set,
};

static struct platform_device s3c_device_aat1290a_led = {
	.name	= "aat1290a-led",
	.id	= -1,
	.dev	= {
		.platform_data	= &aat1290a_led_data,
	},
};
#endif

static struct platform_device *cam_dev[] __initdata = {
#ifdef CONFIG_S5C73M3
		&msm8960_camera_sensor_s5c73m3,
#endif
#ifdef CONFIG_S5K6A3YX
		&msm8960_camera_sensor_s5k6a3yx,
#endif
};

static ssize_t back_camera_type_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_ISX012)
	char cam_type[] = "SONY_ISX012\n";
#elif defined(CONFIG_S5C73M3)
	char cam_type[] = "S5C73M3\n";
#elif defined(CONFIG_S5K5CCGX)
	char cam_type[] = "SLSI_S5K5CCGX\n";
#else
	char cam_type[] = "Rear default camera\n";
#endif

	return snprintf(buf, sizeof(cam_type), "%s", cam_type);
}

static ssize_t front_camera_type_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_S5K8AAY)
	char cam_type[] = "SLSI_S5K8AA\n";
#elif defined(CONFIG_S5K6A3YX)
	char cam_type[] = "SLSI_S5K6A3YX\n";
#elif defined(CONFIG_DB8131M)
	char cam_type[] = "DUB_DB8131M\n";
#elif defined(CONFIG_SR030PC50)
	char cam_type[] = "SILICON_SR030PC50\n";
#else
	char cam_type[] = "Front default camera\n";
#endif

	return snprintf(buf, sizeof(cam_type), "%s", cam_type);
}

static DEVICE_ATTR(rear_camtype, S_IRUGO, back_camera_type_show, NULL);
static DEVICE_ATTR(front_camtype, S_IRUGO, front_camera_type_show, NULL);

static ssize_t back_camera_firmware_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_ISX012)
	char cam_fw[] = "ISX012\n";
#elif defined(CONFIG_S5K5CCGX)
	char cam_fw[] = "S5K5CCGX\n";
#elif defined(CONFIG_S5C73M3)
	return sprintf(buf, "%s %s", rear_sensor_fw, rear_phone_fw);
#else
	char cam_fw[] = "Rear default camera\n";
	return snprintf(buf, sizeof(cam_fw), "%s", cam_fw);
#endif
}

static ssize_t front_camera_firmware_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_S5K8AAY)
	char cam_fw[] = "S5K8AAY\n";
#elif defined(CONFIG_S5K6A3YX)
	char *cam_fw[] = {"S5K6A3", "S5K6A3"}; /*char cam_fw[] = "S5K6A3YX\n";*/
#elif defined(CONFIG_DB8131M)
	char cam_fw[] = "DB8131M\n";
#elif defined(CONFIG_SR030PC50)
	char cam_fw[] = "SR030PC50\n";
#else
	char cam_fw[] = "Front default camera\n";
#endif

#if defined(CONFIG_S5K6A3YX)
	return sprintf(buf, "%s %s", cam_fw[0], cam_fw[1]);
#else
	return snprintf(buf, sizeof(cam_fw), "%s", cam_fw);
#endif
}

static DEVICE_ATTR(rear_camfw, 0664, back_camera_firmware_show, NULL);
static DEVICE_ATTR(front_camfw, 0664, front_camera_firmware_show, NULL);


static ssize_t cameraflash_file_cmd_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int value;
	int err = 1;
	int flash_rev = 0;

	flash_rev = 0;

	if (strlen(buf) > 2)
		return -err;

	if (isdigit(*buf)) {
		err = kstrtoint(buf, 10, &value);
		if (err < 0)
			pr_err("%s, kstrtoint failed.", __func__);
	} else
		return -err;

#if defined(CONFIG_S5C73M3) && defined(CONFIG_S5K6A3YX) /* D2 */
#ifdef CONFIG_LEDS_AAT1290A
	err = aat1290a_flash_power(value);
	if (err < 0)
		printk(KERN_DEBUG "[%s : %d]aat1290a_flash_power"
			" contorl fail!!\n", __func__, __LINE__);
#endif
#endif
	return size;
}

static DEVICE_ATTR(rear_flash, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cameraflash_file_cmd_store);

static void msm8960_cam_create_node(void)
{
	struct device *cam_dev_back;
	struct device *cam_dev_front;
	struct class *camera_class;

	camera_class = class_create(THIS_MODULE, "camera");

	if (IS_ERR(camera_class)) {
		pr_err("Failed to create class(camera)!\n");
		return;
	}
	cam_dev_back = device_create(camera_class, NULL,
		0, NULL, "rear");

	if (IS_ERR(cam_dev_back)) {
		pr_err("Failed to create cam_dev_back device!\n");
		goto OUT7;
	}

	if (device_create_file(cam_dev_back, &dev_attr_rear_flash) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_flash.attr.name);
		goto OUT6;
	}

	if (device_create_file(cam_dev_back, &dev_attr_rear_camtype) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_camtype.attr.name);
		goto OUT5;
	}

	if (device_create_file(cam_dev_back, &dev_attr_rear_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_rear_camfw.attr.name);
		goto OUT4;
	}

	cam_dev_front = device_create(camera_class, NULL,
		1, NULL, "front");

	if (IS_ERR(cam_dev_front)) {
		pr_err("Failed to create cam_dev_front device!");
		goto OUT3;
	}

	if (device_create_file(cam_dev_front, &dev_attr_front_camtype) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_camtype.attr.name);
		goto OUT2;
	}

	if (device_create_file(cam_dev_front, &dev_attr_front_camfw) < 0) {
		pr_err("Failed to create device file!(%s)!\n",
			dev_attr_front_camfw.attr.name);
		goto OUT1;
	}

	return;

OUT1:
	device_remove_file(cam_dev_back, &dev_attr_front_camtype);
OUT2:
	device_destroy(camera_class, 1);
OUT3:
	device_remove_file(cam_dev_back, &dev_attr_rear_camfw);
OUT4:
	device_remove_file(cam_dev_back, &dev_attr_rear_camtype);
OUT5:
	device_remove_file(cam_dev_back, &dev_attr_rear_flash);
OUT6:
	device_destroy(camera_class, 0);
OUT7:
	return;
}
#ifdef CONFIG_S5C73M3
static struct spi_board_info s5c73m3_spi_info[] __initdata = {
	{
		.modalias		= "s5c73m3_spi",
		.mode			= SPI_MODE_0,
		.bus_num		= 0,
		.chip_select	= 0,
		.max_speed_hz	= 48000000,
	}
};
#endif
#if 0
static struct pm8xxx_mpp_config_data privacy_light_on_config = {
	.type		= PM8XXX_MPP_TYPE_SINK,
	.level		= PM8XXX_MPP_CS_OUT_5MA,
	.control	= PM8XXX_MPP_CS_CTRL_MPP_LOW_EN,
};
static struct pm8xxx_mpp_config_data privacy_light_off_config = {
	.type		= PM8XXX_MPP_TYPE_SINK,
	.level		= PM8XXX_MPP_CS_OUT_5MA,
	.control	= PM8XXX_MPP_CS_CTRL_DISABLE,
};
static int32_t msm_camera_8960_ext_power_ctrl(int enable)
{
	int rc = 0;
	if (enable) {
		rc = pm8xxx_mpp_config(PM8921_MPP_PM_TO_SYS(12),
			&privacy_light_on_config);
	} else {
		rc = pm8xxx_mpp_config(PM8921_MPP_PM_TO_SYS(12),
			&privacy_light_off_config);
	}
	return rc;
}
#endif
static inline int get_mclk_rev(void)
{
#if defined(CONFIG_MACH_M2)
	return ((system_rev >= BOARD_REV13) ? 1 : 0);
#elif defined(CONFIG_MACH_M2_SKT)
	return ((system_rev >= BOARD_REV09) ? 1 : 0);
#elif defined(CONFIG_MACH_M2_DCM) || defined(CONFIG_MACH_K2_KDI)
	return ((system_rev >= BOARD_REV03) ? 1 : 0);
#elif defined(CONFIG_MACH_APEXQ)
	return ((system_rev >= BOARD_REV04) ? 1 : 0);
#elif defined(CONFIG_MACH_COMANCHE)
	return ((system_rev >= BOARD_REV03) ? 1 : 0);
#elif defined(CONFIG_MACH_EXPRESS)
	return ((system_rev >= BOARD_REV03) ? 1 : 0);
#elif defined(CONFIG_MACH_AEGIS2)
	return ((system_rev >= BOARD_REV07) ? 1 : 0);
#elif defined(CONFIG_MACH_JASPER)
	return ((system_rev >= BOARD_REV08) ? 1 : 0);
#elif defined(CONFIG_MACH_STRETTO)
	return 1;
#elif defined(CONFIG_MACH_SUPERIORLTE_SKT)
	return 1;
#else
	return 0;
#endif
}

void __init msm8960_init_cam(void)
{
	int rev = 0;
	struct msm_camera_sensor_info *s_info;

	rev = get_mclk_rev();

#if !defined(CONFIG_MACH_STRETTO)
#if defined(CONFIG_S5C73M3) || defined(CONFIG_S5K6A3YX)\
	|| defined(CONFIG_MACH_APEXQ) || defined(CONFIG_MACH_COMANCHE) \
	|| defined(CONFIG_MACH_EXPRESS) || defined(CONFIG_MACH_GOGH) \
	|| defined(CONFIG_MACH_INFINITE)
	/*|| ((defined(CONFIG_ISX012) || defined(CONFIG_DB8131M))\temp */

	if (rev) {
		int rc;

		struct pm_gpio param_flash = {
			.direction      = PM_GPIO_DIR_OUT,
			.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
			.output_value   = 0,
			.pull	   = PM_GPIO_PULL_NO,
			.vin_sel	= PM_GPIO_VIN_S4,
			.out_strength   = PM_GPIO_STRENGTH_MED,
			.function       = PM_GPIO_FUNC_NORMAL,
		};

		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS
			(PMIC_MSM_FLASH_CNTL_EN), &param_flash);

		if (rc) {
			pr_err("%s pmic gpio config failed\n", __func__);
			return;
		}
		pmic_gpio_msm_flash_cntl_en =
			PM8921_GPIO_PM_TO_SYS(PMIC_MSM_FLASH_CNTL_EN);
	} else {
		pmic_gpio_msm_flash_cntl_en = 0;
	}
	isFlashCntlEn = false;
#endif
#endif

	msm8960_cam_create_node();

	msm_gpiomux_install(msm8960_cam_common_configs,
			ARRAY_SIZE(msm8960_cam_common_configs));

#if defined(CONFIG_S5C73M3) && defined(CONFIG_S5K6A3YX)
#if defined(CONFIG_MACH_M2_DCM) || defined(CONFIG_MACH_K2_KDI)
	gpio_tlmm_config(GPIO_CFG(gpio_rev(CAM_CORE_EN), 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
#else
	gpio_tlmm_config(GPIO_CFG(CAM_CORE_EN, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
#endif
	gpio_tlmm_config(GPIO_CFG(CAM_MIPI_EN, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_CAM_A_EN, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_CAM_SENSOR_EN, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(ISP_RESET, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
#if defined(CONFIG_MACH_M2_DCM) || defined(CONFIG_MACH_K2_KDI)
	gpio_tlmm_config(GPIO_CFG(gpio_rev(CAM2_RST_N), 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
#else
	gpio_tlmm_config(GPIO_CFG(CAM2_RST_N, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
#endif

	gpio_cam_flash_sw = gpio_rev(CAM_FLASH_SW);
	gpio_tlmm_config(GPIO_CFG(gpio_cam_flash_sw, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(gpio_rev(CAM_AF_EN), 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE);

	if (!pmic_gpio_msm_flash_cntl_en) {
		gpio_tlmm_config(GPIO_CFG(GPIO_MSM_FLASH_CNTL_EN, 0,
			GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	}
	gpio_tlmm_config(GPIO_CFG(GPIO_MSM_FLASH_NOW, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_VT_CAM_SEN_DET, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
#endif

#if defined(CONFIG_S5C73M3)
	s_info = &msm_camera_sensor_s5c73m3_data;
#if defined(CONFIG_MACH_M2_DCM) || defined(CONFIG_MACH_K2_KDI)
	s_info->sensor_platform_info->sensor_pwd =
		gpio_rev(CAM_CORE_EN);
#endif
	msm_get_cam_resources(s_info);
	platform_device_register(cam_dev[0]);
#endif
#if defined(CONFIG_S5K6A3YX)
	s_info = &msm_camera_sensor_s5k6a3yx_data;
#if defined(CONFIG_MACH_M2_DCM) || defined(CONFIG_MACH_K2_KDI)
	s_info->sensor_platform_info->sensor_pwd =
		gpio_rev(CAM_CORE_EN);
#endif
	 if (rev) {
		s_info->sensor_platform_info->mclk =
			GPIO_CAM_MCLK2;
		s_info->gpio_conf->cam_gpiomux_conf_tbl =
			msm8960_cam_2d_configs_v2;
		s_info->gpio_conf->cam_gpio_tbl =
			msm_cam_gpio_2d_tbl_v2;
	}

	msm_get_cam_resources(s_info);
	platform_device_register(cam_dev[1]);
	if (spi_register_board_info(
				    s5c73m3_spi_info,
				    ARRAY_SIZE(s5c73m3_spi_info)) != 0)
		pr_err("%s: spi_register_board_info returned error\n",
			__func__);

	pr_err("[%s:%d]setting done!!\n", __func__, __LINE__);
#endif

	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);
#ifdef CONFIG_LEDS_AAT1290A
	platform_device_register(&s3c_device_aat1290a_led);
#endif
}

#ifdef CONFIG_I2C
static struct i2c_board_info msm8960_camera_i2c_boardinfo[] = {
#ifdef CONFIG_S5K6A3YX
	{
		I2C_BOARD_INFO("s5k6a3yx", 0x20),
		.platform_data = &msm_camera_sensor_s5k6a3yx_data,
	},
#endif
};

struct msm_camera_board_info msm8960_camera_board_info = {
	.board_info = msm8960_camera_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(msm8960_camera_i2c_boardinfo),
};

static struct resource msm_camera_resources[] __initdata = {
	{
		.name   = "s3d_rw",
		.start  = 0x008003E0,
		.end    = 0x008003E0 + SZ_16 - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.name   = "s3d_ctl",
		.start  = 0x008020B8,
		.end    = 0x008020B8 + SZ_16 - 1,
		.flags  = IORESOURCE_MEM,
	},
};

void __init msm_get_cam_resources(struct msm_camera_sensor_info *s_info)
{
	s_info->resource = msm_camera_resources;
	s_info->num_resources = ARRAY_SIZE(msm_camera_resources);
}

#endif
#endif
