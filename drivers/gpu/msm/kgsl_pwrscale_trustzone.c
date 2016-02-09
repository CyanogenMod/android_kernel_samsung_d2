/* Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.
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
 * Modified by Paul Reioux (Faux123)
 * 2013-06-20: Added KGSL Simple GPU Governor
 *
 */

#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <mach/socinfo.h>
#include <mach/scm.h>

#include "kgsl.h"
#include "kgsl_pwrscale.h"
#include "kgsl_device.h"

#define TZ_GOVERNOR_PERFORMANCE 0
#define TZ_GOVERNOR_ONDEMAND    1
#define TZ_GOVERNOR_SIMPLE	2
#define TZ_GOVERNOR_TIERED	3

struct tz_priv {
	int governor;
	unsigned int skip_mask;
	unsigned int skip_cnt;
	struct kgsl_power_stats bin;
#ifdef CONFIG_MSM_KGSL_TIERED_GOV
	ktime_t tiered_next_up;
	ktime_t tiered_next_down;
#endif
};
static spinlock_t tz_lock;

#ifdef CONFIG_MSM_KGSL_SIMPLE_GOV
int simple_gpu_active;

static int saved_governor = TZ_GOVERNOR_ONDEMAND;
static struct kgsl_device *saved_gpu_dev;
static struct kgsl_pwrscale *saved_gpu_pwrscale;
#endif


/* FLOOR is 5msec to capture up to 3 re-draws
 * per frame for 60fps content.
 */
#define FLOOR			15000
#define SIMPLE_FLOOR		15000
/* CEILING is 50msec, larger than any standard
 * frame length, but less than the idle timer.
 */
#define CEILING			50000
#define SWITCH_OFF		200
#define SWITCH_OFF_RESET_TH	40
#define SKIP_COUNTER		500
#define TZ_RESET_ID		0x3
#define TZ_UPDATE_ID		0x4

#ifdef CONFIG_MSM_SCM
/* Trap into the TrustZone, and call funcs there. */
static int __secure_tz_entry(u32 cmd, u32 val, u32 id)
{
	int ret;
	spin_lock(&tz_lock);
	__iowmb();
	ret = scm_call_atomic2(SCM_SVC_IO, cmd, val, id);
	spin_unlock(&tz_lock);
	return ret;
}
#else
static int __secure_tz_entry(u32 cmd, u32 val, u32 id)
{
	return 0;
}
#endif /* CONFIG_MSM_SCM */

static ssize_t tz_governor_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	struct tz_priv *priv = pwrscale->priv;
	int ret;

	if (priv->governor == TZ_GOVERNOR_ONDEMAND)
		ret = snprintf(buf, 10, "ondemand\n");
#ifdef CONFIG_MSM_KGSL_SIMPLE_GOV
	else if (priv->governor == TZ_GOVERNOR_SIMPLE)
		ret = snprintf(buf, 8, "simple\n");
#endif
#ifdef CONFIG_MSM_KGSL_TIERED_GOV
	else if (priv->governor == TZ_GOVERNOR_TIERED)
		ret = snprintf(buf, 13, "interactive\n");
#endif
	else
		ret = snprintf(buf, 13, "performance\n");

	return ret;
}

static ssize_t tz_governor_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				 const char *buf, size_t count)
{
	struct tz_priv *priv = pwrscale->priv;
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;

	mutex_lock(&device->mutex);

	if (!strncmp(buf, "ondemand", 8))
		priv->governor = TZ_GOVERNOR_ONDEMAND;
#ifdef CONFIG_MSM_KGSL_SIMPLE_GOV
	else if (!strncmp(buf, "simple", 6))
		priv->governor = TZ_GOVERNOR_SIMPLE;
#endif
#ifdef CONFIG_MSM_KGSL_TIERED_GOV
	else if (!strncmp(buf, "interactive", 11)) {
		priv->governor = TZ_GOVERNOR_TIERED;
		priv->tiered_next_up = priv->tiered_next_down = ktime_get();
	}
#endif
	else if (!strncmp(buf, "performance", 11))
		priv->governor = TZ_GOVERNOR_PERFORMANCE;

	if (priv->governor == TZ_GOVERNOR_PERFORMANCE)
		kgsl_pwrctrl_pwrlevel_change(device, pwr->max_pwrlevel);

#ifdef CONFIG_MSM_KGSL_SIMPLE_GOV
	if (priv->governor == TZ_GOVERNOR_SIMPLE) {
		simple_gpu_active = 1;
	} else {
		saved_governor = priv->governor;
		simple_gpu_active = 0;
	}
#endif

	mutex_unlock(&device->mutex);
	return count;
}

PWRSCALE_POLICY_ATTR(governor, 0644, tz_governor_show, tz_governor_store);

static struct attribute *tz_attrs[] = {
	&policy_attr_governor.attr,
	NULL
};

static struct attribute_group tz_attr_group = {
	.attrs = tz_attrs,
};

static void tz_wake(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct tz_priv *priv = pwrscale->priv;
	if (device->state != KGSL_STATE_NAP &&
	    priv->governor != TZ_GOVERNOR_PERFORMANCE) {
		kgsl_pwrctrl_pwrlevel_change(device,
					device->pwrctrl.default_pwrlevel);
#ifdef CONFIG_MSM_KGSL_TIERED_GOV
		if (priv->governor == TZ_GOVERNOR_TIERED) {
			priv->tiered_next_up =
				priv->tiered_next_down = ktime_get();
		}
#endif
	}
}

#ifdef CONFIG_MSM_KGSL_SIMPLE_GOV
int simple_gpu_default_laziness __read_mostly = 3;
module_param_named(simple_laziness, simple_gpu_default_laziness, int, 0664);

int simple_gpu_ramp_up_threshold __read_mostly = 6000;
module_param_named(simple_ramp_threshold, simple_gpu_ramp_up_threshold, int, 0664);

static int laziness;

static int simple_governor(struct kgsl_device *device, int idle_stat)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;

	if (idle_stat < simple_gpu_ramp_up_threshold)
	{
		if (laziness > -simple_gpu_default_laziness / 2)
			laziness--;

		if (pwr->active_pwrlevel <= pwr->thermal_pwrlevel)
			return 0;

		if (laziness <= -simple_gpu_default_laziness / 2) {
			laziness = 0;
			return -1;
		}
	} else {
		if (laziness < simple_gpu_default_laziness)
			laziness++;

		if (pwr->active_pwrlevel >= pwr->num_pwrlevels - 1)
			return 0;

		if (laziness >= simple_gpu_default_laziness) {
			laziness = 0;
			return 1;
		}
	}

	return 0;
}

void simple_gpu_activate(void)
{
	struct tz_priv *priv;

	if (!saved_gpu_dev)
		return;
	if (!saved_gpu_pwrscale || !saved_gpu_pwrscale->priv)
		return;
	priv = saved_gpu_pwrscale->priv;

	mutex_lock(&saved_gpu_dev->mutex);
	if (simple_gpu_active)
		priv->governor = TZ_GOVERNOR_SIMPLE;
	else
		priv->governor = saved_governor;
	mutex_unlock(&saved_gpu_dev->mutex);
}
#endif

#ifdef CONFIG_MSM_KGSL_TIERED_GOV
/* tiered GPU governor: frequency control based on utilization ranges
 *
 * Each iteration, tiered checks whether the current frequency is still
 * optimial for the current workload.  If an adjustment must be made, a
 * frequency with appropriate utilization limits is selected.  A small time
 * limit is imposed to prevent unnecessary frequency changes, which are
 * somewhat expensive.
 *
 * The efficiency_mode parameter is a catch-all for small changes that attempt
 * to use lower frequencies more often.
 */

/**
 * struct freq_tier:
 * @mhz: GPU frequency for this power level
 * @min_util_mhz: minimum utilization this level may use
 * @max_util_mhz: maximum utilization this level may use
 * @down_delay_us: minimum time before reducing frequency; also sampling period
 * @up_delay_us: minimum time before increasing frequency
 * @period_len_us: period time for accumulated stats
 *
 * Defines the parameters for each GPU power level.  Some utilization overlap
 * between levels is encouraged to prevent excessive frequency changes.
 *
 * NB: ARRAY_SIZE(freq_tier_table) == pwr->num_pwrlevels - 1
 * NB: up_delay_us < down_delay_us
 */
struct freq_tier {
	unsigned long	mhz;
	unsigned long	min_util_mhz;
	unsigned long	max_util_mhz;
	unsigned long	down_delay_us;
	unsigned long	up_delay_us;
	unsigned long	period_len_us;
};
/* FIXME: this should be customizable */
static const struct freq_tier freq_tier_table[] = {
	{ 600,	433,	1000,	100000,	100000,	50000 },
	{ 533,	333,	467,	100000,	67000,	50000 },
	{ 480,	267,	367,	100000,	67000,	50000 },
	{ 400,	200,	300,	67000,	50000,	33000 },
	{ 300,	100,	233,	50000,	17000,	17000 },
	{ 200,	33,	133,	33000,	17000,	8300 },
	{ 128,	0,	67,	17000,	8300,	5600 },
};

/* efficiency_mode:
 * If enabled, makes subtle changes to prefer lower frequencies:
 *  - Searches for lowest appropriate frequencies first
 *  - Attempts to periodically lower frequency
 *  - Clear stats when lowering GPU frequency
 */
static bool efficiency_mode = 0;
module_param_named(tiered_efficiency_mode, efficiency_mode, bool, 0644);

static bool tiered_logspam = 0;
module_param(tiered_logspam, bool, 0644);

static bool always_clear = 1;
module_param_named(tiered_always_clear, always_clear, bool, 0644);

static unsigned long sample_floor = 17000;
module_param_named(tiered_sample_time, sample_floor, ulong, 0644);

static inline bool pwrlevel_suitable(s64 mhz_util,
				     const struct freq_tier *ramp)
{
	return mhz_util >= ramp->min_util_mhz && mhz_util <= ramp->max_util_mhz;
}

static void tiered_idle(struct kgsl_device *dev, struct kgsl_pwrscale *pwrs)
{
	struct kgsl_pwrctrl *pwr = &dev->pwrctrl;
	struct tz_priv *priv = pwrs->priv;
	struct kgsl_power_stats stats;
	const struct freq_tier *ramp;
	int start, delta = 1, end;
	s64 mhz_util;
	ktime_t now;

	dev->ftbl->power_stats(dev, &stats);
	priv->bin.total_time += stats.total_time;
	priv->bin.busy_time += stats.busy_time;
	ramp = &freq_tier_table[pwr->active_pwrlevel];

	/* Don't bother running unless we can make a frequency adjustment and
	 * it's been at least sample_floor usecs since the last run.
	 */
	if (!priv->bin.total_time)
		return;

	now = ktime_get();
	if (ktime_compare(now, priv->tiered_next_up) < 0)
		return;
	priv->tiered_next_up = ktime_add_us(now,
		min_t(unsigned long, sample_floor, ramp->up_delay_us));

	if (pwr->active_pwrlevel == pwr->thermal_pwrlevel &&
	    ktime_compare(now, priv->tiered_next_down) < 0)
		return;

	mhz_util = ramp->mhz * priv->bin.busy_time;
	do_div(mhz_util, priv->bin.total_time);

	if (unlikely(tiered_logspam))
		pr_warn("%s: %lu*%lli/%lli=%lli; d%i\n", __func__, ramp->mhz,
			priv->bin.busy_time, priv->bin.total_time, mhz_util,
			ktime_compare(now, priv->tiered_next_down) >= 0);

	/* Determine which pwrlevels we can search based on ramp delays,
	 * current utilization, and efficiency_mode setting.  With
	 * efficiency_mode, run the search ascending (frequency) rather than
	 * descending.
	 */
	if (mhz_util > ramp->max_util_mhz) {
		if (pwr->active_pwrlevel <= pwr->thermal_pwrlevel)
			goto reset_stats;

		start = pwr->thermal_pwrlevel;
		end = pwr->active_pwrlevel - 1;
	} else if ((mhz_util < ramp->min_util_mhz || efficiency_mode) &&
	           ktime_compare(now, priv->tiered_next_down) >= 0) {
		if (pwr->active_pwrlevel == pwr->num_pwrlevels - 1)
			goto reset_stats;

		start = pwr->active_pwrlevel + !efficiency_mode;
		end = pwr->num_pwrlevels - 1;
	} else {
		goto reset_stats;
	}

	if (efficiency_mode) {
		int tmp = start;
		start = end;
		end = tmp;
		delta = -1;
	}

	while (start != end &&
	       !pwrlevel_suitable(mhz_util, &freq_tier_table[start]))
		start += delta;

	/* During an efficiency_mode periodic sample, the current pwrlevel may
	 * already be optimal.
	 */
	if (start == pwr->active_pwrlevel)
		goto reset_stats;

	/*
	pr_warn("%s: scaling %lu -> %lu (%lli/%lli = %lli)\n", __func__,
		ramp->mhz, freq_tier_table[start].mhz,
		priv->bin.busy_time, priv->bin.total_time, mhz_util);
	*/

	/* Clear the stats when increasing frequency (decreasing with
	 * efficiency_mode) in an attempt to perform subsequent increases
	 * sooner.  Otherwise, scale stats to the new frequency.
	 *
	 * TODO: Just clear instead?
	 */
	if (always_clear ||
	    (efficiency_mode == (start > pwr->active_pwrlevel))) {
		priv->bin.total_time =
			priv->bin.busy_time = 0;
	} else {
		priv->bin.busy_time *= ramp->mhz;
		do_div(priv->bin.busy_time, freq_tier_table[start].mhz);
	}

	/* Calculate next allowed adjustment times
	 */
	ramp = &freq_tier_table[start];
	priv->tiered_next_up = ktime_add_us(now, ramp->up_delay_us);
	priv->tiered_next_down = ktime_add_us(now, ramp->down_delay_us);

	kgsl_pwrctrl_pwrlevel_change(dev, start);

reset_stats:
	/* Scale stats back to the current tier's period length
	 * TODO: Is this optimal?  Clear instead?  Fixed scale?
	 */
	if (priv->bin.total_time > ramp->period_len_us) {
		// <3 int64
		priv->bin.busy_time *= ramp->period_len_us;
		do_div(priv->bin.busy_time, priv->bin.total_time);
		priv->bin.total_time = ramp->period_len_us;
	}
}
#endif

static int idle_scale = 16667;
module_param(idle_scale, int, 0664);

static void tz_idle(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct tz_priv *priv = pwrscale->priv;
	struct kgsl_power_stats stats;
	int val, idle;

	/* In "performance" mode the clock speed always stays
	   the same */
	if (priv->governor == TZ_GOVERNOR_PERFORMANCE)
		return;
#ifdef CONFIG_MSM_KGSL_TIERED_GOV
	else if (priv->governor == TZ_GOVERNOR_TIERED) {
		tiered_idle(device, pwrscale);
		return;
	}
#endif

	device->ftbl->power_stats(device, &stats);
	priv->bin.total_time += stats.total_time;
	priv->bin.busy_time += stats.busy_time;
	/* Do not waste CPU cycles running this algorithm if
	 * the GPU just started, or if less than FLOOR time
	 * has passed since the last run.
	 */
	if (stats.total_time == 0 || priv->bin.total_time <
	    (priv->governor == TZ_GOVERNOR_SIMPLE ? SIMPLE_FLOOR : FLOOR))
		return;
	/* Skip progressively more samples as the frequency stays stable for
	 * long periods.  For non-maximum frequencies, sample more frequently
	 * to allow faster response to demand.
	 */
	if (++priv->skip_cnt & priv->skip_mask)
		return;
	if (priv->skip_cnt > 31 && priv->skip_mask <
	    ((pwr->active_pwrlevel == pwr->thermal_pwrlevel) ? 31 : 7))
		priv->skip_mask = (priv->skip_mask << 1) + 1;

	idle = priv->bin.total_time - priv->bin.busy_time;
	idle = (idle > 0) ? idle : 0;
	idle *= idle_scale;
	do_div(idle, priv->bin.total_time);
	priv->bin.total_time = 0;
	priv->bin.busy_time = 0;
#ifdef CONFIG_MSM_KGSL_SIMPLE_GOV
	if (priv->governor == TZ_GOVERNOR_SIMPLE)
		val = simple_governor(device, idle);
	else
		val = __secure_tz_entry(TZ_UPDATE_ID, idle, device->id);
#else
	val = __secure_tz_entry(TZ_UPDATE_ID, idle, device->id);
#endif
	if (val) {
		kgsl_pwrctrl_pwrlevel_change(device,
					     pwr->active_pwrlevel + val);
		//pr_info("TZ idle stat: %d, TZ PL: %d, TZ out: %d\n",
		//		idle, pwr->active_pwrlevel, val);
		priv->skip_cnt = 0;
		priv->skip_mask = 0;
	}
}

static void tz_busy(struct kgsl_device *device,
	struct kgsl_pwrscale *pwrscale)
{
	device->on_time = ktime_to_us(ktime_get());
}

static void tz_sleep(struct kgsl_device *device,
	struct kgsl_pwrscale *pwrscale)
{
	struct tz_priv *priv = pwrscale->priv;

	__secure_tz_entry(TZ_RESET_ID, 0, device->id);
	priv->skip_cnt = 0;
	priv->skip_mask = 0;
	priv->bin.total_time = 0;
	priv->bin.busy_time = 0;
}

#ifdef CONFIG_MSM_SCM
static int tz_init(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct tz_priv *priv;

	priv = pwrscale->priv = kzalloc(sizeof(struct tz_priv), GFP_KERNEL);
	if (pwrscale->priv == NULL)
		return -ENOMEM;

	priv->governor = TZ_GOVERNOR_ONDEMAND;
	spin_lock_init(&tz_lock);
	kgsl_pwrscale_policy_add_files(device, pwrscale, &tz_attr_group);

#ifdef CONFIG_MSM_KGSL_SIMPLE_GOV
	saved_gpu_dev = device;
	saved_gpu_pwrscale = pwrscale;
#endif

	return 0;
}
#else
static int tz_init(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	return -EINVAL;
}
#endif /* CONFIG_MSM_SCM */

static void tz_close(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	kgsl_pwrscale_policy_remove_files(device, pwrscale, &tz_attr_group);
	kfree(pwrscale->priv);
	pwrscale->priv = NULL;
}

struct kgsl_pwrscale_policy kgsl_pwrscale_policy_tz = {
	.name = "trustzone",
	.init = tz_init,
	.busy = tz_busy,
	.idle = tz_idle,
	.sleep = tz_sleep,
	.wake = tz_wake,
	.close = tz_close
};
EXPORT_SYMBOL(kgsl_pwrscale_policy_tz);
