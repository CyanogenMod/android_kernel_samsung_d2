/*
 * Copyright (C) 2010 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_SEC_BATTERY_H
#define __MACH_SEC_BATTERY_H __FILE__

extern int poweroff_charging;

struct sec_bat_adc_table_data {
	int adc;
	int temperature;
};

struct sec_bat_platform_data {
	char *fuel_gauge_name;
	char *charger_name;
	int (*get_cable_type) (void);
	unsigned int iterm;
	unsigned int charge_duration;
	unsigned int wpc_charge_duration;
	unsigned int recharge_duration;
	unsigned int max_voltage;
	unsigned int recharge_voltage;
	int event_block;
	int high_block;
	int high_recovery;
	int high_recovery_wpc;
	int low_block;
	int low_recovery;
	int lpm_high_block;
	int lpm_high_recovery;
	int lpm_low_block;
	int lpm_low_recovery;
	int batt_int;
	int wpc_charging_current;
};

#define ADJUST_RCOMP_WITH_CHARGING_STATUS
#define ADJUST_RCOMP_WITH_TEMPER

#define POLLING_INTERVAL	(40 * 1000)
#define MEASURE_DSG_INTERVAL	(30 * 1000)
#define MEASURE_CHG_INTERVAL	(30 * 1000)
#define ALARM_INTERVAL		(5 * 60)
#define CHARGING_ALARM_INTERVAL	(40)

#define CURRENT_OF_FULL_CHG_UI		1800	/* 180mA */
#define CURRENT_OF_FULL_CHG		1800	/* 180mA */
#define RCOMP0_TEMP			20	/* 'C */

#define DEFAULT_CHG_TIME	(6 * 60 * 60)	/* 6hr */
#define DEFAULT_RECHG_TIME	(2 * 60 * 60)	/* 2hr */
#define DEFAULT_RECHG_VOLTAGE	(4130 * 1000)	/* 4.13 V */
#define DEFAULT_MAX_VOLTAGE	(4200 * 1000)	/* 4.2 V */
#define DEFAULT_TERMINATION_CURRENT	200

/* all count duration = (count - 1) * poll interval */
#define RE_CHG_COND_COUNT		4
#define TEMP_BLOCK_COUNT		2
#define BAT_DET_COUNT		2
#define FULL_CHG_COND_COUNT		2
#define FULL_CHARGE_COND_VOLTAGE    4000000
#define INIT_CHECK_COUNT	4

#define MAX_BATT_ADC_CHAN 3

#define TEMP_GPIO	PM8XXX_AMUX_MPP_7
#define TEMP_ADC_CHNNEL		ADC_MPP_1_AMUX6

#define BATT_TYPE_NORMAL		0 /* 4.2V battery */
#define BATT_TYPE_JAGUAR		1 /* 4.35V, new active battery */
#define BATT_TYPE_D2_HIGH		2 /* 4.35V battery */
#define BATT_TYPE_D2_ACTIVE		3 /* 4.35V, new active battery */
#define BATT_TYPE_AEGIS2		4 /* 4.35V, new active battery */

#endif /* __MACH_SEC_BATTERY_H */
