/*
 * Copyright (c) 2012, Will Tisdale <willtisdale@gmail.com>. All rights reserved.
 * Copyright (c) 2015, Ryan Pennucci <decimalman@gmail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/*
 * auto_hotplug:
 * A simple, in-kernel hotplug algorithm.  A running load average is
 * maintained; this average determines when CPU cores are on/offlined.
 *
 * Cores will not be onlined while the screen is off.  All cores will be
 * onlined immediately when the screen is turned on.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/hotplug_mgmt.h>
#include <linux/dkp.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define MAX_SAMPLES	(16)

/*
 * We need a mutex to serialize hotplug operations.  hotplug_tick can't sleep,
 * so we need a spinlock as well.
 *
 * hotplug_mutex serializes cpu_up/down and num_online_cpus, and protects
 * target_cpus.  All other state is protected by hotplug_lock.
 */
static struct mutex hotplug_mutex;
static spinlock_t hotplug_lock;

#define HOTPLUG_DISABLED	(1 << 0)
#define HOTPLUG_NOONLINE	(1 << 1)
#define HOTPLUG_NOOFFLINE	(1 << 2)
static unsigned long hotplug_flags = 0;
static int target_cpus = 0;

struct work_struct hotplug_decision_work;
struct work_struct hotplug_hotplug_work;
struct delayed_work hotplug_allow_offline_work;

static unsigned int history[MAX_SAMPLES];
static unsigned int history_sum;
static unsigned int index;
static unsigned long next_decision; // jiffy when decision_fn should run next
static unsigned long next_offline; // when NOOFFLINE should expire

static void hotplug_enable(bool flag);
static void hotplug_tick(int cpu);

static struct hotplug_alg autohp_alg = {
	.name = "auto_hotplug",
	.prio = HP_ALG_KERNEL,
	.init_cb = hotplug_enable,
	.tick_cb = hotplug_tick,
};

static struct hotplug_alg fallback_alg = {
	.name = "auto_hotplug",
	.prio = HP_ALG_FALLBACK,
	.init_cb = hotplug_enable,
	.tick_cb = hotplug_tick,
};

static int enable_autohp = 0;
static void enable_autohp_cb(void)
{
	hotplug_alg_available(&autohp_alg, enable_autohp);
}

static int hotplug_sampling_periods = 6;
static void reset_history(void)
{
	unsigned long flags;

	spin_lock_irqsave(&hotplug_lock, flags);

	index = 0;
	history_sum = 0;
	memset(history, 0, sizeof(history));

	if (time_after(jiffies + hotplug_sampling_periods, next_decision))
		next_decision = jiffies + hotplug_sampling_periods;

	spin_unlock_irqrestore(&hotplug_lock, flags);
}

static int hotplug_sampling_rate = 4;
static int hotplug_enable_all_threshold = 300;
static int hotplug_enable_one_threshold = 200;
static int hotplug_disable_one_threshold = 67;
static __GATTR_NAME(enable_autohp,
	enable, 0, 1, enable_autohp_cb);
static __GATTR_NAME(hotplug_sampling_periods,
	sampling_periods, 2, MAX_SAMPLES, reset_history);
static __GATTR_NAME(hotplug_sampling_rate,
	sampling_rate, 1, 10, NULL);
static __GATTR_NAME(hotplug_enable_all_threshold,
	enable_all_threshold, 100, 1000, NULL);
static __GATTR_NAME(hotplug_enable_one_threshold,
	enable_one_threshold, 100, 1000, NULL);
static __GATTR_NAME(hotplug_disable_one_threshold,
	disable_one_threshold, 0, 1000, NULL);
static struct attribute *hotplug_attrs[] = {
	&gen_attr(enable),
	&gen_attr(sampling_periods),
	&gen_attr(sampling_rate),
	&gen_attr(enable_all_threshold),
	&gen_attr(enable_one_threshold),
	&gen_attr(disable_one_threshold),
	NULL
};
static struct attribute_group hotplug_attr_grp = {
	.attrs = hotplug_attrs,
};
static struct kobject *hotplug_kobject;

static void delay_offline(unsigned long delay)
{
	unsigned long timeout = jiffies + delay;

	hotplug_flags |= HOTPLUG_NOOFFLINE;
	if (time_before(next_offline, timeout))
		next_offline = timeout;
	if (!delayed_work_pending(&hotplug_allow_offline_work))
		schedule_delayed_work_on(0, &hotplug_allow_offline_work, delay);

	if (target_cpus == num_possible_cpus() && time_before(next_decision, timeout))
		next_decision = timeout;
}

static void hotplug_tick(int cpu)
{
	unsigned long flags;

	if (cpu)
		return;

	// Spinlock deliberately ignored
	if (hotplug_flags & HOTPLUG_DISABLED ||
	    time_is_after_jiffies(next_decision - hotplug_sampling_periods))
		return;

	spin_lock_irqsave(&hotplug_lock, flags);

	if (++index >= hotplug_sampling_periods)
		index = 0;
	history_sum -= history[index];
	history[index] = nr_running() * 100;
	history_sum += history[index];

	if (!time_is_after_jiffies(next_decision)) {
		next_decision += hotplug_sampling_rate;
		if (!(hotplug_flags & HOTPLUG_DISABLED))
			schedule_work_on(0, &hotplug_decision_work);
	}

	spin_unlock_irqrestore(&hotplug_lock, flags);
}

static void hotplug_decision_fn(struct work_struct *work)
{
	unsigned long flags;
	unsigned int disable_load, enable_load, avg_running = 0;
	unsigned int online_cpus, available_cpus;

	mutex_lock(&hotplug_mutex);

	online_cpus = num_online_cpus();
	available_cpus = num_possible_cpus();
	disable_load = hotplug_disable_one_threshold * online_cpus;
	enable_load = hotplug_enable_one_threshold * online_cpus;

	spin_lock_irqsave(&hotplug_lock, flags);

	avg_running = history_sum / hotplug_sampling_periods;

	if (!(hotplug_flags & HOTPLUG_NOONLINE)) {
		if (unlikely(avg_running >= hotplug_enable_all_threshold)) {
			target_cpus = available_cpus;
			delay_offline(HZ);
			goto out;
		}

		/*
		 * Online cores sooner: instead of waiting for the average to
		 * rise above the threshold, online cores as soon as they won't
		 * be immediately offlined.
		 */
		if (history[index] >= enable_load &&
		     avg_running >= disable_load + hotplug_disable_one_threshold) {
			target_cpus++;
			goto out;
		}
	} else {
		if (avg_running >= hotplug_enable_all_threshold) {
			target_cpus++;
			goto out;
		} else if (online_cpus == 1) {
			/*
			 * After earlysuspend, we wait for all cores to be
			 * offlined, then stop running.
			 */
			hotplug_flags |= HOTPLUG_DISABLED;
		}
	}

	if (!(hotplug_flags & HOTPLUG_NOOFFLINE)) {
		if (avg_running < disable_load) {
			target_cpus--;
			goto out;
		}
	}

out:
	spin_unlock_irqrestore(&hotplug_lock, flags);

	if (target_cpus > available_cpus)
		target_cpus = available_cpus;
	if (!target_cpus)
		target_cpus = 1;
	if (target_cpus != online_cpus)
		schedule_work_on(0, &hotplug_hotplug_work);

	mutex_unlock(&hotplug_mutex);
}

static void __ref hotplug_hotplug_fn(struct work_struct *work)
{
	int cpu;

	mutex_lock(&hotplug_mutex);

	if (target_cpus == num_online_cpus())
		goto out;

	for_each_possible_cpu(cpu) {
		if (!cpu)
			continue;
		if (cpu < target_cpus && !cpu_online(cpu))
			cpu_up(cpu);
		else if (cpu >= target_cpus && cpu_online(cpu))
			cpu_down(cpu);
	}

out:
	mutex_unlock(&hotplug_mutex);
}

static void hotplug_flag_fn(struct work_struct *work)
{
	unsigned long flags;
	int resched = 0;

	spin_lock_irqsave(&hotplug_lock, flags);

	// HOTPLUG_OFFLINE is currently the only flag using this.
	// if (work == &hotplug_allow_offline_work) {
		if (!time_is_after_jiffies(next_offline))
			hotplug_flags &= ~HOTPLUG_NOOFFLINE;
		else
			resched = next_offline - jiffies;
	// }

	if (resched)
		schedule_delayed_work_on(0, to_delayed_work(work), resched);

	spin_unlock_irqrestore(&hotplug_lock, flags);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void auto_hotplug_early_suspend(struct early_suspend *handler)
{
	unsigned long flags;

	spin_lock_irqsave(&hotplug_lock, flags);
	hotplug_flags |= HOTPLUG_NOONLINE;
	spin_unlock_irqrestore(&hotplug_lock, flags);
}

static void auto_hotplug_late_resume(struct early_suspend *handler)
{
	unsigned long flags;

	mutex_lock(&hotplug_mutex);
	spin_lock_irqsave(&hotplug_lock, flags);

	hotplug_flags = 0;
	target_cpus = num_possible_cpus();
	schedule_work_on(0, &hotplug_hotplug_work);
	delay_offline(HZ);

	spin_unlock_irqrestore(&hotplug_lock, flags);
	mutex_unlock(&hotplug_mutex);
}

static struct early_suspend auto_hotplug_suspend = {
	.suspend = auto_hotplug_early_suspend,
	.resume = auto_hotplug_late_resume,
};
#endif /* CONFIG_HAS_EARLYSUSPEND */

static void hotplug_enable(bool flag)
{
	if (flag) {
		hotplug_flags = 0;
		// Don't run until the buffer is full
		next_decision = jiffies + hotplug_sampling_periods;
#ifdef CONFIG_HAS_EARLYSUSPEND
		register_early_suspend(&auto_hotplug_suspend);
#endif
	} else {
		hotplug_flags = HOTPLUG_DISABLED;
		cancel_work_sync(&hotplug_decision_work);
		cancel_work_sync(&hotplug_hotplug_work);
#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&auto_hotplug_suspend);
#endif
	}
}

static int __init auto_hotplug_init(void)
{
	mutex_init(&hotplug_mutex);
	spin_lock_init(&hotplug_lock);

	INIT_WORK(&hotplug_decision_work, hotplug_decision_fn);
	INIT_WORK(&hotplug_hotplug_work, hotplug_hotplug_fn);
	INIT_DELAYED_WORK(&hotplug_allow_offline_work, hotplug_flag_fn);

	hotplug_register_alg(&autohp_alg);
	hotplug_register_alg(&fallback_alg);
	hotplug_alg_available(&fallback_alg, 1);

	hotplug_kobject = kobject_create_and_add("auto_hotplug",
		dkp_global_kobject);
	if (hotplug_kobject) {
		if (sysfs_create_group(hotplug_kobject, &hotplug_attr_grp))
			printk(KERN_ERR "%s: can't create group\n", __func__);
	}

	return 0;
}
late_initcall(auto_hotplug_init);
