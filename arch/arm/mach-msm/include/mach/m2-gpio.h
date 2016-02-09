/*
 * m2-gpio.h
 *
 * header file supporting gpio functions for Samsung device
 *
 * COPYRIGHT(C) Samsung Electronics Co., Ltd. 2006-2011 All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* MSM8960 GPIO */
#define GPIO_MDP_VSYNC			0
#define GPIO_MHL_RST			1
#define GPIO_MSM_FLASH_CNTL_EN		2
#define GPIO_CAM_MCLK2			2 /* > Rev13 */
#define GPIO_MSM_FLASH_NOW		3
#define GPIO_VFE_CAMIF_TIMER3_INT	4
#define GPIO_CAM_MCLK0			5
#define	CAM_CORE_EN			6
#define CAM_MIPI_EN			7
#define GPIO_CODEC_I2C_SDA              8
#define GPIO_CODEC_I2C_SCL              9
#define GPIO_LCD_22V_EN			10
#define GPIO_MXT_TS_IRQ			11
#ifdef CONFIG_VP_A2220
#define GPIO_A2220_I2C_SDA		12
#define GPIO_A2220_I2C_SCL		13
#endif
#define GPIO_FPGA_CS			14
#define GPIO_MHL_WAKE_UP		15

#define GPIO_MAIN_MIC_BIAS		18
#define GPIO_MHL_EN			19
#define GPIO_FUELGAUGE_I2C_SDA		24
#define GPIO_FUEKGAUGE_I2C_SCL		25

#define GPIO_BT_UART_TXD		26
#define GPIO_BT_UART_RXD		27
#define GPIO_BT_UART_CTS		28
#define GPIO_BT_UART_RTS		29

#define GPIO_NFC_SDA			32
#define GPIO_NFC_SCL			33

#if defined(CONFIG_S5C73M3) || defined(CONFIG_S5K6A3YX)
#define GPIO_CAM_SPI_MOSI		38
#define GPIO_CAM_SPI_MISO		39
#define GPIO_CAM_SPI_SSN		40
#define GPIO_CAM_SPI_SCLK		41
#endif

#define GPIO_SENSOR_SNS_SDA		44
#define GPIO_SENSOR_SNS_SCL		45
#define GPIO_CAM_A_EN			46
#define GPIO_HAPTIC_PWR_EN		47 /* < BOARD_REV13 */

#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH_236
#define GPIO_TOUCH_KEY_INT		42
#define GPIO_TOUCHKEY_SCL		47
#define GPIO_TOUCHKEY_SDA		48
#endif

#define GPIO_HOME_KEY			49 /* >= BOARD_REV13 */

#define GPIO_MXT_TS_LDO_EN		50
#define GPIO_CAM_SENSOR_EN		51
#define GPIO_INOK_INT			52

#if defined(CONFIG_BCM4334) || defined(CONFIG_BCM4334_MODULE)
#define GPIO_WL_REG_ON			43
#define GPIO_WL_HOST_WAKE		54
#endif

#define GPIO_SENSOR_ALS_SDA		63
#define GPIO_SENSOR_ALS_SCL		64
#define GPIO_FUEL_INT			67
#define GPIO_MSENSE_RST			68
#define GPIO_MPU3050_INT		69
#define GPIO_VIB_PWM                    70

#ifdef CONFIG_SAMSUNG_CMC624
#define GPIO_IMA_I2C_SDA		71
#define GPIO_IMA_I2C_SCL		72
#endif
#define GPIO_USB_I2C_SDA		73
#define GPIO_USB_I2C_SCL		74
#define CAM2_RST_N			76
#define GPIO_VIB_ON                     77 /* < BOARD_REV09 */

#define GPIO_KS8851_RST			89
#define GPIO_KS8851_IRQ			90

#define GPIO_NFC_FIRMWARE		92
#define GPIO_MHL_SDA			95
#define GPIO_MHL_SCL			96
#ifdef CONFIG_S5K6A3YX
#define GPIO_VT_CAM_SEN_DET		98
#endif

#define GPIO_MHL_INT			99

#define GPIO_NFC_IRQ			106
#define ISP_RESET			107

/* ES305B GPIO */
#define MSM_AUD_A2220_WAKEUP		79
#define MSM_AUD_A2220_RESET		75

/* PMIC8921 MPP */
#define PMIC_MPP_FLASH_LED_UNLOCK       4

/* PMIC8921 GPIO */
#define PMIC_GPIO_VIB_ON                4
#define PMIC_MSM_FLASH_CNTL_EN          5
#define PMIC_GPIO_RGB_INT		6
#define PMIC_GPIO_ECOMPASS_RST		9
#ifdef CONFIG_SAMSUNG_CMC624
#define PMIC_GPIO_MLCD_ON		10
#define PMIC_GPIO_IMA_PWR_EN		11
#define PMIC_GPIO_IMA_CMC_EN		12
#define PMIC_GPIO_IMA_nRST		13
#define PMIC_GPIO_IMA_SLEEP		14
#endif
#define PMIC_GPIO_SPK_EN		18
#define PMIC_GPIO_VPS_EN		19
#define PMIC_GPIO_NFC_EN		21
#define PMIC_GPIO_OTG_EN		-1
#define PMIC_GPIO_HAPTIC_PWR_EN		22
#define PMIC_GPIO_CODEC_RST		38
#define PMIC_GPIO_OTG_POWER		42
#define PMIC_GPIO_LCD_RST		43

#define PMIC_GPIO_CMC_ESD_DET		15
#define PMIC_GPIO_VGH_ESD_DET		44

#if defined(CONFIG_CHARGER_SMB347)
#define PMIC_GPIO_CHG_EN		PMIC_GPIO_OTG_EN
#define PMIC_GPIO_CHG_STAT		17
#endif
#define PMIC_GPIO_BATT_INT		37

/* gpio for changed list */
enum {
	BOARD_REV00,
	BOARD_REV01,
	BOARD_REV02,
	BOARD_REV03,
	BOARD_REV04,
	BOARD_REV05,
	BOARD_REV06,
	BOARD_REV07,
	BOARD_REV08,
	BOARD_REV09,
	BOARD_REV10,
	BOARD_REV11,
	BOARD_REV12,
	BOARD_REV13,
	BOARD_REV14,
	BOARD_REV15,
	BOARD_REV16,
	BOARD_REV17,
	BOARD_REV18,
	BOARD_REV19,
	BOARD_REV20,
	BOARD_REV21,
	BOARD_REV22,
	BOARD_REV23,
	BOARD_REV24,
	BOARD_REV25,
	GPIO_REV_MAX,
};

enum {
	MDP_VSYNC,
	VOLUME_UP,
	VOLUME_DOWN,
	MHL_EN,
	MHL_SDA,
	GPIO_MAG_RST,
	ALS_INT,
	ALS_SDA,
	ALS_SCL,
	LCD_22V_EN,
	LINEOUT_EN,
	CAM_AF_EN,
	CAM_FLASH_SW,
	A2220_WAKEUP,
	A2220_SDA,
	A2220_SCL,
	BT_HOST_WAKE,
	BT_WAKE,
	BT_EN,
};
