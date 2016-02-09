/*
 *  max17040_battery.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
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
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/max17040_battery.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
#include <mach/sec_debug.h>
#endif

#define MAX17040_VCELL_MSB	0x02
#define MAX17040_VCELL_LSB	0x03
#define MAX17040_SOC_MSB	0x04
#define MAX17040_SOC_LSB	0x05
#define MAX17040_MODE_MSB	0x06
#define MAX17040_MODE_LSB	0x07
#define MAX17040_VER_MSB	0x08
#define MAX17040_VER_LSB	0x09
#define MAX17040_RCOMP_MSB	0x0C
#define MAX17040_RCOMP_LSB	0x0D
#define MAX17040_CMD_MSB	0xFE
#define MAX17040_CMD_LSB	0xFF

#define MAX17048_STATUS_REG	0x1A
#define MAX17048_VALRT_REG	0x14

#define MAX17040_LONG_DELAY		30000 /* msec */
#define LOG_DELTA_VOLTAGE	(150 * 1000) /* 150 mV */
#define MAX17040_FAST_DELAY		500 /* msec */
#define FAST_LOG_COUNT		60	/* 30 sec */

static ssize_t sec_fg_show_property(struct device *dev,
				    struct device_attribute *attr, char *buf);

static ssize_t sec_fg_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count);

extern unsigned int system_rev;

#define MAX17040_CHECK_LOW_VCELL_SOC	2
#define MAX17040_LOW_AVGVCELL	3350000

#define ADC_SAMPLE_COUNT	10
struct sample_info {
	unsigned int cnt;
	int total;
	int average;
	int adc_arr[ADC_SAMPLE_COUNT];
	int index;
};

struct max17040_chip {
	struct i2c_client		*client;
	struct delayed_work		work;
	struct power_supply		battery;
	struct max17040_platform_data	*pdata;
	struct wake_lock	lowbat_wake_lock;
	struct mutex		mutex;

	/* battery voltage */
	int vcell;
	int prevcell;
	int avgvcell;
	/* normal soc (adjust) */
	int soc;
	/* raw soc */
	int raw_soc;
	/* previous soc */
	int lasttime_soc;
	/* work interval */
	unsigned int fg_interval;
	/* log count */
	unsigned int fast_log_count;
	/* current temperature */
	int temperature;
	/* current rcomp */
	u16 rcomp;
	/* new rcomp */
	u16 new_rcomp;
	/* adjust full soc */
	int full_soc;
	int batt_type;
	int chg_state;
	bool is_wakelock_active;

	struct sample_info	sample;
};

static int is_max17048;
static void check_using_max17048(void)
{
#if defined(CONFIG_MACH_JAGUAR)
	if (system_rev <  0x03) {
		is_max17048 = 0;
		return;
	}
#elif defined(CONFIG_MACH_M2) || defined(CONFIG_MACH_M2_DCM)
	if (system_rev <  0x02) {
		is_max17048 = 0;
		return;
	}
#endif
	is_max17048 = 1;
}


static int max17040_write_reg(struct i2c_client *client, int reg, u8 value)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&chip->mutex);

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	mutex_unlock(&chip->mutex);

	return ret;
}

static int max17040_read_reg(struct i2c_client *client, int reg)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&chip->mutex);

	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	mutex_unlock(&chip->mutex);

	return ret;
}

static int max17040_read_word(struct i2c_client *client, int reg)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	int ret;

	mutex_lock(&chip->mutex);

	ret = i2c_smbus_read_word_data(client, reg);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	mutex_unlock(&chip->mutex);

	return ret;
}

#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
static void max17040_dump_regs(struct i2c_client *client)
{
	int i;
	int data;
	char *str = NULL;

	str = kzalloc(sizeof(char)*1024, GFP_KERNEL);
	if (!str)
		return;

	for (i = 0x2; i < 0x1c; i += 2) {
		data = max17040_read_word(client, i);
		sprintf(str+strlen(str), "%04xh,", data);
	}

	data = max17040_read_word(client, 0xff);
	sprintf(str+strlen(str), "%04xh,", data);

	dev_info(&client->dev, "%s", str);

	kfree(str);
}
#endif

static void max17040_reset(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	u16 reset_cmd = 0x5400;

	if (is_max17048) {
		mutex_lock(&chip->mutex);
		i2c_smbus_write_word_data(client, MAX17040_CMD_MSB,
								reset_cmd);
		mutex_unlock(&chip->mutex);
	} else {
		max17040_write_reg(client, MAX17040_CMD_MSB, 0x54);
		max17040_write_reg(client, MAX17040_CMD_LSB, 0x00);
	}
}

static int max17040_get_average_vcell(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	unsigned int cnt = 0;
	int total = 0;
	int average = 0;
	int index = 0;

	cnt = chip->sample.cnt;
	total = chip->sample.total;

	if (cnt < ADC_SAMPLE_COUNT) {
		chip->sample.adc_arr[cnt] = chip->vcell;
		chip->sample.index = cnt;
		chip->sample.cnt = ++cnt;

		total += chip->vcell;
		average = total / cnt;
	} else {
		index = chip->sample.index;
		if (++index >= ADC_SAMPLE_COUNT)
			index = 0;

		total = total -
			chip->sample.adc_arr[index] + chip->vcell;
		average = total / ADC_SAMPLE_COUNT;

		chip->sample.adc_arr[index] = chip->vcell;
		chip->sample.index = index;
	}

	chip->sample.total = total;
	chip->sample.average = average;

	return average;
}

static void max17040_get_vcell(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	u8 data[2];
	int temp;

	if (is_max17048) {
		temp = max17040_read_word(client, MAX17040_VCELL_MSB);
		data[0] = temp & 0xff;
		data[1] = temp >> 8;
	} else {
		data[0] = max17040_read_reg(client, MAX17040_VCELL_MSB);
		data[1] = max17040_read_reg(client, MAX17040_VCELL_LSB);
	}
	chip->prevcell = chip->vcell;
	chip->vcell = ((data[0] << 4) + (data[1] >> 4)) * 1250;
	if (chip->prevcell == 0)
		chip->prevcell = chip->vcell;

	chip->avgvcell = max17040_get_average_vcell(client);
}

static void max17040_get_soc(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	u8 data[2];
	int temp;

	unsigned int soc, psoc, temp_soc, empty_soc, full_soc;
	u64 psoc64 = 0;
	u64 data64[2] = {0, 0};
	u32 divisor = 10000000;

	if (is_max17048) {
		temp = max17040_read_word(client, MAX17040_SOC_MSB);
		data[0] = temp & 0xff;
		data[1] = temp >> 8;
	} else  {
		data[0] = max17040_read_reg(client, MAX17040_SOC_MSB);
		data[1] = max17040_read_reg(client, MAX17040_SOC_LSB);
	}

	if (chip->batt_type) { /* 4.35V battery */
		data64[0] = data[0];
		data64[1] = data[1];
		pr_debug("soc[0] = %lld, soc[1] = %lld\n",
			data64[0], data64[1]);

		/* TempSOC = ((SOC1 * 256) + SOC2) * 0.001953125 */
		psoc64 = ((data64[0]*256 + data64[1]) * 1953125);
		psoc64 = div_u64(psoc64, divisor);
		psoc = psoc64 & 0xffff;
	} else { /* NORMAL (4.2V) */
		psoc = data[0] * 100 + (data[1] * 100) / 256;
	}
	chip->raw_soc = psoc;


	/* calculate adjust soc [[ */
	if ((chip->batt_type == BATT_TYPE_D2_ACTIVE) ||
		(chip->batt_type == BATT_TYPE_D2_HIGH) ||
		(chip->batt_type == BATT_TYPE_GOGH)) {
		empty_soc = 150;
		full_soc = FULL_SOC_DEFAULT;
	} else if (chip->batt_type == BATT_TYPE_AEGIS2) {
		empty_soc = 110;
		full_soc = FULL_SOC_DEFAULT;
	} else {
		empty_soc = EMPTY_SOC;
		full_soc = chip->full_soc;
	}

	/* D2_HIGH, D2_ACTIVE : AdjSOC = ((pSOC - 0.8) * 100) / (100-0.8) */
	/* Jaguar :		AdjSOC = ((pSOC - 0.3) * 100) / (100-0.3) */
	if (psoc > empty_soc) {
		temp_soc = ((psoc - empty_soc) * 10000)/(full_soc - empty_soc);
		pr_debug("[battery] temp_soc = %d, psoc = .8%d\n",
			temp_soc, psoc);
	} else
		temp_soc = 0;

	soc = temp_soc/100;
	/* ]] calculate adjust soc */

	soc = min(soc, (uint)100);

	if (chip->lasttime_soc && soc > chip->lasttime_soc
			&& chip->chg_state == POWER_SUPPLY_STATUS_DISCHARGING) {
		chip->soc = chip->lasttime_soc;
	} else {
		chip->soc = soc;
	}
}

static void max17040_get_version(struct i2c_client *client)
{
	u8 data[2];
	int temp;
	pr_info("%s :\n", __func__);

	if (is_max17048) {
		temp = max17040_read_word(client, MAX17040_VER_MSB);
		data[0] = temp & 0xff;
		data[1] = temp >> 8;
	} else  {
		data[0] = max17040_read_reg(client, MAX17040_VER_MSB);
		data[1] = max17040_read_reg(client, MAX17040_VER_LSB);
	}
	dev_info(&client->dev,
		"MAX17040 Fuel-Gauge Ver %d%d\n", data[0], data[0]);
}

static u16 max17040_get_rcomp(struct i2c_client *client)
{
	u8 data[2];
	int temp;
	u16 ret = 0;

	if (is_max17048) {
		temp = max17040_read_word(client, MAX17040_RCOMP_MSB);
		data[0] = temp & 0xff;	/* RCOMP (0Ch) */
		data[1] = temp >> 8;	/* Alert Threshold (0Dh) */
	} else  {
		data[0] = max17040_read_reg(client, MAX17040_RCOMP_MSB);
		data[1] = max17040_read_reg(client, MAX17040_RCOMP_LSB);
	}

	ret = (u16)(data[0]<<8 | data[1]);
	/* pr_info("MAX17040 Fuel-Gauge RCOMP 0x%x%x\n", msb, lsb); */
	pr_info("%s : current rcomp = 0x%x(%x)\n", __func__, ret, data[0]);

	return ret;
}

static void max17040_set_rcomp(struct i2c_client *client, u16 new_rcomp)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->mutex);

	/*
	pr_info("%s : new rcomp = 0x%x(%d)\n", __func__,
				new_rcomp, new_rcomp>>8);
	*/

	i2c_smbus_write_word_data(client, MAX17040_RCOMP_MSB,
							swab16(new_rcomp));

	mutex_unlock(&chip->mutex);
}

static u16 max17048_get_register_word(struct i2c_client *client, int reg)
{
	int temp;
	u16 ret = 0;

	temp = max17040_read_word(client, reg);
	ret = swab16(temp);

	pr_info("%s : reg(%xh) : 0x%x\n",
		__func__, reg, ret);

	return ret;
}

static void max17048_set_register_word(struct i2c_client *client,
	int reg, u16 reg_value)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->mutex);

	i2c_smbus_write_word_data(client, reg,
							swab16(reg_value));

	mutex_unlock(&chip->mutex);
}


static void max17040_adjust_fullsoc(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	int prev_full_soc = chip->full_soc;

	if (chip->raw_soc < FULL_SOC_LOW) {
		chip->full_soc = FULL_SOC_LOW;
	} else if (chip->raw_soc > FULL_SOC_HIGH) {
		chip->full_soc = (FULL_SOC_HIGH - FULL_KEEP_SOC);
	} else {
		if (chip->raw_soc > (FULL_SOC_LOW + FULL_KEEP_SOC))
			chip->full_soc = chip->raw_soc - FULL_KEEP_SOC;
		else
			chip->full_soc = FULL_SOC_LOW;
	}

	if (prev_full_soc != chip->full_soc)
		pr_info("%s : full_soc = %d\n", __func__, chip->full_soc);
}

static void max17040_work(struct work_struct *work)
{
	struct max17040_chip *chip;

	chip = container_of(work, struct max17040_chip, work.work);

	max17040_get_vcell(chip->client);
	max17040_get_soc(chip->client);

	if (chip->soc != chip->lasttime_soc)
		chip->lasttime_soc = chip->soc;

	pr_debug("%s : VCELL:%dmV, AVGVCELL:%dmV\n", __func__,
		chip->vcell/1000, chip->avgvcell/1000);
	pr_debug("%s : Raw SOC:%d%%, SOC:%d%%\n", __func__,
		chip->raw_soc, chip->soc);
	pr_debug("%s : CONFIG:0x%04x, RATE:0x%04x\n", __func__,
		max17040_read_word(chip->client, MAX17040_RCOMP_MSB),
		max17040_read_word(chip->client, 0x16));
	pr_debug("%s : STATUS:0x%04x, temperature:%d\n", __func__,
		max17040_read_word(chip->client, 0x1a),
		chip->temperature);

#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
	max17040_dump_regs(chip->client);
#endif

	if ((chip->soc >= 5) && (chip->is_wakelock_active)) {
		pr_info("%s : unlock lowbat_wake lock, charging\n", __func__);
		chip->is_wakelock_active = false;
		wake_unlock(&chip->lowbat_wake_lock);
	}

#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
	sec_debug_fuelgauge_log(chip->vcell, (unsigned short)chip->soc, 0);
#endif

	schedule_delayed_work(&chip->work, msecs_to_jiffies(chip->fg_interval));
}

static enum power_supply_property max17040_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
};

#define SEC_FG_ATTR(_name)			\
{						\
	.attr = { .name = #_name,		\
		  .mode = 0664,			\
		  },				\
	.show = sec_fg_show_property,		\
	.store = sec_fg_store,			\
}

static struct device_attribute sec_fg_attrs[] = {
	SEC_FG_ATTR(fg_reset_soc),
	SEC_FG_ATTR(fg_read_soc),
	SEC_FG_ATTR(fg_read_rcomp),
	SEC_FG_ATTR(fg_read_fsoc),
};

enum {
	FG_RESET_SOC = 0,
	FG_READ_SOC,
	FG_READ_RCOMP,
	FG_READ_FSOC,
};

static ssize_t sec_fg_show_property(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct max17040_chip *chip = container_of(psy,
						  struct max17040_chip,
						  battery);

	int i = 0;
	const ptrdiff_t off = attr - sec_fg_attrs;

	switch (off) {
	case FG_READ_SOC:
		max17040_get_soc(chip->client);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			chip->soc);
		break;
	case FG_READ_RCOMP:
		chip->rcomp = max17040_get_rcomp(chip->client);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x\n",
			chip->rcomp);
		break;
	case FG_READ_FSOC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			chip->full_soc);
		break;
	default:
		i = -EINVAL;
	}

	return i;
}

static ssize_t sec_fg_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct max17040_chip *chip = container_of(psy,
						  struct max17040_chip,
						  battery);

	int x = 0;
	int ret = 0;
	const ptrdiff_t off = attr - sec_fg_attrs;

	switch (off) {
	case FG_RESET_SOC:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 1)
				max17040_reset(chip->client);
			ret = count;
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int fuelgauge_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(sec_fg_attrs); i++) {
		rc = device_create_file(dev, &sec_fg_attrs[i]);
		if (rc)
			goto fg_attrs_failed;
	}
	goto succeed;

fg_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_fg_attrs[i]);
succeed:
	return rc;
}

static irqreturn_t max17040_int_work_func(int irq, void *max_chip)
{
	struct max17040_chip *chip = max_chip;
#if 0
	u8 data[2];
#endif
	u16 ret = 0;

	pr_info("[ALERT] %s\n", __func__);

	wake_lock(&chip->lowbat_wake_lock);
	chip->is_wakelock_active = true;

	max17040_get_soc(chip->client);
	max17040_get_vcell(chip->client);
	max17040_get_rcomp(chip->client);

	pr_info("[ALERT] vcell(%d), soc(%d)\n", chip->vcell, chip->soc);
	pr_info("[ALERT] STATUS reg :\n");
	max17048_get_register_word(chip->client, MAX17048_STATUS_REG);
	pr_info("[ALERT] VALRT reg :\n");
	max17048_get_register_word(chip->client, MAX17048_VALRT_REG);

	if (chip->soc >= 5) {
		max17048_set_register_word(chip->client,
			MAX17048_VALRT_REG, 0x00ff);
		max17048_get_register_word(chip->client, MAX17048_VALRT_REG);

		chip->is_wakelock_active = false;
		wake_unlock(&chip->lowbat_wake_lock);
	} else {
		if (chip->pdata->low_batt_cb)
			chip->pdata->low_batt_cb();
	}

	/* clear ALRT bit, rewrite low batt thrshld */
	ret = max17040_get_rcomp(chip->client);
	ret = ((ret & 0xff00) | 0x1c);
	max17040_set_rcomp(chip->client, ret);
	max17040_get_rcomp(chip->client);

	return IRQ_HANDLED;
}

static void max17040_rcomp_update(struct i2c_client *client,
	int temp, int chg_state)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	int starting_rcomp = 0;
	int temp_cohot;
	int temp_cocold;
	int new_rcomp = 0;
	int rcomp0 = 0;

	if ((chip->batt_type == BATT_TYPE_D2_ACTIVE) ||\
		(chip->batt_type == BATT_TYPE_JAGUAR) ||\
		(chip->batt_type == BATT_TYPE_AEGIS2) ||\
		(chip->batt_type == BATT_TYPE_GOGH)) {
		temp_cohot = -300;		/* Cohot (-0.3) */
		temp_cocold = -6075;	/* Cocold (-6.075) */
	} else { /* Jaguar, D2_HIGH */
		temp_cohot = 0;		/* Cohot (0) */
		temp_cocold = -11050;	/* Cocold (-11.05) */
	}

	if (chip->batt_type) { /* normal battery(4.2V) is 0 */

		/* set up RCOMP value */
		if (chip->batt_type == BATT_TYPE_D2_HIGH) {
			if (chg_state == POWER_SUPPLY_STATUS_CHARGING)
				chip->pdata->rcomp_value = 0x701d;
			else
				chip->pdata->rcomp_value = 0xa01d;
		} else if (chip->batt_type == BATT_TYPE_D2_ACTIVE) {
			if (chg_state == POWER_SUPPLY_STATUS_CHARGING)
				chip->pdata->rcomp_value = 0x851c;
			else
				chip->pdata->rcomp_value = 0x6d1c;
		} else if (chip->batt_type == BATT_TYPE_GOGH) {
			if (chg_state == POWER_SUPPLY_STATUS_CHARGING)
				chip->pdata->rcomp_value = 0x701c;
			else
				chip->pdata->rcomp_value = 0x691c;
		}

		starting_rcomp = (int)(chip->pdata->rcomp_value >> 8);

		/*if (temp > RCOMP0_TEMP)
			new_rcomp =
				starting_rcomp +
				(((temp-20) * temp_cohot) / 1000);*/
		if (temp < RCOMP0_TEMP)
			new_rcomp =
				starting_rcomp +
				(((temp-20) * temp_cocold) / 1000);
		else
			new_rcomp = starting_rcomp;

		if (new_rcomp > 255)
			new_rcomp = 255;
		chip->new_rcomp = ((u8)new_rcomp << 8) | (chip->rcomp&0xFF);

		if (chip->rcomp != chip->new_rcomp) {
			pr_info("%s : temp(%d), rcomp 0x%x -> 0x%x (%d)\n",
				__func__, temp,
				chip->rcomp,
				chip->new_rcomp,
				chip->new_rcomp>>8);
			chip->rcomp = chip->new_rcomp;
			max17040_set_rcomp(client, chip->new_rcomp);
		}
	} else {

		rcomp0 = (int)(chip->pdata->rcomp_value >> 8);
		rcomp0 *= 10;

		if (temp < RCOMP0_TEMP) {
			new_rcomp = (rcomp0 + 50*(RCOMP0_TEMP-temp))/10;
			new_rcomp = max(new_rcomp, 0);
			new_rcomp = min(new_rcomp, 255);
			chip->new_rcomp
				= ((u8)new_rcomp << 8) | (chip->rcomp&0xFF);
		} else if (temp > RCOMP0_TEMP) {
			new_rcomp = (rcomp0 - 18*(temp-RCOMP0_TEMP))/10;
			new_rcomp = max(new_rcomp, 0);
			new_rcomp = min(new_rcomp, 255);
			chip->new_rcomp
				= ((u8)new_rcomp << 8) | (chip->rcomp&0xFF);
		} else
			chip->new_rcomp = chip->pdata->rcomp_value;

		if (chip->rcomp != chip->new_rcomp) {
			pr_info("%s : 0x%x -> 0x%x (%d)\n", __func__,
							chip->rcomp,
							chip->new_rcomp,
							chip->new_rcomp>>8);
		}
	}
}

static int max17040_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct max17040_chip *chip = container_of(psy,
				struct max17040_chip, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		max17040_get_vcell(chip->client);
		val->intval = chip->vcell;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		switch (val->intval) {
		case 0:	/*normal soc */
			max17040_get_soc(chip->client);
			val->intval = chip->soc;
			break;
		case 1: /*raw soc */
			max17040_get_soc(chip->client);
			val->intval = chip->raw_soc;
			break;
		case 2: /*rcomp */
			val->intval = chip->rcomp;
			break;
		case 3: /*full soc  */
			val->intval = chip->full_soc;
			break;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int max17040_set_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    const union power_supply_propval *val)
{
	struct max17040_chip *chip = container_of(psy,
				struct max17040_chip, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_TEMP:
		chip->temperature = val->intval;
		pr_debug("%s: current temperature = %d\n",
			__func__, chip->temperature);
		if (chip->batt_type)
				max17040_rcomp_update(chip->client,
					chip->temperature, chip->chg_state);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		chip->chg_state = val->intval;
		switch (val->intval) {
		case POWER_SUPPLY_STATUS_FULL:
			pr_debug("%s: charger full state!\n", __func__);
			/* adjust full soc */
			max17040_adjust_fullsoc(chip->client);
			break;
		case POWER_SUPPLY_STATUS_CHARGING:
		case POWER_SUPPLY_STATUS_DISCHARGING:
			pr_debug("%s: charger state!(%d)\n",
				__func__, chip->chg_state);
			max17040_rcomp_update(chip->client,
				chip->temperature, chip->chg_state);
			break;
		default:
			return -EINVAL;
		}
		break;
	case POWER_SUPPLY_PROP_FUELGAUGE_STATE:
		pr_info("%s: fuelgauge reset\n", __func__);
		max17040_reset(chip->client);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int __devinit max17040_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17040_chip *chip;
	int ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	pr_info("%s: MAX17043 driver Loading!\n", __func__);

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	if (!chip->pdata) {
		pr_err("%s: no fuel gauge platform data\n",	__func__);
		goto err_kfree;
	}

	chip->fg_interval = MAX17040_LONG_DELAY;
	chip->fast_log_count = 0;
	chip->temperature = RCOMP0_TEMP;
	chip->full_soc = FULL_SOC_DEFAULT;
	chip->prevcell = chip->vcell = 0;
	chip->chg_state = POWER_SUPPLY_STATUS_DISCHARGING;

	i2c_set_clientdata(client, chip);

	mutex_init(&chip->mutex);

	chip->pdata->hw_init(); /* important */

	check_using_max17048();

	max17040_get_version(client);
	chip->rcomp = max17040_get_rcomp(client);

	chip->batt_type = BATT_TYPE_D2_ACTIVE;

	if (chip->batt_type) {
		chip->pdata->rcomp_value = chip->rcomp;
		pr_info("[max17048] using RCOMP (0x%x)\n",
			chip->pdata->rcomp_value);
	} else {
		chip->pdata->rcomp_value = 0xe71f;
		pr_info("[max17048] using 4.2V battery\n");
	}

	chip->rcomp = chip->pdata->rcomp_value;
	chip->new_rcomp = chip->pdata->rcomp_value;
	max17040_set_rcomp(client, chip->new_rcomp);
	chip->rcomp = max17040_get_rcomp(client);

	chip->battery.name		= "fuelgauge",
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY,
	chip->battery.get_property	= max17040_get_property,
	chip->battery.set_property	= max17040_set_property,
	chip->battery.properties	= max17040_battery_props,
	chip->battery.num_properties	= ARRAY_SIZE(max17040_battery_props),
	chip->is_wakelock_active	= false;

	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
		goto err_psy_register;
	}

	wake_lock_init(&chip->lowbat_wake_lock, WAKE_LOCK_SUSPEND,
										"fuelgague-lowbat");

	ret = request_threaded_irq(chip->client->irq, NULL,
			max17040_int_work_func, IRQF_TRIGGER_FALLING,
			"max17040", chip);
	if (ret) {
		pr_err("%s : Failed to request fuelgauge irq\n", __func__);
		goto err_request_irq;
	}

	ret = enable_irq_wake(chip->client->irq);
	if (ret) {
		pr_err("%s : Failed to enable fuelgauge irq wake\n", __func__);
		goto err_irq_wake;
	}

	/* update rcomp test */
	/*
	static int itemp;
	for (itemp=100; itemp>-100; itemp--) {
		max17040_rcomp_update(client, itemp);
	}
	*/

	/* create fuelgauge attributes */
	fuelgauge_create_attrs(chip->battery.dev);

	INIT_DELAYED_WORK_DEFERRABLE(&chip->work, max17040_work);
	schedule_delayed_work(&chip->work, 0);

	return 0;

err_irq_wake:
	free_irq(chip->client->irq, NULL);
err_request_irq:
	wake_lock_destroy(&chip->lowbat_wake_lock);
	power_supply_unregister(&chip->battery);
err_psy_register:
	mutex_destroy(&chip->mutex);
err_kfree:
	kfree(chip);
	return ret;
}

static int __devexit max17040_remove(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	wake_lock_destroy(&chip->lowbat_wake_lock);
	power_supply_unregister(&chip->battery);
	cancel_delayed_work(&chip->work);
	mutex_destroy(&chip->mutex);
	kfree(chip);
	return 0;
}

#ifdef CONFIG_PM

static int max17040_suspend(struct i2c_client *client,
		pm_message_t state)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	if (chip->is_wakelock_active)
		wake_unlock(&chip->lowbat_wake_lock);

	cancel_delayed_work(&chip->work);
	return 0;
}

static int max17040_resume(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	schedule_delayed_work(&chip->work, 0);
	return 0;
}

#else

#define max17040_suspend NULL
#define max17040_resume NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id max17040_id[] = {
	{ "max17040", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max17040_id);

static struct i2c_driver max17040_i2c_driver = {
	.driver	= {
		.name	= "max17040",
	},
	.probe		= max17040_probe,
	.remove		= __devexit_p(max17040_remove),
	.suspend	= max17040_suspend,
	.resume		= max17040_resume,
	.id_table	= max17040_id,
};

static int __init max17040_init(void)
{
	return i2c_add_driver(&max17040_i2c_driver);
}
module_init(max17040_init);

static void __exit max17040_exit(void)
{
	i2c_del_driver(&max17040_i2c_driver);
}
module_exit(max17040_exit);

MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_DESCRIPTION("MAX17040 Fuel Gauge");
MODULE_LICENSE("GPL");
