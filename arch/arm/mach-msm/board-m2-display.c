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

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <asm/mach-types.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/msm8960-gpio.h>
#include <linux/ion.h>
#include <linux/msm_ion.h>
#include <mach/ion.h>

#include <mach/socinfo.h>

#include "devices.h"
#include "board-8960.h"
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include "devices-msm8x60.h"

#ifdef CONFIG_SAMSUNG_CMC624
/*1.8V*/
#define MLCD_ON		PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_MLCD_ON)
/*CMC_DCDC_EN (1.1V)*/
#define IMA_PWR_EN	PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_IMA_PWR_EN)
/*FAIL_SAFEB*/
#define IMA_CMC_EN	PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_IMA_CMC_EN)
#define IMA_nRST	PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_IMA_nRST) /*RESETB */
#define IMA_SLEEP	PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_IMA_SLEEP) /*SLEEPB*/
#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
#if defined(CONFIG_SAMSUNG_CMC624)
#define CMC_ESD	PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_CMC_ESD_DET)
#define OLED_ESD	PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_VGH_ESD_DET)
#endif
#endif
static struct gpiomux_setting cmc624_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir  = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting cmc624_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir  = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm8x60_cmc624_configs[] __initdata = {
	{
		.gpio      = 71,
		.settings = {
			[GPIOMUX_ACTIVE]	= &cmc624_active_cfg,
			[GPIOMUX_SUSPENDED] = &cmc624_suspend_cfg,
		},
	},
	{
		.gpio      = 72,
		.settings = {
			[GPIOMUX_ACTIVE]	= &cmc624_active_cfg,
			[GPIOMUX_SUSPENDED] = &cmc624_suspend_cfg,
		},
	},

	{
		.gpio      = 10,
		.settings = {
			[GPIOMUX_ACTIVE]	= &cmc624_active_cfg,
			[GPIOMUX_SUSPENDED] = &cmc624_suspend_cfg,
		},
	},
};

static struct i2c_gpio_platform_data cmc624_i2c_gpio_data = {
	.sda_pin    = GPIO_IMA_I2C_SDA,
	.scl_pin    = GPIO_IMA_I2C_SCL,
	.udelay		= 5,
};

static struct platform_device cmc624_i2c_gpio_device = {
	.name       = "i2c-gpio",
	.id     = MSM_CMC624_I2C_BUS_ID,
	.dev        = {
		.platform_data  = &cmc624_i2c_gpio_data,
	},
};
#else
#define OLED_ESD	PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_VGH_ESD_DET)
#endif


#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
static struct sec_esd_platform_data esd_pdata;
static struct platform_device samsung_mipi_esd_refresh_device = {
	.name = "samsung_mipi_esd_refresh",
	.id = -1,
	.dev = {
		.platform_data  = &esd_pdata,
	},
};
#endif
#if defined(CONFIG_FB_MSM_TRIPLE_BUFFER)
/* prim = 540 x 960 x 4(bpp) x 3(pages) */
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL)
#define MSM_FB_PRIM_BUF_SIZE (480 * 800 * 4 * 3)
#elif defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL)
#define MSM_FB_PRIM_BUF_SIZE (1024 * 600 * 4 * 3)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL)
#define MSM_FB_PRIM_BUF_SIZE (1280 * 800 * 4 * 3)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT_PANEL)
#define MSM_FB_PRIM_BUF_SIZE (544 * 960 * 4 * 3)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT_PANEL)
#define MSM_FB_PRIM_BUF_SIZE (1280 * 736 * 4 * 3)
#else
#define MSM_FB_PRIM_BUF_SIZE (480 * 800 * 4 * 3)
#endif
#else
/* prim = 540 x 960 x 4(bpp) x 2(pages) */
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL)
#define MSM_FB_PRIM_BUF_SIZE (480 * 800 * 4 * 2)
#elif defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL)
#define MSM_FB_PRIM_BUF_SIZE (1024 * 600 * 4 * 2)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL)
#define MSM_FB_PRIM_BUF_SIZE (1280 * 800 * 4 * 2)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT_PANEL)
#define MSM_FB_PRIM_BUF_SIZE (544 * 960 * 4 * 2)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT_PANEL)
#define MSM_FB_PRIM_BUF_SIZE (1280 * 736 * 4 * 2)
#else
#define MSM_FB_PRIM_BUF_SIZE (480 * 800 * 4 * 2)
#endif
#endif

#if defined(CONFIG_FB_MSM_MIPI_DSI)
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL)
/* 480 x 800 x 3 x 2 */
#define MIPI_DSI_WRITEBACK_SIZE (480 * 800 * 3 * 2)
#elif defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL)
/* 1024 x 600 x 3 x 2 */
#define MIPI_DSI_WRITEBACK_SIZE (1024 * 600 * 3 * 2)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL)
/* 1280 x 800 x 3 x 2 */
#define MIPI_DSI_WRITEBACK_SIZE (1280 * 800 * 3 * 2)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT_PANEL)
/* 540 x 960 x 3 x 2 */
#define MIPI_DSI_WRITEBACK_SIZE (544 * 960 * 3 * 2)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT_PANEL)
/*1280 x732 x 3 x 2 */
#define MIPI_DSI_WRITEBACK_SIZE (1280 * 736 * 3 * 2)
#endif
#else
#define MIPI_DSI_WRITEBACK_SIZE 0
#endif

/* Note: must be multiple of 4096 */
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE, 4096)

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL)
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((480 * 800 * 3 * 2), 4096)
#elif defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL)
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1024 * 608 * 3 * 2), 4096)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL)
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1280 * 800 * 3 * 2), 4096)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT_PANEL)
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((544 * 960 * 3 * 2), 4096)
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT_PANEL)
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1280 * 736 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1920 * 1200 * 3 * 2), 4096)
#endif
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY1_WRITEBACK */

#define MDP_VSYNC_GPIO 0

#define MIPI_CMD_NOVATEK_QHD_PANEL_NAME	"mipi_cmd_novatek_qhd"
#define MIPI_CMD_NOVATEK_WVGA_PANEL_NAME	"mipi_cmd_novatek_wvga"
#define MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME	"mipi_video_novatek_qhd"
#define MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME	"mipi_video_toshiba_wsvga"
#define MIPI_VIDEO_TOSHIBA_WUXGA_PANEL_NAME	"mipi_video_toshiba_wuxga"
#define MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME	"mipi_video_chimei_wxga"
#define MIPI_VIDEO_CHIMEI_WUXGA_PANEL_NAME	"mipi_video_chimei_wuxga"
#define MIPI_VIDEO_BOEOT_WSVGA_PANEL_NAME	"mipi_video_boeot_tft_wsvga"
#define MIPI_VIDEO_SAMSUNG_WXGA_PANEL_NAME	"mipi_video_samsung_tft_wxga"
#define MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME	"mipi_video_simulator_vga"
#define MIPI_CMD_RENESAS_FWVGA_PANEL_NAME	"mipi_cmd_renesas_fwvga"
#define MIPI_VIDEO_ORISE_720P_PANEL_NAME	"mipi_video_orise_720p"
#define MIPI_CMD_ORISE_720P_PANEL_NAME		"mipi_cmd_orise_720p"
#define HDMI_PANEL_NAME	"hdmi_msm"
#define TVOUT_PANEL_NAME	"tvout_msm"

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
 unsigned char hdmi_is_primary = 1;
#else
 unsigned char hdmi_is_primary;
#endif

unsigned char msm8960_hdmi_as_primary_selected(void)
{
	return hdmi_is_primary;
}

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};

static void set_mdp_clocks_for_wuxga(void);

static int msm_fb_detect_panel(const char *name)
{
	if (machine_is_msm8960_liquid()) {
		if (!strncmp(name, MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;
		
		if (!strncmp(name, MIPI_VIDEO_BOEOT_WSVGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_BOEOT_WSVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;
		if (!strncmp(name, MIPI_VIDEO_SAMSUNG_WXGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_SAMSUNG_WXGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

	} else {
		if (!strncmp(name, MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

#ifndef CONFIG_FB_MSM_MIPI_PANEL_DETECT
		if (!strncmp(name, MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME,
				strnlen(MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_CMD_NOVATEK_QHD_PANEL_NAME,
				strnlen(MIPI_CMD_NOVATEK_QHD_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_CMD_NOVATEK_WVGA_PANEL_NAME,
				strnlen(MIPI_CMD_NOVATEK_WVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_CMD_RENESAS_FWVGA_PANEL_NAME,
				strnlen(MIPI_CMD_RENESAS_FWVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_VIDEO_TOSHIBA_WUXGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_TOSHIBA_WUXGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN))) {
			set_mdp_clocks_for_wuxga();
			return 0;
		}

		if (!strncmp(name, MIPI_VIDEO_ORISE_720P_PANEL_NAME,
				strnlen(MIPI_VIDEO_ORISE_720P_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_CMD_ORISE_720P_PANEL_NAME,
				strnlen(MIPI_CMD_ORISE_720P_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;
#endif
	}

	if (!strncmp(name, HDMI_PANEL_NAME,
			strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
		if (hdmi_is_primary)
			set_mdp_clocks_for_wuxga();
		return 0;
	}

	if (!strncmp(name, TVOUT_PANEL_NAME,
			strnlen(TVOUT_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
		return 0;

	pr_warning("%s: not supported '%s'", __func__, name);
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

static void mipi_dsi_panel_pwm_cfg(void)
{
	int rc;
	static int mipi_dsi_panel_gpio_configured;
	static struct pm_gpio pwm_enable = {
		.direction        = PM_GPIO_DIR_OUT,
		.output_buffer    = PM_GPIO_OUT_BUF_CMOS,
		.output_value     = 1,
		.pull             = PM_GPIO_PULL_NO,
		.vin_sel          = PM_GPIO_VIN_VPH,
		.out_strength     = PM_GPIO_STRENGTH_HIGH,
		.function         = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol      = 0,
		.disable_pin      = 0,
	};
	static struct pm_gpio pwm_mode = {
		.direction        = PM_GPIO_DIR_OUT,
		.output_buffer    = PM_GPIO_OUT_BUF_CMOS,
		.output_value     = 0,
		.pull             = PM_GPIO_PULL_NO,
		.vin_sel          = PM_GPIO_VIN_S4,
		.out_strength     = PM_GPIO_STRENGTH_HIGH,
		.function         = PM_GPIO_FUNC_2,
		.inv_int_pol      = 0,
		.disable_pin      = 0,
	};

	if (mipi_dsi_panel_gpio_configured == 0) {
		/* pm8xxx: gpio-21, Backlight Enable */
		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(21),
					&pwm_enable);
		if (rc != 0)
			pr_err("%s: pwm_enabled failed\n", __func__);

		/* pm8xxx: gpio-24, Bl: Off, PWM mode */
		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(24),
					&pwm_mode);
		if (rc != 0)
			pr_err("%s: pwm_mode failed\n", __func__);

		mipi_dsi_panel_gpio_configured++;
	}
}

static bool dsi_power_on;

#if defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL)
/*
 * Macros to be used in espresso panel power function for
 * controlling regulators.
 */
#define LVDS_REGULATOR_TUNE(regvar, conmdev, supply_name, optmodcurr) \
	do { \
		regvar = regulator_get(conmdev, #supply_name); \
		if (IS_ERR(regvar)) { \
			pr_err("[%s] could not get " \
				#supply_name", rc = %ld\n", __func__,\
				PTR_ERR(regvar)); \
			return -ENODEV; \
		} \
		rc = regulator_set_optimum_mode(regvar, optmodcurr); \
		if (rc < 0) { \
			pr_err("[%s] set_optimum_mode high " \
				#regvar" failed, rc=%d\n", __func__, rc); \
			return -EINVAL; \
		} \
	} while (0);


#define LVDS_REGULATOR_ENABLE(rpm_name, minvolt, maxvolt) \
	do { \
		rpm_vreg_set_voltage(rpm_name, RPM_VREG_VOTER3, \
				minvolt, maxvolt, 1);\
	} while (0);

#define LVDS_REGULATOR_DISABLE(rpm_name) \
	do { \
		rpm_vreg_set_voltage(rpm_name, RPM_VREG_VOTER3, 0, 0, 1);\
	} while (0);

/**
 * Espresso Board DSI power on/off
 *
 * @param on
 *
 * @return int
 */
static int mipi_dsi_espresso_dsi_power(int on)
{
	static struct regulator *vreg_l2_1p2;
	int rc;

	if (!dsi_power_on) {
		/* VDD_MIPI */
		LVDS_REGULATOR_TUNE(vreg_l2_1p2,
				&msm_mipi_dsi1_device.dev, dsi_vdda, 100000)

		dsi_power_on = true;
	}
	if (on) {
		pr_info("%s: power on sequence\n", __func__);

		/* Enable power - DSI */
		LVDS_REGULATOR_ENABLE(RPM_VREG_ID_PM8921_L2, 1200000, 1200000)

	} else {
		pr_info("%s: power off sequence\n", __func__);
		/* Disable power - DSI */
		LVDS_REGULATOR_DISABLE(RPM_VREG_ID_PM8921_L2)
	}
	return 0;
}

static int mipi_dsi_tc35reset_release(void)
{
	/* Perform LVDS_RST */
	gpio_set_value_cansleep(GPIO_LVDS_RST, 1); /* disp enable */
	return 0;
}

static bool espresso_panel_power_on;
/**
 * Espresso Board panel on/off
 *
 * @param on
 *
 * @return int
 */
static int mipi_dsi_espresso_panel_power(int on)
{
	static struct regulator *vreg_l18_lvds_1p2_vddc, *vreg_l16_lvds_3p3v;
	static int led_backlight_reset, check_bootup_for_vreg_l16;
	int rc;

	pr_info("%s: on=%d\n", __func__, on);

	if (!espresso_panel_power_on) {

		/* Configure and Get GPIO LVDS_RST*/
		gpio_tlmm_config(GPIO_CFG(GPIO_LVDS_RST, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
				GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(GPIO_USB_I2C_SDA, 0, GPIO_CFG_INPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				GPIO_CFG_DISABLE);

		gpio_tlmm_config(GPIO_CFG(GPIO_USB_I2C_SCL, 0, GPIO_CFG_INPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				GPIO_CFG_DISABLE);
		led_backlight_reset = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_LCD_RST);
		rc = gpio_request(led_backlight_reset, "led_backlight_reset");
		if (rc) {
			pr_err("request gpio 43 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		gpio_set_value_cansleep(GPIO_LVDS_RST, 0);
		gpio_set_value_cansleep(led_backlight_reset, 0);

		/*
		 * Enabling and tuning Supplies related to processor
		 * DSI.
		 */
		/*
		 * Get the regulator reference for rest of the LVDS related
		 * supplies and setting voltage requirement.
		 */
		/* VDD_MIPI + VDDC */
		LVDS_REGULATOR_TUNE(vreg_l18_lvds_1p2_vddc,
				&msm_mipi_dsi1_device.dev, lvds_1p2, 100000)
		/* VDD_LVDS1_3 */
		LVDS_REGULATOR_TUNE(vreg_l16_lvds_3p3v,
				&msm_mipi_dsi1_device.dev, lvds_3p3, 100000)

		espresso_panel_power_on = true;
	}
	if (on) {
		gpio_set_value_cansleep(GPIO_LVDS_RST, 0);
		pr_info("%s: power on sequence\n", __func__);

		/*
		 *  Enable power - Toshiba D2L
		 */
		/* VDD */
		LVDS_REGULATOR_ENABLE(RPM_VREG_ID_PM8921_L18,
				1200000, 1200000)
		/* VCC_IO */
		if (((machine_is_ESPRESSO_VZW() && (system_rev >= BOARD_REV04)))
			|| machine_is_ESPRESSO10_VZW()
			|| machine_is_ESPRESSO10_SPR()
			|| machine_is_ESPRESSO10_ATT()
			|| machine_is_ESPRESSO_SPR())
			LVDS_REGULATOR_ENABLE(RPM_VREG_ID_PM8921_LVS6, 1, 1)
		else
			LVDS_REGULATOR_ENABLE(RPM_VREG_ID_PM8921_LVS5, 1, 1)
		/* VDD_LVDS */
		if (check_bootup_for_vreg_l16) {
			LVDS_REGULATOR_ENABLE(RPM_VREG_ID_PM8921_L16,
				3300000, 3300000)
		}
		check_bootup_for_vreg_l16 = 1;

		/* Perform LVDS_RST */
		gpio_set_value_cansleep(led_backlight_reset, 1);

	} else {
		pr_info("%s: power off sequence\n", __func__);

		/* Assert reset */
		gpio_set_value_cansleep(GPIO_LVDS_RST, 0);
		mdelay(20);

		/* Disable power - D2L */
		LVDS_REGULATOR_DISABLE(RPM_VREG_ID_PM8921_L18)
		/* Disable LCD Power */
		LVDS_REGULATOR_DISABLE(RPM_VREG_ID_PM8921_L16)
		if (((machine_is_ESPRESSO_VZW() && (system_rev >= BOARD_REV04)))
				|| machine_is_ESPRESSO10_SPR()
				|| machine_is_ESPRESSO10_VZW()
				|| machine_is_ESPRESSO10_ATT()
				|| machine_is_ESPRESSO_SPR())
			LVDS_REGULATOR_DISABLE(RPM_VREG_ID_PM8921_LVS6)
		else
			LVDS_REGULATOR_DISABLE(RPM_VREG_ID_PM8921_LVS5)

		gpio_set_value_cansleep(led_backlight_reset, 0);
	}
	return 0;
}

#undef LVDS_REGULATOR_TUNE
#undef LVDS_REGULATOR_ENABLE
#undef LVDS_REGULATOR_DISABLE
#endif /* CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL */

/**
 * LiQUID panel on/off
 *
 * @param on
 *
 * @return int
 */
static int mipi_dsi_liquid_panel_power(int on)
{
	static struct regulator *reg_l2, *reg_ext_3p3v;
	static int gpio21, gpio24, gpio43;
	int rc;

	mipi_dsi_panel_pwm_cfg();
	pr_debug("%s: on=%d\n", __func__, on);

	gpio21 = PM8921_GPIO_PM_TO_SYS(21); /* disp power enable_n */
	/*Display Enable (rst_n)*/
	gpio43 = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_LCD_RST);
	gpio24 = PM8921_GPIO_PM_TO_SYS(24); /* Backlight PWM */

	if (!dsi_power_on) {

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		reg_ext_3p3v = regulator_get(&msm_mipi_dsi1_device.dev,
			"vdd_lvds_3p3v");
		if (IS_ERR(reg_ext_3p3v)) {
			pr_err("could not get reg_ext_3p3v, rc = %ld\n",
			       PTR_ERR(reg_ext_3p3v));
		    return -ENODEV;
		}

		rc = gpio_request(gpio21, "disp_pwr_en_n");
		if (rc) {
			pr_err("request gpio 21 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = gpio_request(gpio43, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 43 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = gpio_request(gpio24, "disp_backlight_pwm");
		if (rc) {
			pr_err("request gpio 24 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		dsi_power_on = true;
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_ext_3p3v);
		if (rc) {
			pr_err("enable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}

		/* set reset pin before power enable */
		gpio_set_value_cansleep(gpio43, 0); /* disp disable (resx=0) */

		gpio_set_value_cansleep(gpio21, 0); /* disp power enable_n */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 1); /* disp enable */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 0); /* disp enable */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 1); /* disp enable */
		msleep(20);
	} else {
		gpio_set_value_cansleep(gpio43, 0);
		gpio_set_value_cansleep(gpio21, 1);

		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_ext_3p3v);
		if (rc) {
			pr_err("disable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	return 0;
}

static void active_reset_ldi(void)
{

	int gpio43 = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_LCD_RST);
	pr_info("Active Reset: Resettig LCD 1...0..1\n");
	gpio_direction_output(gpio43, 1);
#if defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL)
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL)
	gpio_direction_output(gpio43, 0);
	msleep(20);
	gpio_direction_output(gpio43, 1);
	msleep(20);
#else
	mdelay(10);
	gpio_direction_output(gpio43, 0);
	mdelay(30);
	gpio_direction_output(gpio43, 1);
	mdelay(100);
#endif
#else
	udelay(500);
	gpio_direction_output(gpio43, 0);
	mdelay(5);
	gpio_direction_output(gpio43, 1);
	udelay(500);
#endif /* CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT_PANEL */
}

void pull_ldi_reset_down(void)
{
	int gpio43 = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_LCD_RST);
	gpio_direction_output(gpio43, 0); /*RESETB*/
}

void pull_ldi_reset_up(void)
{
	int gpio43 = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_LCD_RST);
	gpio_direction_output(gpio43, 1); /*RESETB*/
}
#ifdef CONFIG_SAMSUNG_CMC624
void cmc_power(int on)
{
		if (on) {
			/*RESETB->hi, FAILSAFE->lo, SLEEPB->hi */
			pr_info("Performing CMC624 Power On Init\n");
			gpio_direction_output(IMA_nRST, 1); /*RESETB*/
			gpio_direction_output(IMA_SLEEP, 1); /*SLEEPB*/
			udelay(50); /*Wait 50us*/
			/* V_IMA_1.1V on*/
			gpio_direction_output(IMA_PWR_EN, 1);
			udelay(50);
			/*V_IMA_1.8V on*/
			gpio_direction_output(MLCD_ON, 1);
			udelay(50);
			gpio_direction_output(IMA_CMC_EN, 1); /*FAIL_SAFEB*/
			udelay(50);
			gpio_direction_output(IMA_nRST, 0); /*RESETB*/
			mdelay(4);
			gpio_direction_output(IMA_nRST, 1); /*RESETB*/
			return;
		} else {
		      pr_info("CMC Power off ................\n");
			/*FAILSAFE->lo*/
			gpio_direction_output(IMA_CMC_EN, 0);
			/*V_IMA_1.8V off*/
			gpio_direction_output(MLCD_ON, 0);
			/* V_IMA_1.1V off*/
			gpio_direction_output(IMA_PWR_EN, 0);
			/* RESETB->lo, FAILSAFE->lo, SLEEPB->lo */
			gpio_direction_output(IMA_nRST, 0);
			gpio_direction_output(IMA_SLEEP, 0);
			gpio_tlmm_config(GPIO_CFG(GPIO_IMA_I2C_SDA,  0,
			GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_DISABLE);
			gpio_tlmm_config(GPIO_CFG(GPIO_IMA_I2C_SCL, 0,
			GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_DISABLE);
		}
}
#endif
#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
#if defined(CONFIG_SAMSUNG_CMC624)
void set_esd_gpio_config(void)
{
	int rc;
	struct pm_gpio sec_mipi_vgh_esd_det_gpio_cfg = {
		.direction			= PM_GPIO_DIR_IN,
		.pull				= PM_GPIO_PULL_NO,
		.vin_sel			= PM_GPIO_VIN_L17,
		.function			= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol		= 0,
	};
	struct pm_gpio sec_mipi_cmc_esd_det_gpio_cfg = {
		.direction                      = PM_GPIO_DIR_IN,
		.pull                           = PM_GPIO_PULL_DN,
		.vin_sel                        = PM_GPIO_VIN_L17,
		.function                       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol            = 0,
	};
	pm8xxx_gpio_config(OLED_ESD, &sec_mipi_vgh_esd_det_gpio_cfg);
	rc = gpio_request(OLED_ESD, "IMA_ESD_DET");
	if (rc)
		pr_err("request OLED_ESD failed, rc=%d\n", rc);

	pm8xxx_gpio_config(CMC_ESD, &sec_mipi_cmc_esd_det_gpio_cfg);
	rc = gpio_request(CMC_ESD, "IMA_ESD_DET");
	if (rc)
		pr_err("request CMC_ESD failed, rc=%d\n", rc);
}
#elif defined(CONFIG_MACH_JAGUAR) || \
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL)
void set_esd_gpio_config(void)
{
	int rc;
	struct pm_gpio sec_mipi_vgh_esd_det_gpio_cfg = {
		.direction			= PM_GPIO_DIR_IN,
		.pull				= PM_GPIO_PULL_UP_30,
		.vin_sel			= PM_GPIO_VIN_L17,
		.function			= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol		= 0,
	};
	pm8xxx_gpio_config(OLED_ESD, &sec_mipi_vgh_esd_det_gpio_cfg);
	rc = gpio_request(OLED_ESD, "IMA_ESD_DET");
	if (rc)
		pr_err("request OLED_ESD failed, rc=%d\n", rc);
}
#elif defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL)
void set_esd_gpio_config(void)
{
	int rc;
	int esd_gpio = gpio_rev(ESD_DET);

	struct pm_gpio sec_mipi_esd_det_gpio_cfg = {
		.direction			= PM_GPIO_DIR_IN,
		.pull				= PM_GPIO_PULL_NO,
		.vin_sel			= PM_GPIO_VIN_L17,
		.function			= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol		= 0,
	};

	if  (system_rev >= BOARD_REV02) {

		esd_gpio = PM8921_GPIO_PM_TO_SYS(esd_gpio);
		pm8xxx_gpio_config(esd_gpio, &sec_mipi_esd_det_gpio_cfg);
		esd_pdata.esd_gpio_irq = PM8921_GPIO_IRQ(PM8921_IRQ_BASE,
			esd_gpio);
	} else {
		gpio_tlmm_config(GPIO_CFG(esd_gpio,  0,
		GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
		esd_pdata.esd_gpio_irq =  MSM_GPIO_TO_INT(esd_gpio);
	}
	rc = gpio_request(esd_gpio, "ESD_DET");
	if (rc) {
		pr_err("request ESD gpio failed, rc=%d\n", rc);
		return;
	}
}
#endif
#endif

#ifdef CONFIG_SAMSUNG_CMC624 /*change CMC gpio cfg*/
static int  mipi_pmic_gpios_pmconfig(int state)
{
	int ret;

	struct pm_gpio param = {
		.disable_pin = state,
	};

	ret = pm8xxx_gpio_config(MLCD_ON, &param);
	if (ret) {
		pr_err("%s: Failed to configure gpio %d\n", __func__,
			MLCD_ON);
		return ret;
	}
	ret = pm8xxx_gpio_config(IMA_PWR_EN, &param);
	if (ret) {
		pr_err("%s: Failed to configure gpio %d\n", __func__,
			IMA_PWR_EN);
		return ret;
	}
	ret = pm8xxx_gpio_config(IMA_nRST, &param);
	if (ret) {
		pr_err("%s: Failed to configure gpio %d\n", __func__,
			IMA_nRST);
		return ret;
	}
	ret = pm8xxx_gpio_config(IMA_SLEEP, &param);
	if (ret) {
		pr_err("%s: Failed to configure gpio %d\n", __func__,
			IMA_SLEEP);
		return ret;
	}
	ret = pm8xxx_gpio_config(IMA_CMC_EN, &param);
	if (ret) {
		pr_err("%s: Failed to configure gpio %d\n", __func__,
			IMA_CMC_EN);
		return ret;
	}
	return 0;
}
#endif

static int mipi_dsi_cdp_panel_power(int on)
{
	static struct regulator *reg_l8, *reg_l2;
#if defined(CONFIG_MACH_JAGUAR) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_MACH_EXPRESS)
	static struct regulator *reg_l23;
#endif
	static int gpio43;
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT)
	static int gpio_lcd_22v_en;
#endif
	int rc = 0;

	struct pm_gpio gpio43_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_QHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_WVGA_PT)
		.output_value = 1,
#else
		.output_value = 0,
#endif
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
	pr_debug("%s: state : %d\n", __func__, on);

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT)
	gpio_lcd_22v_en = gpio_rev(LCD_22V_EN);
#endif
	gpio43 = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_LCD_RST);
	if (!dsi_power_on) {

		reg_l8 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdc");
		if (IS_ERR(reg_l8)) {
			pr_err("could not get 8921_l8, rc = %ld\n",
				PTR_ERR(reg_l8));
			return -ENODEV;
		}
#if defined(CONFIG_MACH_JAGUAR) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_MACH_EXPRESS)
		reg_l23 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l23)) {
			pr_err("could not get 8921_l23, rc = %ld\n",
				PTR_ERR(reg_l23));
			return -ENODEV;
		}
#else
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT)
		rc = gpio_request(gpio_lcd_22v_en, "lcd_22v_en");
#elif !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT) \
	&& !defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_QHD_PT) \
	&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_WVGA_PT)

		rc = gpio_request(GPIO_LCD_22V_EN, "lcd_22v_en");
#endif
		if (rc) {
			pr_err("request lcd_22v_en failed, rc=%d\n", rc);
			return -ENODEV;
		}
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT)
		gpio_tlmm_config(GPIO_CFG(gpio_lcd_22v_en,  0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
#elif !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT)\
	&& !defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL)\
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_QHD_PT) \
	&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_WVGA_PT)
		gpio_tlmm_config(GPIO_CFG(GPIO_LCD_22V_EN,  0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
#endif
#endif
		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT)
		rc = regulator_set_voltage(reg_l8, 3100000, 3100000);
		if (rc) {
			pr_err("set_voltage l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#else
		rc = regulator_set_voltage(reg_l8, 3000000, 3100000);
		if (rc) {
			pr_err("set_voltage l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif
#if defined(CONFIG_MACH_JAGUAR) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_MACH_EXPRESS)
		rc = regulator_set_voltage(reg_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		/*
		 * turn VDD3.0, VDD1.8 on
		 */
		rc = regulator_set_optimum_mode(reg_l8, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#if defined(CONFIG_MACH_JAGUAR) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_MACH_EXPRESS)
		rc = regulator_set_optimum_mode(reg_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = gpio_request(gpio43, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 43 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = pm8xxx_gpio_config(gpio43, &gpio43_param);
		if (rc) {
			pr_err("gpio_config 43 failed (3), rc=%d\n", rc);
			return -EINVAL;
		}

#if defined(CONFIG_MACH_JAGUAR) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_MACH_EXPRESS)
		rc = regulator_enable(reg_l23);
		if (rc) {
			pr_err("enable l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#else
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT)
		gpio_direction_output(gpio_lcd_22v_en, 1);
#elif !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT)\
	&& !defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_QHD_PT) \
	&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_WVGA_PT)
		gpio_direction_output(GPIO_LCD_22V_EN, 1);
#endif
#endif
		msleep(20);
		rc = regulator_enable(reg_l8);
		if (rc) {
			pr_err("enable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT)\
	|| defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_QHD_PT)\
	|| defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_WVGA_PT)
		udelay(100);
#endif

#if defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT)
		mdelay(150);
#endif
#if defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_QHD_PT) \
	|| defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_WVGA_PT)

		udelay(10);
		active_reset_ldi();
#endif
#if defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT)
		mdelay(50);
#endif
#ifdef CONFIG_SAMSUNG_CMC624
		if (samsung_has_cmc624()) {
			struct pm_gpio cmc_gpio_param = {
				.direction = PM_GPIO_DIR_OUT,
				.output_buffer = PM_GPIO_OUT_BUF_CMOS,
				.output_value = 0,
				.pull = PM_GPIO_PULL_NO,
				.vin_sel = 2,
				.out_strength = PM_GPIO_STRENGTH_HIGH,
				.function = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol = 0,
				.disable_pin = 0,
			};
			gpio_tlmm_config(GPIO_CFG(GPIO_IMA_I2C_SDA, 0,
			GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
			gpio_tlmm_config(GPIO_CFG(GPIO_IMA_I2C_SCL, 0,
			GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);

			pm8xxx_gpio_config(MLCD_ON, &cmc_gpio_param);
			pm8xxx_gpio_config(IMA_PWR_EN, &cmc_gpio_param);
			pm8xxx_gpio_config(IMA_nRST, &cmc_gpio_param);
			pm8xxx_gpio_config(IMA_SLEEP, &cmc_gpio_param);
			pm8xxx_gpio_config(IMA_CMC_EN, &cmc_gpio_param);
			rc = gpio_request(IMA_PWR_EN, "IMA_PWR_EN");
			if (rc) {
				pr_err("request IMA_PWR_EN failed, rc=%d\n",
				rc);
				return -ENODEV;
			}
			rc = gpio_request(IMA_nRST, "IMA_nRST");
			if (rc) {
				pr_err("request IMA_nRST failed, rc=%d\n",
				rc);
				return -ENODEV;
			}
			rc = gpio_request(IMA_SLEEP, "IMA_SLEEP");
			if (rc) {
				pr_err("request IMA_SLEEP failed, rc=%d\n", rc);
				return -ENODEV;
			}
			rc = gpio_request(IMA_CMC_EN, "IMA_CMC_EN");
			if (rc) {
				pr_err("request IMA_CMC_EN failed, rc=%d\n",
				rc);
				return -ENODEV;
			}
			rc = gpio_request(MLCD_ON, "IMA_CMC_EN");
			if (rc) {
				pr_err("request MLCD_ON failed, rc=%d\n", rc);
				return -ENODEV;
			}
			pr_info("All CMC GPIOs configured\n");
			cmc_power(on);
			mdelay(25);
			active_reset_ldi();
			mdelay(5);
			samsung_cmc624_on(1);
		} else {
			gpio_direction_output(gpio43, 1);
		}
#else
		gpio_direction_output(gpio43, 1);
#endif
		dsi_power_on = true;

		return 0;
	}

	if (on) {

		/*
		 * turn VDD3.0, VDD1.8 on
		 */
#ifdef CONFIG_SAMSUNG_CMC624/*change CMC gpio cfg*/
		mipi_pmic_gpios_pmconfig(0);
#endif
		rc = regulator_set_optimum_mode(reg_l8, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#if defined(CONFIG_MACH_JAGUAR) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_MACH_EXPRESS)
		rc = regulator_set_optimum_mode(reg_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#if defined(CONFIG_MACH_JAGUAR) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_MACH_EXPRESS)
		rc = regulator_enable(reg_l23);
		if (rc) {
			pr_err("enable l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#else
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT)
		gpio_direction_output(gpio_lcd_22v_en, 1);
#elif !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT)\
	&& !defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_QHD_PT) \
	&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_WVGA_PT)

		gpio_direction_output(GPIO_LCD_22V_EN, 1);
#endif
#endif

#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL)\
	&& !defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT)
		udelay(500);
#endif
		rc = regulator_enable(reg_l8);
		if (rc) {
			pr_err("enable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}
 #ifdef CONFIG_SAMSUNG_CMC624
 /* Enable CMC Chip */
		if (samsung_has_cmc624())
			cmc_power(on);
 #endif

 #if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL)
		/* Wait 25ms */
		msleep(25);
 #endif
		/* Active Reset */
#ifdef CONFIG_SAMSUNG_CMC624
		if (samsung_has_cmc624()) {
			active_reset_ldi();
			mdelay(5);
			samsung_cmc624_on(1);
		} else {
			active_reset_ldi();
		}
#else
		active_reset_ldi();
#endif
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL)
		udelay(500);
#endif
	} else {

#ifdef CONFIG_SAMSUNG_CMC624
		if (samsung_has_cmc624()) {
			samsung_cmc624_on(0);
			cmc_power(0);
		}
		mipi_pmic_gpios_pmconfig(1);/*change CMC gpio cfg*/
#endif

#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT)
		gpio_set_value_cansleep(gpio43, 0);
#endif

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT) \
	|| defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_QHD_PT)

		msleep(120);
#endif

		rc = regulator_disable(reg_l8);
		if (rc) {
			pr_err("disable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}

#if defined(CONFIG_MACH_JAGUAR) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_MACH_EXPRESS)

		rc = regulator_disable(reg_l23);
		if (rc) {
			pr_err("disable l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#else
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT)
		gpio_direction_output(gpio_lcd_22v_en, 0);
#elif !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT) \
	&& !defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_QHD_PT) \
	&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OLED_VIDEO_WVGA_PT)

		gpio_direction_output(GPIO_LCD_22V_EN, 0);
#endif
#endif
		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_set_optimum_mode(reg_l8, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#if defined(CONFIG_MACH_JAGUAR) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_NOVATEK_BOE_CMD_WVGA_PT_PANEL) \
	|| defined(CONFIG_MACH_EXPRESS)
		rc = regulator_set_optimum_mode(reg_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif
		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT)
		gpio_set_value_cansleep(gpio43, 0);
#endif
	}

	return 0;
}
#if 0
static char mipi_dsi_splash_is_enabled(void);
#endif
static int mipi_dsi_panel_power(int on)
{
	int ret;

	pr_debug("%s: on=%d\n", __func__, on);

	if (machine_is_msm8960_liquid())
		ret = mipi_dsi_liquid_panel_power(on);
#if defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL)
	else if (machine_is_ESPRESSO_VZW()
			|| machine_is_ESPRESSO_SPR()
			|| machine_is_ESPRESSO_ATT()
			|| machine_is_ESPRESSO10_SPR()
			|| machine_is_ESPRESSO10_VZW()
			|| machine_is_ESPRESSO10_ATT())
		ret = mipi_dsi_espresso_dsi_power(on);
#endif
	else
		ret = mipi_dsi_cdp_panel_power(on);

	return ret;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = MDP_VSYNC_GPIO,
	.dsi_power_save = mipi_dsi_panel_power,
#ifdef CONFIG_FB_MSM_MIPI_PANEL_POWERON_LP11
	.dsi_client_power_save = mipi_dsi_espresso_panel_power,
	.dsi_client_reset = mipi_dsi_tc35reset_release,
#endif /* CONFIG_FB_MSM_MIPI_PANEL_POWERON_LP11 */
	.lcd_rst_up = pull_ldi_reset_up,
	.lcd_rst_down = pull_ldi_reset_down,
};
#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors mdp_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 230400000 * 2,
		.ib = 288000000 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 334080000 * 2,
		.ib = 417600000 * 2,
	},
};

static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};

static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};

#endif

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = MDP_VSYNC_GPIO,
	.mdp_max_clk = 200000000,
	.mdp_max_bw = 2000000000,
	.mdp_bw_ab_factor = 230,
	.mdp_bw_ib_factor = 250,
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_42,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_MM_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	.cont_splash_enabled = 0x0,
	.mdp_iommu_split_domain = 0,
};

void __init msm8960_mdp_writeback(struct memtype_reserve* reserve_table)
{
	mdp_pdata.ov0_wb_size = MSM_FB_OVERLAY0_WRITEBACK_SIZE;
	mdp_pdata.ov1_wb_size = MSM_FB_OVERLAY1_WRITEBACK_SIZE;
#if defined(CONFIG_ANDROID_PMEM) && !defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov0_wb_size;
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov1_wb_size;
#endif
}
#if 0
static char mipi_dsi_splash_is_enabled(void)
{
	return mdp_pdata.cont_splash_enabled;
}
#endif
static struct platform_device mipi_dsi_renesas_panel_device = {
	.name = "mipi_renesas",
	.id = 0,
};

static struct platform_device mipi_dsi_simulator_panel_device = {
	.name = "mipi_simulator",
	.id = 0,
};

#define LPM_CHANNEL0 0
#if defined(CONFIG_FB_MSM_MIPI_DSI_TOSHIBA)
static int toshiba_gpio[] = {LPM_CHANNEL0};

static struct mipi_dsi_panel_platform_data toshiba_pdata = {
	.gpio = toshiba_gpio,
	.dsi_pwm_cfg = mipi_dsi_panel_pwm_cfg,
};

static struct platform_device mipi_dsi_toshiba_panel_device = {
	.name = "mipi_toshiba",
	.id = 0,
	.dev = {
		.platform_data = &toshiba_pdata,
	}
};
#endif

#define FPGA_3D_GPIO_CONFIG_ADDR	0xB5
static int dsi2lvds_gpio[2] = {
#if defined(CONFIG_FB_MSM_MIPI_BOEOT_TFT_VIDEO_WSVGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WXGA_PT_PANEL)
	1,/* Backlight PWM-ID=1 for PMIC-GPIO#25 */
	0x0000	/* GPIOs not connected in espresso*/
#else
	0,/* Backlight PWM-ID=0 for PMIC-GPIO#24 */
	0x1F08 /* DSI2LVDS Bridge GPIO Output, mask=0x1f, out=0x08 */

#endif
	};

static struct msm_panel_common_pdata mipi_dsi2lvds_pdata = {
	.gpio_num = dsi2lvds_gpio,
	.cont_splash_enabled = 0x0,
};

static struct mipi_dsi_phy_ctrl dsi_novatek_cmd_mode_phy_db = {

/* DSI_BIT_CLK at 500MHz, 2 lane, RGB888 */
	{0x0F, 0x0a, 0x04, 0x00, 0x20},	/* regulator */
	/* timing   */
	{0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
	0x0c, 0x03, 0x04, 0xa0},
	{0x5f, 0x00, 0x00, 0x10},	/* phy ctrl */
	{0xff, 0x00, 0x06, 0x00},	/* strength */
	/* pll control */
	{0x40, 0xf9, 0x30, 0xda, 0x00, 0x40, 0x03, 0x62,
	0x40, 0x07, 0x03,
	0x00, 0x1a, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01},
};

static struct mipi_dsi_panel_platform_data novatek_pdata = {
	.fpga_3d_config_addr  = FPGA_3D_GPIO_CONFIG_ADDR,
	.fpga_ctrl_mode = FPGA_SPI_INTF,
	.phy_ctrl_settings = &dsi_novatek_cmd_mode_phy_db,
};

static struct platform_device mipi_dsi_novatek_panel_device = {
	.name = "mipi_novatek",
	.id = 0,
	.dev = {
		.platform_data = &novatek_pdata,
	}
};

static struct platform_device mipi_dsi2lvds_bridge_device = {
	.name = "mipi_tc358764",
	.id = 0,
	.dev.platform_data = &mipi_dsi2lvds_pdata,
};
static struct platform_device mipi_dsi_samsung_oled_panel_device = {
	.name = "mipi_samsung_oled",
	.id = 0,
	.dev.platform_data = &mipi_dsi_pdata,
};

static struct platform_device mipi_dsi_orise_panel_device = {
	.name = "mipi_orise",
	.id = 0,
};

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct resource hdmi_msm_resources[] = {
	{
		.name  = "hdmi_msm_qfprom_addr",
		.start = 0x00700000,
		.end   = 0x007060FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_hdmi_addr",
		.start = 0x04A00000,
		.end   = 0x04A00FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_irq",
		.start = HDMI_IRQ,
		.end   = HDMI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_cec_power(int on);
static int hdmi_gpio_config(int on);
static int hdmi_panel_power(int on);

static struct msm_hdmi_platform_data hdmi_msm_data = {
	.irq = HDMI_IRQ,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_cec_power,
	.panel_power = hdmi_panel_power,
	.gpio_config = hdmi_gpio_config,
};

static struct platform_device hdmi_msm_device = {
	.name = "hdmi_msm",
	.id = 0,
	.num_resources = ARRAY_SIZE(hdmi_msm_resources),
	.resource = hdmi_msm_resources,
	.dev.platform_data = &hdmi_msm_data,
};
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
static struct platform_device wfd_panel_device = {
	.name = "wfd_panel",
	.id = 0,
	.dev.platform_data = NULL,
};

static struct platform_device wfd_device = {
	.name          = "msm_wfd",
	.id            = -1,
};
#endif

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors dtv_bus_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 566092800 * 2,
		.ib = 707616000 * 2,
	},
};

static struct msm_bus_paths dtv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(dtv_bus_init_vectors),
		dtv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(dtv_bus_def_vectors),
		dtv_bus_def_vectors,
	},
};
static struct msm_bus_scale_pdata dtv_bus_scale_pdata = {
	dtv_bus_scale_usecases,
	ARRAY_SIZE(dtv_bus_scale_usecases),
	.name = "dtv",
};
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct lcdc_platform_data dtv_pdata = {
	.bus_scale_table = &dtv_bus_scale_pdata,
	.lcdc_power_save = hdmi_panel_power,
};

static int hdmi_panel_power(int on)
{
	int rc;

	pr_debug("%s: HDMI Core: %s\n", __func__, (on ? "ON" : "OFF"));
	rc = hdmi_core_power(on, 1);
	if (rc)
		rc = hdmi_cec_power(on);

	pr_debug("%s: HDMI Core: %s Success\n", __func__, (on ? "ON" : "OFF"));
	return rc;
}
#endif
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static int hdmi_enable_5v(int on)
{
	/* TBD: PM8921 regulator instead of 8901 */
	static struct regulator *reg_8921_hdmi_mvs;	/* HDMI_5V */
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!reg_8921_hdmi_mvs) {
		reg_8921_hdmi_mvs = regulator_get(&hdmi_msm_device.dev,
			"hdmi_mvs");
		if (IS_ERR(reg_8921_hdmi_mvs)) {
			pr_err("'%s' regulator not found, rc=%ld\n",
				"hdmi_mvs", IS_ERR(reg_8921_hdmi_mvs));
			reg_8921_hdmi_mvs = NULL;
			return -ENODEV;
		}
	}

	if (on) {
		rc = regulator_enable(reg_8921_hdmi_mvs);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
			return rc;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_hdmi_mvs);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
}

static int hdmi_core_power(int on, int show)
{
	static struct regulator *reg_8921_l23, *reg_8921_s4;
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	/* TBD: PM8921 regulator instead of 8901 */
	if (!reg_8921_l23) {
		reg_8921_l23 = regulator_get(&hdmi_msm_device.dev, "hdmi_avdd");
		if (IS_ERR(reg_8921_l23)) {
			pr_err("could not get reg_8921_l23, rc = %ld\n",
				PTR_ERR(reg_8921_l23));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_l23, rc=%d\n", rc);
			return -EINVAL;
		}
	}
	if (!reg_8921_s4) {
		reg_8921_s4 = regulator_get(&hdmi_msm_device.dev, "hdmi_vcc");
		if (IS_ERR(reg_8921_s4)) {
			pr_err("could not get reg_8921_s4, rc = %ld\n",
				PTR_ERR(reg_8921_s4));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_s4, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_s4, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_8921_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_8921_l23);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_avdd", rc);
			return rc;
		}
		rc = regulator_enable(reg_8921_s4);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_vcc", rc);
			return rc;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_l23);
		if (rc) {
			pr_err("disable reg_8921_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_8921_s4);
		if (rc) {
			pr_err("disable reg_8921_s4 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_8921_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
}

static int hdmi_gpio_config(int on)
{
	int rc = 0;
	static int prev_on;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(100, "HDMI_DDC_CLK");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_CLK", 100, rc);
			return rc;
		}
		rc = gpio_request(101, "HDMI_DDC_DATA");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_DATA", 101, rc);
			goto error1;
		}
		rc = gpio_request(102, "HDMI_HPD");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_HPD", 102, rc);
			goto error2;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(100);
		gpio_free(101);
		gpio_free(102);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;
	return 0;

error2:
	gpio_free(101);
error1:
	gpio_free(100);
	return rc;
}

static int hdmi_cec_power(int on)
{
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(99, "HDMI_CEC_VAR");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_CEC_VAR", 99, rc);
			goto error;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(99);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
error:
	return rc;
}
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */



void __init msm8960_init_fb(void)
{
	uint32_t soc_platform_version = socinfo_get_version();

#ifdef CONFIG_SAMSUNG_CMC624
	if (samsung_has_cmc624()) {
		msm_gpiomux_install(msm8x60_cmc624_configs,
		ARRAY_SIZE(msm8x60_cmc624_configs));
	}
#endif

	if (SOCINFO_VERSION_MAJOR(soc_platform_version) >= 3)
		mdp_pdata.mdp_rev = MDP_REV_43;

	platform_device_register(&msm_fb_device);

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
	platform_device_register(&wfd_panel_device);
	platform_device_register(&wfd_device);
#endif

	if (machine_is_msm8960_sim())
		platform_device_register(&mipi_dsi_simulator_panel_device);

	if (machine_is_msm8960_rumi3())
		platform_device_register(&mipi_dsi_renesas_panel_device);

	if (!machine_is_msm8960_sim() && !machine_is_msm8960_rumi3()) {
		platform_device_register(&mipi_dsi_novatek_panel_device);
		platform_device_register(&mipi_dsi_orise_panel_device);

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
		platform_device_register(&hdmi_msm_device);
#endif
	}

	if (machine_is_msm8960_liquid())
		platform_device_register(&mipi_dsi2lvds_bridge_device);
	else
		platform_device_register(&mipi_dsi_samsung_oled_panel_device);

#ifdef CONFIG_SAMSUNG_CMC624
	platform_device_register(&cmc624_i2c_gpio_device);
#endif
#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
#if defined(CONFIG_SAMSUNG_CMC624)
	set_esd_gpio_config();
	esd_pdata.esd_gpio_irq =
		PM8921_GPIO_IRQ(PM8921_IRQ_BASE, PMIC_GPIO_VGH_ESD_DET),
	esd_pdata.esd_gpio_cmc_irq	=
		PM8921_GPIO_IRQ(PM8921_IRQ_BASE, PMIC_GPIO_CMC_ESD_DET),
	platform_device_register(&samsung_mipi_esd_refresh_device);
#elif defined(CONFIG_MACH_JAGUAR) || \
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL)
	set_esd_gpio_config();
	esd_pdata.esd_gpio_irq =
	PM8921_GPIO_IRQ(PM8921_IRQ_BASE, PMIC_GPIO_VGH_ESD_DET),
	platform_device_register(&samsung_mipi_esd_refresh_device);
#else
	if (gpio_rev(ESD_DET) > 0) {
		set_esd_gpio_config();
		platform_device_register(&samsung_mipi_esd_refresh_device);
	}
#endif
#endif
	if (machine_is_msm8x60_rumi3()) {
		msm_fb_register_device("mdp", NULL);
		mipi_dsi_pdata.target_type = 1;
	} else
		msm_fb_register_device("mdp", &mdp_pdata);

	/*LCD MDP REV*/
	mipi_dsi_pdata.vsync_gpio = gpio_rev(MDP_VSYNC);
	mdp_pdata.gpio = gpio_rev(MDP_VSYNC);

	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#ifdef CONFIG_MSM_BUS_SCALING
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	msm_fb_register_device("dtv", &dtv_pdata);
#endif
#endif
}

void __init msm8960_allocate_fb_region(void)
{
	void *addr;
	unsigned long size;

	size = MSM_FB_SIZE;
	addr = alloc_bootmem_align(size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
			size, addr, __pa(addr));
}

/**
 * Set MDP clocks to high frequency to avoid DSI underflow
 * when using high resolution 1200x1920 WUXGA panels
 */
static void set_mdp_clocks_for_wuxga(void)
{
	mdp_ui_vectors[0].ab = 2000000000;
	mdp_ui_vectors[0].ib = 2000000000;
	mdp_vga_vectors[0].ab = 2000000000;
	mdp_vga_vectors[0].ib = 2000000000;
	mdp_720p_vectors[0].ab = 2000000000;
	mdp_720p_vectors[0].ib = 2000000000;
	mdp_1080p_vectors[0].ab = 2000000000;
	mdp_1080p_vectors[0].ib = 2000000000;

	if (hdmi_is_primary) {
		dtv_bus_def_vectors[0].ab = 2000000000;
		dtv_bus_def_vectors[0].ib = 2000000000;
	}
}

void __init msm8960_set_display_params(char *prim_panel, char *ext_panel)
{
	int disable_splash = 0;
	if (strnlen(prim_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.prim_panel_name, prim_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.prim_panel_name %s\n",
			msm_fb_pdata.prim_panel_name);

		if (strncmp((char *)msm_fb_pdata.prim_panel_name,
			MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
			strnlen(MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
			/* Disable splash for panels other than Toshiba WSVGA */
			disable_splash = 1;
		}

		if (!strncmp((char *)msm_fb_pdata.prim_panel_name,
			HDMI_PANEL_NAME, strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
			pr_debug("HDMI is the primary display by"
				" boot parameter\n");
			hdmi_is_primary = 1;
			set_mdp_clocks_for_wuxga();
		}
		if (!strncmp((char *)msm_fb_pdata.prim_panel_name,
				MIPI_VIDEO_TOSHIBA_WUXGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_TOSHIBA_WUXGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN))) {
			set_mdp_clocks_for_wuxga();
		}
	}
	if (strnlen(ext_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.ext_panel_name, ext_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.ext_panel_name %s\n",
			msm_fb_pdata.ext_panel_name);
	}

	if (disable_splash)
		mdp_pdata.cont_splash_enabled = 0;
}
