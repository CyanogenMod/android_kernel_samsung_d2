/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/msm_tsens.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/msm_thermal.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <mach/cpufreq.h>

static unsigned int temp_threshold __read_mostly = 50;
module_param(temp_threshold, int, 0644);

static unsigned int limited_max_freq = UINT_MAX;
int limited_gpu_pwrlevel = 0;

static struct temp_limit {
	unsigned int thresh;
	unsigned int max_freq;
	int gpu_pwrlevel;
} temp_limits[] = {
	{ 12, 702000,  6 },
	{ 9,  918000,  5 },
	{ 5,  1026000, 4 },
	{ 0,  1242000, 3 },
	{ UINT_MAX, UINT_MAX, 0 },
};

static struct delayed_work check_temp_work;

static int msm_thermal_cpufreq_callback(struct notifier_block *nfb,
		unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	unsigned int max = min(limited_max_freq,
		max(policy->max, policy->user_policy.max));

	if (event != CPUFREQ_ADJUST)
		return 0;
	if (policy->max == max)
		return 0;

	cpufreq_verify_within_limits(policy, policy->min, max);

	return 0;
}

static struct notifier_block msm_thermal_cpufreq_notifier = {
	.notifier_call = msm_thermal_cpufreq_callback,
};

static void limit_cpu_freqs(int idx)
{
	struct temp_limit *tl = &temp_limits[idx];
	unsigned int cpu;

	if (limited_max_freq == tl->max_freq)
		return;

	limited_max_freq = tl->max_freq;
	limited_gpu_pwrlevel = tl->gpu_pwrlevel;

	get_online_cpus();
	for_each_online_cpu(cpu)
	{
		cpufreq_update_policy(cpu);
		pr_info("%s: Setting cpu%d max frequency to %d\n",
				KBUILD_MODNAME, cpu, limited_max_freq);
	}
	put_online_cpus();
}

static void check_temp(struct work_struct *work)
{
	struct tsens_device tsens_dev = { .sensor_num = 0 };
	unsigned long temp;
	int i;

	tsens_get_temp(&tsens_dev, &temp);

	for (i = 0; temp_limits[i].max_freq != UINT_MAX; i++) {
		if (temp >= temp_threshold + temp_limits[i].thresh)
			break;
	}

	limit_cpu_freqs(i);

	schedule_delayed_work_on(0, &check_temp_work, HZ);
}

int __init msm_thermal_init(struct msm_thermal_data *pdata)
{
	cpufreq_register_notifier(&msm_thermal_cpufreq_notifier,
			CPUFREQ_POLICY_NOTIFIER);

	INIT_DELAYED_WORK(&check_temp_work, check_temp);
	schedule_delayed_work_on(0, &check_temp_work, 0);

	return 0;
}
