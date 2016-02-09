/*
 *  linux/drivers/cpufreq/cpufreq_performance.c
 *
 *  Copyright (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/init.h>

#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/hotplug_mgmt.h>
#include <linux/atomic.h>

static void __ref do_cpu_up(struct work_struct *work) {
	int j;
	for_each_possible_cpu(j) {
		if (!cpu_online(j))
			cpu_up(j);
	}
}
static DECLARE_DELAYED_WORK(cpu_up_work, do_cpu_up);

static struct hotplug_alg perfgov_alg = {
	.name = "performance",
	.prio = HP_ALG_CPUFREQ,
};
static atomic_t alg_count;

static int cpufreq_governor_performance(struct cpufreq_policy *policy,
					unsigned int event)
{
	switch (event) {
	case CPUFREQ_GOV_START:
		// Wait for apps to finish toggling CPUs
		schedule_delayed_work(&cpu_up_work, 5 * HZ);
		atomic_inc(&alg_count);
	case CPUFREQ_GOV_LIMITS:
		pr_debug("setting to %u kHz because of event %u\n",
						policy->max, event);
		__cpufreq_driver_target(policy, policy->max,
						CPUFREQ_RELATION_H);
		break;
	case CPUFREQ_GOV_STOP:
		atomic_dec(&alg_count);
		break;
	default:
		break;
	}
	hotplug_alg_available(&perfgov_alg, !!atomic_read(&alg_count));
	return 0;
}

#ifdef CONFIG_CPU_FREQ_GOV_PERFORMANCE_MODULE
static
#endif
struct cpufreq_governor cpufreq_gov_performance = {
	.name		= "performance",
	.governor	= cpufreq_governor_performance,
	.owner		= THIS_MODULE,
};


static int __init cpufreq_gov_performance_init(void)
{
	hotplug_register_alg(&perfgov_alg);
	return cpufreq_register_governor(&cpufreq_gov_performance);
}


static void __exit cpufreq_gov_performance_exit(void)
{
	hotplug_unregister_alg(&perfgov_alg);
	cpufreq_unregister_governor(&cpufreq_gov_performance);
}


MODULE_AUTHOR("Dominik Brodowski <linux@brodo.de>");
MODULE_DESCRIPTION("CPUfreq policy governor 'performance'");
MODULE_LICENSE("GPL");

fs_initcall(cpufreq_gov_performance_init);
module_exit(cpufreq_gov_performance_exit);
