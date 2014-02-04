/*
 * driver/misc/fsa9480.c - FSA9480 micro USB switch device driver
 *
 * Copyright (C) 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Wonguk Jeong <wonguk.jeong@samsung.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c/fsa9485.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/pmic8058.h>
#include <linux/input.h>
#include <linux/sii9234.h>

/* FSA9480 I2C registers */
#define FSA9485_REG_DEVID		0x01
#define FSA9485_REG_CTRL		0x02
#define FSA9485_REG_INT1		0x03
#define FSA9485_REG_INT2		0x04
#define FSA9485_REG_INT1_MASK		0x05
#define FSA9485_REG_INT2_MASK		0x06
#define FSA9485_REG_ADC			0x07
#define FSA9485_REG_TIMING1		0x08
#define FSA9485_REG_TIMING2		0x09
#define FSA9485_REG_DEV_T1		0x0a
#define FSA9485_REG_DEV_T2		0x0b
#define FSA9485_REG_BTN1		0x0c
#define FSA9485_REG_BTN2		0x0d
#define FSA9485_REG_CK			0x0e
#define FSA9485_REG_CK_INT1		0x0f
#define FSA9485_REG_CK_INT2		0x10
#define FSA9485_REG_CK_INTMASK1		0x11
#define FSA9485_REG_CK_INTMASK2		0x12
#define FSA9485_REG_MANSW1		0x13
#define FSA9485_REG_MANSW2		0x14
#define FSA9485_REG_MANUAL_OVERRIDES1	0x1B
#define FSA9485_REG_RESERVED_1D		0x1D
#define FSA9485_REG_RESERVED_20		0x20


/* Control */
#define CON_SWITCH_OPEN		(1 << 4)
#define CON_RAW_DATA		(1 << 3)
#define CON_MANUAL_SW		(1 << 2)
#define CON_WAIT		(1 << 1)
#define CON_INT_MASK		(1 << 0)
#define CON_MASK		(CON_SWITCH_OPEN | CON_RAW_DATA | \
				CON_MANUAL_SW | CON_WAIT)

/* Device Type 1 */
#define DEV_USB_OTG		(1 << 7)
#define DEV_DEDICATED_CHG	(1 << 6)
#define DEV_USB_CHG		(1 << 5)
#define DEV_CAR_KIT		(1 << 4)
#define DEV_UART		(1 << 3)
#define DEV_USB			(1 << 2)
#define DEV_AUDIO_2		(1 << 1)
#define DEV_AUDIO_1		(1 << 0)

#define DEV_T1_USB_MASK		(DEV_USB_OTG | DEV_USB_CHG | DEV_USB)
#define DEV_T1_UART_MASK	(DEV_UART)
#define DEV_T1_CHARGER_MASK	(DEV_DEDICATED_CHG | DEV_CAR_KIT)

/* Device Type 2 */
#define DEV_AUDIO_DOCK		(1 << 8)
#define DEV_SMARTDOCK	(1 << 7)
#define DEV_AV			(1 << 6)
#define DEV_TTY			(1 << 5)
#define DEV_PPD			(1 << 4)
#define DEV_JIG_UART_OFF	(1 << 3)
#define DEV_JIG_UART_ON		(1 << 2)
#define DEV_JIG_USB_OFF		(1 << 1)
#define DEV_JIG_USB_ON		(1 << 0)

#define DEV_T2_USB_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON)
#define DEV_T2_UART_MASK	(DEV_JIG_UART_OFF)
#define DEV_T2_JIG_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON | \
				DEV_JIG_UART_OFF)

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 */
#define SW_VAUDIO		((4 << 5) | (4 << 2) | (1 << 1) | (1 << 0))
#define SW_UART			((3 << 5) | (3 << 2))
#define SW_AUDIO		((2 << 5) | (2 << 2) | (1 << 1) | (1 << 0))
#define SW_DHOST		((1 << 5) | (1 << 2) | (1 << 1) | (1 << 0))
#define SW_AUTO			((0 << 5) | (0 << 2))
#define SW_USB_OPEN		(1 << 0)
#define SW_ALL_OPEN		(0)

/* Interrupt 1 */
#define INT_DETACH		(1 << 1)
#define INT_ATTACH		(1 << 0)

#define	ADC_GND			0x00
#define	ADC_MHL			0x01
#define	ADC_DOCK_PREV_KEY 0x04
#define	ADC_DOCK_NEXT_KEY 0x07
#define	ADC_DOCK_VOL_DN		0x0a
#define	ADC_DOCK_VOL_UP		0x0b
#define	ADC_DOCK_PLAY_PAUSE_KEY 0x0d
#define	ADC_CEA936ATYPE1_CHG	0x17
#define	ADC_JIG_USB_OFF		0x18
#define	ADC_JIG_USB_ON		0x19
#define	ADC_DESKDOCK		0x1a
#define	ADC_CEA936ATYPE2_CHG	0x1b
#define	ADC_JIG_UART_OFF	0x1c
#define	ADC_JIG_UART_ON		0x1d
#define	ADC_CARDOCK		0x1d
#define	ADC_OPEN		0x1f

extern int force_fast_charge;

int uart_connecting = 0;
EXPORT_SYMBOL(uart_connecting);

int detached_status;
EXPORT_SYMBOL(detached_status);

struct fsa9485_usbsw {
	struct i2c_client		*client;
	struct fsa9485_platform_data	*pdata;
	int				dev1;
	int				dev2;
	int				mansw;
	int				dock_attached;

	struct input_dev	*input;
	int			previous_key;

	struct delayed_work	init_work;
	struct mutex		mutex;
	int				adc;
	int				deskdock;
};

enum {
	DOCK_KEY_NONE			= 0,
	DOCK_KEY_VOL_UP_PRESSED,
	DOCK_KEY_VOL_UP_RELEASED,
	DOCK_KEY_VOL_DOWN_PRESSED,
	DOCK_KEY_VOL_DOWN_RELEASED,
	DOCK_KEY_PREV_PRESSED,
	DOCK_KEY_PREV_RELEASED,
	DOCK_KEY_PLAY_PAUSE_PRESSED,
	DOCK_KEY_PLAY_PAUSE_RELEASED,
	DOCK_KEY_NEXT_PRESSED,
	DOCK_KEY_NEXT_RELEASED,

};

static struct fsa9485_usbsw *local_usbsw;

#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
#define MHL_DEVICE 2
static int isDeskdockconnected;
#endif

static void DisableFSA9480Interrupts(void)
{
	struct i2c_client *client = local_usbsw->client;
	int value, ret;

	value = i2c_smbus_read_byte_data(client, FSA9485_REG_CTRL);
	value |= 0x01;

	ret = i2c_smbus_write_byte_data(client, FSA9485_REG_CTRL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

}

static void EnableFSA9480Interrupts(void)
{
	struct i2c_client *client = local_usbsw->client;
	int value, ret;

	value = i2c_smbus_read_byte_data(client, FSA9485_REG_CTRL);
	value &= 0xFE;

	ret = i2c_smbus_write_byte_data(client, FSA9485_REG_CTRL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

}

void FSA9485_CheckAndHookAudioDock(int value)
{
	struct i2c_client *client = local_usbsw->client;
	struct fsa9485_platform_data *pdata = local_usbsw->pdata;
	int ret = 0;

	if (value) {
		pr_info("FSA9485_CheckAndHookAudioDock ON\n");
			if (pdata->dock_cb)
				pdata->dock_cb(FSA9485_ATTACHED_DESK_DOCK);

			ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_MANSW1, SW_AUDIO);

			if (ret < 0)
				dev_err(&client->dev, "%s: err %d\n",
							__func__, ret);

			ret = i2c_smbus_read_byte_data(client,
							FSA9485_REG_CTRL);

			if (ret < 0)
				dev_err(&client->dev, "%s: err %d\n",
							__func__, ret);

			ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_CTRL,
					ret & ~CON_MANUAL_SW & ~CON_RAW_DATA);
			if (ret < 0)
				dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
		} else {
			dev_info(&client->dev,
			"FSA9485_CheckAndHookAudioDock Off\n");

			if (pdata->dock_cb)
				pdata->dock_cb(FSA9485_DETACHED_DOCK);

			ret = i2c_smbus_read_byte_data(client,
						FSA9485_REG_CTRL);
			if (ret < 0)
				dev_err(&client->dev,
					"%s: err %d\n", __func__, ret);

			ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_CTRL,
					ret | CON_MANUAL_SW | CON_RAW_DATA);
			if (ret < 0)
				dev_err(&client->dev,
					"%s: err %d\n", __func__, ret);
	}
}

static void fsa9485_reg_init(struct fsa9485_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	unsigned int ctrl = CON_MASK;
	int ret;
	pr_info("fsa9485_reg_init is called\n");
	/* mask interrupts (unmask attach/detach only) */
	ret = i2c_smbus_write_word_data(client, FSA9485_REG_INT1_MASK, 0x18fc);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	/* mask all car kit interrupts */
	ret = i2c_smbus_write_word_data(client,
					FSA9485_REG_CK_INTMASK1, 0x07ff);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	/* ADC Detect Time: 500ms */
	ret = i2c_smbus_write_byte_data(client, FSA9485_REG_TIMING1, 0x0);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	usbsw->mansw = i2c_smbus_read_byte_data(client, FSA9485_REG_MANSW1);
	if (usbsw->mansw < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, usbsw->mansw);

	if (usbsw->mansw)
		ctrl &= ~CON_MANUAL_SW;	/* Manual Switching Mode */
	else
		ctrl &= ~(CON_INT_MASK);

	ret = i2c_smbus_write_byte_data(client, FSA9485_REG_CTRL, ctrl);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	/* apply Battery Charging Spec. 1.1 @TA/USB detect */
	ret = i2c_smbus_write_byte_data(client, FSA9485_REG_RESERVED_20, 0x04);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = i2c_smbus_read_byte_data(client, FSA9485_REG_DEVID);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	dev_info(&client->dev, " fsa9485_reg_init dev ID: 0x%x\n", ret);
}

static ssize_t fsa9485_show_control(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, FSA9485_REG_CTRL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return snprintf(buf, 13, "CONTROL: %02x\n", value);
}

static ssize_t fsa9485_show_device_type(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, FSA9485_REG_DEV_T1);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return snprintf(buf, 11, "DEVICE_TYPE: %02x\n", value);
}

static ssize_t fsa9485_show_manualsw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, FSA9485_REG_MANSW1);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if (value == SW_VAUDIO)
		return snprintf(buf, 7, "VAUDIO\n");
	else if (value == SW_UART)
		return snprintf(buf, 5, "UART\n");
	else if (value == SW_AUDIO)
		return snprintf(buf, 6, "AUDIO\n");
	else if (value == SW_DHOST)
		return snprintf(buf, 6, "DHOST\n");
	else if (value == SW_AUTO)
		return snprintf(buf, 5, "AUTO\n");
	else
		return snprintf(buf, 4, "%x", value);
}

static ssize_t fsa9485_set_manualsw(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value, ret;
	unsigned int path = 0;

	value = i2c_smbus_read_byte_data(client, FSA9485_REG_CTRL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if ((value & ~CON_MANUAL_SW) !=
			(CON_SWITCH_OPEN | CON_RAW_DATA | CON_WAIT))
		return 0;

	if (!strncmp(buf, "VAUDIO", 6)) {
		path = SW_VAUDIO;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "UART", 4)) {
		path = SW_UART;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "AUDIO", 5)) {
		path = SW_AUDIO;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "DHOST", 5)) {
		path = SW_DHOST;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "AUTO", 4)) {
		path = SW_AUTO;
		value |= CON_MANUAL_SW;
	} else {
		dev_err(dev, "Wrong command\n");
		return 0;
	}

	usbsw->mansw = path;

	ret = i2c_smbus_write_byte_data(client, FSA9485_REG_MANSW1, path);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = i2c_smbus_write_byte_data(client, FSA9485_REG_CTRL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return count;
}
static ssize_t fsa9480_show_usb_state(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int device_type;
	unsigned char device_type1, device_type2;

	device_type = i2c_smbus_read_word_data(client, FSA9485_REG_DEV_T1);
	if (device_type < 0) {
		dev_err(&client->dev, "%s: err %d ", __func__, device_type);
		return (ssize_t)device_type;
	}
	device_type1 = device_type & 0xff;
	device_type2 = device_type >> 8;

	if (device_type1 & DEV_T1_USB_MASK || device_type2 & DEV_T2_USB_MASK)
		return snprintf(buf, 22, "USB_STATE_CONFIGURED\n");

	return snprintf(buf, 25, "USB_STATE_NOTCONFIGURED\n");
}

static ssize_t fsa9485_show_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int adc;

	adc = i2c_smbus_read_byte_data(client, FSA9485_REG_ADC);
	if (adc < 0) {
		dev_err(&client->dev,
			"%s: err at read adc %d\n", __func__, adc);
		return snprintf(buf, 9, "UNKNOWN\n");
	}

	return snprintf(buf, 4, "%x\n", adc);
}

static ssize_t fsa9485_reset(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int ret;

	if (!strncmp(buf, "1", 1)) {
		dev_info(&client->dev, "fsa9480 reset after delay 1000 msec.\n");
		mdelay(1000);
		ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_MANUAL_OVERRIDES1, 0x01);
		if (ret < 0)
				dev_err(&client->dev,
					"cannot soft reset, err %d\n", ret);

	dev_info(&client->dev, "fsa9480_reset_control done!\n");
	} else {
		dev_info(&client->dev,
					"fsa9480_reset_control, but not reset_value!\n");
	}

	fsa9485_reg_init(usbsw);

	return count;
}


static DEVICE_ATTR(control, S_IRUGO, fsa9485_show_control, NULL);
static DEVICE_ATTR(device_type, S_IRUGO, fsa9485_show_device_type, NULL);
static DEVICE_ATTR(switch, S_IRUGO | S_IWUSR,
		fsa9485_show_manualsw, fsa9485_set_manualsw);
static DEVICE_ATTR(usb_state, S_IRUGO, fsa9480_show_usb_state, NULL);
static DEVICE_ATTR(adc, S_IRUGO, fsa9485_show_adc, NULL);
static DEVICE_ATTR(reset_switch, S_IWUSR | S_IWGRP, NULL, fsa9485_reset);

static struct attribute *fsa9485_attributes[] = {
	&dev_attr_control.attr,
	&dev_attr_device_type.attr,
	&dev_attr_switch.attr,
	NULL
};

static const struct attribute_group fsa9485_group = {
	.attrs = fsa9485_attributes,
};

void fsa9485_otg_detach(void)
{
	unsigned int data = 0;
	int ret;
	struct i2c_client *client = local_usbsw->client;

	if (local_usbsw->dev1 & DEV_USB_OTG) {
		dev_info(&client->dev, "%s: real device\n", __func__);
		data = 0x00;
		ret = i2c_smbus_write_byte_data(client,
						FSA9485_REG_MANSW2, data);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		data = SW_ALL_OPEN;
		ret = i2c_smbus_write_byte_data(client,
						FSA9485_REG_MANSW1, data);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);

		data = 0x1A;
		ret = i2c_smbus_write_byte_data(client,
						FSA9485_REG_CTRL, data);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
	} else
		dev_info(&client->dev, "%s: not real device\n", __func__);
}
EXPORT_SYMBOL(fsa9485_otg_detach);


void fsa9485_manual_switching(int path)
{
	struct i2c_client *client = local_usbsw->client;
	int value, ret;
	unsigned int data = 0;

	value = i2c_smbus_read_byte_data(client, FSA9485_REG_CTRL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if ((value & ~CON_MANUAL_SW) !=
			(CON_SWITCH_OPEN | CON_RAW_DATA | CON_WAIT))
		return;

	if (path == SWITCH_PORT_VAUDIO) {
		data = SW_VAUDIO;
		value &= ~CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_UART) {
		data = SW_UART;
		value &= ~CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_AUDIO) {
		data = SW_AUDIO;
		value &= ~CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_USB) {
		data = SW_DHOST;
		value &= ~CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_AUTO) {
		data = SW_AUTO;
		value |= CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_USB_OPEN) {
		data = SW_USB_OPEN;
		value &= ~CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_ALL_OPEN) {
		data = SW_ALL_OPEN;
		value &= ~CON_MANUAL_SW;
	} else {
		pr_info("%s: wrong path (%d)\n", __func__, path);
		return;
	}

	local_usbsw->mansw = data;

	/* path for FTM sleep */
	if (path ==  SWITCH_PORT_ALL_OPEN) {
		ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_MANUAL_OVERRIDES1, 0x0a);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);

		ret = i2c_smbus_write_byte_data(client,
						FSA9485_REG_MANSW1, data);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);

		ret = i2c_smbus_write_byte_data(client,
						FSA9485_REG_MANSW2, data);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);

		ret = i2c_smbus_write_byte_data(client,
						FSA9485_REG_CTRL, value);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
	} else {
		ret = i2c_smbus_write_byte_data(client,
						FSA9485_REG_MANSW1, data);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);

		ret = i2c_smbus_write_byte_data(client,
						FSA9485_REG_CTRL, value);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
	}

}
EXPORT_SYMBOL(fsa9485_manual_switching);

static int fsa9485_detect_dev(struct fsa9485_usbsw *usbsw)
{
	int device_type, ret;
	unsigned int val1, val2, adc;
	struct fsa9485_platform_data *pdata = usbsw->pdata;
	struct i2c_client *client = usbsw->client;
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
	u8 mhl_ret = 0;
#endif
	device_type = i2c_smbus_read_word_data(client, FSA9485_REG_DEV_T1);
	if (device_type < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, device_type);
		return device_type;
	}
	val1 = device_type & 0xff;
	val2 = device_type >> 8;
	adc = i2c_smbus_read_byte_data(client, FSA9485_REG_ADC);

	if (usbsw->dock_attached)
		pdata->dock_cb(FSA9485_DETACHED_DOCK);

	if (adc == 0x10)
		val2 = DEV_SMARTDOCK;
	else if (adc == 0x12)
		val2 = DEV_AUDIO_DOCK;

	dev_info(&client->dev, "dev1: 0x%x, dev2: 0x%x adc : 0x%x\n", val1, val2, adc);

	/* Attached */
	if (val1 || val2) {
		/* USB */
		if (val1 & DEV_USB || val2 & DEV_T2_USB_MASK) {
			dev_info(&client->dev, "usb connect\n");

			msleep(10);

			ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_MANSW1, SW_AUTO);
			if (ret < 0)
				dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);

			ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_CTRL, 0x1E);
			
			if (pdata->usb_cb) {
				if (force_fast_charge != 0) {
					dev_info(&client->dev, "[imoseyon] fastcharge\n");
					pdata->charger_cb(FSA9485_ATTACHED);
				} else pdata->usb_cb(FSA9485_ATTACHED);
			}

			if (usbsw->mansw) {
				ret = i2c_smbus_write_byte_data(client,
				FSA9485_REG_MANSW1, usbsw->mansw);

				if (ret < 0)
					dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
			}
		/* USB_CDP */
		} else if (val1 & DEV_USB_CHG) {
			dev_info(&client->dev, "usb_cdp connect\n");

			if (pdata->usb_cdp_cb)
				pdata->usb_cdp_cb(FSA9485_ATTACHED);
			if (usbsw->mansw) {
				ret = i2c_smbus_write_byte_data(client,
				FSA9485_REG_MANSW1, usbsw->mansw);

				if (ret < 0)
					dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
			}

		/* UART */
		} else if (val1 & DEV_T1_UART_MASK || val2 & DEV_T2_UART_MASK) {
			uart_connecting = 1;
			dev_info(&client->dev, "uart connect\n");
			i2c_smbus_write_byte_data(client,
						FSA9485_REG_CTRL, 0x1E);
			if (pdata->uart_cb)
				pdata->uart_cb(FSA9485_ATTACHED);

			if (usbsw->mansw) {
				ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_MANSW1, SW_UART);

				if (ret < 0)
					dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
			}
		/* CHARGER */
		} else if (val1 & DEV_T1_CHARGER_MASK) {
			dev_info(&client->dev, "charger connect\n");

			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_ATTACHED);
		/* for SAMSUNG OTG */
		} else if (val1 & DEV_USB_OTG) {
			dev_info(&client->dev, "otg connect\n");
			if (pdata->otg_cb)
				pdata->otg_cb(FSA9485_ATTACHED);
			i2c_smbus_write_byte_data(client,
						FSA9485_REG_MANSW1, 0x27);
			i2c_smbus_write_byte_data(client,
						FSA9485_REG_MANSW2, 0x02);
			msleep(50);
			i2c_smbus_write_byte_data(client,
						FSA9485_REG_CTRL, 0x1a);
		/* JIG */
		} else if (val2 & DEV_T2_JIG_MASK) {
			dev_info(&client->dev, "jig connect\n");

			if (pdata->jig_cb)
				pdata->jig_cb(FSA9485_ATTACHED);
		/* Desk Dock */
		} else if (val2 & DEV_AV) {
			if ((adc & 0x1F) == ADC_DESKDOCK) {
				pr_info("FSA Deskdock Attach\n");
				FSA9485_CheckAndHookAudioDock(1);
				usbsw->deskdock = 1;
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
				isDeskdockconnected = 1;
#endif
				i2c_smbus_write_byte_data(client,
						FSA9485_REG_RESERVED_20, 0x08);
			} else {
				pr_info("FSA MHL Attach\n");
				i2c_smbus_write_byte_data(client,
						FSA9485_REG_RESERVED_20, 0x08);
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
				DisableFSA9480Interrupts();
				if (!isDeskdockconnected)
					mhl_ret = mhl_onoff_ex(1);

				if (mhl_ret != MHL_DEVICE &&
						(adc & 0x1F) == 0x1A) {
					FSA9485_CheckAndHookAudioDock(1);
					isDeskdockconnected = 1;
				}
				EnableFSA9480Interrupts();
#else
				pr_info("FSA mhl attach, but not support MHL feature!\n");
#endif
			}
		/* Car Dock */
		} else if (val2 & DEV_JIG_UART_ON) {
			if (pdata->dock_cb)
				pdata->dock_cb(FSA9485_ATTACHED_CAR_DOCK);
			ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_MANSW1, SW_AUDIO);

			if (ret < 0)
				dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);

			ret = i2c_smbus_read_byte_data(client,
					FSA9485_REG_CTRL);
			if (ret < 0)
				dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);

			ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_CTRL, ret & ~CON_MANUAL_SW);
			if (ret < 0)
				dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
			usbsw->dock_attached = FSA9485_ATTACHED;
		/* SmartDock */
		} else if (val2 & DEV_SMARTDOCK) {
			usbsw->adc = adc;
			dev_info(&client->dev, "smart dock connect\n");

			usbsw->mansw = SW_DHOST;
			ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_MANSW1, SW_DHOST);
			if (ret < 0)
				dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
			ret = i2c_smbus_read_byte_data(client,
					FSA9485_REG_CTRL);
			if (ret < 0)
				dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
			ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_CTRL, ret & ~CON_MANUAL_SW);
			if (ret < 0)
				dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);

			if (pdata->smartdock_cb)
				pdata->smartdock_cb(FSA9485_ATTACHED);
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
			mhl_onoff_ex(1);
#endif
		} else if (val2 & DEV_AUDIO_DOCK) {
			usbsw->adc = adc;
			dev_info(&client->dev, "audio dock connect\n");

			usbsw->mansw = SW_DHOST;
			ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_MANSW1, SW_DHOST);
			if (ret < 0)
				dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
			ret = i2c_smbus_read_byte_data(client,
					FSA9485_REG_CTRL);
			if (ret < 0)
				dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
			ret = i2c_smbus_write_byte_data(client,
					FSA9485_REG_CTRL, ret & ~CON_MANUAL_SW);
			if (ret < 0)
				dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);

			if (pdata->smartdock_cb)
				pdata->audio_dock_cb(FSA9485_ATTACHED);
		}
	/* Detached */
	} else {
		/* USB */
		if (usbsw->dev1 & DEV_USB ||
				usbsw->dev2 & DEV_T2_USB_MASK) {
			if (pdata->usb_cb)
				pdata->usb_cb(FSA9485_DETACHED);
		} else if (usbsw->dev1 & DEV_USB_CHG) {
			if (pdata->usb_cdp_cb)
				pdata->usb_cdp_cb(FSA9485_DETACHED);

		/* UART */
		} else if (usbsw->dev1 & DEV_T1_UART_MASK ||
				usbsw->dev2 & DEV_T2_UART_MASK) {
			if (pdata->uart_cb)
				pdata->uart_cb(FSA9485_DETACHED);
			uart_connecting = 0;
			dev_info(&client->dev, "[FSA9485] uart disconnect\n");

		/* CHARGER */
		} else if (usbsw->dev1 & DEV_T1_CHARGER_MASK) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_DETACHED);
		/* for SAMSUNG OTG */
		} else if (usbsw->dev1 & DEV_USB_OTG) {
			i2c_smbus_write_byte_data(client,
						FSA9485_REG_CTRL, 0x1E);
		/* JIG */
		} else if (usbsw->dev2 & DEV_T2_JIG_MASK) {
			if (pdata->jig_cb)
				pdata->jig_cb(FSA9485_DETACHED);
		/* Desk Dock */
		} else if (usbsw->dev2 & DEV_AV) {

			pr_info("FSA MHL Detach\n");
			i2c_smbus_write_byte_data(client,
					FSA9485_REG_RESERVED_20, 0x04);
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
			if (isDeskdockconnected)
				FSA9485_CheckAndHookAudioDock(0);
#if defined CONFIG_MHL_D3_SUPPORT
			mhl_onoff_ex(false);
			detached_status = 1;
#endif
			isDeskdockconnected = 0;
#else
			if (usbsw->deskdock) {
				FSA9485_CheckAndHookAudioDock(0);
				usbsw->deskdock = 0;
			} else {
				pr_info("FSA detach mhl cable, but not support MHL feature\n");
			}
#endif
		/* Car Dock */
		} else if (usbsw->dev2 & DEV_JIG_UART_ON) {
			if (pdata->dock_cb)
				pdata->dock_cb(FSA9485_DETACHED_DOCK);
				ret = i2c_smbus_read_byte_data(client,
						FSA9485_REG_CTRL);
				if (ret < 0)
					dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);

				ret = i2c_smbus_write_byte_data(client,
						FSA9485_REG_CTRL,
						ret | CON_MANUAL_SW);
				if (ret < 0)
					dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
				usbsw->dock_attached = FSA9485_DETACHED;
		} else if (usbsw->adc == 0x10) {
			dev_info(&client->dev, "smart dock disconnect\n");

			ret = i2c_smbus_read_byte_data(client,
						FSA9485_REG_CTRL);
				if (ret < 0)
					dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
				ret = i2c_smbus_write_byte_data(client,
						FSA9485_REG_CTRL,
						ret | CON_MANUAL_SW);
				if (ret < 0)
					dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);

			if (pdata->smartdock_cb)
				pdata->smartdock_cb(FSA9485_DETACHED);
			usbsw->adc = 0;
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
			mhl_onoff_ex(false);
#endif
		} else if (usbsw->adc == 0x12) {
			dev_info(&client->dev, "audio dock disconnect\n");

			ret = i2c_smbus_read_byte_data(client,
						FSA9485_REG_CTRL);
				if (ret < 0)
					dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
				ret = i2c_smbus_write_byte_data(client,
						FSA9485_REG_CTRL,
						ret | CON_MANUAL_SW);
				if (ret < 0)
					dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);

			if (pdata->smartdock_cb)
				pdata->audio_dock_cb(FSA9485_DETACHED);
			usbsw->adc = 0;
		}

	}
	usbsw->dev1 = val1;
	usbsw->dev2 = val2;

	return adc;
}

static int fsa9485_check_dev(struct fsa9485_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int device_type;
	device_type = i2c_smbus_read_word_data(client, FSA9485_REG_DEV_T1);
	if (device_type < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, device_type);
		return 0;
	}
	return device_type;
}

static int fsa9485_handle_dock_vol_key(struct fsa9485_usbsw *info, int adc)
{
	struct input_dev *input = info->input;
	int pre_key = info->previous_key;
	unsigned int code=0;
	int state=0;

	if (adc == ADC_OPEN) {
		switch (pre_key) {
		case DOCK_KEY_VOL_UP_PRESSED:
			code = KEY_VOLUMEUP;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_UP_RELEASED;
			break;
		case DOCK_KEY_VOL_DOWN_PRESSED:
			code = KEY_VOLUMEDOWN;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_DOWN_RELEASED;
			break;
		case DOCK_KEY_PREV_PRESSED:
			code = KEY_PREVIOUSSONG;
			state = 0;
			info->previous_key = DOCK_KEY_PREV_RELEASED;
			break;
		case DOCK_KEY_PLAY_PAUSE_PRESSED:
			code = KEY_PLAYPAUSE;
			state = 0;
			info->previous_key = DOCK_KEY_PLAY_PAUSE_RELEASED;
			break;
		case DOCK_KEY_NEXT_PRESSED:
			code = KEY_NEXTSONG;
			state = 0;
			info->previous_key = DOCK_KEY_NEXT_RELEASED;
			break;
		default:
			return 0;
		}
		input_event(input, EV_KEY, code, state);
		input_sync(input);
		return 0;
	}

	if (pre_key == DOCK_KEY_NONE) {
		if (adc != ADC_DOCK_VOL_UP && adc != ADC_DOCK_VOL_DN
			&& adc != ADC_DOCK_PREV_KEY && adc != ADC_DOCK_NEXT_KEY
			&& adc != ADC_DOCK_PLAY_PAUSE_KEY)
			return 0;
	}


	switch (adc) {
	case ADC_DOCK_VOL_UP:
		code = KEY_VOLUMEUP;
		state = 1;
		info->previous_key = DOCK_KEY_VOL_UP_PRESSED;
		break;
	case ADC_DOCK_VOL_DN:
		code = KEY_VOLUMEDOWN;
		state = 1;
		info->previous_key = DOCK_KEY_VOL_DOWN_PRESSED;
		break;
	case ADC_DOCK_PREV_KEY-1 ... ADC_DOCK_PREV_KEY+1:
		code = KEY_PREVIOUSSONG;
		state = 1;
		info->previous_key = DOCK_KEY_PREV_PRESSED;
		break;
	case ADC_DOCK_PLAY_PAUSE_KEY-1 ... ADC_DOCK_PLAY_PAUSE_KEY+1:
		code = KEY_PLAYPAUSE;
		state = 1;
		info->previous_key = DOCK_KEY_PLAY_PAUSE_PRESSED;
		break;
	case ADC_DOCK_NEXT_KEY-1 ... ADC_DOCK_NEXT_KEY+1:
		code = KEY_NEXTSONG;
		state = 1;
		info->previous_key = DOCK_KEY_NEXT_PRESSED;
		break;
	case ADC_DESKDOCK:
		if (pre_key == DOCK_KEY_VOL_UP_PRESSED) {
			code = KEY_VOLUMEUP;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_UP_RELEASED;
		} else if (pre_key == DOCK_KEY_VOL_DOWN_PRESSED) {
			code = KEY_VOLUMEDOWN;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_DOWN_RELEASED;
		} else if (pre_key == DOCK_KEY_PREV_PRESSED) {
			code = KEY_PREVIOUSSONG;
			state = 0;
			info->previous_key = DOCK_KEY_PREV_RELEASED;
		} else if (pre_key == DOCK_KEY_PLAY_PAUSE_PRESSED) {
			code = KEY_PLAYPAUSE;
			state = 0;
			info->previous_key = DOCK_KEY_PLAY_PAUSE_RELEASED;
		} else if (pre_key == DOCK_KEY_NEXT_PRESSED) {
			code = KEY_NEXTSONG;
			state = 0;
			info->previous_key = DOCK_KEY_NEXT_RELEASED;
		} else {
			return 0;
		}
		break;
	default:
		break;
		return 0;
	}

	input_event(input, EV_KEY, code, state);
	input_sync(input);

	return 1;
}

static irqreturn_t fsa9485_irq_thread(int irq, void *data)
{
	struct fsa9485_usbsw *usbsw = data;
	struct i2c_client *client = usbsw->client;
	int intr, intr2, detect;

	/* FSA9485 : Read interrupt -> Read Device
	 FSA9485 : Read Device -> Read interrupt */

	pr_info("fsa9485_irq_thread is called\n");
	/* device detection */
	mutex_lock(&usbsw->mutex);
	detect = fsa9485_detect_dev(usbsw);
	mutex_unlock(&usbsw->mutex);
	pr_info("%s: detect dev_adc: %x\n", __func__, detect);

	/* read and clear interrupt status bits */
	intr = i2c_smbus_read_word_data(client, FSA9485_REG_INT1);
	dev_info(&client->dev, "%s: intr : 0x%x intr2 : 0x%x\n",
					__func__, intr & 0xff, intr >> 8);
	intr2 = intr >> 8;

	if (intr < 0) {
		msleep(100);
		dev_err(&client->dev, "%s: err %d\n", __func__, intr);
		intr = i2c_smbus_read_word_data(client, FSA9485_REG_INT1);
		if (intr < 0)
			dev_err(&client->dev,
				"%s: err at read %d\n", __func__, intr);
		fsa9485_reg_init(usbsw);
		return IRQ_HANDLED;
	} else if (intr == 0) {
		/* interrupt was fired, but no status bits were set,
		so device was reset. In this case, the registers were
		reset to defaults so they need to be reinitialised. */
		fsa9485_reg_init(usbsw);
	}
	/* ADC_value(key pressed) changed at AV_Dock.*/
	if (intr2) {
		if (intr2 & 0x4) { /* for adc change */
			fsa9485_handle_dock_vol_key(usbsw, detect);
			dev_info(&client->dev,
					"intr2: 0x%x, adc_val: %x\n",
							intr2, detect);
		} else if (intr2 & 0x2) { /* for smart dock */
			i2c_smbus_read_word_data(client, FSA9485_REG_INT1);

		} else if (intr2 & 0x1) { /* for av change (desk dock, hdmi) */
			dev_info(&client->dev,
				"%s enter Av charing\n", __func__);
			fsa9485_detect_dev(usbsw);
		} else {
			dev_info(&client->dev,
				"%s intr2 but, nothing happend, intr2: 0x%x\n",
				__func__, intr2);
		}
		return IRQ_HANDLED;
	}

	return IRQ_HANDLED;
}

static int fsa9485_irq_init(struct fsa9485_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int ret;

	if (client->irq) {
		ret = request_threaded_irq(client->irq, NULL,
			fsa9485_irq_thread, IRQF_TRIGGER_FALLING,
			"fsa9485 micro USB", usbsw);
		if (ret) {
			dev_err(&client->dev, "failed to reqeust IRQ\n");
			return ret;
		}

		ret = enable_irq_wake(client->irq);
		if (ret < 0)
			dev_err(&client->dev,
				"failed to enable wakeup src %d\n", ret);
	}

	return 0;
}

static void fsa9485_init_detect(struct work_struct *work)
{
	struct fsa9485_usbsw *usbsw = container_of(work,
			struct fsa9485_usbsw, init_work.work);
	int ret = 0;

	dev_info(&usbsw->client->dev, "%s\n", __func__);

	mutex_lock(&usbsw->mutex);
	fsa9485_detect_dev(usbsw);
	mutex_unlock(&usbsw->mutex);

	ret = fsa9485_irq_init(usbsw);
	if (ret)
		dev_info(&usbsw->client->dev,
				"failed to enable  irq init %s\n", __func__);
}

static int __devinit fsa9485_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct fsa9485_usbsw *usbsw;
	int ret = 0;
	struct input_dev *input;
	struct device *switch_dev;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	input = input_allocate_device();
	usbsw = kzalloc(sizeof(struct fsa9485_usbsw), GFP_KERNEL);
	if (!usbsw || !input) {
		dev_err(&client->dev, "failed to allocate driver data\n");
		kfree(usbsw);
		return -ENOMEM;
	}

	usbsw->input = input;
	input->name = client->name;
	input->phys = "deskdock-key/input0";
	input->dev.parent = &client->dev;
	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0001;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	input_set_capability(input, EV_KEY, KEY_VOLUMEUP);
	input_set_capability(input, EV_KEY, KEY_VOLUMEDOWN);
	input_set_capability(input, EV_KEY, KEY_PLAYPAUSE);
	input_set_capability(input, EV_KEY, KEY_PREVIOUSSONG);
	input_set_capability(input, EV_KEY, KEY_NEXTSONG);

	ret = input_register_device(input);
	if (ret) {
		dev_err(&client->dev,
			"input_register_device %s: err %d\n", __func__, ret);
		input_free_device(input);
		kfree(usbsw);
		return ret;
	}

	usbsw->client = client;
	usbsw->pdata = client->dev.platform_data;
	if (!usbsw->pdata)
		goto fail1;

	i2c_set_clientdata(client, usbsw);

	mutex_init(&usbsw->mutex);

	local_usbsw = usbsw;

	if (usbsw->pdata->cfg_gpio)
		usbsw->pdata->cfg_gpio();

	fsa9485_reg_init(usbsw);

	uart_connecting = 0;

	ret = sysfs_create_group(&client->dev.kobj, &fsa9485_group);
	if (ret) {
		dev_err(&client->dev,
				"failed to create fsa9485 attribute group\n");
		goto fail2;
	}

	/* make sysfs node /sys/class/sec/switch/usb_state */
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_dev)) {
		pr_err("[FSA9485] Failed to create device (switch_dev)!\n");
		ret = PTR_ERR(switch_dev);
		goto fail2;
	}

	ret = device_create_file(switch_dev, &dev_attr_usb_state);
	if (ret < 0) {
		pr_err("[FSA9485] Failed to create file (usb_state)!\n");
		goto err_create_file_state;
	}

	ret = device_create_file(switch_dev, &dev_attr_adc);
	if (ret < 0) {
		pr_err("[FSA9485] Failed to create file (adc)!\n");
		goto err_create_file_adc;
	}

	ret = device_create_file(switch_dev, &dev_attr_reset_switch);
	if (ret < 0) {
		pr_err("[FSA9485] Failed to create file (reset_switch)!\n");
		goto err_create_file_reset_switch;
	}

	dev_set_drvdata(switch_dev, usbsw);
	/* fsa9485 dock init*/
	if (usbsw->pdata->dock_init)
		usbsw->pdata->dock_init();

	/* fsa9485 reset */
	if (usbsw->pdata->reset_cb)
		usbsw->pdata->reset_cb();

	/* set fsa9485 init flag. */
	if (usbsw->pdata->set_init_flag)
		usbsw->pdata->set_init_flag();

	/* initial cable detection */
	INIT_DELAYED_WORK(&usbsw->init_work, fsa9485_init_detect);
	schedule_delayed_work(&usbsw->init_work, msecs_to_jiffies(2700));

	return 0;

err_create_file_reset_switch:
	device_remove_file(switch_dev, &dev_attr_reset_switch);
err_create_file_adc:
	device_remove_file(switch_dev, &dev_attr_adc);
err_create_file_state:
	device_remove_file(switch_dev, &dev_attr_usb_state);
fail2:
	if (client->irq)
		free_irq(client->irq, usbsw);
fail1:
	input_unregister_device(input);
	mutex_destroy(&usbsw->mutex);
	kfree(usbsw);
	return ret;
}

static int __devexit fsa9485_remove(struct i2c_client *client)
{
	struct fsa9485_usbsw *usbsw = i2c_get_clientdata(client);

	cancel_delayed_work(&usbsw->init_work);
	if (client->irq) {
		disable_irq_wake(client->irq);
		free_irq(client->irq, usbsw);
	}
	mutex_destroy(&usbsw->mutex);

	sysfs_remove_group(&client->dev.kobj, &fsa9485_group);
	kfree(usbsw);
	return 0;
}

static int fsa9485_resume(struct i2c_client *client)
{
	struct fsa9485_usbsw *usbsw = i2c_get_clientdata(client);

/* add for fsa9485_irq_thread i2c error during wakeup */
	fsa9485_check_dev(usbsw);

	i2c_smbus_read_byte_data(client, FSA9485_REG_INT1);

	/* device detection */
	mutex_lock(&usbsw->mutex);
	fsa9485_detect_dev(usbsw);
	mutex_unlock(&usbsw->mutex);

	return 0;
}


static const struct i2c_device_id fsa9485_id[] = {
	{"fsa9485", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, fsa9485_id);

static struct i2c_driver fsa9485_i2c_driver = {
	.driver = {
		.name = "fsa9485",
	},
	.probe = fsa9485_probe,
	.remove = __devexit_p(fsa9485_remove),
	.resume = fsa9485_resume,
	.id_table = fsa9485_id,
};

module_i2c_driver(fsa9485_i2c_driver);

MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_DESCRIPTION("FSA9485 USB Switch driver");
MODULE_LICENSE("GPL");
