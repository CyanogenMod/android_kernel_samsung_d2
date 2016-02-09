/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ARCH_ARM_MACH_MSM_BOARD_MSM8960_H
#define __ARCH_ARM_MACH_MSM_BOARD_MSM8960_H

#include <linux/regulator/msm-gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <mach/irqs.h>
#include <mach/rpm-regulator.h>
#include <mach/msm_memtypes.h>
#include <mach/msm_rtb.h>
#include <mach/msm_cache_dump.h>

/* Macros assume PMIC GPIOs and MPPs start at 1 */
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define PM8921_MPP_BASE			(PM8921_GPIO_BASE + PM8921_NR_GPIOS)
#define PM8921_MPP_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_MPP_BASE)
#define PM8921_IRQ_BASE			(NR_MSM_IRQS + NR_GPIO_IRQS)

extern struct pm8xxx_regulator_platform_data
	msm_pm8921_regulator_pdata[] __devinitdata;

extern int msm_pm8921_regulator_pdata_len __devinitdata;

#define GPIO_VREG_ID_EXT_5V		0
#define GPIO_VREG_ID_EXT_L2		1
#define GPIO_VREG_ID_EXT_3P3V		2
#define GPIO_VREG_ID_EXT_OTG_SW		3

extern struct gpio_regulator_platform_data
	msm_gpio_regulator_pdata[] __devinitdata;

extern struct regulator_init_data msm_saw_regulator_pdata_s5;
extern struct regulator_init_data msm_saw_regulator_pdata_s6;

extern struct rpm_regulator_platform_data msm_rpm_regulator_pdata __devinitdata;

/* GPIO SX150X */
#if defined(CONFIG_TOUCHSCREEN_MMS144) || \
	defined(CONFIG_TOUCHSCREEN_MMS136) || \
	defined(CONFIG_TOUCHSCREEN_MMS136_TABLET)
extern void __init mms_tsp_input_init(void);
#endif

#define _GPAIR(i, t, v) (i == t) ? v :
#define gpio_rev(i) ( \
	_GPAIR(i,0,0)	_GPAIR(i,1,50)	_GPAIR(i,2,81)	_GPAIR(i,3,19)  \
	_GPAIR(i,4,95)	_GPAIR(i,5,9)	_GPAIR(i,6,6)	_GPAIR(i,7,12)  \
	_GPAIR(i,8,13)	_GPAIR(i,9,4)	_GPAIR(i,10,5)	_GPAIR(i,11,77) \
	_GPAIR(i,12,80)	_GPAIR(i,13,35)	_GPAIR(i,14,36)	_GPAIR(i,15,37) \
	_GPAIR(i,16,10)	_GPAIR(i,17,79)	_GPAIR(i,18,82)	(~0) )

#if defined(CONFIG_MACH_ESPRESSO_VZW)
extern void __init usb_switch_init(void);
#endif

#define PLATFORM_IS_CHARM25() \
	(machine_is_msm8960_cdp() && \
		(socinfo_get_platform_subtype() == 1) \
	)

#ifdef CONFIG_SAMSUNG_CMC624
extern int samsung_cmc624_on(int enable);
extern int samsung_has_cmc624(void);
#endif

enum {
	SX150X_CAM,
	SX150X_LIQUID,
};

#ifdef CONFIG_CAMERON_HEALTH
extern bool is_cameron_health_connected;
extern void msm_otg_set_cameronhealth_state(bool enable);
#endif
extern struct sx150x_platform_data msm8960_sx150x_data[];
extern struct msm_camera_board_info msm8960_camera_board_info;

int msm8960_get_cable_type(void);
void __init msm8960_init_cam(void);
void __init msm8960_init_fb(void);
void __init msm8960_init_pmic(void);
void __init msm8960_init_mmc(void);
int __init msm8960_init_gpiomux(void);
void __init msm8960_allocate_fb_region(void);
void __init msm8960_set_display_params(char *prim_panel, char *ext_panel);
void __init msm8960_pm8921_gpio_mpp_init(void);
void __init msm8960_mdp_writeback(struct memtype_reserve *reserve_table);
extern int poweroff_charging;
#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4
#define MSM_8960_GSBI3_QUP_I2C_BUS_ID 3
#define MSM_8960_GSBI8_QUP_I2C_BUS_ID 8
#define MSM_8960_GSBI10_QUP_I2C_BUS_ID 10

extern struct msm_rtb_platform_data msm8960_rtb_pdata;
extern struct msm_cache_dump_platform_data msm8960_cache_dump_pdata;
extern void msm8960_add_vidc_device(void);
extern void msm_otg_set_cable_state(int);
extern void msm_otg_set_vbus_state(int);
extern void msm_otg_set_charging_state(bool enable);
extern void msm_otg_set_id_state(bool enable);
extern void msm_otg_set_smartdock_state(bool enable);


#if defined(CONFIG_BCM4334) || defined(CONFIG_BCM4334_MODULE)
int __init brcm_wlan_init(void);
int brcm_wifi_status_register(
        void (*callback)(int card_present, void *dev_id), void *dev_id);
#endif
#endif
