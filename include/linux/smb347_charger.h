/*
 *  Copyright (C) 2011 Samsung Electronics
 *  jongmyeong ko <jongmyeong.ko@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SMB347_CHARGER_H_
#define __SMB347_CHARGER_H_

struct smb347_platform_data {
	void (*hw_init) (void);
	int (*chg_intr_trigger) (int);
	int enable;
	int stat;
	unsigned int inok;
#ifdef CONFIG_WIRELESS_CHARGING
	void (*smb347_wpc_cb) (void);
#endif
	int (*smb347_get_cable) (void);
};

extern int poweroff_charging;

#endif
