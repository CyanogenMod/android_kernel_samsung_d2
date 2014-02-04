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
#include <linux/kernel.h>
#include <linux/msm_thermal.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/i2c/sx150x.h>
#include <linux/i2c/isl9519.h>
#include <linux/gpio.h>
#include <linux/msm_ssbi.h>
#include <linux/regulator/gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/slimbus/slimbus.h>
#include <linux/bootmem.h>
#include <linux/msm_kgsl.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/platform_data/qcom_crypto_device.h>
#include <linux/platform_data/qcom_wcnss_device.h>
#include <linux/platform_data/mms_ts.h>
#include <linux/leds.h>
#include <linux/leds-pm8xxx.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/msm_tsens.h>
#include <linux/ks8851.h>
#include <linux/i2c/isa1200.h>
#include <linux/memory.h>
#include <linux/memblock.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <asm/hardware/gic.h>
#include <asm/mach/mmc.h>

#include <mach/msm_dcvs.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_spi.h>
#ifdef CONFIG_USB_MSM_OTG_72K
#include <mach/msm_hsusb.h>
#else
#include <linux/usb/msm_hsusb.h>
#endif
#include <linux/usb/android.h>
#include <mach/usbdiag.h>
#include <mach/socinfo.h>
#include <mach/rpm.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/dma.h>
#include <mach/msm_dsps.h>
#include <mach/msm_xo.h>
#include <mach/restart.h>
#include <mach/msm8960-gpio.h>
#include <mach/kgsl.h>
#ifdef CONFIG_SENSORS_AK8975
#include <linux/i2c/ak8975.h>
#endif
#ifdef CONFIG_MPU_SENSORS_MPU6050B1
#include <linux/mpu6x.h>
#endif
#ifdef CONFIG_MPU_SENSORS_MPU6050B1_411
#include <linux/mpu_411.h>
#endif

#ifdef CONFIG_OPTICAL_GP2A
#include <linux/i2c/gp2a.h>
#endif
#ifdef CONFIG_OPTICAL_GP2AP020A00F
#include <linux/i2c/gp2ap020.h>
#endif
#ifdef CONFIG_VP_A2220
#include <sound/a2220.h>
#endif
#ifdef CONFIG_INPUT_BMP180
#include <linux/input/bmp180.h>
#endif
#ifdef CONFIG_WCD9310_CODEC
#include <linux/slimbus/slimbus.h>
#include <linux/mfd/wcd9xxx/core.h>
#include <linux/mfd/wcd9xxx/pdata.h>
#endif
#ifdef CONFIG_KEYBOARD_GPIO
#include <linux/gpio_keys.h>
#endif
#ifdef CONFIG_USB_SWITCH_FSA9485
#include <linux/i2c/fsa9485.h>
#include <linux/switch.h>
#endif

#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
#include <linux/sii9234.h>
#endif

#include <linux/power_supply.h>
#ifdef CONFIG_PM8921_CHARGER
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#endif
#ifdef CONFIG_PM8921_SEC_CHARGER
#include <linux/mfd/pm8xxx/pm8921-sec-charger.h>
#endif
#ifdef CONFIG_BATTERY_SEC
#include <linux/sec_battery.h>
#endif
#ifdef CONFIG_CHARGER_SMB347
#include <linux/smb347_charger.h>
#endif
#ifdef CONFIG_BATTERY_MAX17040
#include <linux/max17040_battery.h>
#endif
#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH_236
#include <linux/i2c/cypress_touchkey.h>
#endif
#include <linux/msm_ion.h>
#include <mach/ion.h>
#include <mach/mdm2.h>
#include <mach/mdm-peripheral.h>
#include <mach/msm_rtb.h>
#include <mach/msm_cache_dump.h>
#include <mach/scm.h>
#include <mach/iommu_domains.h>

#include <linux/fmem.h>

#include "timer.h"
#include "devices.h"
#include "devices-msm8x60.h"
#include "spm.h"
#include "board-8960.h"
#include "pm.h"
#include <mach/cpuidle.h>
#include "rpm_resources.h"
#include <mach/mpm.h>
#include "acpuclock.h"
#include "rpm_log.h"
#include "smd_private.h"
#ifdef CONFIG_NFC_PN544
#include <linux/pn544.h>
#endif /* CONFIG_NFC_PN544	*/
#include "mach/board-msm8960-camera.h"
#include "pm-boot.h"
#include "msm_watchdog.h"
#ifdef CONFIG_SENSORS_CM36651
#include <linux/i2c/cm36651.h>
#endif
#ifdef CONFIG_REGULATOR_MAX8952
#include <linux/regulator/max8952.h>
#include <linux/regulator/machine.h>
#endif
#ifdef CONFIG_VIBETONZ
#include <linux/vibrator.h>
#endif

#ifdef CONFIG_SAMSUNG_JACK
#include <linux/sec_jack.h>
#endif

#ifdef CONFIG_KEXEC_HARDBOOT
#include <asm/kexec.h>
#endif

extern unsigned int system_rev;
#ifdef CONFIG_TOUCHSCREEN_MMS144
struct tsp_callbacks *charger_callbacks;
#endif
extern int force_fast_charge;

static struct platform_device msm_fm_platform_init = {
	.name = "iris_fm",
	.id   = -1,
};

#ifdef CONFIG_SEC_DEBUG
#include <mach/sec_debug.h>
#endif
unsigned int gpio_table[][GPIO_REV_MAX] = {
/* GPIO_INDEX	Rev	{#00,#01,#02,#03,#04,#05,#06,#07,#08, ..., #12 }, */
/* MDP_VSYNC	*/	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* VOLUME_UP	*/	{ 80, 80, 80, 80, 80, 80, 50, 50, 50, 50, 50, 50, 50 },
/* VOLUME_DOWN	*/	{ 81, 81, 81, 81, 81, 81, 81, 81, 81, 81, 81, 81, 81 },
/* GPIO_MAG_RST */	{ 66, 66, 66, 66, 48, 48, 48, 48,  9,  9,  9, 48,  9 },
/* ALS_INT */		{ 42, 42, 42, 42, 42, 42, 42, 42,  6,  6,  6, 42,  6 },
#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F)  \
	|| defined(CONFIG_SENSORS_CM36651)
/* ALS_SDA */		{ 63, 63, 63, 63, 12, 12, 12, 12, 12, 12, 12, 12, 12 },
/* ALS_SCL */		{ 64, 64, 64, 64, 13, 13, 13, 13, 13, 13, 13, 13, 13 },
#endif
/* LCD_22V_EN */	{ 10, 10, 10, 10,  4,  4,  4,  4,  4,  4,  4,  4,  4 },
/* LINEOUT_EN */	{ -1, -1, -1, 17,  5,  5,  5,  5,  5,  5,  5,  5,  5 },
/* A2220_WAKEUP */	{ 79, 79, 79, 79, 35, 35, 35, 35, 35, 35, 35, 35, 35 },
/* A2220_SDA */		{ 12, 12, 12, 12, 36, 36, 36, 36, 36, 36, 36, 36, 36 },
/* A2220_SCL */		{ 13, 13, 13, 13, 37, 37, 37, 37, 37, 37, 37, 37, 37 },
/* CAM_AF_EN */		{ 54, 54, 54, 54, 77, 77, 77, 77, 77, 77, 77, 77, 77 },
/* CAM_FLASH_SW */	{ 49, 49, 49, 49, 49, 49, 49, 49, 80, 80, 80, 80, 80 },
/* BT_HOST_WAKE */	{ -1, -1, -1, -1, 10, 10, 10, 10, 10, 10, 10, 10, 10 },
/* BT_WAKE */		{ -1, -1, -1, -1, 79, 79, 79, 79, 79, 79, 79, 79, 79 },
/* BT_EN */		{ -1, -1, -1, -1, 82, 82, 82, 82, 82, 82, 82, 82, 82 },
};

int gpio_rev(unsigned int index)
{
	if (system_rev >= GPIO_REV_MAX)
		return -EINVAL;

	if (system_rev < BOARD_REV12)
		return gpio_table[index][system_rev];
	else
		return gpio_table[index][BOARD_REV12];
}

#if defined(CONFIG_OPTICAL_GP2AP020A00F)
static void gp2a_power_on(int onoff);
#endif

#if defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1_411)
static void mpu_power_on(int onoff);
#endif

#ifdef CONFIG_SENSORS_AK8975
static void akm_power_on(int onoff);
#endif

#ifdef CONFIG_INPUT_BMP180
static void bmp180_power_on(int onoff);
#endif

#ifdef CONFIG_SENSORS_CM36651
static void cm36651_power_on(int onoff);
#endif

#if defined(CONFIG_SENSORS_AK8975) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_INPUT_BMP180) || defined(CONFIG_OPTICAL_GP2A) || \
	defined(CONFIG_OPTICAL_GP2AP020A00F) || \
	defined(CONFIG_SENSORS_CM36651)
enum {
	SNS_PWR_OFF,
	SNS_PWR_ON,
	SNS_PWR_KEEP
};
#endif


#if defined(CONFIG_SENSORS_AK8975) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_INPUT_BMP180) || defined(CONFIG_OPTICAL_GP2A)
static void sensor_power_on_vdd(int, int);

#endif

#ifndef CONFIG_S5C73M3
#define KS8851_RST_GPIO		89
#define KS8851_IRQ_GPIO		90
#endif


#define HAP_SHIFT_LVL_OE_GPIO	47

#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)

struct sx150x_platform_data msm8960_sx150x_data[] = {
	[SX150X_CAM] = {
		.gpio_base         = GPIO_CAM_EXPANDER_BASE,
		.oscio_is_gpo      = false,
		.io_pullup_ena     = 0x0,
		.io_pulldn_ena     = 0xc0,
		.io_open_drain_ena = 0x0,
		.irq_summary       = -1,
	},
};

#endif

#ifdef CONFIG_I2C

#define MSM_8960_GSBI3_QUP_I2C_BUS_ID 3
#define MSM_8960_GSBI10_QUP_I2C_BUS_ID 10

#endif

#if 0
static struct gpiomux_setting sec_ts_ldo_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sec_ts_ldo_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm8960_sec_ts_configs[] = {
	{	/* TS LDO EN */
		.gpio = 10,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sec_ts_ldo_act_cfg,
			[GPIOMUX_SUSPENDED] = &sec_ts_ldo_sus_cfg,
		},
	},
};
#endif
#define MSM_PMEM_ADSP_SIZE         0x9600000 /* 150 Mbytes */
#define MSM_PMEM_AUDIO_SIZE        0x4CF000 /* 1.375 Mbytes */
#define MSM_PMEM_SIZE 0x2800000 /* 40 Mbytes */
#define MSM_LIQUID_PMEM_SIZE 0x4000000 /* 64 Mbytes */
#define MSM_HDMI_PRIM_PMEM_SIZE 0x4000000 /* 64 Mbytes */

#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
#define HOLE_SIZE	0x20000
#define MSM_CONTIG_MEM_SIZE  0x65000
#ifdef CONFIG_MSM_IOMMU
#define MSM_ION_MM_SIZE            0x6000000
#define MSM_ION_SF_SIZE            0x0
#define MSM_ION_QSECOM_SIZE        0x780000 /* (7.5MB) */
#ifdef CONFIG_CMA
#define MSM_ION_HEAP_NUM	8
#else
#define MSM_ION_HEAP_NUM	7
#endif
#else
#define MSM_ION_MM_SIZE		MSM_PMEM_ADSP_SIZE
#define MSM_ION_SF_SIZE		0x5000000 /* 80MB */
#define MSM_ION_QSECOM_SIZE	0x1700000 /* (24MB) */
#ifdef CONFIG_CMA
#define MSM_ION_HEAP_NUM	9
#else
#define MSM_ION_HEAP_NUM	8
#endif
#endif
#define MSM_ION_MM_FW_SIZE	(0x200000 - HOLE_SIZE) /* 128kb */
#define MSM_ION_MFC_SIZE	SZ_8K
#define MSM_ION_AUDIO_SIZE	MSM_PMEM_AUDIO_SIZE

#define MSM_LIQUID_ION_MM_SIZE (MSM_ION_MM_SIZE + 0x600000)
#define MSM_LIQUID_ION_SF_SIZE MSM_LIQUID_PMEM_SIZE
#define MSM_HDMI_PRIM_ION_SF_SIZE MSM_HDMI_PRIM_PMEM_SIZE

#define MSM_MM_FW_SIZE      (0x200000 - HOLE_SIZE) /* 2mb -128kb*/
#define MSM8960_FIXED_AREA_START (0xa0000000 - (MSM_ION_MM_FW_SIZE + \
                            HOLE_SIZE))
#define MAX_FIXED_AREA_SIZE 0x10000000
#define MSM8960_FW_START    MSM8960_FIXED_AREA_START
#define MSM_ION_ADSP_SIZE   SZ_8M

static unsigned msm_ion_sf_size = MSM_ION_SF_SIZE;
#else
#define MSM_CONTIG_MEM_SIZE  0x110C000
#define MSM_ION_HEAP_NUM    1
#endif

#ifdef CONFIG_KERNEL_MSM_CONTIG_MEM_REGION
static unsigned msm_contig_mem_size = MSM_CONTIG_MEM_SIZE;
static int __init msm_contig_mem_size_setup(char *p)
{
	msm_contig_mem_size = memparse(p, NULL);
	return 0;
}
early_param("msm_contig_mem_size", msm_contig_mem_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
static unsigned pmem_size = MSM_PMEM_SIZE;
static unsigned pmem_param_set;
static int __init pmem_size_setup(char *p)
{
	pmem_size = memparse(p, NULL);
	pmem_param_set = 1;
	return 0;
}
early_param("pmem_size", pmem_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;

static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_adsp_size", pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;

static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = {.platform_data = &android_pmem_pdata},
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};
static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};
#endif
#endif

struct fmem_platform_data msm8960_fmem_pdata = {
};

#define DSP_RAM_BASE_8960 0x8da00000
#define DSP_RAM_SIZE_8960 0x1800000
static int dspcrashd_pdata_8960 = 0xDEADDEAD;

static struct resource resources_dspcrashd_8960[] = {
	{
		.name   = "msm_dspcrashd",
		.start  = DSP_RAM_BASE_8960,
		.end    = DSP_RAM_BASE_8960 + DSP_RAM_SIZE_8960,
		.flags  = IORESOURCE_DMA,
	},
};

static struct platform_device msm_device_dspcrashd_8960 = {
	.name           = "msm_dspcrashd",
	.num_resources  = ARRAY_SIZE(resources_dspcrashd_8960),
	.resource       = resources_dspcrashd_8960,
	.dev = { .platform_data = &dspcrashd_pdata_8960 },
};

static struct memtype_reserve msm8960_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

#if defined(CONFIG_MSM_RTB)
static struct msm_rtb_platform_data msm_rtb_pdata = {
	.size = SZ_1M,
};

static int __init msm_rtb_set_buffer_size(char *p)
{
	int s;

	s = memparse(p, NULL);
	msm_rtb_pdata.size = ALIGN(s, SZ_4K);
	return 0;
}
early_param("msm_rtb_size", msm_rtb_set_buffer_size);

static struct platform_device msm_rtb_device = {
	.name           = "msm_rtb",
	.id             = -1,
	.dev            = {
		.platform_data = &msm_rtb_pdata,
	},
};
#endif

static void __init reserve_rtb_memory(void)
{
#if defined(CONFIG_MSM_RTB)
	msm8960_reserve_table[MEMTYPE_EBI1].size += msm_rtb_pdata.size;
#endif
}

static int msm8960_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

#define FMEM_ENABLED 0

#ifdef CONFIG_ION_MSM
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct ion_cp_heap_pdata cp_mm_msm8960_ion_pdata = {
	.permission_type = IPT_TYPE_MM_CARVEOUT,
	.align = SZ_64K,
	.reusable = FMEM_ENABLED,
	.mem_is_fmem = FMEM_ENABLED,
	.fixed_position = FIXED_MIDDLE,
	.iommu_map_all = 1,
	.iommu_2x_map_domain = VIDEO_DOMAIN,
#ifdef CONFIG_CMA
	.is_cma = 1,
#endif
};

static struct ion_cp_heap_pdata cp_mfc_msm8960_ion_pdata = {
	.permission_type = IPT_TYPE_MFC_SHAREDMEM,
	.align = PAGE_SIZE,
	.reusable = 0,
	.mem_is_fmem = FMEM_ENABLED,
	.fixed_position = FIXED_HIGH,
};

static struct ion_co_heap_pdata co_msm8960_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
	.mem_is_fmem = 0,
};

static struct ion_co_heap_pdata fw_co_msm8960_ion_pdata = {
	.adjacent_mem_id = ION_CP_MM_HEAP_ID,
	.align = SZ_128K,
	.mem_is_fmem = FMEM_ENABLED,
	.fixed_position = FIXED_LOW,
};
#endif

static u64 msm_dmamask = DMA_BIT_MASK(32);

static struct platform_device ion_mm_heap_device = {
	.name = "ion-mm-heap-device",
	.id = -1,
	.dev = {
		.dma_mask = &msm_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	}
};

#ifdef CONFIG_CMA
static struct platform_device ion_adsp_heap_device = {
	.name = "ion-adsp-heap-device",
	.id = -1,
	.dev = {
		.dma_mask = &msm_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	}
};
#endif

/**
 * These heaps are listed in the order they will be allocated. Due to
 * video hardware restrictions and content protection the FW heap has to
 * be allocated adjacent (below) the MM heap and the MFC heap has to be
 * allocated after the MM heap to ensure MFC heap is not more than 256MB
 * away from the base address of the FW heap.
 * However, the order of FW heap and MM heap doesn't matter since these
 * two heaps are taken care of by separate code to ensure they are adjacent
 * to each other.
 * Don't swap the order unless you know what you are doing!
 */
struct ion_platform_heap msm8960_heaps[] = {
		{
			.id = ION_SYSTEM_HEAP_ID,
			.type   = ION_HEAP_TYPE_SYSTEM,
			.name   = ION_VMALLOC_HEAP_NAME,
		},
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
		{
			.id = ION_CP_MM_HEAP_ID,
			.type   = ION_HEAP_TYPE_CP,
			.name   = ION_MM_HEAP_NAME,
			.size   = MSM_ION_MM_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &cp_mm_msm8960_ion_pdata,
			.priv   = &ion_mm_heap_device.dev,
		},
		{
			.id = ION_MM_FIRMWARE_HEAP_ID,
			.type   = ION_HEAP_TYPE_CARVEOUT,
			.name   = ION_MM_FIRMWARE_HEAP_NAME,
			.size   = MSM_ION_MM_FW_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &fw_co_msm8960_ion_pdata,
		},
		{
			.id = ION_CP_MFC_HEAP_ID,
			.type   = ION_HEAP_TYPE_CP,
			.name   = ION_MFC_HEAP_NAME,
			.size   = MSM_ION_MFC_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &cp_mfc_msm8960_ion_pdata,
		},
#ifndef CONFIG_MSM_IOMMU
		{
			.id = ION_SF_HEAP_ID,
			.type   = ION_HEAP_TYPE_CARVEOUT,
			.name   = ION_SF_HEAP_NAME,
			.size   = MSM_ION_SF_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_msm8960_ion_pdata,
		},
#endif
		{
			.id = ION_IOMMU_HEAP_ID,
			.type   = ION_HEAP_TYPE_IOMMU,
			.name   = ION_IOMMU_HEAP_NAME,
		},
		{
			.id = ION_QSECOM_HEAP_ID,
			.type   = ION_HEAP_TYPE_CARVEOUT,
			.name   = ION_QSECOM_HEAP_NAME,
			.size   = MSM_ION_QSECOM_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_msm8960_ion_pdata,
		},
		{
			.id = ION_AUDIO_HEAP_ID,
			.type   = ION_HEAP_TYPE_CARVEOUT,
			.name   = ION_AUDIO_HEAP_NAME,
			.size   = MSM_ION_AUDIO_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_msm8960_ion_pdata,
		},
#ifdef CONFIG_CMA
		{
			.id     = ION_ADSP_HEAP_ID,
			.type   = ION_HEAP_TYPE_DMA,
			.name   = ION_ADSP_HEAP_NAME,
			.size   = MSM_ION_ADSP_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_msm8960_ion_pdata,
			.priv   = &ion_adsp_heap_device.dev,
		},
#endif
#endif
};

static struct ion_platform_data msm8960_ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.heaps = msm8960_heaps,
};

static struct platform_device msm8960_ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &msm8960_ion_pdata },
};
#endif

struct platform_device msm8960_fmem_device = {
	.name = "fmem",
	.id = 1,
	.dev = { .platform_data = &msm8960_fmem_pdata },
};

static void __init adjust_mem_for_liquid(void)
{
	unsigned int i;

		if (machine_is_msm8960_liquid())
			msm_ion_sf_size = MSM_LIQUID_ION_SF_SIZE;

		if (msm8960_hdmi_as_primary_selected())
			msm_ion_sf_size = MSM_HDMI_PRIM_ION_SF_SIZE;

		if (machine_is_msm8960_liquid() ||
			msm8960_hdmi_as_primary_selected()) {
			for (i = 0; i < msm8960_ion_pdata.nr; i++) {
				if (msm8960_ion_pdata.heaps[i].id ==
							ION_SF_HEAP_ID) {
					msm8960_ion_pdata.heaps[i].size =
						msm_ion_sf_size;
					pr_debug("msm_ion_sf_size 0x%x\n",
						msm_ion_sf_size);
					break;
				}
			}
		}
}

static void __init reserve_mem_for_ion(enum ion_memory_types mem_type,
				      unsigned long size)
{
	msm8960_reserve_table[mem_type].size += size;
}

static void __init msm8960_reserve_fixed_area(unsigned long fixed_area_size)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	int ret;

	if (fixed_area_size > MAX_FIXED_AREA_SIZE)
		panic("fixed area size is larger than %dM\n",
			MAX_FIXED_AREA_SIZE >> 20);

	reserve_info->fixed_area_size = fixed_area_size;
	reserve_info->fixed_area_start = MSM8960_FW_START;

	ret = memblock_remove(reserve_info->fixed_area_start,
		reserve_info->fixed_area_size);
	BUG_ON(ret);
#endif
}


/**
 * Reserve memory for ION and calculate amount of reusable memory for fmem.
 * We only reserve memory for heaps that are not reusable. However, we only
 * support one reusable heap at the moment so we ignore the reusable flag for
 * other than the first heap with reusable flag set. Also handle special case
 * for video heaps (MM,FW, and MFC). Video requires heaps MM and MFC to be
 * at a higher address than FW in addition to not more than 256MB away from the
 * base address of the firmware. This means that if MM is reusable the other
 * two heaps must be allocated in the same region as FW. This is handled by the
 * mem_is_fmem flag in the platform data. In addition the MM heap must be
 * adjacent to the FW heap for content protection purposes.
 */
static void __init reserve_ion_memory(void)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	unsigned int i;
	int ret;
	unsigned int fixed_size = 0;
	unsigned int fixed_low_size, fixed_middle_size, fixed_high_size;
	unsigned long fixed_low_start, fixed_middle_start, fixed_high_start;
	unsigned long cma_alignment;
	unsigned int low_use_cma = 0;
	unsigned int middle_use_cma = 0;
	unsigned int high_use_cma = 0;

	adjust_mem_for_liquid();
	fixed_low_size = 0;
	fixed_middle_size = 0;
	fixed_high_size = 0;

	cma_alignment = PAGE_SIZE << max(MAX_ORDER, pageblock_order);

	for (i = 0; i < msm8960_ion_pdata.nr; ++i) {
		struct ion_platform_heap *heap =
						&(msm8960_ion_pdata.heaps[i]);
		int align = SZ_4K;
		int iommu_map_all = 0;
		int adjacent_mem_id = INVALID_HEAP_ID;
		int use_cma = 0;

		if (heap->extra_data) {
			int fixed_position = NOT_FIXED;

			switch ((int)heap->type) {
			case ION_HEAP_TYPE_CP:
				fixed_position = ((struct ion_cp_heap_pdata *)
					heap->extra_data)->fixed_position;
				align = ((struct ion_cp_heap_pdata *)
						heap->extra_data)->align;
				iommu_map_all =
					((struct ion_cp_heap_pdata *)
					heap->extra_data)->iommu_map_all;
				if (((struct ion_cp_heap_pdata *)
					heap->extra_data)->is_cma) {
					heap->size = ALIGN(heap->size,
							cma_alignment);
					use_cma = 1;
				}
				break;
			case ION_HEAP_TYPE_DMA:
					use_cma = 1;
				/* Purposely fall through here */
			case ION_HEAP_TYPE_CARVEOUT:
				fixed_position = ((struct ion_co_heap_pdata *)
					heap->extra_data)->fixed_position;
				adjacent_mem_id = ((struct ion_co_heap_pdata *)
					heap->extra_data)->adjacent_mem_id;
				break;
			default:
				break;
			}

			if (iommu_map_all) {
				if (heap->size & (SZ_64K-1)) {
					heap->size = ALIGN(heap->size, SZ_64K);
					pr_info("Heap %s not aligned to 64K. Adjusting size to %x\n",
						heap->name, heap->size);
				}
			}

			if (fixed_position != NOT_FIXED)
				fixed_size += heap->size;
			else if (!use_cma)
				reserve_mem_for_ion(MEMTYPE_EBI1, heap->size);

			if (fixed_position == FIXED_LOW) {
				fixed_low_size += heap->size;
				low_use_cma = use_cma;
			} else if (fixed_position == FIXED_MIDDLE) {
				fixed_middle_size += heap->size;
				middle_use_cma = use_cma;
			} else if (fixed_position == FIXED_HIGH) {
				fixed_high_size += heap->size;
				high_use_cma = use_cma;
			} else if (use_cma) {
				/*
				 * Heaps that use CMA but are not part of the
				 * fixed set. Create wherever.
				 */
				dma_declare_contiguous(
					heap->priv,
					heap->size,
					0,
					0xa0000000);
			}
		}
	}

	if (!fixed_size)
		return;


	/*
	 * Given the setup for the fixed area, we can't round up all sizes.
	 * Some sizes must be set up exactly and aligned correctly. Incorrect
	 * alignments are considered a configuration issue
	 */

	fixed_low_start = MSM8960_FIXED_AREA_START;
	if (low_use_cma) {
		BUG_ON(!IS_ALIGNED(fixed_low_start, cma_alignment));
		BUG_ON(!IS_ALIGNED(fixed_low_size + HOLE_SIZE, cma_alignment));
	} else {
		BUG_ON(!IS_ALIGNED(fixed_low_size + HOLE_SIZE, SECTION_SIZE));
		ret = memblock_remove(fixed_low_start,
					  fixed_low_size + HOLE_SIZE);
		BUG_ON(ret);
	}

	fixed_middle_start = fixed_low_start + fixed_low_size + HOLE_SIZE;
	if (middle_use_cma) {
		BUG_ON(!IS_ALIGNED(fixed_middle_start, cma_alignment));
		BUG_ON(!IS_ALIGNED(fixed_middle_size, cma_alignment));
	} else {
		BUG_ON(!IS_ALIGNED(fixed_middle_size, SECTION_SIZE));
		ret = memblock_remove(fixed_middle_start, fixed_middle_size);
		BUG_ON(ret);
	}

	fixed_high_start = fixed_middle_start + fixed_middle_size;
	if (high_use_cma) {
		fixed_high_size = ALIGN(fixed_high_size, cma_alignment);
		BUG_ON(!IS_ALIGNED(fixed_high_start, cma_alignment));
	} else {
		/* This is the end of the fixed area so it's okay to round up */
		fixed_high_size = ALIGN(fixed_high_size, SECTION_SIZE);
		ret = memblock_remove(fixed_high_start, fixed_high_size);
		BUG_ON(ret);
	}

	for (i = 0; i < msm8960_ion_pdata.nr; ++i) {
		struct ion_platform_heap *heap = &(msm8960_ion_pdata.heaps[i]);

		if (heap->extra_data) {
			int fixed_position = NOT_FIXED;
			struct ion_cp_heap_pdata *pdata = NULL;

			switch ((int) heap->type) {
			case ION_HEAP_TYPE_CP:
				pdata =
				(struct ion_cp_heap_pdata *)heap->extra_data;
				fixed_position = pdata->fixed_position;
				break;
			case ION_HEAP_TYPE_CARVEOUT:
			case ION_HEAP_TYPE_DMA:
				fixed_position = ((struct ion_co_heap_pdata *)
					heap->extra_data)->fixed_position;
				break;
			default:
				break;
			}

			switch (fixed_position) {
			case FIXED_LOW:
				heap->base = fixed_low_start;
				break;
			case FIXED_MIDDLE:
				heap->base = fixed_middle_start;
				if (middle_use_cma) {
					ret = dma_declare_contiguous(
						&ion_mm_heap_device.dev,
						heap->size,
						fixed_middle_start,
						0xa0000000);
					WARN_ON(ret);
				}
				pdata->secure_base = fixed_middle_start
							- HOLE_SIZE;
				pdata->secure_size = HOLE_SIZE + heap->size;
				break;
			case FIXED_HIGH:
				heap->base = fixed_high_start;
				break;
			default:
				break;
			}
		}
	}
#endif
}

static void ion_adjust_secure_allocation(void)
{
	int i;

	for (i = 0; i < msm8960_ion_pdata.nr; i++) {
		struct ion_platform_heap *heap =
			&(msm8960_ion_pdata.heaps[i]);


		if (heap->extra_data) {
			switch ((int) heap->type) {
			case ION_HEAP_TYPE_CP:
				if (cpu_is_msm8960()) {
					((struct ion_cp_heap_pdata *)
					heap->extra_data)->allow_nonsecure_alloc =
						1;
				}

			}
		}
	}
}

static void __init reserve_mdp_memory(void)
{
	msm8960_mdp_writeback(msm8960_reserve_table);
}

#if defined(CONFIG_MSM_CACHE_DUMP)
static struct msm_cache_dump_platform_data msm_cache_dump_pdata = {
	.l2_size = L2_BUFFER_SIZE,
};

static struct platform_device msm_cache_dump_device = {
	.name           = "msm_cache_dump",
	.id             = -1,
	.dev            = {
		.platform_data = &msm_cache_dump_pdata,
	},
};

#endif

static void reserve_cache_dump_memory(void)
{
#ifdef CONFIG_MSM_CACHE_DUMP
	unsigned int spare;
	unsigned int l1_size;
	unsigned int total;
	int ret;

	ret = scm_call(L1C_SERVICE_ID, L1C_BUFFER_GET_SIZE_COMMAND_ID, &spare,
		sizeof(spare), &l1_size, sizeof(l1_size));

	if (ret)
		/* Fall back to something reasonable here */
		l1_size = L1_BUFFER_SIZE;

	total = l1_size + L2_BUFFER_SIZE;

	msm8960_reserve_table[MEMTYPE_EBI1].size += total;
	msm_cache_dump_pdata.l1_size = l1_size;
#endif
}

static void __init msm8960_calculate_reserve_sizes(void)
{
	reserve_ion_memory();
	reserve_mdp_memory();
	reserve_rtb_memory();
	reserve_cache_dump_memory();
	msm8960_reserve_table[MEMTYPE_EBI1].size += msm_contig_mem_size;
}

static struct reserve_info msm8960_reserve_info __initdata = {
	.memtype_reserve_table = msm8960_reserve_table,
	.calculate_reserve_sizes = msm8960_calculate_reserve_sizes,
	.reserve_fixed_area = msm8960_reserve_fixed_area,
	.paddr_to_memtype = msm8960_paddr_to_memtype,
};

static void __init msm8960_early_memory(void)
{
	reserve_info = &msm8960_reserve_info;
}

static char prim_panel_name[PANEL_NAME_MAX_LEN];
static char ext_panel_name[PANEL_NAME_MAX_LEN];
static int __init prim_display_setup(char *param)
{
	if (strnlen(param, PANEL_NAME_MAX_LEN))
		strlcpy(prim_panel_name, param, PANEL_NAME_MAX_LEN);
	return 0;
}
early_param("prim_display", prim_display_setup);

static int __init ext_display_setup(char *param)
{
	if (strnlen(param, PANEL_NAME_MAX_LEN))
		strlcpy(ext_panel_name, param, PANEL_NAME_MAX_LEN);
	return 0;
}
early_param("ext_display", ext_display_setup);

static void __init msm8960_reserve(void)
{
	msm8960_set_display_params(prim_panel_name, ext_panel_name);
	msm_reserve();
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	add_persistent_ram();
#endif

#ifdef CONFIG_KEXEC_HARDBOOT
	memblock_remove(KEXEC_HB_PAGE_ADDR, SZ_4K);
#endif
}

static void __init msm8960_allocate_memory_regions(void)
{
	msm8960_allocate_fb_region();
}
#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH_236
static void cypress_power_onoff(int onoff)
{
	int ret, rc;
	static struct regulator *reg_l29, *reg_l10;

	pr_debug("power on entry\n");

	if (!reg_l29) {
		reg_l29 = regulator_get(NULL, "8921_l29");
		ret = regulator_set_voltage(reg_l29, 1800000, 1800000);

		if (IS_ERR(reg_l29)) {
			pr_err("could not get 8921_l29, rc = %ld\n",
				PTR_ERR(reg_l29));
			return;
		}
	}

	if (!reg_l10) {
		reg_l10 = regulator_get(NULL, "8921_l10");
		ret = regulator_set_voltage(reg_l10, 3000000, 3000000);

		if (IS_ERR(reg_l10)) {
			pr_err("could not get 8921_l10, rc = %ld\n",
				PTR_ERR(reg_l10));
			return;
		}
	}

	if (onoff) {
		ret = regulator_enable(reg_l29);
		rc =  regulator_enable(reg_l10);
		if (ret) {
			pr_err("enable l29 failed, rc=%d\n", ret);
			return;
		}
		if (rc) {
			pr_err("enable l10 failed, rc=%d\n", ret);
			return;
		}
		pr_info("cypress_power_on is finished.\n");
	} else {
		ret = regulator_disable(reg_l29);
		rc =  regulator_disable(reg_l10);
		if (ret) {
			pr_err("disable l29 failed, rc=%d\n", ret);
			return;
		}
		if (rc) {
			pr_err("enable l29failed, rc=%d\n", ret);
			return;
		}
		pr_info("cypress_power_off is finished.\n");
	}
}

static u8 touchkey_keycode[] = {KEY_BACK, KEY_MENU};

static struct cypress_touchkey_platform_data cypress_touchkey_pdata = {
	.gpio_int = GPIO_TOUCH_KEY_INT,
	.touchkey_keycode = touchkey_keycode,
	.power_onoff = cypress_power_onoff,
};

static struct i2c_board_info touchkey_i2c_devices_info[] __initdata = {
	{
		I2C_BOARD_INFO("cypress_touchkey", 0x20),
		.platform_data = &cypress_touchkey_pdata,
		.irq = MSM_GPIO_TO_INT(GPIO_TOUCH_KEY_INT),
	},
};

static struct i2c_gpio_platform_data  cypress_touchkey_i2c_gpio_data = {
	.sda_pin		= GPIO_TOUCHKEY_SDA,
	.scl_pin		= GPIO_TOUCHKEY_SCL,
	.udelay			= 0,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device touchkey_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= MSM_TOUCHKEY_I2C_BUS_ID,
	.dev.platform_data	= &cypress_touchkey_i2c_gpio_data,
};

#endif

#ifdef CONFIG_USB_SWITCH_FSA9485
static enum cable_type_t set_cable_status;
#ifdef CONFIG_MHL_NEW_CBUS_MSC_CMD
static void fsa9485_mhl_cb(bool attached, int mhl_charge)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_mhl_cb attached (%d), mhl_charge(%d)\n",
			attached, mhl_charge);

	if (attached) {
		switch (mhl_charge) {
		case 0:
		case 1:
			set_cable_status = CABLE_TYPE_USB;
			break;
		case 2:
			set_cable_status = CABLE_TYPE_AC;
			break;
		}
	} else {
		set_cable_status = CABLE_TYPE_NONE;
	}

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_USB:
		value.intval = POWER_SUPPLY_TYPE_USB;
		break;
	case CABLE_TYPE_AC:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("%s: invalid cable :%d\n", __func__, set_cable_status);
		return;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
		&value);
	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}
#else
static void fsa9485_mhl_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_mhl_cb attached %d\n", attached);
	set_cable_status = attached ? CABLE_TYPE_MISC : CABLE_TYPE_NONE;

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_MISC:
		value.intval = POWER_SUPPLY_TYPE_MISC;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("%s: invalid cable :%d\n", __func__, set_cable_status);
		return;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
		&value);
	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}
#endif
static void fsa9485_otg_cb(bool attached)
{
	pr_info("fsa9485_otg_cb attached %d\n", attached);

	if (attached) {
		pr_info("%s set id state\n", __func__);
//		msm_otg_set_id_state(attached);
	}
}

static void fsa9485_usb_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_usb_cb attached %d\n", attached);
	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_NONE;

	if (system_rev >= 0x4) {
//		if (attached) {
			pr_info("%s set vbus state\n", __func__);
			msm_otg_set_vbus_state(attached);
//		}
	}

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_USB:
		value.intval = POWER_SUPPLY_TYPE_USB;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("%s: invalid cable :%d\n", __func__, set_cable_status);
		return;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
		&value);
	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

static void fsa9485_charger_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9480_charger_cb attached %d\n", attached);
	set_cable_status = attached ? CABLE_TYPE_AC : CABLE_TYPE_NONE;

	msm_otg_set_charging_state(attached);

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

#ifdef CONFIG_TOUCHSCREEN_MMS144
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, attached);
#endif

	switch (set_cable_status) {
	case CABLE_TYPE_AC:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
		&value);
	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

static void fsa9485_uart_cb(bool attached)
{
	pr_info("fsa9485_uart_cb attached %d\n", attached);

	set_cable_status = attached ? CABLE_TYPE_UARTOFF : CABLE_TYPE_NONE;
}

static struct switch_dev switch_dock = {
	.name = "dock",
};

static void fsa9485_jig_cb(bool attached)
{
	pr_info("fsa9485_jig_cb attached %d\n", attached);

	set_cable_status = attached ? CABLE_TYPE_JIG : CABLE_TYPE_NONE;
}

static void fsa9485_dock_cb(int attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9480_dock_cb attached %d\n", attached);
	switch_set_state(&switch_dock, attached);

	set_cable_status = attached ? CABLE_TYPE_CARDOCK : CABLE_TYPE_NONE;

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_CARDOCK:
		value.intval = POWER_SUPPLY_TYPE_CARDOCK;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
		&value);
	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

static void fsa9485_usb_cdp_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_usb_cdp_cb attached %d\n", attached);

	set_cable_status =
		attached ? CABLE_TYPE_CDP : CABLE_TYPE_NONE;

	if (system_rev >= 0x4) {
		if (attached) {
			pr_info("%s set vbus state\n", __func__);
			msm_otg_set_vbus_state(attached);
		}
	}

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_CDP:
		value.intval = POWER_SUPPLY_TYPE_USB_CDP;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
		&value);
	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}

}
static void fsa9485_smartdock_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_smartdock_cb attached %d\n", attached);

	set_cable_status =
		attached ? CABLE_TYPE_SMART_DOCK : CABLE_TYPE_NONE;

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_SMART_DOCK:
		value.intval = POWER_SUPPLY_TYPE_USB_CDP;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
		&value);
	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}

//	msm_otg_set_smartdock_state(attached);
}

static void fsa9485_audio_dock_cb(bool attached)
{
	pr_info("fsa9485_audio_dock_cb attached %d\n", attached);

//	msm_otg_set_smartdock_state(attached);
}

static int fsa9485_dock_init(void)
{
	int ret;

	/* for CarDock, DeskDock */
	ret = switch_dev_register(&switch_dock);
	if (ret < 0) {
		pr_err("Failed to register dock switch. %d\n", ret);
		return ret;
	}
	return 0;
}

int msm8960_get_cable_type(void)
{
#ifdef CONFIG_WIRELESS_CHARGING
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return -1;
	}
#endif

	pr_info("cable type (%d) -----\n", set_cable_status);

	if (set_cable_status != CABLE_TYPE_NONE) {
		switch (set_cable_status) {
		case CABLE_TYPE_MISC:
#ifdef CONFIG_MHL_NEW_CBUS_MSC_CMD
			fsa9485_mhl_cb(1 , 0);
#else
			fsa9485_mhl_cb(1);
#endif
			break;
		case CABLE_TYPE_USB:
			if (!force_fast_charge) fsa9485_usb_cb(1);
				else fsa9485_charger_cb(1);
			break;
		case CABLE_TYPE_AC:
			fsa9485_charger_cb(1);
			break;
#ifdef CONFIG_WIRELESS_CHARGING
		case CABLE_TYPE_WPC:
			value.intval = POWER_SUPPLY_TYPE_WPC;
			ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
				&value);
			break;
#endif
		default:
			pr_err("invalid status:%d\n", set_cable_status);
			break;
		}
	}
	return set_cable_status;
}

static struct i2c_gpio_platform_data fsa_i2c_gpio_data = {
	.sda_pin		= GPIO_USB_I2C_SDA,
	.scl_pin		= GPIO_USB_I2C_SCL,
	.udelay			= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device fsa_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= MSM_FSA9485_I2C_BUS_ID,
	.dev.platform_data	= &fsa_i2c_gpio_data,
};

static struct fsa9485_platform_data fsa9485_pdata = {
	.otg_cb = fsa9485_otg_cb,
	.usb_cb = fsa9485_usb_cb,
	.charger_cb = fsa9485_charger_cb,
	.uart_cb = fsa9485_uart_cb,
	.jig_cb = fsa9485_jig_cb,
	.dock_cb = fsa9485_dock_cb,
	.dock_init = fsa9485_dock_init,
	.usb_cdp_cb = fsa9485_usb_cdp_cb,
	.smartdock_cb = fsa9485_smartdock_cb,
	.audio_dock_cb = fsa9485_audio_dock_cb,
};

static struct i2c_board_info micro_usb_i2c_devices_info[] __initdata = {
	{
		I2C_BOARD_INFO("fsa9485", 0x4A >> 1),
		.platform_data = &fsa9485_pdata,
		.irq = MSM_GPIO_TO_INT(14),
	},
};

#endif

#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)

static void msm8960_mhl_gpio_init(void)
{
	int ret;

	ret = gpio_request(GPIO_MHL_EN, "mhl_en");
	if (ret < 0) {
		pr_err("mhl_en gpio_request is failed\n");
		return;
	}

	ret = gpio_request(GPIO_MHL_RST, "mhl_rst");
	if (ret < 0) {
		pr_err("mhl_rst gpio_request is failed\n");
		return;
	}

}

static void mhl_gpio_config(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_MHL_EN, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(GPIO_MHL_RST, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
}

static struct i2c_gpio_platform_data mhl_i2c_gpio_data = {
	.sda_pin   = GPIO_MHL_SDA,
	.scl_pin    = GPIO_MHL_SCL,
	.udelay    = 3,/*(i2c clk speed: 500khz / udelay)*/
};

static struct platform_device mhl_i2c_gpio_device = {
	.name       = "i2c-gpio",
	.id     = MSM_MHL_I2C_BUS_ID,
	.dev        = {
		.platform_data  = &mhl_i2c_gpio_data,
	},
};
/*
gpio_interrupt pin is very changable each different h/w_rev or  board.
*/
int get_mhl_int_irq(void)
{
	return  MSM_GPIO_TO_INT(GPIO_MHL_INT);
}

static struct regulator *mhl_l12;

static void sii9234_hw_onoff(bool onoff)
{
	int rc;
	/*VPH_PWR : mhl_power_source
	VMHL_3.3V, VSIL_A_1.2V, VMHL_1.8V
	just power control with HDMI_EN pin or control Regulator12*/
	if (onoff) {
		gpio_tlmm_config(GPIO_CFG(GPIO_MHL_EN, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
		mhl_l12 = regulator_get(NULL, "8921_l12");
		rc = regulator_set_voltage(mhl_l12, 1200000, 1200000);
			if (rc)
				pr_err("error setting voltage\n");
			rc = regulator_enable(mhl_l12);
				if (rc)
					pr_err("error enabling regulator\n");
			usleep(1*1000);
		gpio_direction_output(GPIO_MHL_EN, 1);
	} else {
		gpio_direction_output(GPIO_MHL_EN, 0);
		if (mhl_l12) {
			rc = regulator_disable(mhl_l12);
				if (rc)
					pr_err("error disabling regulator\n");
		}

		usleep_range(10000, 20000);

		if (gpio_direction_output(GPIO_MHL_RST, 0))
			pr_err("%s error in making GPIO_MHL_RST Low\n"
			, __func__);

		gpio_tlmm_config(GPIO_CFG(GPIO_MHL_EN, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
	}

	return;
}

static void sii9234_hw_reset(void)
{
	usleep_range(10000, 20000);
	if (gpio_direction_output(GPIO_MHL_RST, 1))
		printk(KERN_ERR "%s error in making GPIO_MHL_RST HIGH\n",
			 __func__);

	usleep_range(5000, 20000);
	if (gpio_direction_output(GPIO_MHL_RST, 0))
		printk(KERN_ERR "%s error in making GPIO_MHL_RST Low\n",
			 __func__);

	usleep_range(10000, 20000);
	if (gpio_direction_output(GPIO_MHL_RST, 1))
		printk(KERN_ERR "%s error in making GPIO_MHL_RST HIGH\n",
			 __func__);
	msleep(30);
}

struct sii9234_platform_data sii9234_pdata = {
	.get_irq = get_mhl_int_irq,
	.hw_onoff = sii9234_hw_onoff,
	.hw_reset = sii9234_hw_reset,
	.gpio_cfg = mhl_gpio_config,
	.swing_level = 0xEB,
#if defined(CONFIG_VIDEO_MHL_V2)
	.vbus_present = fsa9485_mhl_cb,
#endif
};

static struct i2c_board_info mhl_i2c_board_info[] = {
	{
		I2C_BOARD_INFO("sii9234_mhl_tx", 0x72>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_tpi", 0x7A>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_hdmi_rx", 0x92>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_cbus", 0xC8>>1),
		.platform_data = &sii9234_pdata,
	},
};
#endif

#ifdef CONFIG_BATTERY_SEC
static int is_sec_battery_using(void)
{
	if (system_rev >= 0x4)
		return 1;
	else
		return 0;
}

int check_battery_type(void)
{
	return BATT_TYPE_D2_ACTIVE;
}

static struct sec_bat_platform_data sec_bat_pdata = {
	.fuel_gauge_name	= "fuelgauge",
	.charger_name	= "sec-charger",
	.get_cable_type	= msm8960_get_cable_type,
	.sec_battery_using = is_sec_battery_using,
	.check_batt_type = check_battery_type,
	.iterm = 150,
	.charge_duration = 6 * 60 * 60,
	.wpc_charge_duration = 8 * 60 * 60,
	.recharge_duration = 1.5 * 60 * 60,
	.max_voltage = 4350 * 1000,
	.recharge_voltage = 4280 * 1000,
	.event_block = 600,
	.high_block = 500,
	.high_recovery = 450,
	.low_block = -50,
	.low_recovery = 0,
	.lpm_high_block = 480,
	.lpm_high_recovery = 450,
	.lpm_low_block = -50,
	.lpm_low_recovery = 0,
	.wpc_charging_current = 500,
};

static struct platform_device sec_device_battery = {
	.name = "sec-battery",
	.id = -1,
	.dev.platform_data = &sec_bat_pdata,
};

static void check_highblock_temp(void)
{
	if (system_rev < 0x6)
		sec_bat_pdata.high_block = 600;
}

#endif /* CONFIG_BATTERY_SEC */

static int is_smb347_using(void)
{
	if (system_rev >= 0x4)
		return 1;
	else
		return 0;
}

#ifdef CONFIG_CHARGER_SMB347
static int is_smb347_inok_using(void)
{
	if (system_rev >= 0x6)
		return 1;

	return 0;
}

#ifdef CONFIG_WIRELESS_CHARGING
static void smb347_wireless_cb(void)
{
	set_cable_status = CABLE_TYPE_WPC;
}
#endif

void smb347_hw_init(void)
{
	struct pm_gpio batt_int_param = {
		.direction	= PM_GPIO_DIR_IN,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.function	= PM_GPIO_FUNC_NORMAL,
	};
	int rc = 0;

	gpio_tlmm_config(GPIO_CFG(GPIO_INOK_INT,  0, GPIO_CFG_INPUT,
	 GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);

	rc = gpio_request(GPIO_INOK_INT, "wpc-detect");
	if (rc < 0) {
		pr_err("%s: GPIO_INOK_INT gpio_request failed\n", __func__);
		return;
	}
	rc = gpio_direction_input(GPIO_INOK_INT);
	if (rc < 0) {
		pr_err("%s: GPIO_INOK_INT gpio_direction_input failed\n",
			__func__);
		return;
	}

	if (system_rev >= 0xd) {
		sec_bat_pdata.batt_int =
		PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_BATT_INT);
		pm8xxx_gpio_config(
			PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_BATT_INT),
			&batt_int_param);
		gpio_request(
			PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_BATT_INT),
			"batt_int");
		gpio_direction_input(PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_BATT_INT));
	}
	pr_debug("%s : share gpioi2c with max17048\n", __func__);
}

static int smb347_intr_trigger(int status)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;

	if (!psy) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return -ENODEV;
	}
	pr_info("%s : charging status =%d\n", __func__, status);

	value.intval = status;

	/*if (status)
		value.intval = POWER_SUPPLY_STATUS_CHARGING;
	else
		value.intval = POWER_SUPPLY_STATUS_DISCHARGING;*/

	return psy->set_property(psy, POWER_SUPPLY_PROP_STATUS, &value);
}

static struct smb347_platform_data smb347_pdata = {
	.hw_init = smb347_hw_init,
	.chg_intr_trigger = smb347_intr_trigger,
	.enable = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_OTG_EN),
	.stat = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_CHG_STAT),
	.smb347_using = is_smb347_using,
	.inok = GPIO_INOK_INT,
	.smb347_inok_using = is_smb347_inok_using,
#ifdef CONFIG_WIRELESS_CHARGING
	.smb347_wpc_cb = smb347_wireless_cb,
#endif
	.smb347_get_cable = msm8960_get_cable_type,
};
#endif /* CONFIG_CHARGER_SMB347 */

#ifdef CONFIG_BATTERY_MAX17040
void max17040_hw_init(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_FUEKGAUGE_I2C_SCL, 0, GPIO_CFG_OUTPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(GPIO_FUELGAUGE_I2C_SDA,  0, GPIO_CFG_OUTPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_set_value(GPIO_FUEKGAUGE_I2C_SCL, 1);
	gpio_set_value(GPIO_FUELGAUGE_I2C_SDA, 1);

	gpio_tlmm_config(GPIO_CFG(GPIO_FUEL_INT,  0, GPIO_CFG_INPUT,
	 GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
}

static int max17040_low_batt_cb(void)
{
#ifdef CONFIG_BATTERY_SEC
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;
	pr_err("%s: Low battery alert\n", __func__);


	if (!psy) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return -ENODEV;
	}

	value.intval = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
	return psy->set_property(psy, POWER_SUPPLY_PROP_CAPACITY_LEVEL, &value);
#else
	pr_err("%s: Low battery alert\n", __func__);

	return 0;
#endif
}

static struct max17040_platform_data max17043_pdata = {
	.hw_init = max17040_hw_init,
	.low_batt_cb = max17040_low_batt_cb,
	.check_batt_type = check_battery_type,
	.rcomp_value = 0x6d1c,
};

static struct i2c_gpio_platform_data fuelgauge_i2c_gpio_data = {
	.sda_pin		= GPIO_FUELGAUGE_I2C_SDA,
	.scl_pin		= GPIO_FUEKGAUGE_I2C_SCL,
	.udelay			= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device fuelgauge_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= MSM_FUELGAUGE_I2C_BUS_ID,
	.dev.platform_data	= &fuelgauge_i2c_gpio_data,
};

static struct i2c_board_info fg_i2c_board_info[] = {
	{
		I2C_BOARD_INFO("max17040", (0x6D >> 1)),
		.platform_data = &max17043_pdata,
		.irq		= MSM_GPIO_TO_INT(GPIO_FUEL_INT),
	}
};

static struct i2c_board_info fg_smb_i2c_board_info[] = {
#ifdef CONFIG_CHARGER_SMB347
	{
		I2C_BOARD_INFO("smb347", (0x0C >> 1)),
		.platform_data = &smb347_pdata,
		.irq	= PM8921_GPIO_IRQ(PM8921_IRQ_BASE, PMIC_GPIO_CHG_STAT),
	},
#endif
	{
		I2C_BOARD_INFO("max17040", (0x6D >> 1)),
		.platform_data = &max17043_pdata,
		.irq		= MSM_GPIO_TO_INT(GPIO_FUEL_INT),
	}
};
#endif /* CONFIG_BATTERY_MAX17040 */

#ifdef CONFIG_VIBETONZ
static struct vibrator_platform_data msm_8960_vibrator_pdata = {
	.vib_model = HAPTIC_PWM,
	.vib_pwm_gpio = GPIO_VIB_PWM,
	.haptic_pwr_en_gpio = GPIO_HAPTIC_PWR_EN,
	.vib_en_gpio = GPIO_VIB_ON,
	.is_pmic_vib_en = 0,
	.is_pmic_haptic_pwr_en = 0,
};
static struct platform_device vibetonz_device = {
	.name = "tspdrv",
	.id = -1,
	.dev = {
		.platform_data = &msm_8960_vibrator_pdata ,
	},
};
#endif /* CONFIG_VIBETONZ */

#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F) \
	|| defined(CONFIG_SENSORS_CM36651)
static struct i2c_gpio_platform_data opt_i2c_gpio_data = {
	.sda_pin = GPIO_SENSOR_ALS_SDA,
	.scl_pin = GPIO_SENSOR_ALS_SCL,
	.udelay = 5,
};

static struct platform_device opt_i2c_gpio_device = {
	.name = "i2c-gpio",
	.id = MSM_OPT_I2C_BUS_ID,
	.dev = {
		.platform_data = &opt_i2c_gpio_data,
	},
};
#if defined(CONFIG_SENSORS_CM36651)
static void cm36651_led_onoff(int);

static struct cm36651_platform_data cm36651_pdata = {
	.cm36651_led_on = cm36651_led_onoff,
	.cm36651_power_on = cm36651_power_on,
	.irq = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_RGB_INT),
	.threshold = 15,
};
#endif
static struct i2c_board_info opt_i2c_borad_info[] = {
	{
#if defined(CONFIG_OPTICAL_GP2A)
		I2C_BOARD_INFO("gp2a", 0x88>>1),
#elif defined(CONFIG_OPTICAL_GP2AP020A00F)
		I2C_BOARD_INFO("gp2a", 0x72>>1),
#endif
	},
#if defined(CONFIG_SENSORS_CM36651)
	{
		I2C_BOARD_INFO("cm36651", (0x30 >> 1)),
		.platform_data = &cm36651_pdata,
	},
#endif
};
#if 0
static void gp2a_led_onoff(int);

#if defined(CONFIG_OPTICAL_GP2A)
static struct opt_gp2a_platform_data opt_gp2a_data = {
	.gp2a_led_on	= gp2a_led_onoff,
	.power_on = sensor_power_on_vdd,
	.irq = PM8921_GPIO_IRQ(PM8921_IRQ_BASE, PMIC_GPIO_RGB_INT),
	.ps_status = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_RGB_INT),
};
#elif defined(CONFIG_OPTICAL_GP2AP020A00F)
static struct gp2a_platform_data opt_gp2a_data = {
	.gp2a_led_on	= gp2a_led_onoff,
	.power_on = gp2a_power_on,
	.p_out = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_RGB_INT),
	.adapt_num	= MSM_OPT_I2C_BUS_ID,
	.addr = 0x72>>1,
	.version = 0,
};
#endif

#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F)
static struct platform_device opt_gp2a = {
	.name = "gp2a-opt",
	.id = -1,
	.dev        = {
		.platform_data  = &opt_gp2a_data,
	},
};
#endif
#endif
#endif
#ifdef CONFIG_MPU_SENSORS_MPU6050B1_411
	struct mpu_platform_data mpu6050_data = {
	.int_config = 0x10,
	.orientation = {0, -1, 0,
			1, 0, 0,
			0, 0, 1},
	.poweron = mpu_power_on,
	};
	/* compass */
	static struct ext_slave_platform_data inv_mpu_ak8963_data = {
	.bus		= EXT_SLAVE_BUS_PRIMARY,
	.orientation = {0, 1, 0,
			-1, 0, 0,
			0, 0, 1},
	};

#if 0
	static struct ext_slave_platform_data inv_mpu_ak8963_data_03 = {
	.bus		= EXT_SLAVE_BUS_PRIMARY,
	.orientation = {-1, 0, 0,
			0, 1, 0,
			0, 0, -1},
	};
#endif

	struct mpu_platform_data mpu6050_data_01 = {
	.int_config = 0x10,
	.orientation = {0, -1, 0,
			1, 0, 0,
			0, 0, 1},
	.poweron = mpu_power_on,
	};
	/* compass */
	static struct ext_slave_platform_data inv_mpu_ak8963_data_01 = {
	.bus		= EXT_SLAVE_BUS_PRIMARY,
	.orientation = {0, 1, 0,
			-1, 0, 0,
			0, 0, 1},
	};
	struct mpu_platform_data mpu6050_data_00 = {
	.int_config = 0x10,
	.orientation = {0, -1, 0,
			-1, 0, 0,
			0, 0, -1},
	.poweron = mpu_power_on,
	};
	/* compass */
	static struct ext_slave_platform_data inv_mpu_ak8963_data_00 = {
	.bus		= EXT_SLAVE_BUS_PRIMARY,
	.orientation = {1, 0, 0,
			0, 1, 0,
			0, 0, 1},
	};
#endif

#ifdef CONFIG_MPU_SENSORS_MPU6050B1
#define SENSOR_MPU_NAME			"mpu6050B1"
static struct mpu_platform_data mpu_data = {
	.int_config = 0x12,
	.orientation = {0, -1, 0,
			1, 0, 0,
			0, 0, 1},
	/* accel */
	.accel = {
		  .get_slave_descr = mantis_get_slave_descr,
		  .adapt_num = MSM_SNS_I2C_BUS_ID,
		  .bus = EXT_SLAVE_BUS_SECONDARY,
		  .address = 0x68,
		  .orientation = {0, -1, 0,
				  1, 0, 0,
				  0, 0, 1},
		  },
	/* compass */
	.compass = {
		    .get_slave_descr = ak8975_get_slave_descr,
		    .adapt_num = MSM_SNS_I2C_BUS_ID,
		    .bus = EXT_SLAVE_BUS_PRIMARY,
		    .address = 0x0C,
		    .orientation = {0, 1, 0,
				    -1, 0, 0,
				    0, 0, 1},
		    },
	.poweron = mpu_power_on,
};

static struct mpu_platform_data mpu_data_01 = {
	.int_config = 0x12,
	.orientation = {0, -1, 0,
			1, 0, 0,
			0, 0, 1},
	/* accel */
	.accel = {
		  .get_slave_descr = mantis_get_slave_descr,
		  .adapt_num = MSM_SNS_I2C_BUS_ID,
		  .bus = EXT_SLAVE_BUS_SECONDARY,
		  .address = 0x68,
		  .orientation = {0, -1, 0,
				  1, 0, 0,
				  0, 0, 1},
		  },
	/* compass */
	.compass = {
		    .get_slave_descr = ak8975_get_slave_descr,
		    .adapt_num = MSM_SNS_I2C_BUS_ID,
		    .bus = EXT_SLAVE_BUS_PRIMARY,
		    .address = 0x0C,
		    .orientation = {0, 1, 0,
				    -1, 0, 0,
				    0, 0, 1},
		    },
	.poweron = mpu_power_on,
};

static struct mpu_platform_data mpu_data_00 = {
	.int_config = 0x12,
	.orientation = {0, -1, 0,
			-1, 0, 0,
			0, 0, -1},
	/* accel */
	.accel = {
		  .get_slave_descr = mantis_get_slave_descr,
		  .adapt_num = MSM_SNS_I2C_BUS_ID,
		  .bus = EXT_SLAVE_BUS_SECONDARY,
		  .address = 0x68,
		  .orientation = {0, -1, 0,
				  -1, 0, 0,
				  0, 0, -1},
		  },
	/* compass */
	.compass = {
		    .get_slave_descr = ak8975_get_slave_descr,
		    .adapt_num = MSM_SNS_I2C_BUS_ID,
		    .bus = EXT_SLAVE_BUS_PRIMARY,
		    .address = 0x0C,
		    .orientation = {1, 0, 0,
				    0, 1, 0,
				    0, 0, 1},
		    },
	.poweron = mpu_power_on,
};
#endif /*CONFIG_MPU_SENSORS_MPU6050B1 */

#if defined(CONFIG_SENSORS_AK8975) || defined(CONFIG_INPUT_BMP180) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1_411)

#ifdef CONFIG_SENSORS_AK8975
static struct akm8975_platform_data akm8975_pdata = {
	.gpio_data_ready_int = GPIO_MSENSE_RST,
	.power_on = akm_power_on,
};
#endif
#ifdef CONFIG_INPUT_BMP180
static struct bmp_i2c_platform_data bmp180_pdata = {
	.power_on = bmp180_power_on,
};
#endif

static struct i2c_board_info sns_i2c_borad_info[] = {
#ifdef CONFIG_MPU_SENSORS_MPU6050B1
	{
	 I2C_BOARD_INFO(SENSOR_MPU_NAME, 0x68),
	 .irq = MSM_GPIO_TO_INT(GPIO_MPU3050_INT),
	 .platform_data = &mpu_data,
	 },
#endif
#ifdef CONFIG_SENSORS_AK8975
	{
		I2C_BOARD_INFO("ak8975", 0x0C),
		.platform_data = &akm8975_pdata,
		.irq = MSM_GPIO_TO_INT(GPIO_MSENSE_RST),
	},
#endif
#ifdef CONFIG_MPU_SENSORS_MPU6050B1_411
	{
		I2C_BOARD_INFO("mpu6050", 0x68),
		.irq = MSM_GPIO_TO_INT(GPIO_MPU3050_INT),
		.platform_data = &mpu6050_data,
	 },
#endif
#ifdef CONFIG_MPU_SENSORS_AK8975_411
	{
		I2C_BOARD_INFO("ak8975_mod", 0x0C),
		.platform_data = &inv_mpu_ak8963_data,
		.irq = MSM_GPIO_TO_INT(GPIO_MSENSE_RST),
	},
#endif
#ifdef CONFIG_INPUT_BMP180
	{
		I2C_BOARD_INFO("bmp180", 0x77),
		.platform_data = &bmp180_pdata,
	},
#endif

};
#endif

#if defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1_411)
static void mpl_init(void)
{
	int ret = 0;
	ret = gpio_request(GPIO_MPU3050_INT, "MPUIRQ");
	if (ret)
		pr_err("%s gpio request %d err\n", __func__, GPIO_MPU3050_INT);
	else
		gpio_direction_input(GPIO_MPU3050_INT);

#if defined(CONFIG_MPU_SENSORS_MPU6050B1)
	if (system_rev == BOARD_REV01)
		mpu_data = mpu_data_01;
	else if (system_rev < BOARD_REV01)
		mpu_data = mpu_data_00;
	mpu_data.reset = gpio_rev(GPIO_MAG_RST);
#elif defined(CONFIG_MPU_SENSORS_MPU6050B1_411)
	if (system_rev == BOARD_REV01) {
		mpu6050_data = mpu6050_data_01;
		inv_mpu_ak8963_data = inv_mpu_ak8963_data_01;
	} else if (system_rev < BOARD_REV01) {
		mpu6050_data = mpu6050_data_00;
		inv_mpu_ak8963_data = inv_mpu_ak8963_data_00;
	}
	if (system_rev < BOARD_REV08 || system_rev == BOARD_REV11)
		mpu6050_data.reset = gpio_rev(GPIO_MAG_RST);
	else
		mpu6050_data.reset =
			PM8921_GPIO_PM_TO_SYS(gpio_rev(GPIO_MAG_RST));
#endif
}
#endif

#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F) \
	|| defined(CONFIG_SENSORS_CM36651)
static void opt_init(void)
{
	int ret = 0;
	int prox_int = gpio_rev(ALS_INT);
	struct pm_gpio prox_cfg = {
		.direction = PM_GPIO_DIR_IN,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
	};

	if (system_rev < BOARD_REV08 || system_rev == BOARD_REV11) {
		gpio_tlmm_config(GPIO_CFG(prox_int, 0,
			GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	} else {
		prox_int = PM8921_GPIO_PM_TO_SYS(prox_int);
		pm8xxx_gpio_config(prox_int, &prox_cfg);
	}

	gpio_tlmm_config(GPIO_CFG(gpio_rev(ALS_SDA), 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(gpio_rev(ALS_SCL), 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	ret = gpio_request(prox_int, "PSVOUT");
	if (ret) {
		pr_err("%s gpio request %d err\n", __func__, prox_int);
	} else {
		gpio_direction_input(prox_int);
		gpio_free(prox_int);
	}
}
#endif

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

static void samsung_sys_class_init(void)
{
	pr_info("samsung sys class init.\n");

	sec_class = class_create(THIS_MODULE, "sec");

	if (IS_ERR(sec_class)) {
		pr_err("Failed to create class(sec)!\n");
		return;
	}

	pr_info("samsung sys class end.\n");
};

#if defined(CONFIG_NFC_PN544)
static void pn544_conf_gpio(void)
{
	pr_debug("pn544_conf_gpio\n");

	gpio_tlmm_config(GPIO_CFG(GPIO_NFC_SDA, 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(GPIO_NFC_SCL, 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	return;
}

static int __init pn544_init(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_NFC_IRQ, 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	pn544_conf_gpio();
	return 0;
}
#endif

#if defined(CONFIG_SENSORS_AK8975) || defined(CONFIG_INPUT_BMP180) ||\
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1_411)

static int __init sensor_device_init(void)
{
	int ret = 0;
	int mag_rst = gpio_rev(GPIO_MAG_RST);
	struct pm_gpio mag_rst_cfg = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_MED,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};

	if (system_rev < BOARD_REV08 || system_rev == BOARD_REV11) {
		gpio_tlmm_config(GPIO_CFG(mag_rst, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
	} else {
		mag_rst = PM8921_GPIO_PM_TO_SYS(mag_rst);
		pm8xxx_gpio_config(mag_rst, &mag_rst_cfg);
	}
	sensor_power_on_vdd(SNS_PWR_ON, SNS_PWR_ON);
	ret = gpio_request(mag_rst, "MAG_RST");
	if (ret) {
		pr_err("%s gpio request %d err\n", __func__, mag_rst);
	} else {
		gpio_direction_output(mag_rst, 0);
		usleep_range(20, 20);
		gpio_set_value_cansleep(mag_rst, 1);
	}

	return 0;
}
#endif

#if defined(CONFIG_SENSORS_AK8975) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_INPUT_BMP180) || defined(CONFIG_OPTICAL_GP2A) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1_411)
static struct regulator *vsensor_2p85, *vsensor_1p8;
static int sensor_power_2p85_cnt, sensor_power_1p8_cnt;

static void sensor_power_on_vdd(int onoff_l9, int onoff_lvs4)
{
	int ret;

	if (vsensor_2p85 == NULL) {
		vsensor_2p85 = regulator_get(NULL, "8921_l9");
		if (IS_ERR(vsensor_2p85))
			return ;

		ret = regulator_set_voltage(vsensor_2p85, 2850000, 2850000);
		if (ret)
			pr_err("%s: error vsensor_2p85 setting voltage ret=%d\n",
				__func__, ret);
	}
	if (vsensor_1p8 == NULL) {
		vsensor_1p8 = regulator_get(NULL, "8921_lvs4");
		if (IS_ERR(vsensor_1p8))
			return ;
	}

	if (onoff_l9 == SNS_PWR_ON) {
		sensor_power_2p85_cnt++;
		ret = regulator_enable(vsensor_2p85);
		if (ret)
			pr_err("%s: error enabling regulator\n", __func__);
	} else if ((onoff_l9 == SNS_PWR_OFF)) {
		sensor_power_2p85_cnt--;
		if (regulator_is_enabled(vsensor_2p85)) {
			ret = regulator_disable(vsensor_2p85);
			if (ret)
				pr_err("%s: error vsensor_2p85 enabling regulator\n",
				__func__);
		}
	}
	if (onoff_lvs4 == SNS_PWR_ON) {
		sensor_power_1p8_cnt++;
		ret = regulator_enable(vsensor_1p8);
		if (ret)
			pr_err("%s: error enabling regulator\n", __func__);
	} else if ((onoff_lvs4 == SNS_PWR_OFF)) {
		sensor_power_1p8_cnt--;
		if (regulator_is_enabled(vsensor_1p8)) {
			ret = regulator_disable(vsensor_1p8);
			if (ret)
				pr_err("%s: error vsensor_1p8 enabling regulator\n",
				__func__);
		}
	}
}

#endif
#if defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1_411)
static void mpu_power_on(int onoff)
{
	sensor_power_on_vdd(onoff, onoff);
}
#endif

#ifdef CONFIG_SENSORS_AK8975
static void akm_power_on(int onoff)
{
	sensor_power_on_vdd(onoff, onoff);
}
#endif

#ifdef CONFIG_INPUT_BMP180
static void bmp180_power_on(int onoff)
{
	sensor_power_on_vdd(SNS_PWR_KEEP, onoff);
}
#endif

#if defined(CONFIG_OPTICAL_GP2AP020A00F)
static void gp2a_power_on(int onoff)
{
	sensor_power_on_vdd(onoff, onoff);
}
#endif

#if defined(CONFIG_SENSORS_CM36651)
static void cm36651_power_on(int onoff)
{
	sensor_power_on_vdd(onoff, onoff);
}
#endif

#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F)
static void gp2a_led_onoff(int onoff)
{
	static struct regulator *reg_8921_leda;
	static int prev_on;
	int rc;

	if (onoff == prev_on)
		return;

	if (!reg_8921_leda) {
		reg_8921_leda = regulator_get(NULL, "8921_l16");
		rc = regulator_set_voltage(reg_8921_leda,
			3000000, 3000000);
		if (rc)
			pr_err("%s: error reg_8921_leda setting  ret=%d\n",
				__func__, rc);
	}

	if (onoff) {
		rc = regulator_enable(reg_8921_leda);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"reg_8921_leda", rc);
			return;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_leda);
		if (rc) {
			pr_err("'%s' regulator disable failed, rc=%d\n",
				"reg_8921_leda", rc);
			return;
		}
		pr_debug("%s(off): success\n", __func__);
	}
	prev_on = onoff;
}
#endif

#if defined(CONFIG_SENSORS_CM36651)

static void cm36651_led_onoff(int onoff)
{
	static struct regulator *reg_8921_leda;
	static int prev_on;
	int rc;

	if (onoff == prev_on)
		return;

	if (!reg_8921_leda) {
		reg_8921_leda = regulator_get(NULL, "8921_l16");
		rc = regulator_set_voltage(reg_8921_leda,
			2800000, 2800000);
		if (rc)
			pr_err("%s: error reg_8921_leda setting  ret=%d\n",
				__func__, rc);
			return;
	}

	if (onoff) {
		rc = regulator_enable(reg_8921_leda);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"reg_8921_leda", rc);
			return;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_leda);
		if (rc) {
			pr_err("'%s' regulator disable failed, rc=%d\n",
				"reg_8921_leda", rc);
			return;
		}
		pr_debug("%s(off): success\n", __func__);
	}
	prev_on = onoff;
}
#endif

#ifdef CONFIG_VP_A2220
static int a2220_hw_init(void)
{
	int rc = 0;

	rc = gpio_request(gpio_rev(A2220_WAKEUP), "a2220_wakeup");
	if (rc < 0) {
		pr_err("%s: gpio request wakeup pin failed\n", __func__);
		goto err_alloc_data_failed;
	}

	rc = gpio_direction_output(gpio_rev(A2220_WAKEUP), 1);
	if (rc < 0) {
		pr_err("%s: request wakeup gpio direction failed\n", __func__);
		goto err_free_gpio;
	}

	rc = gpio_request(MSM_AUD_A2220_RESET, "a2220_reset");
	if (rc < 0) {
		pr_err("%s: gpio request reset pin failed\n", __func__);
		goto err_free_gpio;
	}

	rc = gpio_direction_output(MSM_AUD_A2220_RESET, 1);
	if (rc < 0) {
		pr_err("%s: request reset gpio direction failed\n", __func__);
		goto err_free_gpio_all;
	}
	gpio_set_value(gpio_rev(A2220_WAKEUP), 1);
	gpio_set_value(MSM_AUD_A2220_RESET, 1);
	return rc;

err_free_gpio_all:
	gpio_free(MSM_AUD_A2220_RESET);
err_free_gpio:
	gpio_free(gpio_rev(A2220_WAKEUP));
err_alloc_data_failed:
	pr_err("a2220_probe - failed\n");
	return rc;
}

static struct a2220_platform_data a2220_data = {
	.a2220_hw_init = a2220_hw_init,
	.gpio_reset = MSM_AUD_A2220_RESET,
};

static struct i2c_board_info a2220_device[] __initdata = {
	{
		I2C_BOARD_INFO("audience_a2220", 0x3E),
		.platform_data = &a2220_data,
	},
};

static struct i2c_gpio_platform_data  a2220_i2c_gpio_data = {
	.udelay			= 1,
};
#if 0
static struct platform_device a2220_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= MSM_A2220_I2C_BUS_ID,
	.dev.platform_data	= &a2220_i2c_gpio_data,
};
#endif
static struct gpiomux_setting a2220_gsbi_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm8960_a2220_configs[] = {
	{
		.settings = {
			[GPIOMUX_SUSPENDED] = &a2220_gsbi_config,
		},
	},
	{
		.settings = {
			[GPIOMUX_SUSPENDED] = &a2220_gsbi_config,
		},
	},
};
#endif
#ifdef CONFIG_WCD9310_CODEC

#define TABLA_INTERRUPT_BASE (NR_MSM_IRQS + NR_GPIO_IRQS + NR_PM8921_IRQS)

/* Micbias setting is based on 8660 CDP/MTP/FLUID requirement
 * 4 micbiases are used to power various analog and digital
 * microphones operating at 1800 mV. Technically, all micbiases
 * can source from single cfilter since all microphones operate
 * at the same voltage level. The arrangement below is to make
 * sure all cfilters are exercised. LDO_H regulator ouput level
 * does not need to be as high as 2.85V. It is choosen for
 * microphone sensitivity purpose.
 */
#ifndef CONFIG_SLIMBUS_MSM_CTRL
static struct wcd9xxx_pdata tabla_i2c_platform_data = {
	.irq = MSM_GPIO_TO_INT(58),
	.irq_base = TABLA_INTERRUPT_BASE,
	.num_irqs = NR_TABLA_IRQS,
	.reset_gpio = PM8921_GPIO_PM_TO_SYS(38),
	.micbias = {
		.ldoh_v = TABLA_LDOH_2P85_V,
		.cfilt1_mv = 1800,
		.cfilt2_mv = 2700,
		.cfilt3_mv = 1800,
		.bias1_cfilt_sel = TABLA_CFILT1_SEL,
		.bias2_cfilt_sel = TABLA_CFILT2_SEL,
		.bias3_cfilt_sel = TABLA_CFILT3_SEL,
		.bias4_cfilt_sel = TABLA_CFILT3_SEL,
	},
	.regulator = {
	{
		.name = "CDC_VDD_CP",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_CP_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_RX",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_RX_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_TX",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_TX_CUR_MAX,
	},
	{
		.name = "VDDIO_CDC",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_VDDIO_CDC_CUR_MAX,
	},
	{
		.name = "VDDD_CDC_D",
		.min_uV = 1225000,
		.max_uV = 1225000,
		.optimum_uA = WCD9XXX_VDDD_CDC_D_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_A_1P2V",
		.min_uV = 1225000,
		.max_uV = 1225000,
		.optimum_uA = WCD9XXX_VDDD_CDC_A_CUR_MAX,
	},
	},
};
#endif
#endif
#define MSM_WCNSS_PHYS	0x03000000
#define MSM_WCNSS_SIZE	0x280000

static struct resource resources_wcnss_wlan[] = {
	{
		.start	= RIVA_APPS_WLAN_RX_DATA_AVAIL_IRQ,
		.end	= RIVA_APPS_WLAN_RX_DATA_AVAIL_IRQ,
		.name	= "wcnss_wlanrx_irq",
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= RIVA_APPS_WLAN_DATA_XFER_DONE_IRQ,
		.end	= RIVA_APPS_WLAN_DATA_XFER_DONE_IRQ,
		.name	= "wcnss_wlantx_irq",
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= MSM_WCNSS_PHYS,
		.end	= MSM_WCNSS_PHYS + MSM_WCNSS_SIZE - 1,
		.name	= "wcnss_mmio",
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= 84,
		.end	= 88,
		.name	= "wcnss_gpios_5wire",
		.flags	= IORESOURCE_IO,
	},
};

static struct qcom_wcnss_opts qcom_wcnss_pdata = {
	.has_48mhz_xo	= 1,
};

static struct platform_device msm_device_wcnss_wlan = {
	.name		= "wcnss_wlan",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(resources_wcnss_wlan),
	.resource	= resources_wcnss_wlan,
	.dev		= {.platform_data = &qcom_wcnss_pdata},
};

#ifdef CONFIG_QSEECOM
/* qseecom bus scaling */
static struct msm_bus_vectors qseecom_clks_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ib = 0,
		.ab = 0,
	},
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_SPS,
		.ib = 0,
		.ab = 0,
	},
	{
		.src = MSM_BUS_MASTER_SPDM,
		.dst = MSM_BUS_SLAVE_SPDM,
		.ib = 0,
		.ab = 0,
	},
};

static struct msm_bus_vectors qseecom_enable_dfab_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ib = (492 * 8) * 1000000UL,
		.ab = (492 * 8) *  100000UL,
	},
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_SPS,
		.ib = (492 * 8) * 1000000UL,
		.ab = (492 * 8) * 100000UL,
	},
	{
		.src = MSM_BUS_MASTER_SPDM,
		.dst = MSM_BUS_SLAVE_SPDM,
		.ib = 0,
		.ab = 0,
	},
};

static struct msm_bus_vectors qseecom_enable_sfpb_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ib = 0,
		.ab = 0,
	},
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_SPS,
		.ib = 0,
		.ab = 0,
	},
	{
		.src = MSM_BUS_MASTER_SPDM,
		.dst = MSM_BUS_SLAVE_SPDM,
		.ib = (64 * 8) * 1000000UL,
		.ab = (64 * 8) *  100000UL,
	},
};

static struct msm_bus_vectors qseecom_enable_dfab_sfpb_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ib = (492 * 8) * 1000000UL,
		.ab = (492 * 8) *  100000UL,
	},
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_SPS,
		.ib = (492 * 8) * 1000000UL,
		.ab = (492 * 8) * 100000UL,
	},
	{
		.src = MSM_BUS_MASTER_SPDM,
		.dst = MSM_BUS_SLAVE_SPDM,
		.ib = (64 * 8) * 1000000UL,
		.ab = (64 * 8) *  100000UL,
	},
};

static struct msm_bus_paths qseecom_hw_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(qseecom_clks_init_vectors),
		qseecom_clks_init_vectors,
	},
	{
		ARRAY_SIZE(qseecom_enable_dfab_vectors),
		qseecom_enable_dfab_vectors,
	},
	{
		ARRAY_SIZE(qseecom_enable_sfpb_vectors),
		qseecom_enable_sfpb_vectors,
	},
	{
		ARRAY_SIZE(qseecom_enable_dfab_sfpb_vectors),
		qseecom_enable_dfab_sfpb_vectors,
	},
};

static struct msm_bus_scale_pdata qseecom_bus_pdata = {
	qseecom_hw_bus_scale_usecases,
	ARRAY_SIZE(qseecom_hw_bus_scale_usecases),
	.name = "qsee",
};

static struct platform_device qseecom_device = {
	.name		= "qseecom",
	.id		= 0,
	.dev		= {
		.platform_data = &qseecom_bus_pdata,
	},
};
#endif
#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

#define QCE_SIZE		0x10000
#define QCE_0_BASE		0x18500000

#define QCE_HW_KEY_SUPPORT	0
#define QCE_SHA_HMAC_SUPPORT	1
#define QCE_SHARE_CE_RESOURCE	1
#define QCE_CE_SHARED		0

/* Begin Bus scaling definitions */
static struct msm_bus_vectors crypto_hw_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ADM_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
	{
		.src = MSM_BUS_MASTER_ADM_PORT1,
		.dst = MSM_BUS_SLAVE_GSBI1_UART,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors crypto_hw_active_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ADM_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 70000000UL,
		.ib = 70000000UL,
	},
	{
		.src = MSM_BUS_MASTER_ADM_PORT1,
		.dst = MSM_BUS_SLAVE_GSBI1_UART,
		.ab = 2480000000UL,
		.ib = 2480000000UL,
	},
};

static struct msm_bus_paths crypto_hw_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(crypto_hw_init_vectors),
		crypto_hw_init_vectors,
	},
	{
		ARRAY_SIZE(crypto_hw_active_vectors),
		crypto_hw_active_vectors,
	},
};

static struct msm_bus_scale_pdata crypto_hw_bus_scale_pdata = {
		crypto_hw_bus_scale_usecases,
		ARRAY_SIZE(crypto_hw_bus_scale_usecases),
		.name = "cryptohw",
};
/* End Bus Scaling Definitions*/

static struct resource qcrypto_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

static struct resource qcedev_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)

static struct msm_ce_hw_support qcrypto_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
	.bus_scale_table = &crypto_hw_bus_scale_pdata,
};

static struct platform_device qcrypto_device = {
	.name		= "qcrypto",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcrypto_resources),
	.resource	= qcrypto_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcrypto_ce_hw_suppport,
	},
};
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

static struct msm_ce_hw_support qcedev_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
	.bus_scale_table = &crypto_hw_bus_scale_pdata,
};

static struct platform_device qcedev_device = {
	.name		= "qce",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcedev_resources),
	.resource	= qcedev_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcedev_ce_hw_suppport,
	},
};
#endif

#define MDM2AP_ERRFATAL			70
#define AP2MDM_ERRFATAL			95
#define MDM2AP_STATUS			69
#define AP2MDM_STATUS			94
#define AP2MDM_PMIC_RESET_N		80
#define AP2MDM_KPDPWR_N			81

static struct resource mdm_resources[] = {
	{
		.start	= MDM2AP_ERRFATAL,
		.end	= MDM2AP_ERRFATAL,
		.name	= "MDM2AP_ERRFATAL",
		.flags	= IORESOURCE_IO,
	},
	{
		.start	= AP2MDM_ERRFATAL,
		.end	= AP2MDM_ERRFATAL,
		.name	= "AP2MDM_ERRFATAL",
		.flags	= IORESOURCE_IO,
	},
	{
		.start	= MDM2AP_STATUS,
		.end	= MDM2AP_STATUS,
		.name	= "MDM2AP_STATUS",
		.flags	= IORESOURCE_IO,
	},
	{
		.start	= AP2MDM_STATUS,
		.end	= AP2MDM_STATUS,
		.name	= "AP2MDM_STATUS",
		.flags	= IORESOURCE_IO,
	},
	{
		.start	= AP2MDM_PMIC_RESET_N,
		.end	= AP2MDM_PMIC_RESET_N,
		.name	= "AP2MDM_PMIC_RESET_N",
		.flags	= IORESOURCE_IO,
	},
	{
		.start	= AP2MDM_KPDPWR_N,
		.end	= AP2MDM_KPDPWR_N,
		.name	= "AP2MDM_KPDPWR_N",
		.flags	= IORESOURCE_IO,
	},
};

static struct mdm_platform_data mdm_platform_data = {
	.mdm_version = "2.5",
};

static struct platform_device mdm_device = {
	.name		= "mdm2_modem",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(mdm_resources),
	.resource	= mdm_resources,
	.dev		= {
		.platform_data = &mdm_platform_data,
	},
};

static struct platform_device *mdm_devices[] __initdata = {
	&mdm_device,
};

#define MSM_SHARED_RAM_PHYS 0x80000000

static void __init msm8960_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_msm8960_io();

	if (socinfo_init() < 0)
		pr_err("socinfo_init() failed!\n");
#ifdef CONFIG_SEC_DEBUG
	sec_getlog_supply_meminfo(0x40000000, 0x80000000, 0x00, 0x00);
#endif

}

static void __init msm8960_init_irq(void)
{
	struct msm_mpm_device_data *data = NULL;

#ifdef CONFIG_MSM_MPM
	data = &msm8960_mpm_dev_data;
#endif

	msm_mpm_irq_extn_init(data);
	gic_init(0, GIC_PPI_START, MSM_QGIC_DIST_BASE,
						(void *)MSM_QGIC_CPU_BASE);
}

static void __init msm8960_init_buses(void)
{
#ifdef CONFIG_MSM_BUS_SCALING
	msm_bus_rpm_set_mt_mask();
	msm_bus_8960_apps_fabric_pdata.rpm_enabled = 1;
	msm_bus_8960_sys_fabric_pdata.rpm_enabled = 1;
	msm_bus_8960_mm_fabric_pdata.rpm_enabled = 1;
	msm_bus_apps_fabric.dev.platform_data =
		&msm_bus_8960_apps_fabric_pdata;
	msm_bus_sys_fabric.dev.platform_data = &msm_bus_8960_sys_fabric_pdata;
	msm_bus_mm_fabric.dev.platform_data = &msm_bus_8960_mm_fabric_pdata;
	msm_bus_sys_fpb.dev.platform_data = &msm_bus_8960_sys_fpb_pdata;
	msm_bus_cpss_fpb.dev.platform_data = &msm_bus_8960_cpss_fpb_pdata;
#endif
}

#ifdef CONFIG_S5C73M3
static struct msm_spi_platform_data msm8960_qup_spi_gsbi11_pdata = {
	.max_clock_speed = 48000000, /*15060000,*/
};
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static struct msm_otg_platform_data msm_otg_pdata;
#else
static bool vbus_is_on;
static void msm_hsusb_vbus_power_max8627(bool on)
{
	int rc;
	static struct regulator *mvs_otg_switch;
	struct pm_gpio param = {
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
		.output_value	= 1,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength	= PM_GPIO_STRENGTH_MED,
		.function	= PM_GPIO_FUNC_NORMAL,
	};

	pr_info("%s, attached %d, vbus_is_on %d\n", __func__, on, vbus_is_on);

	if (vbus_is_on == on)
		return;

	if (on) {
		mvs_otg_switch = regulator_get(&msm8960_device_otg.dev,
					       "vbus_otg");
		if (IS_ERR(mvs_otg_switch)) {
			pr_err("Unable to get mvs_otg_switch\n");
			return;
		}

		rc = gpio_request(PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_OTG_EN),
						"usb_5v_en");
		if (rc < 0) {
			pr_err("failed to request usb_5v_en gpio\n");
			goto put_mvs_otg;
		}

		if (regulator_enable(mvs_otg_switch)) {
			pr_err("unable to enable mvs_otg_switch\n");
			goto free_usb_5v_en;
		}

		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_OTG_EN),
				&param);
		if (rc < 0) {
			pr_err("failed to configure usb_5v_en gpio\n");
			goto disable_mvs_otg;
		}
		vbus_is_on = true;
		return;
	} else {
		gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_OTG_EN),
					0);
#ifdef CONFIG_USB_SWITCH_FSA9485
		fsa9485_otg_detach();
#endif
	}

disable_mvs_otg:
	regulator_disable(mvs_otg_switch);
free_usb_5v_en:
	gpio_free(PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_OTG_EN));
put_mvs_otg:
	regulator_put(mvs_otg_switch);
	vbus_is_on = false;
}

#ifdef CONFIG_CHARGER_SMB347
static void msm_hsusb_vbus_power_smb347s(bool on)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;
	int ret = 0;

	pr_info("%s, attached %d, vbus_is_on %d\n", __func__, on, vbus_is_on);

	/* If VBUS is already on (or off), do nothing. */
	if (vbus_is_on == on)
		return;

	if (on)
		value.intval = POWER_SUPPLY_CAPACITY_OTG_ENABLE;
	else
		value.intval = POWER_SUPPLY_CAPACITY_OTG_DISABLE;

	if (psy) {
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_OTG, &value);
		if (ret) {
			pr_err("%s: fail to set power_suppy otg property(%d)\n",
				__func__, ret);
		}
#ifdef CONFIG_USB_SWITCH_FSA9485
		if (!on)
			fsa9485_otg_detach();
#endif
		vbus_is_on = on;
	} else {
		pr_err("%s : psy is null!\n", __func__);
	}
}
#endif

static int msm_hsusb_vbus_power(bool on)
{
	if (system_rev < BOARD_REV04)
		msm_hsusb_vbus_power_max8627(on);
#ifdef CONFIG_CHARGER_SMB347
	else
		msm_hsusb_vbus_power_smb347s(on);
#endif
	return 0;
}

static int phy_settings[] = {
	0x44, 0x80,
	0x7F, 0x81,
	0x3C, 0x82,
	0x13, 0x83,
	-1,
};

static int wr_phy_init_seq[] = {
	0x44, 0x80, /* set VBUS valid threshold
			and disconnect valid threshold */
	0x38, 0x81, /* update DC voltage level */
	0x14, 0x82, /* set preemphasis and rise/fall time */
	0x13, 0x83, /* set source impedance adjusment */
	-1};

static int liquid_v1_phy_init_seq[] = {
	0x44, 0x80,/* set VBUS valid threshold
			and disconnect valid threshold */
	0x3C, 0x81,/* update DC voltage level */
	0x18, 0x82,/* set preemphasis and rise/fall time */
	0x23, 0x83,/* set source impedance sdjusment */
	-1};

#ifdef CONFIG_MSM_BUS_SCALING
/* Bandwidth requests (zero) if no vote placed */
static struct msm_bus_vectors usb_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

/* Bus bandwidth requests in Bytes/sec */
static struct msm_bus_vectors usb_max_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 60000000,		/* At least 480Mbps on bus. */
		.ib = 960000000,	/* MAX bursts rate */
	},
};

static struct msm_bus_paths usb_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(usb_init_vectors),
		usb_init_vectors,
	},
	{
		ARRAY_SIZE(usb_max_vectors),
		usb_max_vectors,
	},
};

static struct msm_bus_scale_pdata usb_bus_scale_pdata = {
	usb_bus_scale_usecases,
	ARRAY_SIZE(usb_bus_scale_usecases),
	.name = "usb",
};
#endif

static struct msm_otg_platform_data msm_otg_pdata = {
	.mode			= USB_OTG,
	.otg_control		= OTG_PMIC_CONTROL,
	.phy_type		= SNPS_28NM_INTEGRATED_PHY,
	.pmic_id_irq		= PM8921_USB_ID_IN_IRQ(PM8921_IRQ_BASE),
	.vbus_power		= msm_hsusb_vbus_power,
	.power_budget		= 750,
	.phy_init_seq = phy_settings,
	.smb347s		= true,
#ifdef CONFIG_MSM_BUS_SCALING
	.bus_scale_table	= &usb_bus_scale_pdata,
#endif
};

#ifdef CONFIG_USB_HOST_NOTIFY
static void __init msm_otg_power_init(void)
{
	if (system_rev >= BOARD_REV02) {
		msm_otg_pdata.otg_power_gpio =
			PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_OTG_POWER);
		msm_otg_pdata.otg_power_irq =
			PM8921_GPIO_IRQ(PM8921_IRQ_BASE, PMIC_GPIO_OTG_POWER);
	}
	if (system_rev >= BOARD_REV04)
		msm_otg_pdata.smb347s = true;
	else
		msm_otg_pdata.smb347s = false;
}
#endif

#ifdef CONFIG_USB_EHCI_MSM_HSIC
#define HSIC_HUB_RESET_GPIO	91
static struct msm_hsic_host_platform_data msm_hsic_pdata = {
	.strobe		= 150,
	.data		= 151,
};
#else
static struct msm_hsic_host_platform_data msm_hsic_pdata;
#endif

#define PID_MAGIC_ID		0x71432909
#define SERIAL_NUM_MAGIC_ID	0x61945374
#define SERIAL_NUMBER_LENGTH	127
#define DLOAD_USB_BASE_ADD	0x2A03F0C8

struct magic_num_struct {
	uint32_t pid;
	uint32_t serial_num;
};

struct dload_struct {
	uint32_t	reserved1;
	uint32_t	reserved2;
	uint32_t	reserved3;
	uint16_t	reserved4;
	uint16_t	pid;
	char		serial_number[SERIAL_NUMBER_LENGTH];
	uint16_t	reserved5;
	struct magic_num_struct magic_struct;
};

static int usb_diag_update_pid_and_serial_num(uint32_t pid, const char *snum)
{
	struct dload_struct __iomem *dload = 0;

	dload = ioremap(DLOAD_USB_BASE_ADD, sizeof(*dload));
	if (!dload) {
		pr_err("%s: cannot remap I/O memory region: %08x\n",
					__func__, DLOAD_USB_BASE_ADD);
		return -ENXIO;
	}

	pr_debug("%s: dload:%p pid:%x serial_num:%s\n",
				__func__, dload, pid, snum);
	/* update pid */
	dload->magic_struct.pid = PID_MAGIC_ID;
	dload->pid = pid;

	/* update serial number */
	dload->magic_struct.serial_num = 0;
	if (!snum) {
		memset(dload->serial_number, 0, SERIAL_NUMBER_LENGTH);
		goto out;
	}

	dload->magic_struct.serial_num = SERIAL_NUM_MAGIC_ID;
	strlcpy(dload->serial_number, snum, SERIAL_NUMBER_LENGTH);
out:
	iounmap(dload);
	return 0;
}

static struct android_usb_platform_data android_usb_pdata = {
	.update_pid_and_serial_num = usb_diag_update_pid_and_serial_num,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

static uint8_t spm_wfi_cmd_sequence[] __initdata = {
	0x03, 0x0f,
};

static uint8_t spm_retention_cmd_sequence[] __initdata = {
	0x00, 0x05, 0x03, 0x0D,
	0x0B, 0x00, 0x0f,
};

static uint8_t spm_power_collapse_without_rpm[] __initdata = {
	0x00, 0x24, 0x54, 0x10,
	0x09, 0x03, 0x01,
	0x10, 0x54, 0x30, 0x0C,
	0x24, 0x30, 0x0f,
};

static uint8_t spm_power_collapse_with_rpm[] __initdata = {
	0x00, 0x24, 0x54, 0x10,
	0x09, 0x07, 0x01, 0x0B,
	0x10, 0x54, 0x30, 0x0C,
	0x24, 0x30, 0x0f,
};

/* 8960AB has a different command to assert apc_pdn */
static uint8_t spm_power_collapse_without_rpm_krait_v3[] __initdata = {
	0x00, 0x24, 0x84, 0x10,
	0x09, 0x03, 0x01,
	0x10, 0x84, 0x30, 0x0C,
	0x24, 0x30, 0x0f,
};

static uint8_t spm_power_collapse_with_rpm_krait_v3[] __initdata = {
	0x00, 0x24, 0x84, 0x10,
	0x09, 0x07, 0x01, 0x0B,
	0x10, 0x84, 0x30, 0x0C,
	0x24, 0x30, 0x0f,
};

static struct msm_spm_seq_entry msm_spm_boot_cpu_seq_list[] __initdata = {
	[0] = {
		.mode = MSM_SPM_MODE_CLOCK_GATING,
		.notify_rpm = false,
		.cmd = spm_wfi_cmd_sequence,
	},

	[1] = {
		.mode = MSM_SPM_MODE_POWER_RETENTION,
		.notify_rpm = false,
		.cmd = spm_retention_cmd_sequence,
	},

	[2] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = false,
		.cmd = spm_power_collapse_without_rpm,
	},
	[3] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = true,
		.cmd = spm_power_collapse_with_rpm,
	},
};

static struct msm_spm_seq_entry msm_spm_nonboot_cpu_seq_list[] __initdata = {
	[0] = {
		.mode = MSM_SPM_MODE_CLOCK_GATING,
		.notify_rpm = false,
		.cmd = spm_wfi_cmd_sequence,
	},

	[1] = {
		.mode = MSM_SPM_MODE_POWER_RETENTION,
		.notify_rpm = false,
		.cmd = spm_retention_cmd_sequence,
	},

	[2] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = false,
		.cmd = spm_power_collapse_without_rpm,
	},

	[3] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = true,
		.cmd = spm_power_collapse_with_rpm,
	},
};

static struct msm_spm_platform_data msm_spm_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
#if defined(CONFIG_MSM_AVS_HW)
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_CTL] = 0x58589464,
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_HYSTERESIS] = 0x00020000,
#endif
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x03020004,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x0084009C,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x00A4001C,
		.vctl_timeout_us = 50,
		.num_modes = ARRAY_SIZE(msm_spm_boot_cpu_seq_list),
		.modes = msm_spm_boot_cpu_seq_list,
	},
	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
#if defined(CONFIG_MSM_AVS_HW)
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_CTL] = 0x58589464,
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_HYSTERESIS] = 0x00020000,
#endif
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x03020004,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x0084009C,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x00A4001C,
		.vctl_timeout_us = 50,
		.num_modes = ARRAY_SIZE(msm_spm_nonboot_cpu_seq_list),
		.modes = msm_spm_nonboot_cpu_seq_list,
	},
};

static uint8_t l2_spm_wfi_cmd_sequence[] __initdata = {
			0x00, 0x20, 0x03, 0x20,
			0x00, 0x0f,
};

static uint8_t l2_spm_gdhs_cmd_sequence[] __initdata = {
			0x00, 0x20, 0x34, 0x64,
			0x48, 0x07, 0x48, 0x20,
			0x50, 0x64, 0x04, 0x34,
			0x50, 0x0f,
};
static uint8_t l2_spm_power_off_cmd_sequence[] __initdata = {
			0x00, 0x10, 0x34, 0x64,
			0x48, 0x07, 0x48, 0x10,
			0x50, 0x64, 0x04, 0x34,
			0x50, 0x0F,
};

static struct msm_spm_seq_entry msm_spm_l2_seq_list[] __initdata = {
	[0] = {
		.mode = MSM_SPM_L2_MODE_RETENTION,
		.notify_rpm = false,
		.cmd = l2_spm_wfi_cmd_sequence,
	},
	[1] = {
		.mode = MSM_SPM_L2_MODE_GDHS,
		.notify_rpm = true,
		.cmd = l2_spm_gdhs_cmd_sequence,
	},
	[2] = {
		.mode = MSM_SPM_L2_MODE_POWER_COLLAPSE,
		.notify_rpm = true,
		.cmd = l2_spm_power_off_cmd_sequence,
	},
};

static struct msm_spm_platform_data msm_spm_l2_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW_L2_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x02020204,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x00A000AE,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x00A00020,
		.modes = msm_spm_l2_seq_list,
		.num_modes = ARRAY_SIZE(msm_spm_l2_seq_list),
	},
};

#define PM_HAP_EN_GPIO		PM8921_GPIO_PM_TO_SYS(33)
#define PM_HAP_LEN_GPIO		PM8921_GPIO_PM_TO_SYS(20)

static struct msm_xo_voter *xo_handle_d1;

static int isa1200_power(int on)
{
	int rc = 0;

	gpio_set_value(HAP_SHIFT_LVL_OE_GPIO, !!on);

	rc = on ? msm_xo_mode_vote(xo_handle_d1, MSM_XO_MODE_ON) :
			msm_xo_mode_vote(xo_handle_d1, MSM_XO_MODE_OFF);
	if (rc < 0) {
		pr_err("%s: failed to %svote for TCXO D1 buffer%d\n",
				__func__, on ? "" : "de-", rc);
		goto err_xo_vote;
	}

	return 0;

err_xo_vote:
	gpio_set_value(HAP_SHIFT_LVL_OE_GPIO, !on);
	return rc;
}

static int isa1200_dev_setup(bool enable)
{
	int rc = 0;

	struct pm_gpio hap_gpio_config = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_NO,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
		.vin_sel        = 2,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
	};

	if (enable == true) {
		rc = pm8xxx_gpio_config(PM_HAP_EN_GPIO, &hap_gpio_config);
		if (rc) {
			pr_err("%s: pm8921 gpio %d config failed(%d)\n",
					__func__, PM_HAP_EN_GPIO, rc);
			return rc;
		}

		rc = pm8xxx_gpio_config(PM_HAP_LEN_GPIO, &hap_gpio_config);
		if (rc) {
			pr_err("%s: pm8921 gpio %d config failed(%d)\n",
					__func__, PM_HAP_LEN_GPIO, rc);
			return rc;
		}

		rc = gpio_request(HAP_SHIFT_LVL_OE_GPIO, "hap_shft_lvl_oe");
		if (rc) {
			pr_err("%s: unable to request gpio %d (%d)\n",
					__func__, HAP_SHIFT_LVL_OE_GPIO, rc);
			return rc;
		}

		rc = gpio_direction_output(HAP_SHIFT_LVL_OE_GPIO, 0);
		if (rc) {
			pr_err("%s: Unable to set direction\n", __func__);
			goto free_gpio;
		}

		xo_handle_d1 = msm_xo_get(MSM_XO_TCXO_D1, "isa1200");
		if (IS_ERR(xo_handle_d1)) {
			rc = PTR_ERR(xo_handle_d1);
			pr_err("%s: failed to get the handle for D1(%d)\n",
							__func__, rc);
			goto gpio_set_dir;
		}
	} else {
		gpio_free(HAP_SHIFT_LVL_OE_GPIO);

		msm_xo_put(xo_handle_d1);
	}

	return 0;

gpio_set_dir:
	gpio_set_value(HAP_SHIFT_LVL_OE_GPIO, 0);
free_gpio:
	gpio_free(HAP_SHIFT_LVL_OE_GPIO);
	return rc;
}

static struct isa1200_regulator isa1200_reg_data[] = {
	{
		.name = "vcc_i2c",
		.min_uV = ISA_I2C_VTG_MIN_UV,
		.max_uV = ISA_I2C_VTG_MAX_UV,
		.load_uA = ISA_I2C_CURR_UA,
	},
};

static struct isa1200_platform_data isa1200_1_pdata = {
	.name = "vibrator",
	.dev_setup = isa1200_dev_setup,
	.power_on = isa1200_power,
	.hap_en_gpio = PM_HAP_EN_GPIO,
	.hap_len_gpio = PM_HAP_LEN_GPIO,
	.max_timeout = 15000,
	.mode_ctrl = PWM_GEN_MODE,
	.pwm_fd = {
		.pwm_div = 256,
	},
	.is_erm = false,
	.smart_en = true,
	.ext_clk_en = true,
	.chip_en = 1,
	.regulator_info = isa1200_reg_data,
	.num_regulators = ARRAY_SIZE(isa1200_reg_data),
};

static struct i2c_board_info msm_isa1200_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("isa1200_1", 0x90>>1),
		.platform_data = &isa1200_1_pdata,
	},
};

#ifdef CONFIG_NFC_PN544
static struct i2c_gpio_platform_data pn544_i2c_gpio_data = {
	.sda_pin = GPIO_NFC_SDA,
	.scl_pin = GPIO_NFC_SCL,
	.udelay = 5,
};

static struct platform_device pn544_i2c_gpio_device = {
	.name = "i2c-gpio",
	.id = MSM_NFC_I2C_BUS_ID,
	.dev = {
		.platform_data  = &pn544_i2c_gpio_data,
	},
};

static struct pn544_i2c_platform_data pn544_pdata = {
	.conf_gpio = pn544_conf_gpio,
	.irq_gpio = GPIO_NFC_IRQ,
	.ven_gpio = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_EN),
	.firm_gpio = GPIO_NFC_FIRMWARE,
};

static struct i2c_board_info pn544_info[] __initdata = {
	{
		I2C_BOARD_INFO("pn544", 0x2b),
		.irq = MSM_GPIO_TO_INT(GPIO_NFC_IRQ),
		.platform_data = &pn544_pdata,
	},
};
#endif /* CONFIG_NFC_PN544	*/

/* configuration data */
static const u8 mxt_config_data[] = {
	/* T6 Object */
	0, 0, 0, 0, 0, 0,
	/* T38 Object */
	11, 2, 0, 11, 11, 11, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T7 Object */
	100, 16, 50,
	/* T8 Object */
	8, 0, 0, 0, 0, 0, 8, 14, 50, 215,
	/* T9 Object */
	131, 0, 0, 26, 42, 0, 32, 63, 3, 5,
	0, 2, 1, 113, 10, 10, 8, 10, 255, 2,
	85, 5, 0, 0, 20, 20, 75, 25, 202, 29,
	10, 10, 45, 46,
	/* T15 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
	/* T18 Object */
	0, 0,
	/* T22 Object */
	5, 0, 0, 0, 0, 0, 0, 0, 30, 0,
	0, 0, 5, 8, 10, 13, 0,
	/* T24 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* T25 Object */
	3, 0, 188, 52, 52, 33, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T27 Object */
	0, 0, 0, 0, 0, 0, 0,
	/* T28 Object */
	0, 0, 0, 8, 12, 60,
	/* T40 Object */
	0, 0, 0, 0, 0,
	/* T41 Object */
	0, 0, 0, 0, 0, 0,
	/* T43 Object */
	0, 0, 0, 0, 0, 0,
};

static void mxt_init_hw_liquid(void)
{
	int rc;

	rc = gpio_request(GPIO_MXT_TS_IRQ, "mxt_ts_irq_gpio");
	if (rc) {
		pr_err("%s: unable to request mxt_ts_irq gpio [%d]\n",
				__func__, GPIO_MXT_TS_IRQ);
		return;
	}

	rc = gpio_direction_input(GPIO_MXT_TS_IRQ);
	if (rc) {
		pr_err("%s: unable to set_direction for mxt_ts_irq gpio [%d]\n",
				__func__, GPIO_MXT_TS_IRQ);
		goto err_irq_gpio_req;
	}

	rc = gpio_request(GPIO_MXT_TS_LDO_EN, "mxt_ldo_en_gpio");
	if (rc) {
		pr_err("%s: unable to request mxt_ldo_en gpio [%d]\n",
				__func__, GPIO_MXT_TS_LDO_EN);
		goto err_irq_gpio_req;
	}

	rc = gpio_direction_output(GPIO_MXT_TS_LDO_EN, 1);
	if (rc) {
		pr_err("%s: unable to set_direction for mxt_ldo_en gpio [%d]\n",
				__func__, GPIO_MXT_TS_LDO_EN);
		goto err_ldo_gpio_req;
	}
#if !defined(CONFIG_WIRELESS_CHARGING)
	rc = gpio_request(GPIO_MXT_TS_RESET, "mxt_reset_gpio");
	if (rc) {
		pr_err("%s: unable to request mxt_reset gpio [%d]\n",
				__func__, GPIO_MXT_TS_RESET);
		goto err_ldo_gpio_set_dir;
	}

	rc = gpio_direction_output(GPIO_MXT_TS_RESET, 1);
	if (rc) {
		pr_err("%s: unable to set_direction for mxt_reset gpio [%d]\n",
				__func__, GPIO_MXT_TS_RESET);
		goto err_reset_gpio_req;
	}
#endif
	return;

err_ldo_gpio_req:
	gpio_free(GPIO_MXT_TS_LDO_EN);
err_irq_gpio_req:
	gpio_free(GPIO_MXT_TS_IRQ);
}
#if 0
static struct mxt_platform_data mxt_platform_data = {
	.config			= mxt_config_data,
	.config_length		= ARRAY_SIZE(mxt_config_data),
	.irqflags		= IRQF_TRIGGER_FALLING,
	.x_size			= 1365,
	.y_size			= 767,
	.i2c_pull_up		= true,
};

static struct i2c_board_info mxt_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("atmel_mxt_ts", 0x5b),
		.platform_data = &mxt_platform_data,
		.irq = MSM_GPIO_TO_INT(GPIO_MXT_TS_IRQ),
	},
};
#endif
#ifndef CONFIG_SLIMBUS_MSM_CTRL
#define TABLA_I2C_SLAVE_ADDR	0x0d
#define TABLA_ANALOG_I2C_SLAVE_ADDR	0x77
#define TABLA_DIGITAL1_I2C_SLAVE_ADDR	0x66
#define TABLA_DIGITAL2_I2C_SLAVE_ADDR	0x55

static struct i2c_board_info tabla_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("tabla top level", TABLA_I2C_SLAVE_ADDR),
		.platform_data = &tabla_i2c_platform_data,
	},
	{
		I2C_BOARD_INFO("tabla analog", TABLA_ANALOG_I2C_SLAVE_ADDR),
		.platform_data = &tabla_i2c_platform_data,
	},
	{
		I2C_BOARD_INFO("tabla digital1", TABLA_DIGITAL1_I2C_SLAVE_ADDR),
		.platform_data = &tabla_i2c_platform_data,
	},
	{
		I2C_BOARD_INFO("tabla digital2", TABLA_DIGITAL2_I2C_SLAVE_ADDR),
		.platform_data = &tabla_i2c_platform_data,
	},
};
#endif
static struct i2c_board_info sii_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("Sil-9244", 0x39),
		.flags = I2C_CLIENT_WAKE,
		.irq = MSM_GPIO_TO_INT(15),
	},
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi4_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
};

#ifndef CONFIG_SLIMBUS_MSM_CTRL
static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi1_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
};
#endif

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi3_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi7_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi10_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
};
#ifdef CONFIG_VP_A2220
static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi8_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
};
#endif
static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi12_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
};
#if 0
static struct msm_rpm_platform_data msm_rpm_data = {
	.reg_base_addrs = {
		[MSM_RPM_PAGE_STATUS] = MSM_RPM_BASE,
		[MSM_RPM_PAGE_CTRL] = MSM_RPM_BASE + 0x400,
		[MSM_RPM_PAGE_REQ] = MSM_RPM_BASE + 0x600,
		[MSM_RPM_PAGE_ACK] = MSM_RPM_BASE + 0xa00,
	},

	.irq_ack = RPM_APCC_CPU0_GP_HIGH_IRQ,
	.irq_err = RPM_APCC_CPU0_GP_LOW_IRQ,
	.irq_vmpm = RPM_APCC_CPU0_GP_MEDIUM_IRQ,
	.msm_apps_ipc_rpm_reg = MSM_APCS_GCC_BASE + 0x008,
	.msm_apps_ipc_rpm_val = 4,
};
#endif

#ifndef CONFIG_S5C73M3
static struct ks8851_pdata spi_eth_pdata = {
	.irq_gpio = KS8851_IRQ_GPIO,
	.rst_gpio = KS8851_RST_GPIO,
};

static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias               = "ks8851",
		.irq                    = MSM_GPIO_TO_INT(KS8851_IRQ_GPIO),
		.max_speed_hz           = 19200000,
		.bus_num                = 0,
		.chip_select            = 0,
		.mode                   = SPI_MODE_0,
		.platform_data		= &spi_eth_pdata
	},
	{
		.modalias               = "dsi_novatek_3d_panel_spi",
		.max_speed_hz           = 10800000,
		.bus_num                = 0,
		.chip_select            = 1,
		.mode                   = SPI_MODE_0,
	},
};
#endif
static struct platform_device msm_device_saw_core0 = {
	.name          = "saw-regulator",
	.id            = 0,
	.dev	= {
		.platform_data = &msm_saw_regulator_pdata_s5,
	},
};

static struct platform_device msm_device_saw_core1 = {
	.name          = "saw-regulator",
	.id            = 1,
	.dev	= {
		.platform_data = &msm_saw_regulator_pdata_s6,
	},
};

static struct tsens_platform_data msm_tsens_pdata  = {
		.slope			= {910, 910, 910, 910, 910},
		.tsens_factor		= 1000,
		.hw_type		= MSM_8960,
		.tsens_num_sensor	= 5,
};

static struct platform_device msm_tsens_device = {
	.name   = "tsens8960-tm",
	.id = -1,
};

static struct msm_thermal_data msm_thermal_pdata = {
	.sensor_id = 0,
	.poll_ms = 250,
	.limit_temp_degC = 60,
	.temp_hysteresis_degC = 10,
	.freq_step = 2,
};

/* Bluetooth */
#ifdef CONFIG_BT_BCM4334
static struct platform_device bcm4334_bluetooth_device = {
	.name = "bcm4334_bluetooth",
	.id = -1,
};
#endif

#ifdef CONFIG_MSM_FAKE_BATTERY
static struct platform_device fish_battery_device = {
	.name = "fish_battery",
};
#endif

static struct platform_device msm8960_device_ext_5v_vreg __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= PM8921_MPP_PM_TO_SYS(7),
	.dev	= {
		.platform_data = &msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_5V],
	},
};
#if 0
static struct platform_device msm8960_device_ext_l2_vreg __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= 91,
	.dev	= {
		.platform_data = &msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_L2],
	},
};
#endif
static struct platform_device msm8960_device_ext_3p3v_vreg __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= PM8921_GPIO_PM_TO_SYS(17),
	.dev	= {
		.platform_data =
			&msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_3P3V],
	},
};
#ifdef CONFIG_KEYBOARD_GPIO
static struct gpio_keys_button gpio_keys_button[] = {
	{
		.code			= KEY_VOLUMEUP,
		.type			= EV_KEY,
		.gpio			= -1,
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= 5, /* ms */
		.desc			= "Vol Up",
	},
	{
		.code			= KEY_VOLUMEDOWN,
		.type			= EV_KEY,
		.gpio			= -1,
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= 5, /* ms */
		.desc			= "Vol Down",
	},
	{
		.code			= KEY_HOMEPAGE,
		.type			= EV_KEY,
		.gpio			= -1,
		.active_low		= 1,
		.wakeup			= 1,
		.debounce_interval	= 5, /* ms */
		.desc			= "Home",
	},
};
static struct gpio_keys_platform_data gpio_keys_platform_data = {
	.buttons	= gpio_keys_button,
	.nbuttons	= ARRAY_SIZE(gpio_keys_button),
	.rep		= 0,
};

static struct platform_device msm8960_gpio_keys_device = {
	.name	= "sec_keys",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_keys_platform_data,
	}
};
#endif

static struct platform_device msm8960_device_ext_otg_sw_vreg __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= PM8921_GPIO_PM_TO_SYS(42),
	.dev	= {
		.platform_data =
			&msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_OTG_SW],
	},
};

static struct platform_device msm8960_device_rpm_regulator __devinitdata = {
	.name	= "rpm-regulator",
	.id	= -1,
	.dev	= {
		.platform_data = &msm_rpm_regulator_pdata,
	},
};

static struct msm_rpm_log_platform_data msm_rpm_log_pdata = {
	.phys_addr_base = 0x0010C000,
	.reg_offsets = {
		[MSM_RPM_LOG_PAGE_INDICES] = 0x00000080,
		[MSM_RPM_LOG_PAGE_BUFFER]  = 0x000000A0,
	},
	.phys_size = SZ_8K,
	.log_len = 4096,		  /* log's buffer length in bytes */
	.log_len_mask = (4096 >> 2) - 1,  /* length mask in units of u32 */
};

static struct platform_device msm_rpm_log_device = {
	.name	= "msm_rpm_log",
	.id	= -1,
	.dev	= {
		.platform_data = &msm_rpm_log_pdata,
	},
};

#ifdef CONFIG_SAMSUNG_JACK
#define PMIC_GPIO_EAR_DET		36
#define PMIC_GPIO_SHORT_SENDEND		32
#define PMIC_GPIO_EAR_MICBIAS_EN	3

static struct sec_jack_zone jack_zones[] = {
	[0] = {
		.adc_high	= 3,
		.delay_ms	= 10,
		.check_count	= 10,
		.jack_type	= SEC_HEADSET_3POLE,
	},
	[1] = {
		.adc_high	= 630,
		.delay_ms	= 10,
		.check_count	= 10,
		.jack_type	= SEC_HEADSET_3POLE,
	},
	[2] = {
		.adc_high	= 1720,
		.delay_ms	= 10,
		.check_count	= 10,
		.jack_type	= SEC_HEADSET_4POLE,
	},
	[3] = {
		.adc_high	= 9999,
		.delay_ms	= 10,
		.check_count	= 10,
		.jack_type	= SEC_HEADSET_4POLE,
	},
};

/* To support 3-buttons earjack */
static struct sec_jack_buttons_zone jack_buttons_zones[] = {
	{
		.code		= KEY_MEDIA,
		.adc_low	= 0,
		.adc_high	= 93,
	},
	{
		.code		= KEY_VOLUMEUP,
		.adc_low	= 94,
		.adc_high	= 217,
	},
	{
		.code		= KEY_VOLUMEDOWN,
		.adc_low	= 218,
		.adc_high	= 450,
	},
};

static int get_sec_det_jack_state(void)
{
	return (gpio_get_value_cansleep(
		PM8921_GPIO_PM_TO_SYS(
		PMIC_GPIO_EAR_DET))) ^ 1;
}

static int get_sec_send_key_state(void)
{
	struct pm_gpio ear_micbiase = {
		.direction		= PM_GPIO_DIR_OUT,
		.pull			= PM_GPIO_PULL_NO,
		.out_strength		= PM_GPIO_STRENGTH_HIGH,
		.function		= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol		= 0,
		.vin_sel		= PM_GPIO_VIN_S4,
		.output_buffer		= PM_GPIO_OUT_BUF_CMOS,
		.output_value		= 0,
	};

	if (get_sec_det_jack_state()) {
		pm8xxx_gpio_config(
			PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_MICBIAS_EN),
			&ear_micbiase);
		gpio_set_value_cansleep(
			PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_MICBIAS_EN),
			1);
	}
	return (gpio_get_value_cansleep(
		PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_SHORT_SENDEND))) ^ 1;

	return 0;
}

static void set_sec_micbias_state(bool state)
{
	pr_info("sec_jack: ear micbias %s\n", state ? "on" : "off");
	gpio_set_value_cansleep(
		PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_MICBIAS_EN),
		state);
}

static int sec_jack_get_adc_value(void)
{
	int rc = 0;
	int retVal = 0;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_mpp_config_read(
			PM8XXX_AMUX_MPP_3,
			ADC_MPP_1_AMUX6_SCALE_DEFAULT,
			&result);
	if (rc) {
		pr_err("%s : error reading mpp %d, rc = %d\n",
			__func__, PM8XXX_AMUX_MPP_3, rc);
		return rc;
	}
	retVal = ((int)result.physical)/1000;
	return retVal;
}

static struct sec_jack_platform_data sec_jack_data = {
	.get_det_jack_state	= get_sec_det_jack_state,
	.get_send_key_state	= get_sec_send_key_state,
	.set_micbias_state	= set_sec_micbias_state,
	.get_adc_value		= sec_jack_get_adc_value,
	.zones			= jack_zones,
	.num_zones		= ARRAY_SIZE(jack_zones),
	.buttons_zones		= jack_buttons_zones,
	.num_buttons_zones	= ARRAY_SIZE(jack_buttons_zones),
	.det_int		= PM8921_GPIO_IRQ(PM8921_IRQ_BASE,
						PMIC_GPIO_EAR_DET),
	.send_int		= PM8921_GPIO_IRQ(PM8921_IRQ_BASE,
						PMIC_GPIO_SHORT_SENDEND),
};

static struct platform_device sec_device_jack = {
	.name           = "sec_jack",
	.id             = -1,
	.dev            = {
		.platform_data  = &sec_jack_data,
	},
};
#endif

static struct platform_device *common_devices[] __initdata = {
	&msm8960_device_dmov,
	&msm_device_smd,
	&msm8960_device_uart_gsbi5,
	&msm_device_uart_dm6,
	&msm_device_saw_core0,
	&msm_device_saw_core1,
	&msm8960_device_ext_5v_vreg,
	&msm8960_device_ssbi_pmic,
	&msm8960_device_ext_otg_sw_vreg,
#ifndef CONFIG_SLIMBUS_MSM_CTRL
	&msm8960_device_qup_i2c_gsbi1,
#endif
#ifdef CONFIG_S5C73M3
	&msm8960_device_qup_spi_gsbi11,
#endif
	&msm8960_device_qup_i2c_gsbi3,
	&msm8960_device_qup_i2c_gsbi4,
	&msm8960_device_qup_i2c_gsbi7,
	&msm8960_device_qup_i2c_gsbi10,
#ifdef CONFIG_VP_A2220
	&msm8960_device_qup_i2c_gsbi8,
#endif
#ifndef CONFIG_MSM_DSPS
	&msm8960_device_qup_i2c_gsbi12,
#endif
	&msm_slim_ctrl,
	&msm_device_wcnss_wlan,
#if defined(CONFIG_QSEECOM)
	&qseecom_device,
#endif
#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
	defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)
	&qcrypto_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
	defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
	&qcedev_device,
#endif
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
	&msm_device_sps,
#ifdef CONFIG_MSM_FAKE_BATTERY
	&fish_battery_device,
#endif
	&msm8960_fmem_device,
#ifdef CONFIG_KEYBOARD_GPIO
	&msm8960_gpio_keys_device,
#endif
	&msm_device_vidc,
	&msm_device_bam_dmux,
	&msm_fm_platform_init,

#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
#ifdef CONFIG_MSM_USE_TSIF1
	&msm_device_tsif[1],
#else
	&msm_device_tsif[0],
#endif
#endif

#ifdef CONFIG_HW_RANDOM_MSM
	&msm_device_rng,
#endif
#ifdef CONFIG_ION_MSM
	&msm8960_ion_dev,
#endif
	&msm8960_rpm_device,
	&msm_rpm_log_device,
	&msm8960_rpm_stat_device,
	&msm_device_tz_log,
#if 0
#ifdef CONFIG_MSM_QDSS
	&msm_etb_device,
	&msm_tpiu_device,
	&msm_funnel_device,
	&msm_etm_device,
#endif
#endif
	&msm_device_dspcrashd_8960,
	&msm8960_device_watchdog,
#ifdef CONFIG_MSM_RTB
	&msm_rtb_device,
#endif
	&msm8960_device_cache_erp,
#ifdef CONFIG_MSM_CACHE_DUMP
	&msm_cache_dump_device,
#endif
	&msm8960_iommu_domain_device,
	&msm_tsens_device,
	&msm8960_cpu_slp_status,
};

static struct platform_device *m2_att_devices[] __initdata = {
	&msm_8960_q6_lpass,
	&msm_8960_q6_mss_sw,
	&msm_8960_q6_mss_fw,
	&msm_8960_riva,
	&msm_pil_tzapps,
	&msm_pil_vidc,
	&msm8960_device_otg,
	&msm8960_device_gadget_peripheral,
	&msm_device_hsusb_host,
	&android_usb_device,
	&msm_pcm,
	&msm_multi_ch_pcm,
	&msm_lowlatency_pcm,
	&msm_pcm_routing,
#ifdef CONFIG_SLIMBUS_MSM_CTRL
	&msm_cpudai0,
	&msm_cpudai1,
#else
	&msm_i2s_cpudai0,
	&msm_i2s_cpudai1,
#endif
	&msm_cpudai_hdmi_rx,
	&msm_cpudai_bt_rx,
	&msm_cpudai_bt_tx,
	&msm_cpudai_fm_rx,
	&msm_cpudai_fm_tx,
	&msm_cpudai_auxpcm_rx,
	&msm_cpudai_auxpcm_tx,
	&msm_cpu_fe,
	&msm_stub_codec,
#ifdef CONFIG_MSM_GEMINI
	&msm8960_gemini_device,
#endif
	&msm_voice,
	&msm_voip,
	&msm_lpa_pcm,
	&msm_cpudai_afe_01_rx,
	&msm_cpudai_afe_01_tx,
	&msm_cpudai_afe_02_rx,
	&msm_cpudai_afe_02_tx,
	&msm_pcm_afe,
	&msm_compr_dsp,

#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
	&mhl_i2c_gpio_device,
#endif
#ifdef CONFIG_USB_SWITCH_FSA9485
	&fsa_i2c_gpio_device,
#endif
#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH_236
	&touchkey_i2c_gpio_device,
#endif
#ifdef CONFIG_BATTERY_MAX17040
	&fuelgauge_i2c_gpio_device,
#endif
#ifdef CONFIG_BATTERY_SEC
	&sec_device_battery,
#endif
#ifdef CONFIG_SAMSUNG_JACK
	&sec_device_jack,
#endif
	&msm_cpudai_incall_music_rx,
	&msm_cpudai_incall_record_rx,
	&msm_cpudai_incall_record_tx,
	&msm_pcm_hostless,
	&msm_bus_apps_fabric,
	&msm_bus_sys_fabric,
	&msm_bus_mm_fabric,
	&msm_bus_sys_fpb,
	&msm_bus_cpss_fpb,
	&pn544_i2c_gpio_device,
#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F) \
	|| defined(CONFIG_SENSORS_CM36651)
	&opt_i2c_gpio_device,
#if 0
#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F)
	&opt_gp2a,
#endif
#endif
#endif
#ifdef CONFIG_BT_BCM4334
	&bcm4334_bluetooth_device,
#endif
#ifdef CONFIG_VIBETONZ
	&vibetonz_device,
#endif /* CONFIG_VIBETONZ */
};

static void __init msm8960_i2c_init(void)
{
	msm8960_device_qup_i2c_gsbi4.dev.platform_data =
					&msm8960_i2c_qup_gsbi4_pdata;

#ifndef CONFIG_SLIMBUS_MSM_CTRL
	msm8960_device_qup_i2c_gsbi1.dev.platform_data =
					&msm8960_i2c_qup_gsbi1_pdata;
#endif

	msm8960_device_qup_i2c_gsbi7.dev.platform_data =
					&msm8960_i2c_qup_gsbi7_pdata;

	msm8960_device_qup_i2c_gsbi3.dev.platform_data =
					&msm8960_i2c_qup_gsbi3_pdata;

	msm8960_device_qup_i2c_gsbi10.dev.platform_data =
					&msm8960_i2c_qup_gsbi10_pdata;
#ifdef CONFIG_VP_A2220
	msm8960_device_qup_i2c_gsbi8.dev.platform_data =
					&msm8960_i2c_qup_gsbi8_pdata;
#endif
	msm8960_device_qup_i2c_gsbi12.dev.platform_data =
					&msm8960_i2c_qup_gsbi12_pdata;
}

static void __init msm8960_gfx_init(void)
{
	struct kgsl_device_platform_data *kgsl_3d0_pdata =
		msm_kgsl_3d0.dev.platform_data;
	uint32_t soc_platform_version = socinfo_get_version();

	kgsl_3d0_pdata->iommu_count = 1;

	if (SOCINFO_VERSION_MAJOR(soc_platform_version) == 1) {
		kgsl_3d0_pdata->pwrlevel[0].gpu_freq = 320000000;
		kgsl_3d0_pdata->pwrlevel[1].gpu_freq = 266667000;
	}
	if (SOCINFO_VERSION_MAJOR(soc_platform_version) >= 3) {
		/* 8960v3 GPU registers returns 5 for patch release
		 * but it should be 6, so dummy up the chipid here
		 * based the platform type
		 */
		kgsl_3d0_pdata->chipid = ADRENO_CHIPID(2, 2, 0, 6);
	}

	/* Register the 3D core */
	platform_device_register(&msm_kgsl_3d0);

	/* Register the 2D cores if we are not 8960PRO */
	if (!cpu_is_msm8960ab()) {
		platform_device_register(&msm_kgsl_2d0);
		platform_device_register(&msm_kgsl_2d1);
	}
}

static struct msm_rpmrs_level msm_rpmrs_levels[] = {
	{
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1, 784, 180000, 100,
	},

	{
		MSM_PM_SLEEP_MODE_RETENTION,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		415, 715, 340827, 475,
	},
#if defined(CONFIG_MSM_STANDALONE_POWER_COLLAPSE)
	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1300, 228, 1200000, 2000,
	},
#endif
	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, GDHS, MAX, ACTIVE),
		false,
		2000, 138, 1208400, 3200,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		6000, 119, 1850300, 9000,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, GDHS, MAX, ACTIVE),
		false,
		9200, 68, 2839200, 16400,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, MAX, ACTIVE),
		false,
		10300, 63, 3128000, 18200,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		18000, 10, 4602600, 27000,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, RET_HIGH, RET_LOW),
		false,
		20000, 2, 5752000, 32000,
	},
};

static struct msm_rpmrs_platform_data msm_rpmrs_data __initdata = {
	.levels = &msm_rpmrs_levels[0],
	.num_levels = ARRAY_SIZE(msm_rpmrs_levels),
	.vdd_mem_levels  = {
		[MSM_RPMRS_VDD_MEM_RET_LOW]	= 750000,
		[MSM_RPMRS_VDD_MEM_RET_HIGH]	= 750000,
		[MSM_RPMRS_VDD_MEM_ACTIVE]	= 1050000,
		[MSM_RPMRS_VDD_MEM_MAX]		= 1150000,
	},
	.vdd_dig_levels = {
		[MSM_RPMRS_VDD_DIG_RET_LOW]	= 500000,
		[MSM_RPMRS_VDD_DIG_RET_HIGH]	= 750000,
		[MSM_RPMRS_VDD_DIG_ACTIVE]	= 950000,
		[MSM_RPMRS_VDD_DIG_MAX]		= 1150000,
	},
	.vdd_mask = 0x7FFFFF,
	.rpmrs_target_id = {
		[MSM_RPMRS_ID_PXO_CLK]		= MSM_RPM_ID_PXO_CLK,
		[MSM_RPMRS_ID_L2_CACHE_CTL]	= MSM_RPM_ID_LAST,
		[MSM_RPMRS_ID_VDD_DIG_0]	= MSM_RPM_ID_PM8921_S3_0,
		[MSM_RPMRS_ID_VDD_DIG_1]	= MSM_RPM_ID_PM8921_S3_1,
		[MSM_RPMRS_ID_VDD_MEM_0]	= MSM_RPM_ID_PM8921_L24_0,
		[MSM_RPMRS_ID_VDD_MEM_1]	= MSM_RPM_ID_PM8921_L24_1,
		[MSM_RPMRS_ID_RPM_CTL]		= MSM_RPM_ID_RPM_CTL,
	},
};

static struct msm_pm_boot_platform_data msm_pm_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_TZ,
};

#ifdef CONFIG_I2C
#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_FLUID (1 << 4)
#define I2C_LIQUID (1 << 5)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

#ifdef CONFIG_MSM_CAMERA
static struct i2c_board_info msm_camera_boardinfo[] __initdata = {
#ifdef CONFIG_S5C73M3
	{
	I2C_BOARD_INFO("s5c73m3", 0x78>>1),
	},
#endif
#ifdef CONFIG_S5K6A3YX
	{
	I2C_BOARD_INFO("s5k6a3yx", 0x20),
	},
#endif
#ifdef CONFIG_MSM_CAMERA_FLASH_SC628A
	{
	I2C_BOARD_INFO("sc628a", 0x6E),
	},
#endif
};
#endif

/*add for D2_ATT CAM_ISP_CORE power setting by MAX8952*/
#ifdef CONFIG_REGULATOR_MAX8952
static int max8952_is_used(void)
{
	if (system_rev >= 0x3)
		return 1;
	else
		return 0;
}

static struct regulator_consumer_supply max8952_consumer =
	REGULATOR_SUPPLY("cam_isp_core", NULL);

static struct max8952_platform_data m2_att_max8952_pdata = {
	.gpio_vid0	= -1, /* NOT controlled by GPIO, HW default high*/
	.gpio_vid1	= -1, /* NOT controlled by GPIO, HW default high*/
	.gpio_en	= CAM_CORE_EN, /* Controlled by GPIO, High enable */
	.default_mode	= 3, /* vid0 = 1, vid1 = 1 */
	.dvs_mode	= { 33, 33, 33, 43 }, /* 1.1V, 1.1V, 1.1V, 1.2V*/
	.sync_freq	= 0, /* default: fastest */
	.ramp_speed	= 0, /* default: fastest */
	.reg_data	= {
		.constraints	= {
			.name		= "CAM_ISP_CORE",
			.min_uV		= 770000,
			.max_uV		= 1400000,
			.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
					  REGULATOR_CHANGE_STATUS,
			.always_on	= 0,
			.boot_on	= 0,
		},
		.num_consumer_supplies	= 1,
		.consumer_supplies	= &max8952_consumer,
	},
};
#endif /*CONFIG_REGULATOR_MAX8952*/

#ifdef CONFIG_SAMSUNG_CMC624
static struct i2c_board_info cmc624_i2c_borad_info[] = {
	{
		I2C_BOARD_INFO("cmc624", 0x38),
	},
};
#endif

#ifdef CONFIG_REGULATOR_MAX8952
static struct i2c_board_info cmc624_max8952_i2c_borad_info[] = {
	{
		I2C_BOARD_INFO("cmc624", 0x38),
	},

	{
		I2C_BOARD_INFO("max8952", 0xC0>>1),
		.platform_data = &m2_att_max8952_pdata,
	},
};
#endif /*CONFIG_REGULATOR_MAX8952*/

/* Sensors DSPS platform data */
#ifdef CONFIG_MSM_DSPS
#define DSPS_PIL_GENERIC_NAME		"dsps"
#endif /* CONFIG_MSM_DSPS */

static void __init msm8960_init_dsps(void)
{
#ifdef CONFIG_MSM_DSPS
	struct msm_dsps_platform_data *pdata =
		msm_dsps_device.dev.platform_data;
	pdata->pil_name = DSPS_PIL_GENERIC_NAME;
	pdata->gpios = NULL;
	pdata->gpios_num = 0;

	platform_device_register(&msm_dsps_device);
#endif /* CONFIG_MSM_DSPS */
}

static int hsic_peripheral_status = 1;
static DEFINE_MUTEX(hsic_status_lock);

void peripheral_connect()
{
	mutex_lock(&hsic_status_lock);
	if (hsic_peripheral_status)
		goto out;
	platform_device_add(&msm_device_hsic_host);
	hsic_peripheral_status = 1;
out:
	mutex_unlock(&hsic_status_lock);
}
EXPORT_SYMBOL(peripheral_connect);

void peripheral_disconnect()
{
	mutex_lock(&hsic_status_lock);
	if (!hsic_peripheral_status)
		goto out;
	platform_device_del(&msm_device_hsic_host);
	hsic_peripheral_status = 0;
out:
	mutex_unlock(&hsic_status_lock);
}
EXPORT_SYMBOL(peripheral_disconnect);

static void __init msm8960_init_hsic(void)
{
#ifdef CONFIG_USB_EHCI_MSM_HSIC
	uint32_t version = socinfo_get_version();

	if (SOCINFO_VERSION_MAJOR(version) == 1)
		return;

	if (PLATFORM_IS_CHARM25() || machine_is_msm8960_liquid())
		platform_device_register(&msm_device_hsic_host);
#endif
}

#ifdef CONFIG_ISL9519_CHARGER
static struct isl_platform_data isl_data __initdata = {
	.valid_n_gpio		= 0,	/* Not required when notify-by-pmic */
	.chg_detection_config	= NULL,	/* Not required when notify-by-pmic */
	.max_system_voltage	= 4200,
	.min_system_voltage	= 3200,
	.chgcurrent		= 1000, /* 1900, */
	.term_current		= 400,	/* Need fine tuning */
	.input_current		= 2048,
};

static struct i2c_board_info isl_charger_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("isl9519q", 0x9),
		.irq		= 0,	/* Not required when notify-by-pmic */
		.platform_data	= &isl_data,
	},
};
#endif /* CONFIG_ISL9519_CHARGER */

static struct i2c_registry msm8960_i2c_devices[] __initdata = {
#ifdef CONFIG_MSM_CAMERA
	{
		I2C_SURF | I2C_FFA | I2C_FLUID | I2C_LIQUID | I2C_RUMI,
		MSM_8960_GSBI4_QUP_I2C_BUS_ID,
		msm_camera_boardinfo,
		ARRAY_SIZE(msm_camera_boardinfo),
	},
#endif
#ifdef CONFIG_ISL9519_CHARGER
	{
		I2C_LIQUID,
		MSM_8960_GSBI10_QUP_I2C_BUS_ID,
		isl_charger_i2c_info,
		ARRAY_SIZE(isl_charger_i2c_info),
	},
#endif /* CONFIG_ISL9519_CHARGER */
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_MHL_I2C_BUS_ID,
		mhl_i2c_board_info,
		ARRAY_SIZE(mhl_i2c_board_info),
	},
#endif
#ifdef CONFIG_USB_SWITCH_FSA9485
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_FSA9485_I2C_BUS_ID,
		micro_usb_i2c_devices_info,
		ARRAY_SIZE(micro_usb_i2c_devices_info),
	},
#endif
#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH_236
{
	I2C_SURF | I2C_FFA | I2C_FLUID,
	MSM_TOUCHKEY_I2C_BUS_ID,
	touchkey_i2c_devices_info,
	ARRAY_SIZE(touchkey_i2c_devices_info),
	},
#endif
#ifdef CONFIG_VP_A2220
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_8960_GSBI8_QUP_I2C_BUS_ID,
		a2220_device,
		ARRAY_SIZE(a2220_device),
	},
#endif
#ifdef CONFIG_NFC_PN544
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_NFC_I2C_BUS_ID,
		pn544_info,
		ARRAY_SIZE(pn544_info),
	},
#endif /* CONFIG_NFC_PN544	*/
#if 0
	{
		I2C_LIQUID,
		MSM_8960_GSBI3_QUP_I2C_BUS_ID,
		mxt_device_info,
		ARRAY_SIZE(mxt_device_info),
	},
#endif
#if defined(CONFIG_SENSORS_AK8975) || defined(CONFIG_INPUT_BMP180) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1_411)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_SNS_I2C_BUS_ID,
		sns_i2c_borad_info,
		ARRAY_SIZE(sns_i2c_borad_info),
	},
#endif
#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F) \
	|| defined(CONFIG_SENSORS_CM36651)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_OPT_I2C_BUS_ID,
		opt_i2c_borad_info,
		ARRAY_SIZE(opt_i2c_borad_info),
	},
#endif
#ifdef CONFIG_SAMSUNG_CMC624
	{
		I2C_SURF | I2C_FFA | I2C_FLUID ,
		MSM_CMC624_I2C_BUS_ID,
		cmc624_i2c_borad_info,
		ARRAY_SIZE(cmc624_i2c_borad_info),
	},
#endif
	{
		I2C_LIQUID,
		MSM_8960_GSBI10_QUP_I2C_BUS_ID,
		sii_device_info,
		ARRAY_SIZE(sii_device_info),
	},
	{
		I2C_LIQUID,
		MSM_8960_GSBI10_QUP_I2C_BUS_ID,
		msm_isa1200_board_info,
		ARRAY_SIZE(msm_isa1200_board_info),
	},
#ifndef CONFIG_SLIMBUS_MSM_CTRL
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_8960_GSBI1_QUP_I2C_BUS_ID,
		tabla_device_info,
		ARRAY_SIZE(tabla_device_info),
	},
#endif
};
#endif /* CONFIG_I2C */

static void __init register_i2c_devices(void)
{
#ifdef CONFIG_I2C
	u8 mach_mask = 0;
	int i;

#ifdef CONFIG_REGULATOR_MAX8952
struct i2c_registry cmc624_max8952_i2c_devices = {
		I2C_SURF | I2C_FFA | I2C_FLUID ,
		MSM_CMC624_I2C_BUS_ID,
		cmc624_max8952_i2c_borad_info,
		ARRAY_SIZE(cmc624_max8952_i2c_borad_info),
	};
#endif /*CONFIG_REGULATOR_MAX8952*/

#ifdef CONFIG_BATTERY_MAX17040
	struct i2c_registry msm8960_fg_i2c_devices = {
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_FUELGAUGE_I2C_BUS_ID,
		fg_i2c_board_info,
		ARRAY_SIZE(fg_i2c_board_info),
	};
#endif /* CONFIG_BATTERY_MAX17040 */
#if defined(CONFIG_CHARGER_SMB347)
	struct i2c_registry msm8960_fg_smb_i2c_devices = {
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_FUELGAUGE_I2C_BUS_ID,
		fg_smb_i2c_board_info,
		ARRAY_SIZE(fg_smb_i2c_board_info),
	};
#endif

	/* Build the matching 'supported_machs' bitmask */
	if (machine_is_msm8960_cdp())
		mach_mask = I2C_SURF;
	else if (machine_is_msm8960_rumi3())
		mach_mask = I2C_RUMI;
	else if (machine_is_msm8960_sim())
		mach_mask = I2C_SIM;
	else if (machine_is_msm8960_fluid())
		mach_mask = I2C_FLUID;
	else if (machine_is_msm8960_liquid())
		mach_mask = I2C_LIQUID;
	else if (machine_is_msm8960_mtp())
		mach_mask = I2C_FFA;
	else if (machine_is_M2_ATT())
		mach_mask = I2C_FFA;
	else
		pr_err("unmatched machine ID in register_i2c_devices\n");

	/* Run the array and install devices as appropriate */
	for (i = 0; i < ARRAY_SIZE(msm8960_i2c_devices); ++i) {
		if (msm8960_i2c_devices[i].machs & mach_mask)
			i2c_register_board_info(msm8960_i2c_devices[i].bus,
						msm8960_i2c_devices[i].info,
						msm8960_i2c_devices[i].len);
	}

#ifdef CONFIG_SAMSUNG_CMC624
#ifdef CONFIG_REGULATOR_MAX8952
	if (max8952_is_used()) {
		i2c_register_board_info(cmc624_max8952_i2c_devices.bus,
					cmc624_max8952_i2c_devices.info,
					cmc624_max8952_i2c_devices.len);
	}
#endif /*CONFIG_REGULATOR_MAX8952*/
#endif /*CONFIG_SAMSUNG_CMC624*/

#if defined(CONFIG_BATTERY_MAX17040)
	if (!is_smb347_using()) {
		i2c_register_board_info(msm8960_fg_i2c_devices.bus,
				msm8960_fg_i2c_devices.info,
				msm8960_fg_i2c_devices.len);
	}
#if defined(CONFIG_CHARGER_SMB347)
	else {
		i2c_register_board_info(msm8960_fg_smb_i2c_devices.bus,
					msm8960_fg_smb_i2c_devices.info,
					msm8960_fg_smb_i2c_devices.len);
	}
#endif /* CONFIG_CHARGER_SMB347 */
#endif /* CONFIG_BATTERY_MAX17040 */
#endif
}

static void __init gpio_rev_init(void)
{
	/*KEY REV*/
	gpio_keys_button[0].gpio = gpio_rev(VOLUME_UP);
	gpio_keys_button[1].gpio = gpio_rev(VOLUME_DOWN);
	gpio_keys_platform_data.nbuttons = 2;
	if (system_rev >= BOARD_REV08) {
		gpio_tlmm_config(GPIO_CFG(GPIO_HOME_KEY, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
		gpio_keys_button[2].gpio = GPIO_HOME_KEY;
		gpio_keys_platform_data.nbuttons = ARRAY_SIZE(gpio_keys_button);
	}
#if defined(CONFIG_SENSORS_CM36651)
	if (system_rev < BOARD_REV08 || system_rev == BOARD_REV11)
		cm36651_pdata.irq = gpio_rev(ALS_INT);
#endif
#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F) \
	|| defined(CONFIG_SENSORS_CM36651)
	opt_i2c_gpio_data.sda_pin = gpio_rev(ALS_SDA);
	opt_i2c_gpio_data.scl_pin = gpio_rev(ALS_SCL);
#if defined(CONFIG_OPTICAL_GP2A)
	if (system_rev < BOARD_REV08 || system_rev == BOARD_REV11) {
		opt_gp2a_data.irq = MSM_GPIO_TO_INT(gpio_rev(ALS_INT));
		opt_gp2a_data.ps_status = gpio_rev(ALS_INT);
	}
#elif defined(CONFIG_OPTICAL_GP2AP020A00F)
	if (system_rev < BOARD_REV08 || system_rev == BOARD_REV11)
		opt_gp2a_data.p_out = gpio_rev(ALS_INT);
#endif
#endif
	a2220_i2c_gpio_data.sda_pin = gpio_rev(A2220_SDA);
	a2220_i2c_gpio_data.scl_pin = gpio_rev(A2220_SCL);
	a2220_data.gpio_wakeup = gpio_rev(A2220_WAKEUP);
	msm8960_a2220_configs[0].gpio = gpio_rev(A2220_SDA);
	msm8960_a2220_configs[1].gpio = gpio_rev(A2220_SCL);
#ifdef CONFIG_VIBETONZ
	if (system_rev >= BOARD_REV04) {
		msm_8960_vibrator_pdata.vib_en_gpio = PMIC_GPIO_VIB_ON;
		msm_8960_vibrator_pdata.is_pmic_vib_en = 1;
	}
	if (system_rev >= BOARD_REV08) {
		msm_8960_vibrator_pdata.haptic_pwr_en_gpio = \
						PMIC_GPIO_HAPTIC_PWR_EN;
		msm_8960_vibrator_pdata.is_pmic_haptic_pwr_en = 1;
	}
#endif /* CONFIG_VIBETONZ */
}

#ifdef CONFIG_SAMSUNG_JACK
static struct pm_gpio ear_det = {
	.direction		= PM_GPIO_DIR_IN,
	.pull			= PM_GPIO_PULL_NO,
	.vin_sel		= PM_GPIO_VIN_S4,
	.function		= PM_GPIO_FUNC_NORMAL,
	.inv_int_pol	= 0,
};

static struct pm_gpio short_sendend = {
	.direction		= PM_GPIO_DIR_IN,
	.pull			= PM_GPIO_PULL_NO,
	.vin_sel		= PM_GPIO_VIN_S4,
	.function		= PM_GPIO_FUNC_NORMAL,
	.inv_int_pol	= 0,
};

static struct pm_gpio ear_micbiase = {
	.direction		= PM_GPIO_DIR_OUT,
	.pull			= PM_GPIO_PULL_NO,
	.out_strength	= PM_GPIO_STRENGTH_HIGH,
	.function		= PM_GPIO_FUNC_NORMAL,
	.inv_int_pol	= 0,
	.vin_sel		= PM_GPIO_VIN_S4,
	.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
	.output_value	= 0,
};

static int secjack_gpio_init(void)
{
	int rc;

	rc = pm8xxx_gpio_config(
		PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_DET),
					&ear_det);
	if (rc) {
		pr_err("%s PMIC_GPIO_EAR_DET config failed\n", __func__);
		return rc;
	}
	rc = pm8xxx_gpio_config(
		PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_SHORT_SENDEND),
					&short_sendend);
	if (rc) {
		pr_err("%s PMIC_GPIO_SHORT_SENDEND config failed\n", __func__);
		return rc;
	}
	rc = gpio_request(
		PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_MICBIAS_EN),
			"EAR_MICBIAS");
	if (rc) {
		pr_err("failed to request ear micbias gpio\n");
		gpio_free(PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_MICBIAS_EN));
		return rc;
	}
	rc = pm8xxx_gpio_config(
			PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_MICBIAS_EN),
			&ear_micbiase);
	if (rc) {
		pr_err("%s PMIC_GPIO_EAR_MICBIAS_EN config failed\n", __func__);
		return rc;
	} else {
		gpio_direction_output(PM8921_GPIO_PM_TO_SYS(
			PMIC_GPIO_EAR_MICBIAS_EN), 0);
	}

	return rc;
}
#endif

void main_mic_bias_init(void)
{
	int ret;
	ret = gpio_request(GPIO_MAIN_MIC_BIAS, "LDO_BIAS");
	if (ret) {
		pr_err("%s: ldo bias gpio %d request"
				"failed\n", __func__, GPIO_MAIN_MIC_BIAS);
		return;
	}
	gpio_direction_output(GPIO_MAIN_MIC_BIAS, 0);
}

static int configure_codec_lineout_gpio(void)
{
	int ret;
	struct pm_gpio param = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.pull	   = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	ret = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(
				gpio_rev(LINEOUT_EN)), &param);
	if (ret) {
		pr_err("%s: Failed to configure Lineout EN"
			" gpio %d\n", __func__,
			PM8921_GPIO_PM_TO_SYS(gpio_rev(LINEOUT_EN)));
		return ret;
	} else
		gpio_direction_output(PM8921_GPIO_PM_TO_SYS(
					gpio_rev(LINEOUT_EN)), 1);
	return 0;
}

static int tabla_codec_ldo_en_init(void)
{
	int ret;

	ret = gpio_request(PM8921_GPIO_PM_TO_SYS(gpio_rev(LINEOUT_EN)),
				 "LINEOUT_EN");
	if (ret) {
		pr_err("%s:External LDO  gpio %d request"
			"failed\n", __func__,
			 PM8921_GPIO_PM_TO_SYS(gpio_rev(LINEOUT_EN)));
		return ret;
	}
	gpio_direction_output(PM8921_GPIO_PM_TO_SYS(gpio_rev(LINEOUT_EN)), 0);
	return 0;
}

static void __init msm8960ab_update_krait_spm(void)
{
	int i;

	/* Reset the AVS registers until we have support for AVS */
	for (i = 0; i < ARRAY_SIZE(msm_spm_data); i++) {
		struct msm_spm_platform_data *pdata = &msm_spm_data[i];
		pdata->reg_init_values[MSM_SPM_REG_SAW2_AVS_CTL] = 0;
		pdata->reg_init_values[MSM_SPM_REG_SAW2_AVS_HYSTERESIS] = 0;
	}

	/* Update the SPM sequences for SPC and PC */
	for (i = 0; i < ARRAY_SIZE(msm_spm_data); i++) {
		int j;
		struct msm_spm_platform_data *pdata = &msm_spm_data[i];
		for (j = 0; j < pdata->num_modes; j++) {
			if (pdata->modes[j].cmd ==
					spm_power_collapse_without_rpm)
				pdata->modes[j].cmd =
				spm_power_collapse_without_rpm_krait_v3;
			else if (pdata->modes[j].cmd ==
					spm_power_collapse_with_rpm)
				pdata->modes[j].cmd =
				spm_power_collapse_with_rpm_krait_v3;
		}
	}
}

static void __init msm8960_tsens_init(void)
{
	if (cpu_is_msm8960())
		if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 1)
			return;

	msm_tsens_early_init(&msm_tsens_pdata);
}

static void __init samsung_m2_att_init(void)
{
#ifdef CONFIG_SEC_DEBUG
	sec_debug_init();
#endif

	if (meminfo_init(SYS_MEMORY, SZ_256M) < 0)
		pr_err("meminfo_init() failed!\n");

	platform_device_register(&msm_gpio_device);
	msm8960_tsens_init();
	msm_thermal_init(&msm_thermal_pdata);
	BUG_ON(msm_rpm_init(&msm8960_rpm_data));
	BUG_ON(msm_rpmrs_levels_init(&msm_rpmrs_data));

	gpio_rev_init();
	regulator_suppress_info_printing();
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");
		platform_device_register(&msm8960_device_rpm_regulator);
		msm_clock_init(&msm8960_clock_init_data);
	if (machine_is_msm8960_liquid())
		msm_otg_pdata.mhl_enable = true;
	msm8960_device_otg.dev.platform_data = &msm_otg_pdata;
	if (machine_is_msm8960_mtp() || machine_is_msm8960_fluid() ||
		machine_is_msm8960_cdp()) {
		msm_otg_pdata.phy_init_seq = wr_phy_init_seq;
	} else if (machine_is_msm8960_liquid()) {
			msm_otg_pdata.phy_init_seq =
				liquid_v1_phy_init_seq;
	}
	android_usb_pdata.swfi_latency =
		msm_rpmrs_levels[0].latency_us;

#ifdef CONFIG_USB_HOST_NOTIFY
	msm_otg_power_init();
#endif
#ifdef CONFIG_VP_A2220
	if (system_rev > BOARD_REV02)
		msm_gpiomux_install(msm8960_a2220_configs,
			ARRAY_SIZE(msm8960_a2220_configs));
#endif

#ifdef CONFIG_USB_EHCI_MSM_HSIC
	if (machine_is_msm8960_liquid()) {
		if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) >= 2)
			msm_hsic_pdata.hub_reset = HSIC_HUB_RESET_GPIO;
	}
#endif
	msm_device_hsic_host.dev.platform_data = &msm_hsic_pdata;
	msm8960_init_gpiomux();

#ifndef CONFIG_S5C73M3
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
#endif
#ifdef CONFIG_S5C73M3
	msm8960_device_qup_spi_gsbi11.dev.platform_data =
				&msm8960_qup_spi_gsbi11_pdata;
#endif
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	add_ramconsole_devices();
#endif

#ifndef CONFIG_S5C73M3
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
#endif
#ifdef CONFIG_BATTERY_SEC
	check_highblock_temp();
#endif /*CONFIG_BATTERY_SEC*/
	msm8960_init_pmic();
	msm8960_i2c_init();
	msm8960_gfx_init();
	if (cpu_is_msm8960ab())
		msm8960ab_update_krait_spm();
	msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	msm_spm_l2_init(msm_spm_l2_data);
	msm8960_init_buses();
	if (cpu_is_msm8960ab()) {
		platform_add_devices(msm8960ab_footswitch,
				msm8960ab_num_footswitch);
	} else {
		platform_add_devices(msm8960_footswitch,
			msm8960_num_footswitch);
	}
	if (machine_is_msm8960_liquid())
		platform_device_register(&msm8960_device_ext_3p3v_vreg);
	if (cpu_is_msm8960ab())
		platform_device_register(&msm8960ab_device_acpuclk);
	else
		platform_device_register(&msm8960_device_acpuclk);
	platform_add_devices(common_devices, ARRAY_SIZE(common_devices));
	msm8960_pm8921_gpio_mpp_init();
	platform_add_devices(m2_att_devices, ARRAY_SIZE(m2_att_devices));
	msm8960_init_hsic();
	msm8960_init_cam();
	msm8960_init_mmc();
	if (machine_is_msm8960_liquid())
		mxt_init_hw_liquid();
	samsung_sys_class_init();
	mms_tsp_input_init();
#if defined(CONFIG_SENSORS_AK8975) || defined(CONFIG_INPUT_BMP180) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1_411)
	sensor_device_init();
#endif
#if defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1_411)
	mpl_init();
#endif
#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F) \
	|| defined(CONFIG_SENSORS_CM36651)
	opt_init();
#endif
#if defined(CONFIG_NFC_PN544)
	pn544_init();
#endif
	msm8960_mhl_gpio_init();
	register_i2c_devices();
	msm8960_init_fb();
	main_mic_bias_init();
	/* From REV09 onwards LINEOUT_EN is not used */
	if (system_rev < BOARD_REV09) {
		tabla_codec_ldo_en_init();
		configure_codec_lineout_gpio();
	}

#ifdef CONFIG_SAMSUNG_JACK
	if (system_rev < BOARD_REV10) {
		pr_info("%s : system rev = %d, MBHC Using\n",
			__func__, system_rev);
		memset(&sec_jack_data, 0, sizeof(sec_jack_data));
	} else {
		pr_info("%s : system rev = %d, Secjack Using\n",
			__func__, system_rev);
		secjack_gpio_init();
	}
#endif

	msm8960_init_dsps();
#if 0
	msm_pm_set_rpm_wakeup_irq(RPM_APCC_CPU0_WAKE_UP_IRQ);
#endif
	BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));
#if 0
	msm_pm_init_sleep_status_data(&msm_pm_slp_sts_data);
#endif
#if defined(CONFIG_BCM4334) || defined(CONFIG_BCM4334_MODULE)
	printk(KERN_INFO "[WIFI] system_rev = %d\n", system_rev);
	if (system_rev >= 0x3)
		brcm_wlan_init();
#endif
	msm_pm_set_tz_retention_flag(1);

	if (PLATFORM_IS_CHARM25())
		platform_add_devices(mdm_devices, ARRAY_SIZE(mdm_devices));
	ion_adjust_secure_allocation();
}

MACHINE_START(M2_ATT, "SAMSUNG M2_ATT")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.handle_irq = gic_handle_irq,
	.timer = &msm_timer,
	.init_machine = samsung_m2_att_init,
	.init_early = msm8960_allocate_memory_regions,
	.init_very_early = msm8960_early_memory,
	.restart = msm_restart,
MACHINE_END
#endif
