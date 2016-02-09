/*
 *  sec_battery.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2011 Samsung Electronics
 *
 *  <jongmyeong.ko@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/hrtimer.h>
#include <linux/msm_adc.h>
#include <linux/earlysuspend.h>
#include <linux/sec_battery.h>
#include <linux/charging_temperature_data.h>
#include <linux/gpio.h>
#include <mach/msm8960-gpio.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <asm/system_info.h>

#define FG_T_SOC		0
#define FG_T_VCELL		1
#define FG_T_PSOC		2
#define FG_T_RCOMP		3
#define FG_T_FSOC		4

#define BATT_STATUS_MISSING		0
#define BATT_STATUS_PRESENT		1
#define BATT_STATUS_UNKNOWN		2

#ifdef CONFIG_BATTERY_CTIA
#define USE_CALL			(0x1 << 0)
#define USE_VIDEO			(0x1 << 1)
#define USE_MUSIC			(0x1 << 2)
#define USE_BROWSER		(0x1 << 3)
#define USE_HOTSPOT			(0x1 << 4)
#define USE_CAMERA			(0x1 << 5)
#define USE_DATA_CALL		(0x1 << 6)
#define USE_GPS			(0x1 << 7)
#define USE_LTE			(0x1 << 8)
#define USE_WIFI			(0x1 << 9)
#endif /*CONFIG_BATTERY_CTIA*/

#define TOTAL_EVENT_TIME  (10 * 60)	/* 10 minites */

static int is_charging_disabled;

enum cable_type_t {
	CABLE_TYPE_NONE = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_AC,
	CABLE_TYPE_MISC,
	CABLE_TYPE_CARDOCK,
	CABLE_TYPE_UARTOFF,
	CABLE_TYPE_JIG,
	CABLE_TYPE_UNKNOWN,
	CABLE_TYPE_CDP,
	CABLE_TYPE_SMART_DOCK,
#ifdef CONFIG_WIRELESS_CHARGING
	CABLE_TYPE_WPC = 10,
#endif

};

enum batt_full_t {
	BATT_NOT_FULL = 0,
	BATT_FULL,
};

enum {
	BAT_NOT_DETECTED,
	BAT_DETECTED
};

static ssize_t sec_bat_show_property(struct device *dev,
				     struct device_attribute *attr, char *buf);

static ssize_t sec_bat_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count);

struct battest_info {
	int rechg_count;
	int full_count;
	int test_value;
	int test_esuspend;
	bool is_rechg_state;
};

struct sec_temperature_spec {
	int event_block;
	int high_block;
	int high_recovery;
	int low_block;
	int low_recovery;
};

struct sec_bat_info {
	struct device *dev;

	char *fuel_gauge_name;
	char *charger_name;

	struct power_supply psy_bat;
	struct power_supply psy_usb;
	struct power_supply psy_ac;

	struct wake_lock vbus_wake_lock;
	struct wake_lock cable_wake_lock;
	struct wake_lock monitor_wake_lock;
	struct wake_lock test_wake_lock;
	struct wake_lock measure_wake_lock;

#if defined(CONFIG_BATTERY_CTIA)
	struct hrtimer		event_timer;
#endif /*CONFIG_BATTERY_CTIA*/
	struct hrtimer		hrtimer;

	enum cable_type_t cable_type;
	enum cable_type_t prev_cable;
	enum batt_full_t batt_full_status;

	int batt_temp;
	int batt_temp_high_cnt;
	int batt_temp_low_cnt;
	int high_recovery_wpc;
	unsigned int batt_health;
	unsigned int batt_vcell;
	unsigned int batt_soc;
	unsigned int batt_raw_soc;
	unsigned int batt_full_soc;
	unsigned int batt_presoc;
	unsigned int measure_interval;
	int charging_status;
	int otg_state;
	int current_avg;
	unsigned int batt_temp_adc;
	unsigned int batt_temp_radc;
	unsigned int batt_current_adc;
	unsigned int present;
	struct battest_info test_info;
	struct workqueue_struct *monitor_wqueue;
	struct work_struct monitor_work;
	struct delayed_work cable_work;
	struct delayed_work measure_work;
	struct delayed_work otg_work;
	struct early_suspend bat_early_suspend;
	struct sec_temperature_spec tspec;
	struct proc_dir_entry *entry;

	unsigned long charging_start_time;
	unsigned long charging_passed_time;
	unsigned int iterm;
	unsigned int abs_time;
	unsigned int normal_abs_time;
	unsigned int wpc_abs_time;
	unsigned int rechg_time;
	unsigned int vrechg;
	unsigned int vmax;
	unsigned int recharging_status;
	unsigned int is_top_off;
	unsigned int hw_rev;
	unsigned int voice_call_state;
	unsigned int full_cond_voltage;
	unsigned int full_cond_count;
	unsigned int initial_check_count;

	bool charging_enabled;
	bool is_timeout_chgstop;
	bool is_esus_state;
	bool lpm_chg_mode;
	bool slate_mode;
	bool is_rechg_triggered;
	bool dcin_intr_triggered;
	u16 batt_rcomp;
	unsigned int slow_polling;

#ifdef CONFIG_WIRELESS_CHARGING
	bool wc_status;
	int wpc_charging_current;
#endif
#if defined(CONFIG_BATTERY_CTIA)
	ktime_t	cur_time;
	unsigned int batt_use;
	unsigned int batt_use_wait;
#endif /*CONFIG_BATTERY_CTIA*/
	ktime_t	cur_monitor_time;
#if defined(CONFIG_TARGET_LOCALE_USA)
	bool ui_full_charge_status;
	bool cable_uart_off;
#endif
	unsigned int batt_int_irq;
	unsigned int batt_int;
	bool batt_int_irq_use;
	bool factory_mode;
	bool check_full_state;
	unsigned int check_full_state_cnt;
};

static char *supply_list[] = {
	"battery",
};

static enum power_supply_property sec_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TECHNOLOGY,
};

static enum power_supply_property sec_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

#if defined(CONFIG_CHARGER_SMB347) && \
	defined(CONFIG_WIRELESS_CHARGING)
enum {
	INPUT_NONE,
	INPUT_DCIN,
	INPUT_USBIN,
};
#endif

static void  sec_bat_monitoring_alarm(struct sec_bat_info *info,
							int sec)
{
	ktime_t low_interval = ktime_set(sec - 10, 0);
//	ktime_t slack = ktime_set(20, 0);
	ktime_t next;

	next = ktime_add(info->cur_monitor_time, low_interval);
	/* The original slack time called for, 20 seconds, exceeds
        * the length allowed for an unsigned long in nanoseconds. Use
        * ULONG_MAX instead
        */
        hrtimer_start_range_ns(&info->hrtimer,
                next, ULONG_MAX, HRTIMER_MODE_ABS);
}

enum hrtimer_restart sec_bat_monitor_queue(struct hrtimer *hrtimer)
{
	struct sec_bat_info *info =
		container_of(hrtimer, struct sec_bat_info, hrtimer);

	queue_delayed_work(info->monitor_wqueue, &info->measure_work, 0);

	queue_work(info->monitor_wqueue, &info->monitor_work);
	return HRTIMER_NORESTART;
}

static int sec_bat_check_vf(struct sec_bat_info *info)
{
	int health = info->batt_health;

	if (info->present == BATT_STATUS_MISSING && !(info->factory_mode))
		health = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
	else
		health = POWER_SUPPLY_HEALTH_GOOD;

	/* update health */
	if (health != info->batt_health) {
		if (health == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE ||
		    health == POWER_SUPPLY_HEALTH_DEAD) {
			info->batt_health = health;
			pr_info("%s : vf error update\n", __func__);
		} else if (info->batt_health != POWER_SUPPLY_HEALTH_OVERHEAT &&
			   info->batt_health != POWER_SUPPLY_HEALTH_COLD &&
			   health == POWER_SUPPLY_HEALTH_GOOD) {
			info->batt_health = health;
			pr_info("%s : recovery form vf error\n", __func__);
		}
	}

	return 0;
}
#if 0
static int sec_bat_check_detbat(struct sec_bat_info *info)
{
	struct power_supply *psy = power_supply_get_by_name(info->charger_name);
	union power_supply_propval value;
	static int cnt;
	int vf_state = BAT_DETECTED;
	int ret = 0;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	ret = psy->get_property(psy, POWER_SUPPLY_PROP_PRESENT, &value);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to get status(%d)\n",
			__func__, ret);
		return -ENODEV;
	}

	if ((info->cable_type != CABLE_TYPE_NONE) &&
	    (value.intval == BAT_NOT_DETECTED)) {
		if (cnt <= BAT_DET_COUNT)
			cnt++;
		if (cnt >= BAT_DET_COUNT)
			vf_state = BAT_NOT_DETECTED;
		else
			vf_state = BAT_DETECTED;
	} else {
		vf_state = BAT_DETECTED;
		cnt = 0;
	}

	if (info->present == BATT_STATUS_PRESENT
		&& vf_state == BAT_NOT_DETECTED) {
		pr_info("%s : detbat state(->%d) changed\n",
			__func__, vf_state);
		info->present = BATT_STATUS_MISSING;
		cancel_work_sync(&info->monitor_work);
		queue_work(info->monitor_wqueue, &info->monitor_work);
	} else if (info->present == BATT_STATUS_MISSING
			&& vf_state == BAT_DETECTED) {
		pr_info("%s : detbat state(->%d) changed\n",
			__func__, vf_state);
		info->present = BATT_STATUS_PRESENT;
		cancel_work_sync(&info->monitor_work);
		queue_work(info->monitor_wqueue, &info->monitor_work);
	}

	return value.intval;
}
#endif
static int sec_bat_set_fuelgauge_reset(struct sec_bat_info *info)
{
#if defined(CONFIG_BATTERY_MAX17040) || \
	defined(CONFIG_BATTERY_MAX17042)
	struct power_supply *psy = power_supply_get_by_name("fuelgauge");
	union power_supply_propval value;

	if (!psy) {
		pr_err("%s: fail to get fuel gauge ps\n", __func__);
		return -ENODEV;
	}

	psy->set_property(psy,
		POWER_SUPPLY_PROP_FUELGAUGE_STATE, &value);

	return value.intval;
#else
	return -ENODEV;
#endif
}

static int sec_bat_get_fuelgauge_data(struct sec_bat_info *info, int type)
{
	struct power_supply *psy
	    = power_supply_get_by_name(info->fuel_gauge_name);
	union power_supply_propval value;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get fuel gauge ps\n", __func__);
		return -ENODEV;
	}

	switch (type) {
	case FG_T_VCELL:
		value.intval = 0;	/*vcell */
		psy->get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &value);
		break;
	case FG_T_SOC:
		value.intval = 0;	/*normal soc */
		psy->get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &value);
#if defined(CONFIG_TARGET_LOCALE_USA)
		/* According to the SAMSUMG inner charger charging concept,
		   Full charging process should be divided into 2 phase
		   as 1st UI full charging, 2nd real full charging.
		   This is for the 1st UI Full charging.
		 */
		if (info->ui_full_charge_status &&
		    info->charging_status == POWER_SUPPLY_STATUS_CHARGING)
			value.intval = 100;
#endif
		break;
	case FG_T_PSOC:
		value.intval = 1;	/*raw soc */
		psy->get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &value);
		break;
	case FG_T_RCOMP:
		value.intval = 2;	/*rcomp */
		psy->get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &value);
		break;
	case FG_T_FSOC:
		value.intval = 3;	/*full soc */
		psy->get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &value);
		break;
	default:
		return -ENODEV;
	}

	return value.intval;
}

static int sec_bat_get_property(struct power_supply *ps,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_bat_info *info = container_of(ps, struct sec_bat_info,
						 psy_bat);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (info->test_info.test_value == 999) {
			pr_info("%s : test case : %d\n", __func__,
				info->test_info.test_value);
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		} else if (info->is_timeout_chgstop &&
			   info->charging_status == POWER_SUPPLY_STATUS_FULL) {
			/*
			new concept : in case of time-out charging stop,
			Do not update FULL for UI,
			Use same time-out value
			for first charing and re-charging
			 */
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		} else if (info->recharging_status) {
			val->intval = POWER_SUPPLY_STATUS_FULL;
		} else {
#if defined(CONFIG_TARGET_LOCALE_USA)
			if (info->ui_full_charge_status &&
			    info->charging_status ==
			    POWER_SUPPLY_STATUS_CHARGING) {
				val->intval = POWER_SUPPLY_STATUS_FULL;
				break;
			}
#endif
			val->intval = info->charging_status;
		}
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = info->batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = info->present;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = info->batt_temp;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		if (info->charging_status == POWER_SUPPLY_STATUS_DISCHARGING &&
		    info->cable_type != CABLE_TYPE_NONE) {
			val->intval = CABLE_TYPE_NONE;
		} else
			val->intval = info->cable_type;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = sec_bat_get_fuelgauge_data(info, FG_T_VCELL);
		if (val->intval == -1)
			return -EINVAL;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if (!info->is_timeout_chgstop &&
		    info->charging_status == POWER_SUPPLY_STATUS_FULL) {
			val->intval = 100;
			break;
		}
#if defined(CONFIG_TARGET_LOCALE_USA)
		if (info->ui_full_charge_status &&
		    info->charging_status == POWER_SUPPLY_STATUS_CHARGING) {
			val->intval = 100;
			break;
		}
#endif
		val->intval = info->batt_soc;
		if (val->intval == -1)
			return -EINVAL;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sec_bat_handle_charger_topoff(struct sec_bat_info *info)
{
	struct power_supply *psy = power_supply_get_by_name(info->charger_name);
	struct power_supply *psy_fg =
	    power_supply_get_by_name(info->fuel_gauge_name);
	union power_supply_propval value;
	int ret = 0;

	if (!psy || !psy_fg) {
		pr_err("%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	if (info->batt_full_status == BATT_NOT_FULL) {
		info->charging_status = POWER_SUPPLY_STATUS_FULL;
		info->batt_full_status = BATT_FULL;
		info->recharging_status = false;
		info->charging_passed_time = 0;
		info->charging_start_time = 0;
#if defined(CONFIG_TARGET_LOCALE_USA)
		info->ui_full_charge_status = false;
#endif
		/* disable charging */
		value.intval = POWER_SUPPLY_STATUS_DISCHARGING;
		ret = psy->set_property(psy,
			POWER_SUPPLY_PROP_STATUS, &value);
		info->charging_enabled = false;
		/* notify full state to fuel guage */
		value.intval = POWER_SUPPLY_STATUS_FULL;
		ret = psy_fg->set_property(psy_fg, POWER_SUPPLY_PROP_STATUS,
					   &value);
	}
	return ret;
}

static int sec_bat_is_charging(struct sec_bat_info *info)
{
	struct power_supply *psy = power_supply_get_by_name(info->charger_name);
	union power_supply_propval value;
	int ret;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	ret = psy->get_property(psy, POWER_SUPPLY_PROP_STATUS, &value);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to get status(%d)\n", __func__,
			ret);
		return ret;
	}

	return value.intval;
}

static int sec_bat_is_invalid_bmd(struct sec_bat_info *info)
{
	struct power_supply *psy = power_supply_get_by_name(info->charger_name);
	union power_supply_propval value;
	int ret;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	ret = psy->get_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to get online status(%d)\n",
			__func__, ret);
		return ret;
	}

	pr_debug("%s : retun value = %d\n", __func__, value.intval);
	return value.intval;
}

static void sec_otg_work(struct work_struct *work)
{
	struct sec_bat_info *info =
	    container_of(work, struct sec_bat_info, otg_work.work);
	struct power_supply *psy = power_supply_get_by_name(info->charger_name);
	union power_supply_propval val_type;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return;
	}

	val_type.intval = info->otg_state;
	psy->set_property(psy, POWER_SUPPLY_PROP_OTG, &val_type);
}

static int sec_bat_set_property(struct power_supply *ps,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sec_bat_info *info = container_of(ps, struct sec_bat_info,
						 psy_bat);

	int chg_status = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		chg_status = sec_bat_is_charging(info);
		pr_info("%s: %d\n", __func__, chg_status);

		if (val->intval & POWER_SUPPLY_STATUS_FULL) {
			if (val->intval & POWER_SUPPLY_STATUS_CHARGING) {
				pr_info("%s: re-charging!!\n", __func__);
				if (info->charging_status !=
					POWER_SUPPLY_STATUS_FULL) {
					if (info->batt_vcell >=
						info->full_cond_voltage)
						info->charging_status =
						POWER_SUPPLY_STATUS_FULL;
					else {
						pr_info("%s: skip re-charging process!!\n",
							__func__);
						info->charging_status =
						POWER_SUPPLY_STATUS_CHARGING;
						info->charging_enabled = true;
					cancel_work_sync(&info->monitor_work);
						queue_work(info->monitor_wqueue,
							&info->monitor_work);
						return 0;
					}
				}
				info->recharging_status = false;
				info->is_rechg_triggered = true;
				info->test_info.is_rechg_state = true;
				info->test_info.rechg_count = 0;
			} else if (val->intval &
				POWER_SUPPLY_STATUS_DISCHARGING) {
				pr_info("%s: full-charging!!\n", __func__);
			} else {
				pr_err("%s: unknown chg intr state!!\n",
					__func__);
				return -EINVAL;
			}
			cancel_work_sync(&info->monitor_work);
			queue_work(info->monitor_wqueue,
				&info->monitor_work);
		} else {
			if (val->intval & POWER_SUPPLY_STATUS_CHARGING) {
				info->dcin_intr_triggered = true;
				pr_info("%s: charger inserted!!\n", __func__);
			} else if (val->intval &
				POWER_SUPPLY_STATUS_DISCHARGING) {
				info->dcin_intr_triggered = false;
				pr_info("%s: charger removed!!\n", __func__);
			} else {
				pr_err("%s: unknown chg intr state!!\n",
					__func__);
				return -EINVAL;
			}
			cancel_delayed_work(&info->measure_work);
			queue_delayed_work(info->monitor_wqueue,
					   &info->measure_work, HZ);
		}
/*
		if (val->intval == POWER_SUPPLY_STATUS_CHARGING) {
			pr_info("%s: charger inserted!!\n", __func__);
			info->dcin_intr_triggered = true;
			cancel_delayed_work(&info->measure_work);
			queue_delayed_work(info->monitor_wqueue,
					   &info->measure_work, HZ);
		} else if (val->intval == POWER_SUPPLY_STATUS_DISCHARGING) {
			pr_info("%s: charger removed!!\n", __func__);
			info->dcin_intr_triggered = false;
			cancel_delayed_work(&info->measure_work);
			queue_delayed_work(info->monitor_wqueue,
					   &info->measure_work, HZ);
		} else {
			pr_err("%s: unknown chg intr state!!\n", __func__);
			return -EINVAL;
		}
*/
		break;

	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		dev_info(info->dev, "%s: lowbatt intr\n", __func__);
		if (val->intval != POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL)
			return -EINVAL;
		queue_work(info->monitor_wqueue, &info->monitor_work);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		/* cable is attached or detached. called by usb switch ic */
		dev_info(info->dev, "%s: cable was changed(%d), prev(%d)\n",
			__func__, val->intval, info->prev_cable);
		switch (val->intval) {
		case POWER_SUPPLY_TYPE_BATTERY:
			info->cable_type = CABLE_TYPE_NONE;
#if defined(CONFIG_TARGET_LOCALE_USA)
			info->cable_uart_off = false;
#endif
			break;
		case POWER_SUPPLY_TYPE_MAINS:
			info->cable_type = CABLE_TYPE_AC;
			break;
		case POWER_SUPPLY_TYPE_USB:
			info->cable_type = CABLE_TYPE_USB;
			break;
		case POWER_SUPPLY_TYPE_MISC:
			info->cable_type = CABLE_TYPE_MISC;
			break;
		case POWER_SUPPLY_TYPE_CARDOCK:
			info->cable_type = CABLE_TYPE_CARDOCK;
			break;
		case POWER_SUPPLY_TYPE_UARTOFF:
			info->cable_type = CABLE_TYPE_UARTOFF;
#if defined(CONFIG_TARGET_LOCALE_USA)
			info->cable_uart_off = true;
#endif
			break;
#ifdef CONFIG_WIRELESS_CHARGING
		case POWER_SUPPLY_TYPE_WPC:
			info->cable_type = CABLE_TYPE_WPC;
			info->wc_status = true;
			break;
#endif
		case POWER_SUPPLY_TYPE_USB_CDP:
			info->cable_type = CABLE_TYPE_CDP;
			break;
		case POWER_SUPPLY_TYPE_SMART_DOCK:
			info->cable_type = CABLE_TYPE_SMART_DOCK;
			break;
		default:
			return -EINVAL;
		}
		if (info->prev_cable != info->cable_type) {
			queue_delayed_work(info->monitor_wqueue,
				&info->cable_work, 0);
		}

		if (info->cable_type &&
				!info->charging_enabled &&
				(info->charging_status
					!= POWER_SUPPLY_STATUS_FULL) &&
				(info->batt_health
					== POWER_SUPPLY_HEALTH_GOOD)) {
			queue_delayed_work(info->monitor_wqueue,
				&info->cable_work, 0);
		}
		info->prev_cable = info->cable_type;
		break;
	case POWER_SUPPLY_PROP_OTG:
		info->otg_state = val->intval;
		queue_delayed_work(info->monitor_wqueue, &info->otg_work, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_usb_get_property(struct power_supply *ps,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_bat_info *info = container_of(ps, struct sec_bat_info,
						 psy_usb);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	/* Set enable=1 only if the USB charger is connected */
	if (info->cable_type == CABLE_TYPE_USB ||
		info->cable_type == CABLE_TYPE_CDP)
		val->intval = 1;
	else
		val->intval = 0;

	return 0;
}

static int sec_ac_get_property(struct power_supply *ps,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	struct sec_bat_info *info = container_of(ps, struct sec_bat_info,
						 psy_ac);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	/* Set enable=1 only if the AC charger is connected */
	if (info->charging_status == POWER_SUPPLY_STATUS_DISCHARGING &&
	    info->cable_type != CABLE_TYPE_NONE) {
		val->intval = 0;
	} else {
		if (info->cable_type == CABLE_TYPE_MISC ||
		    info->cable_type == CABLE_TYPE_UARTOFF ||
		    info->cable_type == CABLE_TYPE_CARDOCK) {
			if (!info->dcin_intr_triggered)
				val->intval = 0;
			else
				val->intval = 1;
		} else {
			/* org */
			val->intval = (info->cable_type == CABLE_TYPE_AC) ||
#ifdef CONFIG_WIRELESS_CHARGING
				(info->cable_type == CABLE_TYPE_WPC) ||
#endif
			    (info->cable_type == CABLE_TYPE_UNKNOWN);
		}
	}
	pr_debug("%s : cable type = %d, return val = %d\n",
	__func__, info->cable_type, val->intval);
	return 0;
}

#ifdef ADJUST_RCOMP_WITH_TEMPER
static int sec_fg_update_temper(struct sec_bat_info *info)
{
	struct power_supply *psy
	    = power_supply_get_by_name(info->fuel_gauge_name);
	union power_supply_propval value;
	int ret;

	if (!psy) {
		pr_err("%s: fail to get fuelgauge ps\n", __func__);
		return -ENODEV;
	}

	/* Notify temperature to fuel gauge */
	if (info->fuel_gauge_name) {
		value.intval = info->batt_temp / 10;
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_TEMP, &value);
		if (ret) {
			dev_err(info->dev, "%s: fail to set temperature(%d)\n",
				__func__, ret);
			return ret;
		}
	}
	return 0;
}
#endif

static int pm8921_get_temperature_adc(struct sec_bat_info *info)
{
	int rc = 0;
	struct pm8xxx_adc_chan_result result = {0, 0, 0, 0};

	rc = pm8xxx_adc_mpp_config_read(TEMP_GPIO, TEMP_ADC_CHNNEL, &result);
	if (rc) {
		pr_err("error reading mpp %d, rc = %d\n", TEMP_GPIO, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n",
		result.physical, result.measurement);

	info->batt_temp = (int)result.physical;
	info->batt_temp_adc = (int)result.measurement;

#ifdef ADJUST_RCOMP_WITH_TEMPER
	sec_fg_update_temper(info);
#endif

	return info->batt_temp;
}

static int sec_bat_check_temper_adc(struct sec_bat_info *info)
{
	int rescale_adc = 0;
	int health = info->batt_health;
	int high_recovery = info->tspec.high_recovery;

	pm8921_get_temperature_adc(info);

	rescale_adc = info->batt_temp;

	if (info->test_info.test_value == 1) {
		pr_info("%s : test case : %d\n", __func__,
			info->test_info.test_value);
		rescale_adc = info->tspec.high_block + 1;
		if (info->cable_type == CABLE_TYPE_NONE)
			rescale_adc = info->tspec.high_recovery - 1;
		info->batt_temp = rescale_adc;
	}

	if (info->cable_type == CABLE_TYPE_NONE ||
	    info->test_info.test_value == 999) {
		info->batt_temp_high_cnt = 0;
		info->batt_temp_low_cnt = 0;
		health = POWER_SUPPLY_HEALTH_GOOD;
		goto skip_hupdate;
	}
#ifdef CONFIG_WIRELESS_CHARGING
	if ((info->high_recovery_wpc) &&
		(info->cable_type == CABLE_TYPE_WPC)) {
		high_recovery = info->high_recovery_wpc;
	} else {
		high_recovery = info->tspec.high_recovery;
	}
#endif

#if defined(CONFIG_BATTERY_CTIA)
	if (info->batt_use) {
		if (rescale_adc >= info->tspec.event_block) {
			if (health != POWER_SUPPLY_HEALTH_OVERHEAT)
				if (info->batt_temp_high_cnt
					<= TEMP_BLOCK_COUNT)
					info->batt_temp_high_cnt++;
			} else if (rescale_adc <= high_recovery &&
				rescale_adc >= info->tspec.low_recovery) {
				if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
				    health == POWER_SUPPLY_HEALTH_COLD) {
					info->batt_temp_high_cnt = 0;
					info->batt_temp_low_cnt = 0;
				}
			} else if (rescale_adc <= info->tspec.low_block) {
				if (health != POWER_SUPPLY_HEALTH_COLD)
					if (info->batt_temp_low_cnt
						<= TEMP_BLOCK_COUNT)
						info->batt_temp_low_cnt++;
			}
	} else {
		if (rescale_adc >= info->tspec.high_block) {
			if (health != POWER_SUPPLY_HEALTH_OVERHEAT)
				if (info->batt_temp_high_cnt
					<= TEMP_BLOCK_COUNT)
					info->batt_temp_high_cnt++;
			} else if (rescale_adc <= high_recovery &&
				rescale_adc >= info->tspec.low_recovery) {
				if (health == POWER_SUPPLY_HEALTH_OVERHEAT
				|| health == POWER_SUPPLY_HEALTH_COLD) {
					info->batt_temp_high_cnt = 0;
					info->batt_temp_low_cnt = 0;
				}
			} else if (rescale_adc <= info->tspec.low_block) {
				if (health != POWER_SUPPLY_HEALTH_COLD)
					if (info->batt_temp_low_cnt
						<= TEMP_BLOCK_COUNT)
						info->batt_temp_low_cnt++;
			}

	}
#else
	if (rescale_adc >= info->tspec.high_block) {
		if (health != POWER_SUPPLY_HEALTH_OVERHEAT)
			if (info->batt_temp_high_cnt <= TEMP_BLOCK_COUNT)
				info->batt_temp_high_cnt++;
	} else if (rescale_adc <= high_recovery &&
		   rescale_adc >= info->tspec.low_recovery) {
		if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
		    health == POWER_SUPPLY_HEALTH_COLD) {
			info->batt_temp_high_cnt = 0;
			info->batt_temp_low_cnt = 0;
		}
	} else if (rescale_adc <= info->tspec.low_block) {
		if (health != POWER_SUPPLY_HEALTH_COLD)
			if (info->batt_temp_low_cnt <= TEMP_BLOCK_COUNT)
				info->batt_temp_low_cnt++;
	}
#endif
	if (info->batt_temp_high_cnt >= TEMP_BLOCK_COUNT) {
		health = POWER_SUPPLY_HEALTH_OVERHEAT;
		wake_lock_timeout(&info->monitor_wake_lock, 10 * HZ);
	} else if (info->batt_temp_low_cnt >= TEMP_BLOCK_COUNT) {
		health = POWER_SUPPLY_HEALTH_COLD;
		wake_lock_timeout(&info->monitor_wake_lock, 10 * HZ);
	} else {
		health = POWER_SUPPLY_HEALTH_GOOD;
	}
skip_hupdate:
	if (info->batt_health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE &&
	    info->batt_health != POWER_SUPPLY_HEALTH_DEAD &&
	    health != info->batt_health) {
		info->batt_health = health;
		cancel_work_sync(&info->monitor_work);
		queue_work(info->monitor_wqueue, &info->monitor_work);
	}

	return 0;
}

static void check_chgcurrent(struct sec_bat_info *info)
{
	pr_debug("[battery] charging current doesn't exist\n");
}

static void sec_check_chgcurrent(struct sec_bat_info *info)
{
	static int cnt;
	static int cnt_ui;
	bool is_full_condition = false;

	struct power_supply *psy = power_supply_get_by_name(info->charger_name);
	union power_supply_propval value;
	int ret;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return;
	}

	if (info->charging_enabled) {
		check_chgcurrent(info);
		if (info->batt_vcell >= info->full_cond_voltage) {
			/* check full state with SMB charger register */
			ret =
			    psy->get_property(psy,
					      POWER_SUPPLY_PROP_CHARGE_FULL,
					      &value);
			if (ret < 0) {
				dev_err(info->dev,
					"%s: fail to get charge full(%d)\n",
					__func__, ret);
				return;
			}

			if (info->test_info.test_value == 3) {
				value.intval = 0;
				info->batt_current_adc =
					CURRENT_OF_FULL_CHG + 1;
			}

			info->is_top_off = value.intval;

			if (info->batt_current_adc == 0) {
				if (info->is_top_off == 1 &&
					info->batt_presoc >= 99) {
					if (is_full_condition == false)
						wake_lock_timeout(
						&info->monitor_wake_lock,
						10 * HZ);
					is_full_condition = true;
					pr_info
					    ("%s : is_top_off (%d)\n",
					     __func__,
					     info->batt_current_adc);
				} else
					is_full_condition = false;

			} else {
			/* adc data is ok */
#if defined(CONFIG_TARGET_LOCALE_USA)
				if (info->charging_status !=
				    POWER_SUPPLY_STATUS_FULL
				    && info->batt_current_adc <=
				    CURRENT_OF_FULL_CHG_UI
				    && !info->ui_full_charge_status) {
					cnt_ui++;
					if (cnt_ui >= info->full_cond_count) {
						info->ui_full_charge_status =
						    true;
						cnt_ui = 0;
						power_supply_changed(&info->
								     psy_bat);
					}
				}
#endif /* CONFIG_TARGET_LOCALE_USA */
/*
//[Jaguar, D2] disable until ADC ready to get charging current
				if (info->batt_current_adc <=
				    CURRENT_OF_FULL_CHG) {
					is_full_condition = true;
				} else {
					is_full_condition = false;
				}
*/
			}
			if (is_full_condition) {
				cnt++;
				pr_info("%s : full state? %d, %d\n", __func__,
					info->batt_current_adc, cnt);
				if (cnt >= info->full_cond_count) {
					pr_info("%s : full state!! %d/%d\n",
						__func__, cnt,
						info->full_cond_count);
					sec_bat_handle_charger_topoff(info);
					cnt = 0;
				}
			}
		} else {
			cnt = 0;
			cnt_ui = 0;
		}
	} else {
		cnt = 0;
		cnt_ui = 0;
		info->batt_current_adc = 0;
	}
	info->test_info.full_count = cnt;
}

static int sec_check_recharging(struct sec_bat_info *info)
{
	int cnt = 0;
	int ret = 0;

	if (info->charging_status != POWER_SUPPLY_STATUS_FULL ||
	    info->recharging_status != false) {
		return 0;
	}

	for (cnt = 0; cnt < 4; cnt++) {
		info->batt_vcell = sec_bat_get_fuelgauge_data(info, FG_T_VCELL);
		if ((info->batt_vcell > info->vrechg) ||
			(info->charging_status != POWER_SUPPLY_STATUS_FULL))
			return 0;
		else {
			pr_info("%s : cnt = %d\n", __func__, cnt);
			mdelay(5000);
		}
	}
	if (cnt == 4) {
		ret = 1;
		info->test_info.is_rechg_state = true;
		pr_info("if rechg(%d), cnt(%d)\n", ret, cnt);
	} else
		ret = 0;
	pr_info("[BAT] rechg(%d), cnt(%d)\n", ret, cnt);

	info->test_info.rechg_count = cnt;
	return ret;
}

static int sec_bat_notify_vcell2charger(struct sec_bat_info *info)
{
	struct power_supply *psy = power_supply_get_by_name(info->charger_name);
	union power_supply_propval val_vcell;
	int ret;

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	/* Notify Voltage Now */
	val_vcell.intval = info->batt_vcell;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val_vcell);
	if (ret) {
		dev_err(info->dev, "%s: fail to notify vcell now(%d)\n",
			__func__, ret);
		return ret;
	}

	return 0;
}

static void sec_bat_update_info(struct sec_bat_info *info)
{
	info->batt_presoc = info->batt_soc;
	info->batt_raw_soc = sec_bat_get_fuelgauge_data(info, FG_T_PSOC);
	info->batt_soc = sec_bat_get_fuelgauge_data(info, FG_T_SOC);
	info->batt_vcell = sec_bat_get_fuelgauge_data(info, FG_T_VCELL);
	info->batt_rcomp = sec_bat_get_fuelgauge_data(info, FG_T_RCOMP);
	info->batt_full_soc = sec_bat_get_fuelgauge_data(info, FG_T_FSOC);
	sec_bat_notify_vcell2charger(info);
}

static int sec_bat_enable_charging(struct sec_bat_info *info, bool enable)
{
	struct power_supply *psy = power_supply_get_by_name(info->charger_name);
	union power_supply_propval val_type, val_chg_current, val_topoff,
	    val_vcell;
	int ret;

	// ktime_t current_time;
	struct timespec ts;

	// current_time = alarm_get_elapsed_realtime();
	get_monotonic_boottime(&ts);

	if (!psy) {
		dev_err(info->dev, "%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	info->batt_full_status = BATT_NOT_FULL;

	if (enable) {
		switch (info->cable_type) {
		case CABLE_TYPE_USB:
			val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
			val_chg_current.intval = 500;
			info->current_avg = -1;
			break;
		case CABLE_TYPE_AC:
		case CABLE_TYPE_CARDOCK:
		case CABLE_TYPE_UARTOFF:
		case CABLE_TYPE_CDP:
		case CABLE_TYPE_SMART_DOCK:
			val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
			val_chg_current.intval = 900;
			info->current_avg = 0;
			break;
#ifdef CONFIG_WIRELESS_CHARGING
		case CABLE_TYPE_WPC:
			val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
			val_chg_current.intval = info->wpc_charging_current;
			if (info->wpc_charging_current == 700)
				info->current_avg = 0;
			else
				info->current_avg = -1;
			break;
#endif
		case CABLE_TYPE_MISC:
			val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
			val_chg_current.intval = 700;
			info->current_avg = -1;
			break;
		case CABLE_TYPE_UNKNOWN:
			val_type.intval = POWER_SUPPLY_STATUS_CHARGING;
			val_chg_current.intval = 900;
			info->current_avg = 0;
			break;
		default:
			dev_err(info->dev, "%s: Invalid func use\n", __func__);
			return -EINVAL;
		}

		/* Set charging current */
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_CURRENT_NOW,
					&val_chg_current);
		if (ret) {
			dev_err(info->dev, "%s: fail to set charging cur(%d)\n",
				__func__, ret);
			return ret;
		}

		/* Set topoff current */
		val_topoff.intval = info->iterm;
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_CHARGE_FULL,
					&val_topoff);
		if (ret) {
			dev_err(info->dev, "%s: fail to set topoff val(%d)\n",
				__func__, ret);
			return ret;
		}

		/* Notify Voltage Now */
		info->batt_vcell = sec_bat_get_fuelgauge_data(info, FG_T_VCELL);
		val_vcell.intval = info->batt_vcell;
		ret = psy->set_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW,
					&val_vcell);
		if (ret) {
			dev_err(info->dev, "%s: fail to notify vcell now(%d)\n",
				__func__, ret);
			return ret;
		}

		info->charging_start_time = ts.tv_sec;
	} else {
		/* Disable charging */
		val_type.intval = POWER_SUPPLY_STATUS_DISCHARGING;
		info->charging_passed_time = 0;
		info->charging_start_time = 0;
		info->current_avg = -1;

		/* Double check recharging */
		info->recharging_status = false;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_STATUS, &val_type);
	if (ret) {
		dev_err(info->dev, "%s: fail to set charging status(%d)\n",
			__func__, ret);
		return ret;
	}

	if (info->present == BATT_STATUS_MISSING) {
		pr_info("[battery] battery is missing, suspend SMB347 charger\n");
		psy->set_property(psy, POWER_SUPPLY_PROP_PRESENT, &val_type);
	}

	info->charging_enabled = enable;

	return 0;
}

#ifdef ADJUST_RCOMP_WITH_CHARGING_STATUS
static int sec_fg_update_rcomp(struct sec_bat_info *info)
{
	struct power_supply *psy
	    = power_supply_get_by_name(info->fuel_gauge_name);
	union power_supply_propval value;
	int ret;

	if (!psy) {
		pr_err("%s: fail to get fuelgauge ps\n", __func__);
		return -ENODEV;
	}

	if (info->fuel_gauge_name) {
		value.intval = info->charging_status;
		ret = psy->set_property(psy,
			POWER_SUPPLY_PROP_STATUS, &value);
		if (ret) {
			dev_err(info->dev, "%s: fail to set status(%d)\n",
				__func__, ret);
			return ret;
		}
	}
	return 0;
}
#endif

static void sec_bat_handle_unknown_disable(struct sec_bat_info *info)
{
	pr_info(" %s : cable_type = %d\n", __func__, info->cable_type);

	if (info->present == BATT_STATUS_MISSING)
		return;

	info->batt_full_status = BATT_NOT_FULL;
	info->recharging_status = false;
	info->test_info.is_rechg_state = false;
	info->charging_start_time = 0;
#if defined(CONFIG_TARGET_LOCALE_USA)
	info->ui_full_charge_status = false;
#endif
#ifdef CONFIG_WIRELESS_CHARGING
	info->wc_status = false;
#endif
	info->charging_status = POWER_SUPPLY_STATUS_DISCHARGING;
	info->is_timeout_chgstop = false;
#ifdef ADJUST_RCOMP_WITH_CHARGING_STATUS
	sec_fg_update_rcomp(info);
#endif
	sec_bat_enable_charging(info, false);

	power_supply_changed(&info->psy_bat);
}

static void sec_bat_cable_work(struct work_struct *work)
{
	struct sec_bat_info *info = container_of(work, struct sec_bat_info,
						 cable_work.work);
	struct power_supply *psy = power_supply_get_by_name(info->charger_name);
	union power_supply_propval val_input;
	int ret = 0;
	u8 pok_cnt = 0;

	wake_lock(&info->cable_wake_lock);

	if ((info->present == BATT_STATUS_MISSING)
		&& (info->cable_type != CABLE_TYPE_NONE)) {
		pr_info("[battery] VF is open, do not care cable_work\n");
		info->batt_health = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		psy->set_property(psy, POWER_SUPPLY_PROP_PRESENT, &val_input);
		queue_work(info->monitor_wqueue, &info->monitor_work);
		goto cable_skip;
	}

	switch (info->cable_type) {
	case CABLE_TYPE_NONE:
#if defined(CONFIG_CHARGER_SMB347) && \
	defined(CONFIG_WIRELESS_CHARGING)
		/* check charger state*/
		ret = psy->get_property(psy,
			POWER_SUPPLY_PROP_WIRELESS_CHARGING,
			&val_input);
		if (!ret) {
			if (info->wc_status == false &&
				val_input.intval == INPUT_DCIN) {
				info->wc_status = true;
				info->cable_type = CABLE_TYPE_WPC;
				ret = psy->set_property(psy,
					POWER_SUPPLY_PROP_WIRELESS_CHARGING,
					&val_input);
				if (ret < 0)
					pr_err("%s : failed to set charger state(%d)\n",
								__func__, ret);
				queue_delayed_work(info->monitor_wqueue,
					&info->cable_work,
					msecs_to_jiffies(500));
				pr_info("cable none : power source is dcin. retry cable work!!!\n");
				return;
			}
		} else
			pr_err("%s : failed to get input source(%d)\n",
				__func__, ret);
#endif
		/* TODO : check DCIN state again */
		while (1) {
			if ((sec_bat_is_charging(info)
				== POWER_SUPPLY_STATUS_CHARGING)
			    && info->dcin_intr_triggered
			    && !info->slate_mode) {
				pok_cnt++;
				if (pok_cnt >= 2) {
					pr_info("cable none : vdcin ok, skip!!!\n");
					wake_unlock(&info->cable_wake_lock);
					return;
				}
				msleep(300);
			} else {
				pr_info("cable none : vdcin nok, cable is removed.\n");
				break;
			}
		}

/*		if (info->batt_int_irq_use) {
			info->present = BATT_STATUS_PRESENT;
			info->batt_health = POWER_SUPPLY_HEALTH_GOOD;
			disable_irq_nosync(info->batt_int_irq);
		}*/
		wake_lock_timeout(&info->vbus_wake_lock, 5 * HZ);
		cancel_delayed_work(&info->measure_work);
		info->batt_full_status = BATT_NOT_FULL;
		info->recharging_status = false;
		info->test_info.is_rechg_state = false;
		info->charging_start_time = 0;
		info->charging_status = POWER_SUPPLY_STATUS_DISCHARGING;
		info->is_timeout_chgstop = false;
		info->dcin_intr_triggered = false;
#if defined(CONFIG_TARGET_LOCALE_USA)
		info->ui_full_charge_status = false;
#endif
#ifdef CONFIG_WIRELESS_CHARGING
		info->wc_status = false;
#endif
		sec_bat_enable_charging(info, false);
		info->measure_interval = MEASURE_DSG_INTERVAL;
		queue_delayed_work(info->monitor_wqueue, &info->measure_work,
				   HZ / 2);
		break;
	case CABLE_TYPE_MISC:
	case CABLE_TYPE_CARDOCK:
	case CABLE_TYPE_UARTOFF:
		if (!info->dcin_intr_triggered && !info->lpm_chg_mode) {
			wake_lock_timeout(&info->vbus_wake_lock, 5 * HZ);
			pr_info("%s : dock inserted, but dcin nok skip charging!\n",
			     __func__);
			sec_bat_enable_charging(info, true);
			info->charging_enabled = false;
			break;
		}
	case CABLE_TYPE_UNKNOWN:
	case CABLE_TYPE_USB:
	case CABLE_TYPE_AC:
	case CABLE_TYPE_CDP:
	case CABLE_TYPE_SMART_DOCK:
#if defined(CONFIG_CHARGER_SMB347) && \
	defined(CONFIG_WIRELESS_CHARGING)
	case CABLE_TYPE_WPC:
		/* set input state */
		ret = psy->get_property(psy,
			POWER_SUPPLY_PROP_WIRELESS_CHARGING,
			&val_input);
		if (!ret) {
			if (info->wc_status == true &&
				val_input.intval != INPUT_DCIN) {
				info->wc_status = false;
				ret = psy->set_property(psy,
					POWER_SUPPLY_PROP_WIRELESS_CHARGING,
					&val_input);
				if (ret < 0)
					pr_err("%s : failed to set input source(%d)\n",
								__func__, ret);
			}
		} else
			pr_err("%s : failed to get input source(%d)\n",
				__func__, ret);
#endif

/*
		if (((info->cable_type == CABLE_TYPE_AC) ||
			(info->cable_type == CABLE_TYPE_USB)) &&
			(info->batt_int != 0)) {
			enable_irq(info->batt_int_irq);
			info->batt_int_irq_use = 1;
			info->present =
				!gpio_get_value_cansleep(info->batt_int);
		}
*/

		wake_lock_timeout(&info->vbus_wake_lock, 5 * HZ);
		cancel_delayed_work(&info->measure_work);
		info->charging_status = POWER_SUPPLY_STATUS_CHARGING;
		sec_bat_enable_charging(info, true);
		info->measure_interval = MEASURE_CHG_INTERVAL;
		queue_delayed_work(info->monitor_wqueue, &info->measure_work,
				   HZ / 2);
		queue_work(info->monitor_wqueue, &info->monitor_work);
		break;
	default:
		dev_err(info->dev, "%s: Invalid cable type\n", __func__);
		break;
	}

cable_skip:
#ifdef ADJUST_RCOMP_WITH_CHARGING_STATUS
	sec_fg_update_rcomp(info);
#endif
	power_supply_changed(&info->psy_ac);
	power_supply_changed(&info->psy_usb);
	power_supply_changed(&info->psy_bat);

	wake_unlock(&info->cable_wake_lock);
}

static void sec_bat_charging_time_management(struct sec_bat_info *info)
{
	unsigned long charging_time=0;
	// ktime_t	current_time;
	struct timespec ts;

	// current_time = alarm_get_elapsed_realtime();
	get_monotonic_boottime(&ts);

	/* If timeout_chgstop is already set, then re-start charging */
	if (info->is_timeout_chgstop) {
		info->is_timeout_chgstop = false;
		if (info->charging_status == POWER_SUPPLY_STATUS_FULL)
			info->recharging_status = true;
		sec_bat_enable_charging(info, true);
		pr_info("%s: [BATT] chgstop is cancelled\n",
			 __func__);
	}

	if (info->charging_start_time == 0) {
		pr_debug("%s: charging_start_time=0\n", __func__);
		return;
	}

	if (info->charging_enabled) {
		if (ts.tv_sec >= info->charging_start_time)
			charging_time = ts.tv_sec - info->charging_start_time;
		else
			charging_time = 0xFFFFFFFF - info->charging_start_time +
						ts.tv_sec;

		info->charging_passed_time = charging_time;
	}

	switch (info->charging_status) {
	case POWER_SUPPLY_STATUS_FULL:
		if (charging_time > info->rechg_time &&
			info->recharging_status == true) {
			sec_bat_enable_charging(info, false);
			info->recharging_status = false;
			info->is_timeout_chgstop = true;
			pr_info("%s: [BATT]Recharging timer expired\n",
				 __func__);
		}
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
		if (charging_time > info->abs_time) {
			sec_bat_enable_charging(info, false);
			info->charging_status = POWER_SUPPLY_STATUS_FULL;
			info->is_timeout_chgstop = true;

			pr_info("%s: [BATT]Charging timer expired\n",
				 __func__);
		}
		break;
	default:
		info->is_timeout_chgstop = false;
		return;
	}

	dev_dbg(info->dev, "[BATT]Time past : %lu secs, Spec : %d, %d\n",
		charging_time,
		info->abs_time, info->rechg_time);

	return;
}

static void sec_bat_monitor_work(struct work_struct *work)
{
	int i;
	struct sec_bat_info *info = container_of(work, struct sec_bat_info,
						 monitor_work);
	struct power_supply *psy_fg =
	    power_supply_get_by_name(info->fuel_gauge_name);
	struct power_supply *psy_smb =
	    power_supply_get_by_name(info->charger_name);

	union power_supply_propval value;
	struct timespec ts;
	int ret = 0;

	wake_lock(&info->monitor_wake_lock);

	if (!psy_fg) {
		pr_err("%s: fail to get charger ps\n", __func__);
		goto monitoring_skip;
	}

	pm8921_enable_batt_therm(1);
	/* check battery 5 times */
	for (i = 0; i < 5; i++) {
		msleep(500);
		info->present = !gpio_get_value_cansleep(info->batt_int);

		/* If the battery is missing, then check more */
		if (info->present) {
			i++;
			break;
		}
	}
	pm8921_enable_batt_therm(0);
	pr_debug("%s: battery check is %s (%d time%c)\n",
		__func__, info->present ? "present" : "absent",
		i, (i == 1) ? ' ' : 's');

	if ((info->present == BATT_STATUS_MISSING)
			&& (info->cable_type != CABLE_TYPE_NONE)) {
		pr_info("[battery] battery is missing, suspend SMB347 charger\n");
		psy_smb->set_property(psy_smb,
			POWER_SUPPLY_PROP_PRESENT, &value);
	}

#ifdef CONFIG_WIRELESS_CHARGING
	if (info->wpc_abs_time &&
			(info->cable_type == CABLE_TYPE_WPC)) {
		info->abs_time = info->wpc_abs_time;
		pr_debug("[battery] wpc abs time (%d)\n", info->abs_time);
	} else {
		info->abs_time = info->normal_abs_time;
		pr_debug("[battery] normal abs time (%d)\n",
			info->abs_time);
	}
#endif

	sec_bat_charging_time_management(info);

	sec_bat_update_info(info);
	sec_bat_check_vf(info);
	sec_check_chgcurrent(info);

#ifdef ADJUST_RCOMP_WITH_TEMPER
	sec_fg_update_temper(info);
#endif
	pr_debug("[battery] level(%d), vcell(%d), therm(%d)\n",
		info->batt_soc, info->batt_vcell, info->batt_temp);
	pr_debug("[battery] cable_type(%d), chg_status(%d), health(%d)\n",
	 info->cable_type, info->charging_status, info->batt_health);
	pr_debug("[battery] timeout_chgstp(%d), chg_enabled(%d), rechg_status(%d)\n",
		info->is_timeout_chgstop, info->charging_enabled,
		info->recharging_status);

	if (sec_check_recharging(info)) {
		pr_info("%s : rechg triggered!\n", __func__);
		info->is_rechg_triggered = true;
	}

	switch (info->charging_status) {
	case POWER_SUPPLY_STATUS_FULL:
		/* notify full state to fuel guage */
		info->check_full_state = true;
		if (!info->is_timeout_chgstop) {
			value.intval = POWER_SUPPLY_STATUS_FULL;
			ret =
			    psy_fg->set_property(psy_fg,
						 POWER_SUPPLY_PROP_STATUS,
						 &value);
		}

		if (info->is_rechg_triggered &&
		    info->recharging_status == false) {
			info->recharging_status = true;
			sec_bat_enable_charging(info, true);
			info->is_rechg_triggered = false;

			dev_info(info->dev,
				 "%s: Start Recharging, Vcell = %d\n", __func__,
				 info->batt_vcell);
		}
		/* break; */
	case POWER_SUPPLY_STATUS_CHARGING:
		if (info->batt_health == POWER_SUPPLY_HEALTH_OVERHEAT
		    || info->batt_health == POWER_SUPPLY_HEALTH_COLD) {
			sec_bat_enable_charging(info, false);
			info->charging_status =
			    POWER_SUPPLY_STATUS_NOT_CHARGING;
			info->test_info.is_rechg_state = false;

			pr_info("%s: Not charging\n", __func__);
		} else if (info->batt_health ==
			POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
			sec_bat_enable_charging(info, false);
			info->charging_status =
			    POWER_SUPPLY_STATUS_NOT_CHARGING;
			info->test_info.is_rechg_state = false;
			pr_info("%s: Not charging (VF err!)\n",
				 __func__);
		}
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
		dev_dbg(info->dev, "%s: Discharging\n", __func__);
		if ((info->batt_health ==
			POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) &&
			(info->cable_type != CABLE_TYPE_NONE)) {
			sec_bat_enable_charging(info, false);
			info->charging_status =
			    POWER_SUPPLY_STATUS_NOT_CHARGING;
			info->test_info.is_rechg_state = false;
			pr_info("%s: Not charging (VF err!)\n",
				 __func__);
		}
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		if (info->batt_health == POWER_SUPPLY_HEALTH_GOOD) {
			dev_info(info->dev, "%s: recover health state\n",
				 __func__);
			if (info->cable_type != CABLE_TYPE_NONE) {
				sec_bat_enable_charging(info, true);
				info->charging_status
				    = POWER_SUPPLY_STATUS_CHARGING;
			} else
				info->charging_status
				    = POWER_SUPPLY_STATUS_DISCHARGING;
		}
		break;
	default:
		pr_err("%s: Undefined Battery Status\n", __func__);
		goto monitoring_skip;
	}

	/* check default charger state, and set again */
	if (sec_bat_is_charging(info) == POWER_SUPPLY_STATUS_CHARGING &&
	    info->charging_enabled) {
		if (sec_bat_is_invalid_bmd(info)) {
			pr_info("%s : default charger state, set again\n",
				__func__);
			queue_delayed_work(info->monitor_wqueue,
					   &info->cable_work, 0);
		}
	}

	if (info->check_full_state == true &&
		info->charging_status == POWER_SUPPLY_STATUS_DISCHARGING) {
		if (info->check_full_state_cnt <= 1) {
			info->batt_soc = 100;
			info->check_full_state_cnt++;
		} else {
			if (info->batt_presoc > info->batt_soc) {
				info->batt_soc = info->batt_presoc - 1;
			} else {
				info->check_full_state = false;
				info->check_full_state_cnt = 0;
			}
		}
	}

	if (info->batt_soc != info->batt_presoc)
		pr_info("[fg] p:%d, s1:%d, s2:%d, v:%d, t:%d\n",
			info->batt_raw_soc, info->batt_soc, info->batt_presoc,
			info->batt_vcell, info->batt_temp_radc);

	power_supply_changed(&info->psy_bat);

monitoring_skip:
	info->cur_monitor_time = ktime_get_boottime();
	ts = ktime_to_timespec(info->cur_monitor_time);

	sec_bat_monitoring_alarm(info, CHARGING_ALARM_INTERVAL);
	wake_unlock(&info->monitor_wake_lock);

	return;
}

static void sec_bat_measure_work(struct work_struct *work)
{
	struct sec_bat_info *info =
	    container_of(work, struct sec_bat_info, measure_work.work);
	unsigned long flags;
	bool isFirstCheck = false;

	wake_lock(&info->measure_wake_lock);

	sec_bat_check_temper_adc(info);
	/* sec_bat_check_detbat(info); */

	/* check dcin */
	if ((sec_bat_is_charging(info) == POWER_SUPPLY_STATUS_CHARGING) &&
	    (info->charging_status == POWER_SUPPLY_STATUS_DISCHARGING)) {
		pr_info
		    ("%s : dcin ok, but not charging, set cable type again!\n",
		     __func__);
		if (0 == is_charging_disabled) {
			local_irq_save(flags);
			if (info->cable_type == CABLE_TYPE_NONE) {
#if defined(CONFIG_TARGET_LOCALE_USA)
				if (info->cable_uart_off)
					info->cable_type = CABLE_TYPE_UARTOFF;
				else
#endif
					info->cable_type = CABLE_TYPE_UNKNOWN;
			}

			local_irq_restore(flags);
			queue_delayed_work(info->monitor_wqueue,
					   &info->cable_work, HZ);
		}
	} else
	    if ((sec_bat_is_charging(info) == POWER_SUPPLY_STATUS_DISCHARGING)
		&& (info->charging_status != POWER_SUPPLY_STATUS_DISCHARGING)) {
		pr_info("%s : dcin nok, but still charging, just disable charging!\n",
		     __func__);
		local_irq_save(flags);
		if (info->cable_type == CABLE_TYPE_UNKNOWN ||
#ifdef CONFIG_WIRELESS_CHARGING
			info->cable_type == CABLE_TYPE_WPC ||
#endif
		    info->cable_type == CABLE_TYPE_UARTOFF)
			info->cable_type = CABLE_TYPE_NONE;
		local_irq_restore(flags);
		sec_bat_handle_unknown_disable(info);
	}

	if (info->charging_enabled &&
	    (((0 < info->batt_temp_high_cnt)
	      && (info->batt_temp_high_cnt < TEMP_BLOCK_COUNT))
	     || ((0 < info->batt_temp_low_cnt)
		 && (info->batt_temp_low_cnt < TEMP_BLOCK_COUNT)))) {
		isFirstCheck = true;
	} else {
		isFirstCheck = false;
	}

	if (info->initial_check_count) {
		info->initial_check_count--;
		queue_delayed_work(info->monitor_wqueue, &info->measure_work,
				   HZ);
	} else if (isFirstCheck) {
		queue_delayed_work(info->monitor_wqueue, &info->measure_work,
				   HZ);
	} else {
		queue_delayed_work(info->monitor_wqueue, &info->measure_work,
				   msecs_to_jiffies(info->measure_interval));
	}
	wake_unlock(&info->measure_wake_lock);
}

/*
static irqreturn_t batt_removal_handler(int irq, struct sec_bat_info *info)
{
	struct power_supply *psy = power_supply_get_by_name(info->charger_name);
	union power_supply_propval value;
	int ret = 0;

	if (!psy) {
		pr_err("%s: fail to get charger ps\n", __func__);
		return -ENODEV;
	}

	ret = gpio_get_value_cansleep(info->batt_int);
	pr_info("%s, %d\n", __func__, ret);

	if (ret < 0) {
		pr_err("%s batt_in gpio failed ret=%d\n",
			__func__, ret);
		goto err;
	}

	info->present = ret ? 0 : 1;
	if (ret)
		psy->set_property(psy, POWER_SUPPLY_PROP_PRESENT, &value);
err:
	return IRQ_HANDLED;
}
*/

#define SEC_BATTERY_ATTR(_name)		\
{									\
	.attr = { .name = #_name,		\
		  .mode = 0664,				\
		  /*.owner = THIS_MODULE */},	\
	.show = sec_bat_show_property,	\
	.store = sec_bat_store,			\
}

static struct device_attribute sec_battery_attrs[] = {
	SEC_BATTERY_ATTR(batt_vol),
	SEC_BATTERY_ATTR(batt_read_adj_soc),
	SEC_BATTERY_ATTR(batt_vfocv),
	SEC_BATTERY_ATTR(batt_vf_adc),
	SEC_BATTERY_ATTR(batt_temp),
	SEC_BATTERY_ATTR(batt_temp_adc),
	SEC_BATTERY_ATTR(batt_temp_radc),
	SEC_BATTERY_ATTR(batt_charging_source),
	SEC_BATTERY_ATTR(batt_lp_charging),
	SEC_BATTERY_ATTR(batt_type),
	SEC_BATTERY_ATTR(batt_full_check),
	SEC_BATTERY_ATTR(batt_temp_check),
	SEC_BATTERY_ATTR(batt_temp_adc_spec),
	SEC_BATTERY_ATTR(batt_test_value),
	SEC_BATTERY_ATTR(batt_current_adc),
	SEC_BATTERY_ATTR(batt_esus_test),
	SEC_BATTERY_ATTR(sys_rev),
	SEC_BATTERY_ATTR(batt_read_raw_soc),
	SEC_BATTERY_ATTR(batt_reset_soc),
#ifdef CONFIG_WIRELESS_CHARGING
	SEC_BATTERY_ATTR(wc_status),
	SEC_BATTERY_ATTR(wc_adc),
#endif
#if !defined(CONFIG_BATTERY_CTIA)
	SEC_BATTERY_ATTR(talk_wcdma),
	SEC_BATTERY_ATTR(talk_gsm),
	SEC_BATTERY_ATTR(data_call),
	SEC_BATTERY_ATTR(camera),
	SEC_BATTERY_ATTR(browser),
#endif
	SEC_BATTERY_ATTR(charging_enable),
#if defined(CONFIG_BATTERY_CTIA)
	SEC_BATTERY_ATTR(talk_wcdma),
	SEC_BATTERY_ATTR(talk_gsm),
	SEC_BATTERY_ATTR(call),
	SEC_BATTERY_ATTR(video),
	SEC_BATTERY_ATTR(music),
	SEC_BATTERY_ATTR(browser),
	SEC_BATTERY_ATTR(hotspot),
	SEC_BATTERY_ATTR(camera),
	SEC_BATTERY_ATTR(data_call),
	SEC_BATTERY_ATTR(gps),
	SEC_BATTERY_ATTR(lte),
	SEC_BATTERY_ATTR(wifi),
	SEC_BATTERY_ATTR(batt_use),
#endif /*CONFIG_BATTERY_CTIA*/
	SEC_BATTERY_ATTR(batt_slate_mode),
	SEC_BATTERY_ATTR(current_avg),
	SEC_BATTERY_ATTR(factory_mode),
};

enum {
	BATT_VOL = 0,
	BATT_SOC,
	BATT_VFOCV,
	BATT_VFADC,
	BATT_TEMP,
	BATT_TEMP_ADC,
	BATT_TEMP_RADC,
	BATT_CHARGING_SOURCE,
	BATT_LP_CHARGING,
	BATT_TYPE,
	BATT_FULL_CHECK,
	BATT_TEMP_CHECK,
	BATT_TEMP_ADC_SPEC,
	BATT_TEST_VALUE,
	BATT_CURRENT_ADC,
	BATT_ESUS_TEST,
	BATT_SYSTEM_REV,
	BATT_READ_RAW_SOC,
	BATT_RESET_SOC,
#ifdef CONFIG_WIRELESS_CHARGING
	BATT_WC_STATUS,
	BATT_WC_ADC,
#endif
#if !defined(CONFIG_BATTERY_CTIA)
	BATT_WCDMA_CALL,
	BATT_GSM_CALL,
	BATT_DATACALL,
	BATT_CAMERA,
	BATT_BROWSER,
#endif
	BATT_CHARGING_ENABLE,
#if defined(CONFIG_BATTERY_CTIA)
	BATT_WCDMA_CALL,
	BATT_GSM_CALL,
	BATT_CALL,
	BATT_VIDEO,
	BATT_MUSIC,
	BATT_BROWSER,
	BATT_HOTSPOT,
	BATT_CAMERA,
	BATT_DATA_CALL,
	BATT_GPS,
	BATT_LTE,
	BATT_WIFI,
	BATT_USE,
#endif /*CONFIG_BATTERY_CTIA*/
	BATT_SLATE_MODE,
	BATT_CURR_AVG,
	BATT_FACTORY,
};

#if defined(CONFIG_BATTERY_CTIA)
static void  sec_bat_program_alarm(struct sec_bat_info *info,
								int seconds)
{
	ktime_t low_interval = ktime_set(seconds - 10, 0);
	// ktime_t slack = ktime_set(20, 0);
	ktime_t next;

	next = ktime_add(info->cur_time, low_interval);
	/* The original slack time called for, 20 seconds, exceeds
        * the length allowed for an unsigned long in nanoseconds. Use
        * ULONG_MAX instead
        */
        hrtimer_start_range_ns(&info->event_timer,
                next, ULONG_MAX, HRTIMER_MODE_ABS);
}

enum hrtimer_restart sec_bat_use_timer_func(struct hrtimer *hrtimer)
{
	struct sec_bat_info *info =
		container_of(hrtimer, struct sec_bat_info, event_timer);
	info->batt_use &= (~info->batt_use_wait);
	pr_info("/BATT_USE/ timer expired (0x%x)\n", info->batt_use);
	return HRTIMER_NORESTART;
}

static void sec_bat_use_module(struct sec_bat_info *info,
						int module, int enable)
{
	struct timespec ts;

	/* ignore duplicated deactivation of same event  */
	if (!enable && (info->batt_use == info->batt_use_wait)) {
		pr_info("/BATT_USE/ ignore duplicated same event\n");
		return;
	}

	/*del_timer_sync(&info->bat_use_timer);*/
	hrtimer_cancel(&info->event_timer);
	info->batt_use &= (~info->batt_use_wait);

	if (enable) {
		info->batt_use_wait = 0;
		info->batt_use |= module;

		/* debug msg */
		if (module == USE_CALL)
			pr_info("/BATT_USE/ call 0x%x\n", info->batt_use);
		else if (module == USE_VIDEO)
			pr_info("/BATT_USE/ video 0x%x\n", info->batt_use);
		else if (module == USE_MUSIC)
			pr_info("/BATT_USE/ music 0x%x\n", info->batt_use);
		else if (module == USE_BROWSER)
			pr_info("/BATT_USE/ browser 0x%x\n", info->batt_use);
		else if (module == USE_HOTSPOT)
			pr_info("/BATT_USE/ hotspot 0x%x\n", info->batt_use);
		else if (module == USE_CAMERA)
			pr_info("/BATT_USE/ camera 0x%x\n", info->batt_use);
		else if (module == USE_DATA_CALL)
			pr_info("/BATT_USE/ datacall 0x%x\n", info->batt_use);
		else if (module == USE_LTE)
			pr_info("/BATT_USE/ lte 0x%x\n", info->batt_use);
		else if (module == USE_WIFI)
			pr_info("/BATT_USE/ wifi 0x%x\n", info->batt_use);
	} else {
		if (info->batt_use == 0) {
			pr_info("/BATT_USE/ nothing to clear\n");
			return;	/* nothing to clear */
		}
		info->batt_use_wait = module;
		info->cur_time = ktime_get_boottime();
		ts = ktime_to_timespec(info->cur_time);

		sec_bat_program_alarm(info, TOTAL_EVENT_TIME);
		pr_info("/BATT_USE/ start timer (curr 0x%x, wait 0x%x)\n",
			info->batt_use, info->batt_use_wait);
	}
}

#endif /*CONFIG_BATTERY_CTIA*/

static ssize_t sec_bat_show_property(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct sec_bat_info *info = dev_get_drvdata(dev->parent);
	int i = 0, val = 0;
	const ptrdiff_t off = attr - sec_battery_attrs;

	switch (off) {
	case BATT_VOL:
		val = sec_bat_get_fuelgauge_data(info, FG_T_VCELL);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_SOC:
		val = sec_bat_get_fuelgauge_data(info, FG_T_SOC);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_VFOCV:
		/* max17048 doesn't have VFOCV register */
		pr_err("skip! MAX17048 doesn't have VFOCV register.\n");
		break;
	case BATT_VFADC:
		/* read batt_id */
		break;
	case BATT_TEMP:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", info->batt_temp);
		break;
	case BATT_TEMP_ADC:
		pm8921_get_temperature_adc(info);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->batt_temp_adc);
		break;
	case BATT_TEMP_RADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->batt_temp_radc);
		break;
	case BATT_CHARGING_SOURCE:
		val = info->cable_type;
		/*val = 2; // for lpm test */
		if (info->lpm_chg_mode &&
		    info->cable_type != CABLE_TYPE_NONE &&
		    info->charging_status == POWER_SUPPLY_STATUS_DISCHARGING) {
			val = CABLE_TYPE_NONE;
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
	case BATT_LP_CHARGING:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       poweroff_charging);
		break;
	case BATT_TYPE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", "SDI_SDI");
		break;
	case BATT_FULL_CHECK:
		/* new concept : in case of time-out charging stop,
		   Do not update FULL for UI,
		   Use same time-out value for first charing and re-charging
		 */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       (info->is_timeout_chgstop == false &&
				info->charging_status ==
				POWER_SUPPLY_STATUS_FULL)
			       ? 1 : 0);
		break;
	case BATT_TEMP_CHECK:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			       "%d\n", info->batt_health);
		break;
	case BATT_TEMP_ADC_SPEC:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			       "(HIGH: %d / %d,   LOW: %d / %d)\n",
			       info->tspec.high_block,
			       info->tspec.high_recovery,
			       info->tspec.low_block,
			       info->tspec.low_recovery);
		break;
	case BATT_TEST_VALUE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->test_info.test_value);
		break;
	case BATT_CURRENT_ADC:
		check_chgcurrent(info);
#ifdef CONFIG_WIRELESS_CHARGING
		i += scnprintf(buf + i, PAGE_SIZE - i,
				   "%d\n",
				   (info->wc_status == false) ?
				   info->batt_current_adc : 0);
#else
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			   info->batt_current_adc);
#endif
		break;
	case BATT_ESUS_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->test_info.test_esuspend);
		break;
	case BATT_SYSTEM_REV:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", info->hw_rev);
		break;
	case BATT_READ_RAW_SOC:
		val = sec_bat_get_fuelgauge_data(info, FG_T_PSOC);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		break;
#if defined(CONFIG_BATTERY_CTIA)
	case BATT_WCDMA_CALL:
	case BATT_GSM_CALL:
	case BATT_CALL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(info->batt_use & USE_CALL) ? 1 : 0);
		break;
	case BATT_VIDEO:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(info->batt_use & USE_VIDEO) ? 1 : 0);
		break;
	case BATT_MUSIC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(info->batt_use & USE_MUSIC) ? 1 : 0);
		break;
	case BATT_BROWSER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(info->batt_use & USE_BROWSER) ? 1 : 0);
		break;
	case BATT_HOTSPOT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(info->batt_use & USE_HOTSPOT) ? 1 : 0);
		break;
	case BATT_CAMERA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(info->batt_use & USE_CAMERA) ? 1 : 0);
		break;
	case BATT_DATA_CALL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(info->batt_use & USE_DATA_CALL) ? 1 : 0);
		break;
	case BATT_GPS:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(info->batt_use & USE_GPS) ? 1 : 0);
		break;
	case BATT_LTE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(info->batt_use & USE_GPS) ? 1 : 0);
		break;
	case BATT_WIFI:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(info->batt_use & BATT_WIFI) ? 1 : 0);
		break;
	case BATT_USE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			info->batt_use);
		break;
#endif
	case BATT_CHARGING_ENABLE:
		{
			int val = (info->charging_enabled) ? 1 : 0;
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
		}
		break;
#ifdef CONFIG_WIRELESS_CHARGING
	case BATT_WC_STATUS:
		i += scnprintf(buf + i, PAGE_SIZE - i,
				   "%d\n",
				   (info->wc_status == true) ?
				   1 : 0);
		break;
	case BATT_WC_ADC:
		check_chgcurrent(info);
		i += scnprintf(buf + i, PAGE_SIZE - i,
				   "%d\n",
				   (info->wc_status == true) ?
				   info->batt_current_adc : 0);
		break;
#endif
	case BATT_SLATE_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       info->slate_mode);
		break;
	case BATT_CURR_AVG:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				   info->current_avg);
		break;
	default:
		i = -EINVAL;
	}

	return i;
}

static ssize_t sec_bat_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int ret = 0, x = 0;
	const ptrdiff_t off = attr - sec_battery_attrs;
	struct sec_bat_info *info = dev_get_drvdata(dev->parent);

	switch (off) {
	case BATT_ESUS_TEST:	/* early_suspend test */
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 0) {
				info->test_info.test_esuspend = 0;
				wake_lock_timeout(&info->test_wake_lock,
						  5 * HZ);
				cancel_delayed_work(&info->measure_work);
				info->measure_interval = MEASURE_DSG_INTERVAL;
				queue_delayed_work(info->monitor_wqueue,
						   &info->measure_work, 0);
			} else {
				info->test_info.test_esuspend = 1;
				wake_lock(&info->test_wake_lock);
				cancel_delayed_work(&info->measure_work);
				info->measure_interval = MEASURE_CHG_INTERVAL;
				queue_delayed_work(info->monitor_wqueue,
						   &info->measure_work, 0);
			}
			ret = count;
		}
		break;
	case BATT_TEST_VALUE:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 0)
				info->test_info.test_value = 0;
			else if (x == 1) {
				/* for temp warning event */
				info->test_info.test_value = 1;
			} else if (x == 2) {
				/*
				// for full event
				info->test_info.test_value = 2;
				*/
				/* disable full test interface. */
				info->test_info.test_value = 0;
			} else if (x == 3) {
				/* for abs time event */
				info->test_info.test_value = 3;
			} else if (x == 999) {
				/* for pop-up disable */
				info->test_info.test_value = 999;
				if ((info->batt_health ==
				     POWER_SUPPLY_HEALTH_OVERHEAT)
				    || (info->batt_health ==
					POWER_SUPPLY_HEALTH_COLD)) {
					info->batt_health =
					    POWER_SUPPLY_HEALTH_GOOD;
					queue_work(info->monitor_wqueue,
						   &info->monitor_work);
				}
			} else
				info->test_info.test_value = 0;
			pr_info("%s : test case : %d\n", __func__,
				info->test_info.test_value);
			ret = count;
		}
		break;
	case BATT_RESET_SOC:
		pr_info("battery reset soc\n");
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 1) {
				sec_bat_set_fuelgauge_reset(info);
				ret = count;
			}
		}
		break;

#if !defined(CONFIG_BATTERY_CTIA)
	case BATT_WCDMA_CALL:
	case BATT_GSM_CALL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			info->voice_call_state = x;
			pr_info("%s : voice call = %d, %d\n", __func__,
				x, info->voice_call_state);
		}
		break;
	case BATT_DATACALL:
		if (sscanf(buf, "%d\n", &x) == 1)
			dev_dbg(info->dev, "%s : data call = %d\n", __func__,
				x);
		break;
	case BATT_CAMERA:
		if (sscanf(buf, "%d\n", &x) == 1)
			ret = count;
		break;
	case BATT_BROWSER:
		if (sscanf(buf, "%d\n", &x) == 1)
			ret = count;
		break;
#endif
	case BATT_CHARGING_ENABLE:
		{
			pr_info("%s : BATT_CHARGING_ENABLE buf=[%s]\n",
				__func__, buf);
			if (sscanf(buf, "%d\n", &x) == 1) {
				if (x == 0) {
					is_charging_disabled = 1;
					info->cable_type = CABLE_TYPE_NONE;
					queue_delayed_work(info->monitor_wqueue,
							   &info->cable_work,
							   0);
				} else if (x == 1) {
					is_charging_disabled = 0;
					info->cable_type = CABLE_TYPE_UNKNOWN;
					queue_delayed_work(info->monitor_wqueue,
							   &info->cable_work,
							   0);
				} else
					pr_info("%s:Invalid\n", __func__);
				ret = count;
			}
		}
		break;
#if defined(CONFIG_BATTERY_CTIA)
	case BATT_WCDMA_CALL:
	case BATT_GSM_CALL:
	case BATT_CALL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			sec_bat_use_module(info, USE_CALL, x);
			ret = count;
		}
		pr_debug("[BAT]:%s: camera = %d\n", __func__, x);
		break;
	case BATT_VIDEO:
		if (sscanf(buf, "%d\n", &x) == 1) {
			sec_bat_use_module(info, USE_VIDEO, x);
			ret = count;
		}
		pr_debug("[BAT]:%s: mp3 = %d\n", __func__, x);
		break;
	case BATT_MUSIC:
		if (sscanf(buf, "%d\n", &x) == 1) {
			sec_bat_use_module(info, USE_MUSIC, x);
			ret = count;
		}
		pr_debug("[BAT]:%s: video = %d\n", __func__, x);
		break;
	case BATT_BROWSER:
		if (sscanf(buf, "%d\n", &x) == 1) {
			sec_bat_use_module(info, USE_BROWSER, x);
			ret = count;
		}
		pr_debug("[BAT]:%s: voice call 2G = %d\n", __func__, x);
		break;
	case BATT_HOTSPOT:
		if (sscanf(buf, "%d\n", &x) == 1) {
			sec_bat_use_module(info, USE_HOTSPOT, x);
			ret = count;
		}
		pr_debug("[BAT]:%s: voice call 3G = %d\n", __func__, x);
		break;
	case BATT_CAMERA:
		if (sscanf(buf, "%d\n", &x) == 1) {
			sec_bat_use_module(info, USE_CAMERA, x);
			ret = count;
		}
		pr_debug("[BAT]:%s: data call = %d\n", __func__, x);
		break;
	case BATT_DATA_CALL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			sec_bat_use_module(info, USE_DATA_CALL, x);
			ret = count;
		}
		pr_debug("[BAT]:%s: wifi = %d\n", __func__, x);
		break;
	case BATT_GPS:
		if (sscanf(buf, "%d\n", &x) == 1) {
			sec_bat_use_module(info, USE_GPS, x);
			ret = count;
		}
		break;
	case BATT_LTE:
		if (sscanf(buf, "%d\n", &x) == 1) {
			sec_bat_use_module(info, USE_LTE, x);
			ret = count;
		}
		pr_debug("[BAT]:%s: gps = %d\n", __func__, x);
		break;
	case BATT_WIFI:
		if (sscanf(buf, "%d\n", &x) == 1) {
			sec_bat_use_module(info, USE_WIFI, x);
			ret = count;
		}
		pr_debug("[BAT]:%s: gps = %d\n", __func__, x);
		break;
#endif
	case BATT_SLATE_MODE:
		{
			pr_info("%s : BATT_SLATE_MODE %s\n",
				__func__, buf);
			if (sscanf(buf, "%d\n", &x) == 1) {
				if (x == 1) {
					info->slate_mode = true;
					is_charging_disabled = 1;
					info->cable_type = CABLE_TYPE_NONE;
					queue_delayed_work(info->monitor_wqueue,
							   &info->cable_work,
							   0);
				} else if (x == 0) {
					info->slate_mode = false;
					is_charging_disabled = 0;
					info->cable_type = CABLE_TYPE_UNKNOWN;
					queue_delayed_work(info->monitor_wqueue,
							   &info->cable_work,
							   0);
				} else
					pr_info("%s:Invalid\n", __func__);
				ret = count;
			}
		}
		break;
	case BATT_FACTORY:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x)
				info->factory_mode = true;
			else
				info->factory_mode = false;
			ret = count;
		}
		pr_info("[battery]:%s: factory mode = %d\n", __func__, x);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int sec_bat_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(sec_battery_attrs); i++) {
		rc = device_create_file(dev, &sec_battery_attrs[i]);
		if (rc)
			goto sec_attrs_failed;
	}
	goto succeed;

sec_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_battery_attrs[i]);
succeed:
	return rc;
}

static int sec_bat_read_proc(char *buf, char **start,
			     off_t offset, int count, int *eof, void *data)
{
	struct sec_bat_info *info = data;
	struct timespec cur_time;
	// ktime_t ktime;
	int len = 0;

	// ktime = alarm_get_elapsed_realtime();
	get_monotonic_boottime(&cur_time);

	len = sprintf(buf,
		"%lu, %u, %u, %u, %u, %d, %u, %d, %d, %d, %u, %u, %u, %d, %u, %u, 0x%04x, %u, %lu\n",
		cur_time.tv_sec, info->batt_raw_soc,
		info->batt_soc, info->batt_vcell,
		info->batt_current_adc, info->charging_enabled,
		info->batt_full_status,
		info->test_info.full_count, info->test_info.rechg_count,
		info->test_info.is_rechg_state, info->recharging_status,
		info->batt_temp_radc, info->batt_health, info->charging_status,
		info->present, info->cable_type, info->batt_rcomp,
		info->batt_full_soc, info->charging_passed_time);
	return len;
}

static void sec_bat_early_suspend(struct early_suspend *handle)
{
	struct sec_bat_info *info = container_of(handle, struct sec_bat_info,
						 bat_early_suspend);

	pr_info("%s[BATT]...\n", __func__);
	info->is_esus_state = true;

	return;
}

static void sec_bat_late_resume(struct early_suspend *handle)
{
	struct sec_bat_info *info = container_of(handle, struct sec_bat_info,
						 bat_early_suspend);

	pr_info("%s[BATT]...\n", __func__);
	info->is_esus_state = false;

	return;
}

static __devinit int sec_bat_probe(struct platform_device *pdev)
{
	struct sec_bat_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct sec_bat_info *info;
	int ret = 0;

	pr_info("%s: SEC Battery Driver Loading\n", __func__);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	platform_set_drvdata(pdev, info);

	info->dev = &pdev->dev;
	if (!pdata->fuel_gauge_name || !pdata->charger_name) {
		dev_err(info->dev, "%s: no fuel gauge or charger name\n",
			__func__);
		goto err_kfree;
	}
	pr_info("[battery] enable Vref_batt_therm\n");
	pm8921_enable_batt_therm(1);

	info->fuel_gauge_name = pdata->fuel_gauge_name;
	info->charger_name = pdata->charger_name;
	info->hw_rev = system_rev;
	info->present = BATT_STATUS_UNKNOWN;
	info->current_avg = -1;

	info->psy_bat.name = "battery",
	info->psy_bat.type = POWER_SUPPLY_TYPE_BATTERY,
	info->psy_bat.properties = sec_battery_props,
	info->psy_bat.num_properties = ARRAY_SIZE(sec_battery_props),
	info->psy_bat.get_property = sec_bat_get_property,
	info->psy_bat.set_property = sec_bat_set_property,
	info->psy_usb.name = "usb",
	info->psy_usb.type = POWER_SUPPLY_TYPE_USB,
	info->psy_usb.supplied_to = supply_list,
	info->psy_usb.num_supplicants = ARRAY_SIZE(supply_list),
	info->psy_usb.properties = sec_power_props,
	info->psy_usb.num_properties = ARRAY_SIZE(sec_power_props),
	info->psy_usb.get_property = sec_usb_get_property,
	info->psy_ac.name = "ac",
	info->psy_ac.type = POWER_SUPPLY_TYPE_MAINS,
	info->psy_ac.supplied_to = supply_list,
	info->psy_ac.num_supplicants = ARRAY_SIZE(supply_list),
	info->psy_ac.properties = sec_power_props,
	info->psy_ac.num_properties = ARRAY_SIZE(sec_power_props),
	info->psy_ac.get_property = sec_ac_get_property;

	wake_lock_init(&info->vbus_wake_lock, WAKE_LOCK_SUSPEND,
		       "vbus_present");
	wake_lock_init(&info->monitor_wake_lock, WAKE_LOCK_SUSPEND,
		       "sec-battery-monitor");
	wake_lock_init(&info->cable_wake_lock, WAKE_LOCK_SUSPEND,
		       "sec-battery-cable");
	wake_lock_init(&info->test_wake_lock, WAKE_LOCK_SUSPEND,
		       "bat_esus_test");
	wake_lock_init(&info->measure_wake_lock, WAKE_LOCK_SUSPEND,
		       "sec-battery-measure");

	info->batt_health = POWER_SUPPLY_HEALTH_GOOD;
	info->charging_status = sec_bat_is_charging(info);
	pr_info("%s: initial charging_status = %d\n", __func__,
		info->charging_status);

	if (info->charging_status == POWER_SUPPLY_STATUS_CHARGING) {
		info->cable_type = CABLE_TYPE_UNKNOWN;
		info->prev_cable = CABLE_TYPE_NONE;
	} else {
		info->charging_status = POWER_SUPPLY_STATUS_DISCHARGING;
		info->cable_type = CABLE_TYPE_NONE;
		info->prev_cable = CABLE_TYPE_NONE;
	}

	info->batt_vcell = sec_bat_get_fuelgauge_data(info, FG_T_VCELL);
	info->batt_soc = sec_bat_get_fuelgauge_data(info, FG_T_SOC);
	info->batt_temp = RCOMP0_TEMP * 10;
	info->recharging_status = false;
	info->is_timeout_chgstop = false;
	info->is_esus_state = false;
	info->is_rechg_triggered = false;
	info->dcin_intr_triggered = false;
#if defined(CONFIG_TARGET_LOCALE_USA)
	info->ui_full_charge_status = false;
#endif
	info->initial_check_count = INIT_CHECK_COUNT;

	info->charging_start_time = 0;
	info->slate_mode = false;
	info->factory_mode = false;
	info->check_full_state = false;
	info->check_full_state_cnt = 0;

#ifdef CONFIG_WIRELESS_CHARGING
	info->wpc_charging_current = pdata->wpc_charging_current;
#endif /*CONFIG_WIRELESS_CHARGING*/

	if (poweroff_charging)
		info->lpm_chg_mode = true;
	else
		info->lpm_chg_mode = false;

	if ((poweroff_charging) && (pdata->lpm_high_block != 0)) {
		info->tspec.high_block = pdata->lpm_high_block;
		info->tspec.high_recovery = pdata->lpm_high_recovery;
		info->tspec.low_block = pdata->lpm_low_block;
		info->tspec.low_recovery = pdata->lpm_low_recovery;
	} else {
		if (pdata->high_block != 0) {
			info->tspec.event_block = pdata->event_block;
			info->tspec.high_block = pdata->high_block;
			info->tspec.high_recovery = pdata->high_recovery;
			info->tspec.low_block = pdata->low_block;
			info->tspec.low_recovery = pdata->low_recovery;
		} else {
			info->tspec.event_block = DEFAULT_HIGH_BLOCK_TEMP;
			info->tspec.high_block = DEFAULT_HIGH_BLOCK_TEMP;
			info->tspec.high_recovery = DEFAULT_HIGH_RECOVER_TEMP;
			info->tspec.low_block = DEFAULT_LOW_BLOCK_TEMP;
			info->tspec.low_recovery = DEFAULT_LOW_RECOVER_TEMP;
		}
	}

	if (pdata->high_recovery_wpc)
		info->high_recovery_wpc = pdata->high_recovery_wpc;
	else
		info->high_recovery_wpc = 0;

	if (info->charging_status == POWER_SUPPLY_STATUS_CHARGING)
		info->measure_interval = MEASURE_CHG_INTERVAL;
	else
		info->measure_interval = MEASURE_DSG_INTERVAL;

	if (pdata->max_voltage != 0) {
		info->vmax = pdata->max_voltage;
		info->full_cond_count = FULL_CHG_COND_COUNT;
		info->full_cond_voltage =
			pdata->max_voltage - 100000;
	} else {
		info->vmax = DEFAULT_MAX_VOLTAGE;
		info->full_cond_count = FULL_CHG_COND_COUNT;
		info->full_cond_voltage = FULL_CHARGE_COND_VOLTAGE;
	}

	if (pdata->recharge_voltage != 0)
		info->vrechg = pdata->recharge_voltage;
	else
		info->vrechg = DEFAULT_RECHG_VOLTAGE;

	if (pdata->charge_duration != 0 &&
		pdata->recharge_duration != 0) {
		info->abs_time = pdata->charge_duration;
		info->normal_abs_time = pdata->charge_duration;
		info->rechg_time = pdata->recharge_duration;
	} else {
		info->abs_time = DEFAULT_CHG_TIME;
		info->normal_abs_time = DEFAULT_CHG_TIME;
		info->rechg_time = DEFAULT_RECHG_TIME;
	}

	if (pdata->wpc_charge_duration != 0)
		info->wpc_abs_time = pdata->wpc_charge_duration;
	else
		info->wpc_abs_time = 0;

	if (pdata->iterm != 0)
		info->iterm = pdata->iterm;
	else
		info->iterm = DEFAULT_TERMINATION_CURRENT;

	ret = power_supply_register(&pdev->dev, &info->psy_bat);
	if (ret) {
		dev_err(info->dev, "%s: failed to register psy_bat\n",
			__func__);
		goto err_wakelock_free;
	}

	ret = power_supply_register(&pdev->dev, &info->psy_usb);
	if (ret) {
		dev_err(info->dev, "%s: failed to register psy_usb\n",
			__func__);
		goto err_supply_unreg_bat;
	}

	ret = power_supply_register(&pdev->dev, &info->psy_ac);
	if (ret) {
		dev_err(info->dev, "%s: failed to register psy_ac\n", __func__);
		goto err_supply_unreg_usb;
	}

	/* create sec detail attributes */
	ret = sec_bat_create_attrs(info->psy_bat.dev);
	if (ret) {
		dev_err(info->dev, "%s: failed to create attrs\n",
			__func__);
		goto err_supply_unreg_ac;
	}

	info->entry = create_proc_entry("batt_info_proc", S_IRUGO, NULL);
	if (!info->entry)
		dev_err(info->dev, "%s: failed to create proc_entry\n",
			__func__);
	else {
		info->entry->read_proc = sec_bat_read_proc;
		info->entry->data = (struct sec_bat_info *)info;
	}

	info->monitor_wqueue = create_freezable_workqueue(dev_name(&pdev->dev));
	if (!info->monitor_wqueue) {
		dev_err(info->dev, "%s: fail to create workqueue\n", __func__);
		goto err_supply_unreg_ac;
	}

#if defined(CONFIG_BATTERY_CTIA)
	hrtimer_init(&info->event_timer,
			CLOCK_BOOTTIME,
			HRTIMER_MODE_ABS);
	info->event_timer.function = &sec_bat_use_timer_func;
#endif /*CONFIG_BATTERY_CTIA*/
	hrtimer_init(&info->hrtimer,
			CLOCK_BOOTTIME,
			HRTIMER_MODE_ABS);
	info->hrtimer.function = &sec_bat_monitor_queue;

	if (pdata->batt_int != 0) {
		info->batt_int = pdata->batt_int;
		info->batt_int_irq = gpio_to_irq(pdata->batt_int);
		ret = enable_irq_wake(info->batt_int_irq);
		if (ret) {
			pr_err("%s : Failed to enable_irq_wake[%d]\r\n",
				__func__, info->batt_int_irq);
		}

/*		ret = request_threaded_irq(
			info->batt_int_irq, NULL,
			batt_removal_handler,
			(IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING),
			"battery removal", info);

		if (ret) {
			pr_err("%s : Failed to request batt_int irq\n",
				__func__);
			goto err_request_irq;
		}
		disable_irq(info->batt_int_irq);
*/
	}

	info->bat_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	info->bat_early_suspend.suspend = sec_bat_early_suspend;
	info->bat_early_suspend.resume = sec_bat_late_resume;
	register_early_suspend(&info->bat_early_suspend);

	INIT_WORK(&info->monitor_work, sec_bat_monitor_work);
	INIT_DELAYED_WORK_DEFERRABLE(&info->cable_work, sec_bat_cable_work);

	INIT_DELAYED_WORK_DEFERRABLE(&info->measure_work, sec_bat_measure_work);

	INIT_DELAYED_WORK_DEFERRABLE(&info->otg_work, sec_otg_work);

	pdata->get_cable_type();

	queue_delayed_work(info->monitor_wqueue, &info->measure_work, 0);
	queue_work(info->monitor_wqueue, &info->monitor_work);

	return 0;
#if 0
err_request_irq:
	destroy_workqueue(info->monitor_wqueue);
#endif
err_supply_unreg_ac:
	power_supply_unregister(&info->psy_ac);
err_supply_unreg_usb:
	power_supply_unregister(&info->psy_usb);
err_supply_unreg_bat:
	power_supply_unregister(&info->psy_bat);
err_wakelock_free:
	wake_lock_destroy(&info->vbus_wake_lock);
	wake_lock_destroy(&info->monitor_wake_lock);
	wake_lock_destroy(&info->cable_wake_lock);
	wake_lock_destroy(&info->test_wake_lock);
	wake_lock_destroy(&info->measure_wake_lock);

err_kfree:
	kfree(info);

	return ret;
}

static int __devexit sec_bat_remove(struct platform_device *pdev)
{
	struct sec_bat_info *info = platform_get_drvdata(pdev);

	remove_proc_entry("batt_info_proc", NULL);

	flush_workqueue(info->monitor_wqueue);
	destroy_workqueue(info->monitor_wqueue);

	cancel_delayed_work(&info->cable_work);
	cancel_delayed_work(&info->measure_work);
	cancel_delayed_work(&info->otg_work);

	power_supply_unregister(&info->psy_bat);
	power_supply_unregister(&info->psy_usb);
	power_supply_unregister(&info->psy_ac);

	wake_lock_destroy(&info->vbus_wake_lock);
	wake_lock_destroy(&info->monitor_wake_lock);
	wake_lock_destroy(&info->cable_wake_lock);
	wake_lock_destroy(&info->test_wake_lock);
	wake_lock_destroy(&info->measure_wake_lock);

#if defined(CONFIG_BATTERY_CTIA)
	hrtimer_cancel(&info->event_timer);
#endif /*CONFIG_BATTERY_CTIA*/
	hrtimer_cancel(&info->hrtimer);
	kfree(info);

	return 0;
}

static int sec_bat_suspend(struct device *dev)
{
	struct sec_bat_info *info = dev_get_drvdata(dev);
	pr_info("[BATT] battery suspend!##\n");
	cancel_work_sync(&info->monitor_work);
	cancel_delayed_work(&info->cable_work);
	cancel_delayed_work(&info->measure_work);
	cancel_delayed_work(&info->otg_work);
	/*add alarm monitoring */
	info->cur_monitor_time = ktime_get_boottime();

	if (info->cable_type == CABLE_TYPE_NONE) {
		sec_bat_monitoring_alarm(info, ALARM_INTERVAL);
		info->slow_polling = 1;
	} else {
		sec_bat_monitoring_alarm(info, CHARGING_ALARM_INTERVAL);
	}

	return 0;
}

static void sec_bat_resume(struct device *dev)
{
	struct sec_bat_info *info = dev_get_drvdata(dev);
	pr_info("[BATT] battery resume!##\n");
	if (info->slow_polling)
		info->slow_polling = 0;

	queue_delayed_work(info->monitor_wqueue, &info->measure_work, 0);

	queue_work(info->monitor_wqueue, &info->monitor_work);
}

static const struct dev_pm_ops sec_bat_pm_ops = {
	.prepare = sec_bat_suspend,
	.complete = sec_bat_resume,
};

static struct platform_driver sec_bat_driver = {
	.driver = {
		   .name = "sec-battery",
		   .owner = THIS_MODULE,
		   .pm = &sec_bat_pm_ops,
		   },
	.probe = sec_bat_probe,
	.remove = __devexit_p(sec_bat_remove),
};

static int __init sec_bat_init(void)
{
	return platform_driver_register(&sec_bat_driver);
}

static void __exit sec_bat_exit(void)
{
	platform_driver_unregister(&sec_bat_driver);
}

late_initcall(sec_bat_init);
module_exit(sec_bat_exit);

MODULE_DESCRIPTION("SEC battery driver");
MODULE_AUTHOR("<jongmyeong.ko@samsung.com>");
MODULE_AUTHOR("<ms925.kim@samsung.com>");
MODULE_AUTHOR("<joshua.chang@samsung.com>");
MODULE_LICENSE("GPL");
