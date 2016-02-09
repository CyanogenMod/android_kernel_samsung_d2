/*
 *  SMB347-charger.c
 *  SMB347 charger interface driver
 *
 *  Copyright (C) 2011 Samsung Electronics
 *
 *  <jongmyeong.ko@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/power_supply.h>
#include <linux/regulator/machine.h>
#include <linux/smb347_charger.h>
#include <linux/i2c/fsa9485.h>
#include <mach/msm8960-gpio.h>
#include <linux/gpio.h>
#include <asm/system_info.h>

/* Slave address */
#define SMB347_SLAVE_ADDR		0x0C

/* SMB347 Registers. */
#define SMB347_CHARGE_CURRENT		0X00
#define SMB347_INPUT_CURRENTLIMIT	0X01
#define SMB347_VARIOUS_FUNCTIONS	0X02
#define SMB347_FLOAT_VOLTAGE		0X03
#define SMB347_CHARGE_CONTROL		0X04
#define SMB347_STAT_TIMERS_CONTROL	0x05
#define SMB347_PIN_ENABLE_CONTROL	0x06
#define SMB347_THERM_CONTROL_A		0x07
#define SMB347_SYSOK_USB30_SELECTION	0x08
#define SMB347_OTHER_CONTROL_A	0x09
#define SMB347_OTG_TLIM_THERM_CONTROL	0x0A
#define SMB347_LIMIT_CELL_TEMPERATURE_MONITOR	0x0B
#define SMB347_FAULT_INTERRUPT	0x0C
#define SMB347_STATUS_INTERRUPT	0x0D
#define SMB347_I2C_BUS_SLAVE_ADDR	0x0E

#define SMB347_COMMAND_A	0x30
#define SMB347_COMMAND_B	0x31
#define SMB347_COMMAND_C	0x33
#define SMB347_INTERRUPT_STATUS_A	0x35
#define SMB347_INTERRUPT_STATUS_B	0x36
#define SMB347_INTERRUPT_STATUS_C	0x37
#define SMB347_INTERRUPT_STATUS_D	0x38
#define SMB347_INTERRUPT_STATUS_E	0x39
#define SMB347_INTERRUPT_STATUS_F	0x3A
#define SMB347_STATUS_A	0x3B
#define SMB347_STATUS_B	0x3C
#define SMB347_STATUS_C	0x3D
#define SMB347_STATUS_D	0x3E
#define SMB347_STATUS_E	0x3F

/* Status register C */
#define SMB347_CHARGING_ENABLE	(1 << 0)
#define SMB347_CHARGING_STATUS	(1 << 5)
#define SMB347_CHARGER_ERROR	(1 << 6)

/* fast charging current defines */
#define FAST_700mA		700
#define FAST_900mA		900
#define FAST_1200mA	1200
#define FAST_1500mA	1500
#define FAST_1800mA	1800
#define FAST_2000mA	2000
#define FAST_2200mA	2200
#define FAST_2500mA	2500

/* input current limit defines */
#define ICL_300mA		300
#define ICL_500mA		500
#define ICL_700mA		700
#define ICL_900mA		900
#define ICL_1200mA		1200
#define ICL_1500mA		1500
#define ICL_1800mA		1800
#define ICL_2000mA		2000
#define ICL_2200mA		2200
#define ICL_2500mA		2500

#undef SMB347_DEBUG

enum {
	BAT_NOT_DETECTED,
	BAT_DETECTED
};

enum {
	CHG_MODE_NONE,
	CHG_MODE_AC,
	CHG_MODE_USB,
	CHG_MODE_MISC,
	CHG_MODE_UNKNOWN
};

enum {
	OTG_DISABLE,
	OTG_ENABLE
};

enum {
	INPUT_NONE,
	INPUT_DCIN,
	INPUT_USBIN,
};

struct smb347_chip {
	struct i2c_client *client;
	struct delayed_work work;
	struct power_supply psy_bat;
	struct smb347_platform_data *pdata;
	struct mutex mutex;

	int chg_mode;
	unsigned int batt_vcell;
	int chg_set_current;	/* fast charging current */
	int chg_icl;		/* input current limit */
	int lpm_chg_mode;
	unsigned int float_voltage;	/* float voltage */
	int aicl_current;
	int aicl_status;
	int otg_check;
	int input_source;
	int ovp_state;
};

static enum power_supply_property smb347_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
};

static int smb347_disable_charging(struct i2c_client *client);
static int smb347_verA5;

static int smb347_write_reg(struct i2c_client *client, int reg, u8 value)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&chip->mutex);

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0) {
		pr_err("%s: err %d, try again!, reg[0x%x]\n", __func__, ret,
		       reg);
		ret = i2c_smbus_write_byte_data(client, reg, value);
		if (ret < 0)
			pr_err("%s: err %d, reg=0x%x\n", __func__, ret, reg);
	}

	mutex_unlock(&chip->mutex);

	return ret;
}

static int smb347_read_reg(struct i2c_client *client, int reg)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&chip->mutex);

	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0) {
		pr_err("%s: err %d, try again!\n", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, reg);
		if (ret < 0)
			pr_err("%s: err %d\n", __func__, ret);
	}

	mutex_unlock(&chip->mutex);

	return ret;
}

#ifdef SMB347_DEBUG
static void smb347_print_reg(struct i2c_client *client, int reg)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);
	int data = 0;

	mutex_lock(&chip->mutex);

	data = i2c_smbus_read_byte_data(client, reg);

	if (data < 0)
		pr_err("%s: err %d\n", __func__, data);
	else
		pr_info("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);

	mutex_unlock(&chip->mutex);
}

static void smb347_print_all_regs(struct i2c_client *client)
{
	smb347_print_reg(client, 0x00);
	smb347_print_reg(client, 0x01);
	smb347_print_reg(client, 0x02);
	smb347_print_reg(client, 0x03);
	smb347_print_reg(client, 0x04);
	smb347_print_reg(client, 0x05);
	smb347_print_reg(client, 0x06);
	smb347_print_reg(client, 0x07);
	smb347_print_reg(client, 0x08);
	smb347_print_reg(client, 0x09);
	smb347_print_reg(client, 0x0A);
	smb347_print_reg(client, 0x30);
	smb347_print_reg(client, 0x31);
	smb347_print_reg(client, 0x32);
	smb347_print_reg(client, 0x33);
	smb347_print_reg(client, 0x34);
	smb347_print_reg(client, 0x35);
	smb347_print_reg(client, 0x36);
	smb347_print_reg(client, 0x37);
	smb347_print_reg(client, 0x38);
	smb347_print_reg(client, 0x39);
	smb347_print_reg(client, 0x3A);
	smb347_print_reg(client, 0x3B);
	smb347_print_reg(client, 0x3C);
	smb347_print_reg(client, 0x3D);
	smb347_print_reg(client, 0x3E);
	smb347_print_reg(client, 0x3F);
}
#endif

static void check_smb347_version(void)
{
#if defined(CONFIG_MACH_M2)
	if (system_rev >= 0xd)
		smb347_verA5 = 1;
#elif defined(CONFIG_MACH_M2_SKT)
	if (system_rev >= 0x7)
		smb347_verA5 = 1;
#elif defined(CONFIG_MACH_M2_DCM) || defined(CONFIG_MACH_K2_KDI)
	if (system_rev >= 0x3)
		smb347_verA5 = 1;
#elif defined(CONFIG_MACH_JAGUAR)
	if (system_rev >= 0xf)
		smb347_verA5 = 1;
#elif defined(CONFIG_MACH_AEGIS2)
	if (system_rev >= 0x4)
		smb347_verA5 = 1;
#elif defined(CONFIG_MACH_SUPERIORLTE_SKT)
	smb347_verA5 = 1;
#else
	smb347_verA5 = 0;
#endif
}

static void smb347_allow_volatile_writes(struct i2c_client *client)
{
	int val, reg;
	u8 data;

	/* Allow volatile writes to CONFIG registers */
	reg = SMB347_COMMAND_A;
	val = smb347_read_reg(client, reg);
	if ((val >= 0) && !(val & 0x80)) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);
		data |= (0x1 << 7);
		if (smb347_write_reg(client, reg, data) < 0)
			pr_err("%s : error!\n", __func__);
		val = smb347_read_reg(client, reg);
		if (val >= 0) {
			data = (u8) data;
			pr_debug("%s : => reg (0x%x) = 0x%x\n", __func__, reg,
				data);
		}
	}
}

static void smb347_set_command_reg(struct i2c_client *client)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);
	int val, reg;
	u8 data;

	reg = SMB347_COMMAND_B;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);
		if (chip->chg_mode == CHG_MODE_AC ||
		    chip->chg_mode == CHG_MODE_MISC ||
		    chip->chg_mode == CHG_MODE_UNKNOWN) {
			/* CommandB : High-current mode */
			data = 0x03;
		} else if (chip->chg_mode == CHG_MODE_USB) {
			/* CommandB : USB5 */
			data = 0x02;
		} else {
			/* CommandB : USB1 */
			data = 0x00;
		}
		if (smb347_write_reg(client, reg, data) < 0)
			pr_err("%s : error!\n", __func__);
		val = smb347_read_reg(client, reg);
		if (val >= 0) {
			data = (u8) data;
			pr_debug("%s : => reg (0x%x) = 0x%x\n", __func__, reg,
				data);
		}
	}
}

static void smb347_enter_suspend(struct i2c_client *client)
{
	u8 data = 0;

	pr_info("%s: ENTER SUSPEND\n", __func__);
	smb347_write_reg(client, SMB347_COMMAND_A, 0x80);
	smb347_write_reg(client, SMB347_PIN_ENABLE_CONTROL, 0x18);
	data = (data | 0x4);
	smb347_write_reg(client, SMB347_COMMAND_A, data);
}
static int smb347_get_current_input_source(struct i2c_client *client)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);
	int val, val2 = 0;

	val = smb347_read_reg(client, SMB347_INTERRUPT_STATUS_F);
	/*
	if (val >= 0)
		val |= !gpio_get_value(chip->pdata->inok);
	else
		val = !gpio_get_value(chip->pdata->inok);
	*/
	if (val & 0x01) {
		val = ((smb347_read_reg(client, SMB347_STATUS_E) & 0x80) >> 7)
			+ INPUT_DCIN;
	} else {
		pr_info("%s : power is not ok(%d)\n", __func__, val);
		val = INPUT_NONE;
	}

	val2 = smb347_read_reg(client, SMB347_INTERRUPT_STATUS_E);
	if (val2 & 0x04) {
		pr_info("%s:OVP cut off input power\n", __func__);
		chip->ovp_state = 1;
	}

	return val;
}

static void smb347_charger_function_conrol(struct i2c_client *client)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);
	int val, reg;
	u8 data, set_data;

	smb347_allow_volatile_writes(client);

	/* Charge curr : Fast-chg 1500mA */
	/* Pre-charge curr 250mA, Term curr 200mA */
	smb347_write_reg(client, SMB347_CHARGE_CURRENT, 0x7C);

	/* Input current limit */
	reg = SMB347_INPUT_CURRENTLIMIT;
	val = smb347_read_reg(client, reg);

	chip->input_source = smb347_get_current_input_source(chip->client);

	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);

#ifdef CONFIG_WIRELESS_CHARGING
		if (chip->input_source == INPUT_DCIN) {
			pr_info("[battery] INPUT_DCIN\n");
			data &= 0x0f;
			if (chip->chg_mode == CHG_MODE_MISC) {
				pr_info("[battery] wpc 700mA\n");
				data |= 0x20;
			} else {
				pr_info("[battery] wpc 500mA\n");
				data |= 0x10;
			}
		}
#endif /*CONFIG_WIRELESS_CHARGING*/

		pr_info("[battery] INPUT_USBIN\n");
		data &= 0xf0;
		if (chip->chg_mode == CHG_MODE_AC) {
			/* 900mA limit */
			set_data = smb347_verA5 ? 0x4 : 0x3;
		} else if (chip->chg_mode == CHG_MODE_MISC)
			set_data = 0x2;	/* 700mA limit */
		else
			set_data = 0x1;	/* 500mA limit */

		data |= set_data;

		pr_info("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);
			/* this can be changed with top-off setting */
			if (smb347_write_reg(client, reg, data) < 0)
				pr_err("%s : error!\n", __func__);
			val = smb347_read_reg(client, reg);
			if (val >= 0) {
				data = (u8) val;
			pr_debug("%s : charging_current=> reg(0x%x)= 0x%x\n",
					__func__, reg, data);
			}
		}

	reg = SMB347_VARIOUS_FUNCTIONS;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;

#ifdef CONFIG_WIRELESS_CHARGING
		if (chip->input_source == INPUT_DCIN) {
			/* disable AICL, MaxSystemVolt = Vflt + 0.1V */
			data &= (0xCF);
			/* enable VCHG */
			data |= 0x1;
		} else {
			pr_debug("%s : reg (0x%x) = 0x%x\n",
				__func__, reg, data);
			data |= 0x11; /* Enable AICL, VCHG */
			data &= (0xdf); /* Max System voltage =Vflt + 0.1v */
		}
#else
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);
		data |= 0x11; /* Enable AICL, VCHG */
		data &= (0xdf); /* Max System voltage =Vflt + 0.1v */
#endif
		if (smb347_write_reg(client, reg, data) < 0)
			pr_err("%s : error!\n", __func__);
	}

	/* Float voltage : 4.36V Vprechg : 2.4V */
	smb347_write_reg(client, SMB347_FLOAT_VOLTAGE, 0x2B);

	/* Charge control (Auto Recharge)
	 * Automatic Recharge Disabled , Current Termination Enabled,
	 * BMD disable, Recharge Threshold	=50mV,
	 * APSD enable */
	smb347_write_reg(client, SMB347_CHARGE_CONTROL, 0x2C);
	/* smb347_write_reg(client, SMB347_CHARGE_CONTROL, 0x84); */

	/* STAT, Timer control : STAT active low, Complete time out 1527min. */
	smb347_write_reg(client, SMB347_STAT_TIMERS_CONTROL, 0x1A);

#ifdef CONFIG_MACH_JAGUAR
	/* Pin enable control : Charger enable control EN Pin - Active Low */
	/*	: USB5/1/HC or USB9/1.5/HC Control - Pin Control */
	/*	: USB5/1/HC Input state - Tri-state Input */
	if (smb347_verA5) {
		smb347_write_reg(client, SMB347_PIN_ENABLE_CONTROL, 0x01);
		pr_debug("[battery]%s A5 battery is using\n",
			__func__);
	} else {
		smb347_write_reg(client, SMB347_PIN_ENABLE_CONTROL, 0x60);
		pr_debug("[battery]%s A5 battery is not using\n",
			__func__);
	}
#else
	/* Pin enable control : Register - Active High */
	/*	: USB5/1/HC or USB9/1.5/HC Control - Register Control */
	/*	: USB5/1/HC Input state - Tri-State Input*/
	smb347_write_reg(client, SMB347_PIN_ENABLE_CONTROL, 0x01);
#endif /* CONFIG_MACH_JAGUAR */

	/* Therm control : Therm monitor disable */
	smb347_write_reg(client, SMB347_THERM_CONTROL_A, 0x3F);

	/* USB selection : USB2.0(100mA/500mA), INOK polarity Active low */
	smb347_write_reg(client, SMB347_SYSOK_USB30_SELECTION, 0x08);

	/* Other control, Low batt detection disable */
	smb347_write_reg(client, SMB347_OTHER_CONTROL_A, 0x00);

	/* OTG tlim therm control UVLO 2.7V current limit 100mA*/
	smb347_write_reg(client, SMB347_OTG_TLIM_THERM_CONTROL, 0x30);

	/* Limit cell temperature */
	smb347_write_reg(client, SMB347_LIMIT_CELL_TEMPERATURE_MONITOR, 0x01);

	/* Fault interrupt : Clear */
	smb347_write_reg(client, SMB347_FAULT_INTERRUPT, 0x00);

	/* STATUS ingerrupt : Clear (Auto Recharg) */
	smb347_write_reg(client, SMB347_STATUS_INTERRUPT, 0x02);
	/* smb347_write_reg(client, SMB347_STATUS_INTERRUPT, 0x00); */

}

static int smb347_watchdog_control(struct i2c_client *client, bool enable)
{
	return 0;
}

static int smb347_check_charging_status(struct i2c_client *client)
{
	int val, reg;
	u8 data = 0;
	int ret = -1;

	reg = SMB347_STATUS_C;	/* SMB328A_BATTERY_CHARGING_STATUS_C */
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);

		ret = (data & (0x3 << 1)) >> 1;
		pr_debug("%s : status = 0x%x\n", __func__, data);
	}

	return ret;
}

static bool smb347_check_is_charging(struct i2c_client *client)
{
	int val, reg;
	u8 data = 0;
	bool ret = false;

	reg = SMB347_STATUS_C;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);

		if (data & 0x1)
			ret = true;	/* charger enabled */
	}

	return ret;
}

static bool smb347_check_bat_full(struct i2c_client *client)
{
	int val, reg;
	bool ret = false;

	reg = SMB347_STATUS_C;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		/* At least one charge cycle terminated */
		/*Charge current < Termination Current */
		if (val & SMB347_CHARGING_STATUS)
			ret = true;	/* full */
	}

	return ret;
}

/* vf check */
static bool smb347_check_bat_missing(struct i2c_client *client)
{
	int val, reg;
	u8 data = 0;
	bool ret = false;

	/* SMB328A_BATTERY_CHARGING_STATUS_B */
	reg = SMB347_INTERRUPT_STATUS_B;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);

		if (data & (0x1 << 4)) {
			pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg,
				data);
			pr_info("%s: Battery Missing\n", __func__);
			ret = true;	/* missing battery */
		}
	}

	return ret;
}

/* whether valid dcin or not */
static bool smb347_check_vdcin(struct i2c_client *client)
{
	int val, reg;
	u8 data = 0;
	bool ret = false;

	reg = SMB347_INTERRUPT_STATUS_F;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);

		if (data & (0x1))
			ret = true;
	}
	return ret;

}

static bool smb347_check_bmd_disabled(struct i2c_client *client)
{
	bool ret = false;
	return ret;
}

static int smb347_chg_get_property(struct power_supply *psy,
				   enum power_supply_property psp,
				   union power_supply_propval *val)
{
	struct smb347_chip *chip = container_of(psy,
						struct smb347_chip, psy_bat);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (smb347_check_vdcin(chip->client))
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		if (smb347_check_bat_missing(chip->client))
			val->intval = BAT_NOT_DETECTED;
		else
			val->intval = BAT_DETECTED;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		pr_debug("%s : check bmd available\n", __func__);
		/* check VF check available */
		if (smb347_check_bmd_disabled(chip->client))
			val->intval = 1;
		else
			val->intval = 0;
		pr_debug("smb347_check_bmd_disabled is %d\n", val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		if (smb347_check_bat_full(chip->client))
			val->intval = 1;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		switch (smb347_check_charging_status(chip->client)) {
		case 0:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
			break;
		case 1:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
			break;
		case 2:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
			break;
		case 3:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		default:
			pr_err("%s : get charge type error!\n", __func__);
			return -EINVAL;
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		if (smb347_check_is_charging(chip->client))
			val->intval = 1;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CURRENT_ADJ:
		pr_debug("%s : get charging current\n", __func__);
		if (chip->chg_set_current != 0)
			val->intval = chip->chg_set_current;
		else
			return -EINVAL;
		break;
#ifdef CONFIG_WIRELESS_CHARGING
	case POWER_SUPPLY_PROP_WIRELESS_CHARGING:
		val->intval = smb347_get_current_input_source(chip->client);
		break;
#endif
	default:
		return -EINVAL;
	}
	return 0;
}

static int smb347_set_top_off(struct i2c_client *client, int top_off)
{
	int val, reg, set_val = 0;
	u8 data;

	smb347_allow_volatile_writes(client);

	/* set termination current */
	reg = SMB347_CHARGE_CURRENT;
	if (top_off == 37)
		set_val = 0;
	else if (top_off >= 50 && top_off <= 250)
		set_val = top_off / 50;
	else if (top_off == 500)
		set_val = 6;
	else if (top_off == 600)
		set_val = 7;

	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);
		data &= ~(0x7 << 0);
		data |= (set_val << 0);
		if (smb347_write_reg(client, reg, data) < 0) {
			pr_err("%s : error!\n", __func__);
			return -1;
		}
		data = smb347_read_reg(client, reg);
		pr_info("%s : => reg (0x%x) = 0x%x\n", __func__, reg, data);
	}
	return 0;
}

static int smb347_set_charging_current(struct i2c_client *client,
				       int chg_current)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);

	pr_info("%s : %d\n", __func__, chg_current);

	if (chg_current < 450 || chg_current > 1200)
		return -EINVAL;

	chip->chg_set_current = chg_current;

	if (chg_current == 500) {
		chip->chg_mode = CHG_MODE_USB;
	} else if (chg_current == 900) {
		chip->chg_mode = CHG_MODE_AC;
	} else if (chg_current == 700) {
		chip->chg_mode = CHG_MODE_MISC;
	} else if (chg_current == 450) {
		chip->chg_mode = CHG_MODE_UNKNOWN;
	} else {
		pr_err("%s : error! invalid setting current (%d)\n",
		       __func__, chg_current);
		chip->chg_mode = CHG_MODE_NONE;
		chip->chg_set_current = 0;
		return -1;
	}

	return 0;
}

static int smb347_set_fast_current(struct i2c_client *client, int fast_current)
{
	int val, reg, set_val = 0;
	u8 data;

	smb347_allow_volatile_writes(client);
	switch (fast_current) {
	case FAST_700mA:
		set_val = 0x0;
		break;
	case FAST_900mA:
		set_val = 0x1;
		break;
	case FAST_1200mA:
		set_val = 0x2;
		break;
	case FAST_1500mA:
		set_val = 0x3;
		break;
	case FAST_1800mA:
		set_val = 0x4;
		break;
	case FAST_2000mA:
		set_val = 0x5;
		break;
	case FAST_2200mA:
		set_val = 0x6;
		break;
	case FAST_2500mA:
		set_val = 7;
		break;
	default:
		set_val = 0;
		break;
	}

	reg = SMB347_CHARGE_CURRENT;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x, set_val = 0x%x\n",
			__func__, reg, data, set_val);
		data &= ~(0x7 << 5);
		data |= (set_val << 5);
		pr_debug("%s : write data = 0x%x\n", __func__, data);
		if (smb347_write_reg(client, reg, data) < 0) {
			pr_err("%s : error!\n", __func__);
			return -1;
		}
		data = smb347_read_reg(client, reg);
		pr_debug("%s : => reg (0x%x) = 0x%x\n", __func__, reg, data);
	}

	return 0;
}

static int smb347_set_input_current_limit(struct i2c_client *client,
					  int input_current)
{
	int val, reg, data;

	smb347_allow_volatile_writes(client);

	if (input_current < ICL_300mA || input_current > ICL_2500mA) {
		pr_err("%s: invalid input_current set value(%d)\n",
		       __func__, input_current);
		return -EINVAL;
	}

	switch (input_current) {
	case ICL_300mA:
		val = 0x20;
		break;
	case ICL_500mA:
		val = 0x21;
		break;
	case ICL_700mA:
		val = 0x22;
		break;
	case ICL_900mA:
		val = 0x23;
		break;
	case ICL_1200mA:
		val = 0x24;
		break;
	case ICL_1500mA:
		val = 0x25;
		break;
	case ICL_1800mA:
		val = 0x26;
		break;
	case ICL_2000mA:
		val = 0x27;
		break;
	case ICL_2200mA:
		val = 0x28;
		break;
	case ICL_2500mA:
		val = 0x29;
		break;
	default:
		val = 7;
		break;
	}

	reg = SMB347_INPUT_CURRENTLIMIT;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x, set_val = 0x%x\n",
			__func__, reg, data, input_current);
		data &= ~(0xff);
		data |= val;
		pr_debug("%s : write data = 0x%x\n", __func__, data);
		if (smb347_write_reg(client, reg, data) < 0) {
			pr_err("%s : error!\n", __func__);
			return -1;
		}
		data = smb347_read_reg(client, reg);
		pr_debug("%s : => reg (0x%x) = 0x%x\n", __func__, reg, data);
	}

	return 0;
}

static int smb347_adjust_charging_current(struct i2c_client *client,
					  int chg_current)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);
	int ret = 0;

	pr_debug("%s :\n", __func__);

	if (chg_current < 500 || chg_current > 1200)
		return -EINVAL;

	chip->chg_set_current = chg_current;

	switch (chg_current) {
	case FAST_700mA:
	case FAST_900mA:
	case FAST_1200mA:
	case FAST_1500mA:
	case FAST_1800mA:
	case FAST_2000mA:
	case FAST_2200mA:
	case FAST_2500mA:
		ret = smb347_set_fast_current(client, chg_current);
		break;
	default:
		pr_err("%s : error! invalid setting current (%d)\n",
		       __func__, chg_current);
		chip->chg_set_current = 0;
		return -EINVAL;
	}

	return ret;
}

static int smb347_get_input_current_limit(struct i2c_client *client)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);
	int val, reg = 0;
	u8 data = 0;

	pr_debug("%s :\n", __func__);

	reg = SMB347_INPUT_CURRENTLIMIT;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);

#ifdef CONFIG_WIRELESS_CHARGING
		if (smb347_get_current_input_source(chip->client)
			== INPUT_DCIN) {
			data &= (0xf << 4);
			data >>= 4;
		} else
#endif
			data &= 0xf;

		if (data > 9) {
			pr_err("%s: invalid icl value(%d)\n", __func__, data);
			return -EINVAL;
		}

		switch (data) {
		case 0:
			chip->chg_icl = ICL_300mA;
			break;
		case 1:
			chip->chg_icl = ICL_500mA;
			break;
		case 2:
			chip->chg_icl = ICL_700mA;
			break;
		case 3:
			chip->chg_icl = ICL_900mA;
			break;
		case 4:
			chip->chg_icl = ICL_1200mA;
			break;
		case 5:
			chip->chg_icl = ICL_1500mA;
			break;
		case 6:
			chip->chg_icl = ICL_1800mA;
			break;
		case 7:
			chip->chg_icl = ICL_2000mA;
			break;
		case 8:
			chip->chg_icl = ICL_2200mA;
			break;
		case 9:
			chip->chg_icl = ICL_2500mA;
			break;
		default:
			chip->chg_icl = ICL_300mA;
			break;
		}

		pr_debug("%s : get icl = %d, data = %d\n",
			__func__, chip->chg_icl, data);
	} else {
		pr_err("%s: get icl failed\n", __func__);
		chip->chg_icl = 0;
		return -EINVAL;
	}

	return 0;
}

static int smb347_get_AICL_status(struct i2c_client *client)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);
	int val, reg = 0;
	u8 data = 0;

	pr_debug("%s :\n", __func__);

	reg = SMB347_STATUS_E;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		dev_dbg(&client->dev, "%s : reg (0x%x) = 0x%x\n",
			__func__, reg, data);

		chip->aicl_status = (data >> 4) & 0x1;

		data &= 0xf;
		if (data <= 3)
			chip->aicl_current = (data + 1) * 200 + 100;
		else if (data >= 4 && data <= 6)
			chip->aicl_current = (data - 3) * 300 + 900;
		else if (data == 7)
			chip->aicl_current = ICL_2000mA;
		else if (data == 8)
			chip->aicl_current = ICL_2200mA;
		else
			chip->aicl_current = ICL_2500mA;

		dev_dbg(&client->dev,
			"%s : get aicl = %d, status = %d, data = %d\n",
			__func__, chip->aicl_current, chip->aicl_status, data);
	} else {
		pr_err("%s: get aicl failed\n", __func__);
		chip->aicl_current = 0;
		return -EINVAL;
	}

	return 0;
}

static int smb347_enable_otg(struct i2c_client *client)
{
	/* TODO later */
	int val, reg;
	u8 data;
	struct smb347_chip *chip = i2c_get_clientdata(client);
	dev_info(&client->dev, "%s\n", __func__);
	reg = SMB347_COMMAND_A;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);

		data |= (0x1 << 4);	/* "1" turn on the otg 5v */
		if (smb347_write_reg(client, reg, data) < 0) {
			pr_err("%s : error!\n", __func__);
			return -1;
		}
		data = smb347_read_reg(client, reg);
		pr_info("%s : => reg (0x%x) = 0x%x\n", __func__, reg, data);
		chip->otg_check = OTG_ENABLE;
	}
	return 0;
}

static int smb347_disable_otg(struct i2c_client *client)
{
	/* TODO later */
	int val, reg;
	u8 data;
	struct smb347_chip *chip = i2c_get_clientdata(client);
	dev_info(&client->dev, "%s\n", __func__);
	reg = SMB347_COMMAND_A;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);

		data &= ~(0x1 << 4);	/* "0" turn off the otg 5v */
		if (smb347_write_reg(client, reg, data) < 0) {
			pr_err("%s : error!\n", __func__);
			return -1;
		}
		data = smb347_read_reg(client, reg);
		pr_info("%s : => reg (0x%x) = 0x%x\n", __func__, reg, data);
		chip->otg_check = OTG_DISABLE;
	}
	return 0;
}

static int smb347_enable_charging(struct i2c_client *client)
{
	int val, reg;
	u8 data;
	struct smb347_chip *chip = i2c_get_clientdata(client);

	pr_debug("%s :\n", __func__);

	/* register control */
	reg = SMB347_COMMAND_A;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);
		if (chip->chg_mode == CHG_MODE_AC ||
		    chip->chg_mode == CHG_MODE_MISC ||
		    chip->chg_mode == CHG_MODE_UNKNOWN)
			data = 0x82;
		else if (chip->chg_mode == CHG_MODE_USB)
			data = 0x82;
		else
			data = 0x80;
		if (smb347_write_reg(client, reg, data) < 0) {
			pr_err("%s : error!\n", __func__);
			return -1;
		}
		data = smb347_read_reg(client, reg);
		pr_debug("%s : => reg (0x%x) = 0x%x\n", __func__, reg, data);
	}
#ifdef CONFIG_MACH_JAGUAR
	gpio_set_value_cansleep(chip->pdata->enable, 0);
#endif
	return 0;
}

static int smb347_disable_charging(struct i2c_client *client)
{
	int val, reg;
	u8 data;
	struct smb347_chip *chip = i2c_get_clientdata(client);

	pr_debug("%s :\n", __func__);

	/* register control */
	reg = SMB347_COMMAND_A;
	val = smb347_read_reg(client, reg);
	if (val >= 0) {
		data = (u8) val;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, data);
		data = 0x80;
		if (smb347_write_reg(client, reg, data) < 0) {
			pr_err("%s : error!\n", __func__);
			return -1;
		}
		data = smb347_read_reg(client, reg);
		pr_debug("%s : => reg (0x%x) = 0x%x\n", __func__, reg, data);
	}

	chip->chg_mode = CHG_MODE_NONE;
	chip->chg_set_current = 0;
	chip->chg_icl = 0;

#ifdef CONFIG_MACH_JAGUAR
	gpio_set_value_cansleep(chip->pdata->enable, 1);
#endif
	return 0;
}

static int smb347_chg_set_property(struct power_supply *psy,
				   enum power_supply_property psp,
				   const union power_supply_propval *val)
{
	struct smb347_chip *chip = container_of(psy,
						struct smb347_chip, psy_bat);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		/* step1) Set charging current */
		ret = smb347_set_charging_current(chip->client, val->intval);
		smb347_set_command_reg(chip->client);
		smb347_charger_function_conrol(chip->client);
		smb347_get_input_current_limit(chip->client);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		/* step2) Set top-off current */
		if (val->intval < 37 || val->intval > 600) {
			pr_err("%s: invalid topoff current(%d)\n",
			       __func__, val->intval);
			return -EINVAL;
		}
		ret = smb347_set_top_off(chip->client, val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/* step3) Notify Vcell Now */
		chip->batt_vcell = val->intval;
		pr_debug("%s : vcell(%d)\n", __func__, chip->batt_vcell);
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		/* step4) Enable/Disable charging */
		if (val->intval == POWER_SUPPLY_STATUS_CHARGING) {
			ret = smb347_enable_charging(chip->client);
			smb347_watchdog_control(chip->client, true);
		} else {
			ret = smb347_disable_charging(chip->client);
			smb347_watchdog_control(chip->client, false);
		}
		break;
	case POWER_SUPPLY_PROP_OTG:
		if (val->intval == POWER_SUPPLY_CAPACITY_OTG_ENABLE) {
			smb347_charger_function_conrol(chip->client);
			ret = smb347_enable_otg(chip->client);
			mdelay(5);
			/*UVLO 2.7V current limit 500mA*/
			smb347_write_reg(chip->client,
				SMB347_OTG_TLIM_THERM_CONTROL, 0x38);
		} else
			ret = smb347_disable_otg(chip->client);
		break;
	case POWER_SUPPLY_PROP_CURRENT_ADJ:
		pr_info("%s : adjust charging current from %d to %d\n",
			__func__, chip->chg_set_current, val->intval);
		if (chip->chg_mode == CHG_MODE_AC) {
			ret =
			    smb347_adjust_charging_current(chip->client,
							   val->intval);
		} else {
			pr_info
			    ("%s : not AC mode, skip fast current adjusting\n",
			     __func__);
		}
		break;
#ifdef CONFIG_WIRELESS_CHARGING
	case POWER_SUPPLY_PROP_WIRELESS_CHARGING:
		pr_info("%s : set input source type(%d)\n",
			__func__, val->intval);
		chip->input_source = val->intval;
		break;
#endif
	case POWER_SUPPLY_PROP_PRESENT:
		pr_info("%s : Battery is removed. Cut off charging.\n",
			__func__);
		smb347_enter_suspend(chip->client);

		break;
	default:
		return -EINVAL;
	}

#ifdef SMB347_DEBUG
	smb347_print_all_regs(chip->client);
#endif
	return ret;
}

static ssize_t sec_smb347_show_property(struct device *dev,
					struct device_attribute *attr,
					char *buf);
static ssize_t sec_smb347_store_property(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count);

#define SEC_SMB347_ATTR(_name)\
{\
	.attr = { .name = #_name,	\
		  .mode = 0664,	\
		 /* .owner = THIS_MODULE */ },	\
	.show = sec_smb347_show_property,		\
	.store = sec_smb347_store_property,	\
}

static struct device_attribute sec_smb347_attrs[] = {
	SEC_SMB347_ATTR(smb_read_3Dh),
	SEC_SMB347_ATTR(smb_wr_icl),
	SEC_SMB347_ATTR(smb_wr_fast),
	SEC_SMB347_ATTR(smb_read_fv),
	SEC_SMB347_ATTR(smb_read_aicl),
#ifdef CONFIG_WIRELESS_CHARGING
	SEC_SMB347_ATTR(smb_input_source),
	SEC_SMB347_ATTR(smb_inok),
#endif
};

enum {
	SMB_READ_3DH = 0,
	SMB_WR_ICL,
	SMB_WR_FAST,
	SMB_READ_FV,
	SMB_READ_AICL,
#ifdef CONFIG_WIRELESS_CHARGING
	SMB_INPUT_SOURCE,
	SMB_INOK,
#endif
};

static ssize_t sec_smb347_show_property(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct smb347_chip *chip = container_of(psy,
						struct smb347_chip,
						psy_bat);

	int i = 0;
	const ptrdiff_t off = attr - sec_smb347_attrs;
	int val, reg;
	u8 data = 0;

	switch (off) {
	case SMB_READ_3DH:
		reg = SMB347_STATUS_C;
		val = smb347_read_reg(chip->client, reg);
		if (val >= 0) {
			data = (u8) val;
			pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg,
				 data);
			i += scnprintf(buf + i, PAGE_SIZE - i,
				       "0x%x (bit6 : %d)\n", data,
				       (data & 0x40) >> 6);
		} else {
			i = -EINVAL;
		}
		break;
	case SMB_WR_ICL:
		reg = SMB347_INPUT_CURRENTLIMIT;
		val = smb347_read_reg(chip->client, reg);
		if (val >= 0) {
			data = (u8) val;
			pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg,
				 data);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d (0x%x)\n",
				       chip->chg_icl, data);
		} else {
			i = -EINVAL;
		}
		break;
	case SMB_WR_FAST:
		reg = SMB347_CHARGE_CURRENT;
		val = smb347_read_reg(chip->client, reg);
		if (val >= 0) {
			data = (u8) val;
			pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg,
				 data);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d (0x%x)\n",
				       chip->chg_set_current, data);
		} else {
			i = -EINVAL;
		}
		break;
	case SMB_READ_FV:
		reg = SMB347_FLOAT_VOLTAGE;
		val = smb347_read_reg(chip->client, reg);
		if (val >= 0) {
			data = (u8) val;
			pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg,
				 data);
			i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x (%dmV)\n",
				       data, 3500 + ((data & 0x3F) * 20));
		} else {
			i = -EINVAL;
		}
		break;
	case SMB_READ_AICL:
		val = smb347_get_AICL_status(chip->client);
		if (val >= 0) {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%dmA (%d)\n",
				       chip->aicl_current, chip->aicl_status);
		} else {
			i = -EINVAL;
		}
		break;
#ifdef CONFIG_WIRELESS_CHARGING
	case SMB_INPUT_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			chip->input_source);
		break;
	case SMB_INOK:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			gpio_get_value(chip->pdata->inok));
		break;
#endif
	default:
		i = -EINVAL;
	}

	return i;
}

static ssize_t sec_smb347_store_property(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct smb347_chip *chip = container_of(psy,
						struct smb347_chip,
						psy_bat);

	int x = 0;
	int ret = 0;
	const ptrdiff_t off = attr - sec_smb347_attrs;

	switch (off) {
	case SMB_WR_ICL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (chip->chg_mode == CHG_MODE_AC) {
				ret =
				    smb347_set_input_current_limit(chip->client,
								   x);
			} else {
				pr_info
				    ("%s : not AC mode, skip icl adjusting\n",
				     __func__);
				ret = count;
			}
		}
		break;
	case SMB_WR_FAST:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (chip->chg_mode == CHG_MODE_AC) {
				ret = smb347_adjust_charging_current
					(chip->client, x);
			} else {
				pr_info("%s : not AC mode, skip SMB_WR_FAST\n",
					__func__);
				ret = count;
			}
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int smb347_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(sec_smb347_attrs); i++) {
		rc = device_create_file(dev, &sec_smb347_attrs[i]);
		if (rc)
			goto smb347_attrs_failed;
	}
	goto succeed;

smb347_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_smb347_attrs[i]);
succeed:
	return rc;
}

static irqreturn_t smb347_int_work_func(int irq, void *smb_chip)
{
	struct smb347_chip *chip = smb_chip;
	int val, reg;
	u8 chg_status = 0;

	pr_debug("%s\n", __func__);

	reg = SMB347_INTERRUPT_STATUS_B;
	val = smb347_read_reg(chip->client, reg);
	if (val >= 0 &&
		val & 0x20) {
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, val);
		pr_info("%s : Battery Missing!!!\n", __func__);
		smb347_enter_suspend(chip->client);
		return IRQ_HANDLED;
	}

	reg = SMB347_STATUS_C;
	val = smb347_read_reg(chip->client, reg);
	if (val >= 0) {
		if (val & 0x01)
			chg_status = POWER_SUPPLY_STATUS_CHARGING;
		else
			chg_status = POWER_SUPPLY_STATUS_DISCHARGING;
		pr_debug("%s : reg (0x%x) = 0x%x\n", __func__, reg, val);
	}

	if (chip->pdata->chg_intr_trigger)
		chip->pdata->chg_intr_trigger((int)(chg_status));

	/*
	   u8 intr_a = 0;
	   u8 intr_b = 0;

	   reg = SMB347_INTERRUPT_STATUS_A;
	   val = smb347_read_reg(chip->client, reg);
	   if (val >= 0) {
	   intr_a = (u8)val;
	   pr_info("%s : reg (0x%x) = 0x%x\n", __func__, reg, intr_a);
	   }
	   reg = SMB347_INTERRUPT_STATUS_B;
	   val = smb347_read_reg(chip->client, reg);
	   if (val >= 0) {
	   intr_b = (u8)val;
	   pr_info("%s : reg (0x%x) = 0x%x\n", __func__, reg, intr_b);
	   }

	reg = SMB347_INTERRUPT_STATUS_C;
	val = smb347_read_reg(chip->client, reg);
	if (val >= 0) {
		intr_c = (u8) val;
		pr_info("%s : reg (0x%x) = 0x%x\n", __func__, reg, intr_c);
	}

	reg = SMB347_STATUS_C;
	val = smb347_read_reg(chip->client, reg);
	if (val >= 0) {
		chg_status = (u8) val;
		pr_info("%s : reg (0x%x) = 0x%x\n", __func__, reg, chg_status);
	}

	if (chip->pdata->chg_intr_trigger)
		chip->pdata->chg_intr_trigger((int)(chg_status & 0x1));
*/
	return IRQ_HANDLED;
}

static void smb347_AICL_enable(struct i2c_client *client, bool en)
{
	int reg, val;
	u8 data;

	smb347_allow_volatile_writes(client);

	reg = SMB347_VARIOUS_FUNCTIONS;
	val = smb347_read_reg(client, reg);

	if (val >= 0) {
		data = (u8) val;
	} else {
		pr_err("%s: fail to read SMB347 02h reg\n", __func__);
		return;
	}

	if (en) {
		pr_info("%s : enable AICL\n", __func__);
		data |= 0x10; /* Enable AICL*/

	} else {
		pr_info("%s : disable AICL\n", __func__);
		/* clear BIT4 (disable AICL) */
		data &= (0xEF);
	}
	if (smb347_write_reg(client, reg, data) < 0)
		pr_err("%s : error!\n", __func__);

}

static irqreturn_t smb347_inok_work_func(int irq, void *smb_chip)
{
	struct smb347_chip *chip = smb_chip;
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;
	int input_source = 0;
	int cable_type = 0;
	int ret = 0;

	if (psy) {
		/* check usbin or dcin status */
		/* 0 : no dcin, 1: DCIN, 2: USBIN  */
		input_source = smb347_get_current_input_source(chip->client);
		switch (input_source) {
		case INPUT_NONE:
			ret = smb347_read_reg(chip->client,
				SMB347_INTERRUPT_STATUS_E);
			if (ret & 0x04) {
				pr_info("%s: OVP cut off input power\n",
					__func__);
				chip->ovp_state = 1;
			} else {
				pr_debug("%s: OVP isn't set\n", __func__);
				chip->ovp_state = 0;
			}

			if (chip->input_source == INPUT_DCIN) {
				/* AICL enable */
				smb347_AICL_enable(chip->client, true);

				value.intval = POWER_SUPPLY_TYPE_BATTERY;
				ret = psy->set_property(psy,
					POWER_SUPPLY_PROP_ONLINE, &value);
				if (ret < 0)
					pr_err("%s : failed to set power_suppy online property(%d)\n",
						__func__, ret);
			} else if (chip->input_source == INPUT_USBIN) {
				ret = psy->get_property(psy,
					POWER_SUPPLY_PROP_PRESENT, &value);

				if (ret < 0)
					pr_err("%s : failed present(%d)\n",
							__func__, ret);

				if (value.intval) {
					value.intval =
						POWER_SUPPLY_TYPE_BATTERY;
					ret = psy->set_property(psy,
					POWER_SUPPLY_PROP_ONLINE, &value);
					if (ret < 0)
						pr_err("%s : failed(%d)\n",
							__func__, ret);
				}
			}
			pr_info("%s : no input source(%d --> %d)\n",
				__func__, chip->input_source, input_source);
			break;
		case INPUT_DCIN:
			/* AICL disable */
			smb347_AICL_enable(chip->client, false);
#ifdef CONFIG_WIRELESS_CHARGING
			value.intval = POWER_SUPPLY_TYPE_WPC;
			ret = psy->set_property(psy,
				POWER_SUPPLY_PROP_ONLINE, &value);
			if (ret < 0)
				pr_err("%s : failed to set power_suppy online property(%d)\n",
					__func__, ret);
#endif
			break;
		case INPUT_USBIN:
			if (chip->pdata->smb347_get_cable) {
				if (chip->ovp_state) {
					cable_type =
						chip->pdata->smb347_get_cable();
					pr_info("%s: Recovery OVP, restart charging (%d)\n",
						__func__, cable_type);
				}
			}
			chip->ovp_state = 0;
			break;
		default:
			pr_err("%s : failed to read input source type(%d)\n",
				__func__, chip->input_source);
			return IRQ_HANDLED;
		}
		chip->input_source = input_source;
	} else
		pr_err("%s : failed to get battery psy\n", __func__);

	return IRQ_HANDLED;
}

static int __devinit smb347_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct smb347_chip *chip;
	int ret = 0;
	int value = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;
	if (!chip->pdata) {
		pr_err("%s: no charger platform data\n", __func__);
		goto err_kfree;
	}

	check_smb347_version();
	pr_info("%s: SMB347 driver Loading!\n", __func__);

	i2c_set_clientdata(client, chip);

	chip->pdata->hw_init();	/* important */
	chip->chg_mode = CHG_MODE_NONE;
	chip->chg_set_current = 0;
	chip->chg_icl = 0;
	chip->float_voltage = 0;
	chip->ovp_state = 0;

	if (poweroff_charging) {
		chip->lpm_chg_mode = 1;
		pr_info("%s : is lpm charging mode (%d)\n",
			__func__, chip->lpm_chg_mode);
	}
	mutex_init(&chip->mutex);

	chip->psy_bat.name = "sec-charger",
	chip->psy_bat.type = POWER_SUPPLY_TYPE_UNKNOWN,
	chip->psy_bat.properties = smb347_battery_props,
	chip->psy_bat.num_properties = ARRAY_SIZE(smb347_battery_props),
	chip->psy_bat.get_property = smb347_chg_get_property,
	chip->psy_bat.set_property = smb347_chg_set_property,
	ret = power_supply_register(&client->dev, &chip->psy_bat);
	if (ret) {
		pr_err("Failed to register power supply psy_bat\n");
		goto err_psy_register;
	}

	/* Vdcin polarity setting */
	value = smb347_read_reg(client, SMB347_SYSOK_USB30_SELECTION);
	value = (value | 0x1);
	ret =
		smb347_write_reg(client, SMB347_SYSOK_USB30_SELECTION, value);
	if (ret < 0) {
		pr_err("%s: INOK polarity setting error!\n", __func__);
	}

	ret =
	    request_threaded_irq(chip->client->irq, NULL, smb347_int_work_func,
				 IRQF_TRIGGER_FALLING, "smb347", chip);
	if (ret) {
		pr_err("%s : Failed to request smb347 charger irq\n", __func__);
		goto err_request_irq;
	}

	ret = enable_irq_wake(chip->client->irq);
	if (ret) {
		pr_err("%s : Failed to enable smb347 charger irq wake\n",
		       __func__);
		goto err_irq_wake;
	}
	ret = request_threaded_irq(
		MSM_GPIO_TO_INT(chip->pdata->inok),
		NULL,
		smb347_inok_work_func,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		"smb347 inok",
		chip);

	if (ret) {
		pr_err("%s : Failed to request smb347 charger inok irq\n",
			__func__);
		goto err_irq_wake;
	}

	ret = enable_irq_wake(
		MSM_GPIO_TO_INT(chip->pdata->inok));
	if (ret) {
		pr_err("%s : Failed to enable smb347 charger inok irq wake\n",
			   __func__);
		goto err_irq_wake2;
	}

#ifdef CONFIG_WIRELESS_CHARGING
	if (smb347_get_current_input_source(chip->client) == INPUT_DCIN) {
		pr_info("[battery] SMB347 driver detect DCIN\n");
		if (chip->pdata->smb347_wpc_cb)
			chip->pdata->smb347_wpc_cb();
	}
#endif

#ifdef SMB347_DEBUG
	smb347_print_all_regs(client);
#endif

	smb347_create_attrs(chip->psy_bat.dev);

	return 0;
err_irq_wake2:
	 free_irq(MSM_GPIO_TO_INT(chip->pdata->inok), NULL);
err_irq_wake:
	free_irq(chip->client->irq, NULL);
err_request_irq:
	power_supply_unregister(&chip->psy_bat);
err_psy_register:
	mutex_destroy(&chip->mutex);
err_kfree:
	kfree(chip);
	return ret;
}

static int __devexit smb347_remove(struct i2c_client *client)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);

	power_supply_unregister(&chip->psy_bat);
	mutex_destroy(&chip->mutex);
	kfree(chip);
	return 0;
}

static int smb347_suspend(struct i2c_client *client, pm_message_t state)
{
	return 0;
}

static int smb347_resume(struct i2c_client *client)
{
	return 0;
}

static void smb347_shutdown(struct i2c_client *client)
{
	struct smb347_chip *chip = i2c_get_clientdata(client);
	if (chip != NULL) {
		if (chip->otg_check == OTG_ENABLE)
			smb347_disable_otg(chip->client);
	}
}

static const struct i2c_device_id smb347_id[] = {
	{"smb347", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, smb347_id);

static struct i2c_driver smb347_i2c_driver = {
	.driver = {
		   .name = "smb347",
		   },
	.probe = smb347_probe,
	.remove = __devexit_p(smb347_remove),
	.suspend = smb347_suspend,
	.resume = smb347_resume,
	.shutdown = smb347_shutdown,
	.id_table = smb347_id,
};

static int __init smb347_init(void)
{
	return i2c_add_driver(&smb347_i2c_driver);
}

module_init(smb347_init);

static void __exit smb347_exit(void)
{
	i2c_del_driver(&smb347_i2c_driver);
}

module_exit(smb347_exit);

MODULE_DESCRIPTION("SMB347 charger control driver");
MODULE_AUTHOR("<jongmyeong.ko@samsung.com>");
MODULE_LICENSE("GPL");
