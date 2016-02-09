/*
 * drivers/cpufreq/cpufreq_asswax.c
 *
 * Copyright (C) 2010 Google, Inc.
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
 * Rewritten by:  Godmachine81
 * Worked on by: Zarboz
 * Original Author: Erasmux
 *
 * A work in progress of merging BrazilianWax and Smartass into AssWAX!
 *
 * Originally Based on the interactive governor By Mike Chan (mike@android.com)
 * which was adaptated to 2.6.29 kernel by Nadlabak (pavel@doshaska.net)
 *
 * SMP support based on mod by faux123
 *
 * For a general overview of asswax see the relavent part in
 * Documentation/cpu-freq/governors.txt
 *
 */

#include <linux/timer.h>
#include <linux/cpumask.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <asm/cputime.h>

/******************** Tunable parameters: ********************/

/*
 * The "ideal" frequency to use when awake. The governor will ramp up faster
 * towards the ideal frequency and slower after it has passed it. Similarly,
 * lowering the frequency towards the ideal frequency is faster than below it.
 */
#ifdef CONFIG_INTERACTION_HINTS
static unsigned int awake_ideal_freq = 594000;

static unsigned int interactive_ideal_freq = 810000;

static unsigned int interactive_timeout = 2;
#else
static unsigned int awake_ideal_freq = 810000;
#endif

/*
 * The "ideal" frequency to use when suspended.
 * When set to 0, the governor will not track the suspended state (meaning
 * that practically when sleep_ideal_freq==0 the awake_ideal_freq is used
 * also when suspended).
 */
static unsigned int sleep_ideal_freq = 384000;

/*
 * Freqeuncy delta when ramping up above the ideal freqeuncy.
 * Zero disables and causes to always jump straight to max frequency.
 * When below the ideal freqeuncy we always ramp up to the ideal freq.
 */
static unsigned int ramp_up_step = 192000;

/*
 * Freqeuncy delta when ramping down below the ideal freqeuncy.
 * Zero disables and will calculate ramp down according to load heuristic.
 * When above the ideal freqeuncy we always ramp down to the ideal freq.
 */
static unsigned int ramp_down_step = 0;

/*
 * CPU freq will be increased if measured load > max_cpu_load;
 */
static unsigned long max_cpu_load = 85;

/*
 * CPU freq will be decreased if measured load < min_cpu_load;
 */
static unsigned long min_cpu_load = 45;

/*
 * The minimum amount of time to spend at a frequency before we can ramp up.
 * Notice we ignore this when we are below the ideal frequency.
 */
static unsigned long up_rate_us = 10000;

/*
 * The minimum amount of time to spend at a frequency before we can ramp down.
 * Notice we ignore this when we are above the ideal frequency.
 */
static unsigned long down_rate_us = 20000;

/*
 * The frequency to set when waking up from sleep.
 * When sleep_ideal_freq=0 this will have no effect.
 */
static unsigned int sleep_wakeup_freq = 1512000;

/*
 * Sampling rate, I highly recommend to leave it at 2.
 */
static unsigned int sample_rate_jiffies = 2;

/*************** End of tunables ***************/

static atomic_t active_count = ATOMIC_INIT(0);

struct asswax_info_s {
	struct cpufreq_policy *cur_policy;
	struct cpufreq_frequency_table *freq_table;
	struct timer_list timer;
	u64 time_in_idle;
	u64 idle_exit_time;
	u64 freq_change_time;
	u64 freq_change_time_in_idle;
	int cur_cpu_load;
	int old_freq;
	int ramp_dir;
	unsigned int enable;
	int ideal_speed;
};
static DEFINE_PER_CPU(struct asswax_info_s, asswax_info);

/* Workqueues handle frequency scaling */
static struct workqueue_struct *up_wq;
static struct workqueue_struct *down_wq;
static struct work_struct freq_scale_work;

static cpumask_t work_cpumask;
static spinlock_t cpumask_lock;

static unsigned int asswax_state = 1; // 0 = suspend, 1 = awake, 2 = interactive, 3 = touched

static struct early_suspend asswax_power_suspend;

//#define DEBUG
#ifndef DEBUG
#define dprintk(x...) do { } while (0)
#else
#define dprintk(flag,msg...) do { \
	if (debug_mask & flag) printk(KERN_DEBUG msg); \
	} while (0)

enum {
	ASSWAX_DEBUG_JUMPS=1,
	ASSWAX_DEBUG_LOAD=2,
	ASSWAX_DEBUG_ALG=4
};

/*
 * Combination of the above debug flags.
 */
static unsigned long debug_mask = 7;
#endif

static int cpufreq_governor_asswax(struct cpufreq_policy *policy,
		unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_ASSWAX
static
#endif
struct cpufreq_governor cpufreq_gov_asswax = {
	.name = "asswax",
	.governor = cpufreq_governor_asswax,
	.max_transition_latency = 9000000,
	.owner = THIS_MODULE,
};

static void asswax_update_min_max(struct asswax_info_s *this_asswax, struct cpufreq_policy *policy, int state) {
	int tmp = 0;
	dprintk(ASSWAX_DEBUG_ALG, "asswax entering state %i on cpu %u\n", state, policy->cpu);
	switch (state) {
	case 0:
		tmp = sleep_ideal_freq;
		break;
	case 1:
		tmp = awake_ideal_freq;
		break;
#ifdef CONFIG_INTERACTION_HINTS
	case 2:
	case 3:
		tmp = interactive_ideal_freq;
		break;
#endif
	}
	this_asswax->ideal_speed =
		policy->max > tmp ? (tmp > policy->min ? tmp : policy->min) : policy->max;
}

static void asswax_update_min_max_allcpus(void) {
	unsigned int i;
	for_each_online_cpu(i) {
		struct asswax_info_s *this_asswax = &per_cpu(asswax_info, i);
		if (this_asswax->enable)
			asswax_update_min_max(this_asswax,this_asswax->cur_policy,asswax_state);
	}
}

inline static unsigned int validate_freq(struct cpufreq_policy *policy, int freq) {
	if (freq > (int)policy->max)
		return policy->max;
	if (freq < (int)policy->min)
		return policy->min;
	return freq;
}

inline static void reset_timer(unsigned long cpu, struct asswax_info_s *this_asswax) {
	this_asswax->time_in_idle = get_cpu_idle_time_us(cpu, &this_asswax->idle_exit_time);
	mod_timer(&this_asswax->timer, jiffies + sample_rate_jiffies);
}

inline static void work_cpumask_set(unsigned long cpu) {
	unsigned long flags;
	spin_lock_irqsave(&cpumask_lock, flags);
	cpumask_set_cpu(cpu, &work_cpumask);
	spin_unlock_irqrestore(&cpumask_lock, flags);
}

inline static int work_cpumask_test_and_clear(unsigned long cpu) {
	unsigned long flags;
	int res = 0;
	spin_lock_irqsave(&cpumask_lock, flags);
	res = cpumask_test_and_clear_cpu(cpu, &work_cpumask);
	spin_unlock_irqrestore(&cpumask_lock, flags);
	return res;
}

#ifdef CONFIG_INTERACTION_HINTS
static void do_disable_interaction(unsigned long data) {
	asswax_state = 1;
	asswax_update_min_max_allcpus();
}
static DEFINE_TIMER(interaction_timer, do_disable_interaction, 0, 0);
static inline void begin_interaction_timeout(void) {
	mod_timer(&interaction_timer, jiffies + interactive_timeout);
}
static inline void end_interaction_timeout(void) {
	if (timer_pending(&interaction_timer))
		del_timer(&interaction_timer);
}
#else
static void begin_interaction_timeout(void) { }
static void end_interaction_timeout(void) { }
#endif

inline static int target_freq(struct cpufreq_policy *policy, struct asswax_info_s *this_asswax,
			      int new_freq, int old_freq, int prefered_relation) {
	int index, target;
	struct cpufreq_frequency_table *table = this_asswax->freq_table;

	if (new_freq == old_freq)
		return 0;
	new_freq = validate_freq(policy,new_freq);
	if (new_freq == old_freq)
		return 0;

	if (table &&
	    !cpufreq_frequency_table_target(policy,table,new_freq,prefered_relation,&index))
	{
		target = table[index].frequency;
		if (target == old_freq) {
			// if for example we are ramping up to *at most* current + ramp_up_step
			// but there is no such frequency higher than the current, try also
			// to ramp up to *at least* current + ramp_up_step.
			if (new_freq > old_freq && prefered_relation==CPUFREQ_RELATION_H
			    && !cpufreq_frequency_table_target(policy,table,new_freq,
							       CPUFREQ_RELATION_L,&index))
				target = table[index].frequency;
			// simlarly for ramping down:
			else if (new_freq < old_freq && prefered_relation==CPUFREQ_RELATION_L
				&& !cpufreq_frequency_table_target(policy,table,new_freq,
								   CPUFREQ_RELATION_H,&index))
				target = table[index].frequency;
		}

		if (target == old_freq) {
			// We should not get here:
			// If we got here we tried to change to a validated new_freq which is different
			// from old_freq, so there is no reason for us to remain at same frequency.
			printk(KERN_WARNING "Smartass: frequency change failed: %d to %d => %d\n",
			       old_freq,new_freq,target);
			return 0;
		}
	}
	else target = new_freq;

	__cpufreq_driver_target(policy, target, prefered_relation);

	dprintk(ASSWAX_DEBUG_JUMPS,"SmartassQ: jumping from %d to %d => %d (%d)\n",
		old_freq,new_freq,target,policy->cur);

	return target;
}

static void cpufreq_asswax_timer(unsigned long cpu)
{
	u64 delta_idle;
	u64 delta_time;
	int cpu_load;
	int old_freq;
	u64 update_time;
	u64 now_idle;
	int queued_work = 0;
	struct asswax_info_s *this_asswax = &per_cpu(asswax_info, cpu);
	struct cpufreq_policy *policy = this_asswax->cur_policy;

	now_idle = get_cpu_idle_time_us(cpu, &update_time);
	old_freq = policy->cur;

	if (this_asswax->idle_exit_time == 0 || update_time == this_asswax->idle_exit_time)
		return;

	delta_idle = (now_idle - this_asswax->time_in_idle);
	delta_time = (update_time - this_asswax->idle_exit_time);

	// If timer ran less than 1ms after short-term sample started, retry.
	if (delta_time < 1000) {
		if (!timer_pending(&this_asswax->timer))
			reset_timer(cpu,this_asswax);
		return;
	}

	if (delta_idle > delta_time)
		cpu_load = 0;
	else
		cpu_load = 100 * (unsigned int)(delta_time - delta_idle) / (unsigned int)delta_time;

	dprintk(ASSWAX_DEBUG_LOAD,"asswaxT @ %d: load %d (delta_time %llu)\n",
		old_freq,cpu_load,delta_time);

	this_asswax->cur_cpu_load = cpu_load;
	this_asswax->old_freq = old_freq;

	// Scale up if load is above max or if there where no idle cycles since coming out of idle,
	// additionally, if we are at or above the ideal_speed, verify we have been at this frequency
	// for at least up_rate_us:
	if (cpu_load > max_cpu_load || delta_idle == 0)
	{
		if (old_freq < policy->max &&
			 (old_freq < this_asswax->ideal_speed || delta_idle == 0 ||
			  (update_time - this_asswax->freq_change_time) >= up_rate_us))
		{
			dprintk(ASSWAX_DEBUG_ALG,"asswaxT @ %d ramp up: load %d (delta_idle %llu)\n",
				old_freq,cpu_load,delta_idle);
			this_asswax->ramp_dir = 1;
			work_cpumask_set(cpu);
			queue_work(up_wq, &freq_scale_work);
			queued_work = 1;
			if (asswax_state == 2 && old_freq == this_asswax->ideal_speed)
				end_interaction_timeout();
		}
		else this_asswax->ramp_dir = 0;
	}
	// Similarly for scale down: load should be below min and if we are at or below ideal
	// frequency we require that we have been at this frequency for at least down_rate_us:
	else if (cpu_load < min_cpu_load && old_freq > policy->min &&
		 (old_freq > this_asswax->ideal_speed ||
		  (update_time - this_asswax->freq_change_time) >= down_rate_us))
	{
		dprintk(ASSWAX_DEBUG_ALG,"asswaxT @ %d ramp down: load %d (delta_idle %llu)\n",
			old_freq,cpu_load,delta_idle);
		this_asswax->ramp_dir = -1;
		work_cpumask_set(cpu);
		queue_work(down_wq, &freq_scale_work);
		queued_work = 1;
	}
	else this_asswax->ramp_dir = 0;

	// To avoid unnecessary load when the CPU is already at high load, we don't
	// reset ourselves if we are at max speed. If and when there are idle cycles,
	// the idle loop will activate the timer.
	// Additionally, if we queued some work, the work task will reset the timer
	// after it has done its adjustments.
	if (!queued_work && old_freq < policy->max)
		reset_timer(cpu,this_asswax);
}

static int cpufreq_idle_notifier(struct notifier_block *nb,
	unsigned long val, void *data) {
	struct asswax_info_s *this_asswax = &per_cpu(asswax_info, smp_processor_id());
	struct cpufreq_policy *policy = this_asswax->cur_policy;

	if (!this_asswax->enable)
		return NOTIFY_DONE;

	if (val == IDLE_START) {
		if (policy->cur == policy->max && !timer_pending(&this_asswax->timer)) {
			reset_timer(smp_processor_id(), this_asswax);
		} else if (policy->cur == policy->min) {
			if (timer_pending(&this_asswax->timer))
				del_timer(&this_asswax->timer);
			else if (asswax_state == 2)
				begin_interaction_timeout();
		}
	} else if (val == IDLE_END) {
		if (policy->cur == policy->min && !timer_pending(&this_asswax->timer))
			reset_timer(smp_processor_id(), this_asswax);
	}

	return NOTIFY_OK;
}
static struct notifier_block cpufreq_idle_nb = {
	.notifier_call = cpufreq_idle_notifier,
};

/* We use the same work function to sale up and down */
static void cpufreq_asswax_freq_change_time_work(struct work_struct *work)
{
	unsigned int cpu;
	int new_freq;
	int old_freq;
	int ramp_dir;
	struct asswax_info_s *this_asswax;
	struct cpufreq_policy *policy;
	unsigned int relation = CPUFREQ_RELATION_L;
	for_each_possible_cpu(cpu) {
		if (!work_cpumask_test_and_clear(cpu))
			continue;
		this_asswax = &per_cpu(asswax_info, cpu);

		ramp_dir = this_asswax->ramp_dir;
		this_asswax->ramp_dir = 0;

		old_freq = this_asswax->old_freq;
		policy = this_asswax->cur_policy;

		if (old_freq != policy->cur) {
			// frequency was changed by someone else?
			// Removing printk to prevent dmesg flooding while using CPU Master or other 3rd Party Cpu freq profilers
			//printk(KERN_WARNING "Smartass: frequency changed by 3rd party: %d to %d\n",
			//       old_freq,policy->cur);
			new_freq = old_freq;			
		}
		else if (ramp_dir > 0 && nr_running() > 1) {
			// ramp up logic:
			if (old_freq < this_asswax->ideal_speed)
				new_freq = this_asswax->ideal_speed;
			else if (ramp_up_step) {
				new_freq = old_freq + ramp_up_step;
				relation = CPUFREQ_RELATION_H;
			}
			else {
				new_freq = policy->max;
				relation = CPUFREQ_RELATION_H;
			}
			dprintk(ASSWAX_DEBUG_ALG,"asswaxQ @ %d ramp up: ramp_dir=%d ideal=%d\n",
				old_freq,ramp_dir,this_asswax->ideal_speed);
		}
		else if (ramp_dir < 0) {
			// ramp down logic:
			if (old_freq > this_asswax->ideal_speed) {
				new_freq = this_asswax->ideal_speed;
				relation = CPUFREQ_RELATION_H;
			}
			else if (ramp_down_step)
				new_freq = old_freq - ramp_down_step;
			else {
				// Load heuristics: Adjust new_freq such that, assuming a linear
				// scaling of load vs. frequency, the load in the new frequency
				// will be max_cpu_load:
				new_freq = old_freq * this_asswax->cur_cpu_load / max_cpu_load;
				if (new_freq > old_freq) // min_cpu_load > max_cpu_load ?!
					new_freq = old_freq -1;
			}
			dprintk(ASSWAX_DEBUG_ALG,"asswaxQ @ %d ramp down: ramp_dir=%d ideal=%d\n",
				old_freq,ramp_dir,this_asswax->ideal_speed);
		}
		else { // ramp_dir==0 ?! Could the timer change its mind about a queued ramp up/down
		       // before the work task gets to run?
		       // This may also happen if we refused to ramp up because the nr_running()==1
			new_freq = old_freq;
			dprintk(ASSWAX_DEBUG_ALG,"asswaxQ @ %d nothing: ramp_dir=%d nr_running=%lu\n",
				old_freq,ramp_dir,nr_running());
		}

		// do actual ramp up (returns 0, if frequency change failed):
		new_freq = target_freq(policy,this_asswax,new_freq,old_freq,relation);
		if (new_freq)
			this_asswax->freq_change_time_in_idle =
				get_cpu_idle_time_us(cpu,&this_asswax->freq_change_time);

		// reset timer:
		if (new_freq < policy->max)
			reset_timer(cpu,this_asswax);
		// if we are maxed out, it is pointless to use the timer
		// (idle cycles wake up the timer when the timer comes)
		else if (timer_pending(&this_asswax->timer))
			del_timer(&this_asswax->timer);

		cpufreq_notify_utilization(policy,
			(this_asswax->cur_cpu_load * policy->cur) / policy->max);
	}
}

#ifdef DEBUG
static ssize_t show_debug_mask(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", debug_mask);
}

static ssize_t store_debug_mask(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0)
		debug_mask = input;
	return count;
}
#endif

static ssize_t show_up_rate_us(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", up_rate_us);
}

static ssize_t store_up_rate_us(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input >= 0 && input <= 100000000)
		up_rate_us = input;
	return count;
}

static ssize_t show_down_rate_us(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", down_rate_us);
}

static ssize_t store_down_rate_us(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input >= 0 && input <= 100000000)
		down_rate_us = input;
	return count;
}

static ssize_t show_sleep_ideal_freq(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", sleep_ideal_freq);
}

static ssize_t store_sleep_ideal_freq(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input >= 0) {
		sleep_ideal_freq = input;
		if (asswax_state == 0)
			asswax_update_min_max_allcpus();
	}
	return count;
}

static ssize_t show_sleep_wakeup_freq(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", sleep_wakeup_freq);
}

static ssize_t store_sleep_wakeup_freq(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input >= 0)
		sleep_wakeup_freq = input;
	return count;
}

static ssize_t show_awake_ideal_freq(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", awake_ideal_freq);
}

static ssize_t store_awake_ideal_freq(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input >= 0) {
		awake_ideal_freq = input;
		if (asswax_state == 1)
			asswax_update_min_max_allcpus();
	}
	return count;
}

#ifdef CONFIG_INTERACTION_HINTS
static ssize_t show_interactive_ideal_freq(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", interactive_ideal_freq);
}

static ssize_t store_interactive_ideal_freq(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input >= 0) {
		interactive_ideal_freq = input;
		if (asswax_state == 1)
			asswax_update_min_max_allcpus();
	}
	return count;
}

static ssize_t show_interactive_timeout_jiffies(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", interactive_timeout);
}

static ssize_t store_interactive_timeout_jiffies(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input >= 0) {
		interactive_timeout = input;
		if (asswax_state == 1)
			asswax_update_min_max_allcpus();
	}
	return count;
}
#endif

static ssize_t show_sample_rate_jiffies(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", sample_rate_jiffies);
}

static ssize_t store_sample_rate_jiffies(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input > 0 && input <= 1000)
		sample_rate_jiffies = input;
	return count;
}

static ssize_t show_ramp_up_step(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", ramp_up_step);
}

static ssize_t store_ramp_up_step(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input >= 0)
		ramp_up_step = input;
	return count;
}

static ssize_t show_ramp_down_step(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", ramp_down_step);
}

static ssize_t store_ramp_down_step(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input >= 0)
		ramp_down_step = input;
	return count;
}

static ssize_t show_max_cpu_load(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", max_cpu_load);
}

static ssize_t store_max_cpu_load(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input > 0 && input <= 100)
		max_cpu_load = input;
	return count;
}

static ssize_t show_min_cpu_load(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", min_cpu_load);
}

static ssize_t store_min_cpu_load(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	ssize_t res;
	unsigned long input;
	res = strict_strtoul(buf, 0, &input);
	if (res >= 0 && input > 0 && input < 100)
		min_cpu_load = input;
	return count;
}

#define define_global_rw_attr(_name)		\
static struct global_attr _name##_attr =	\
	__ATTR(_name, 0644, show_##_name, store_##_name)

#ifdef DEBUG
define_global_rw_attr(debug_mask);
#endif
define_global_rw_attr(up_rate_us);
define_global_rw_attr(down_rate_us);
define_global_rw_attr(sleep_ideal_freq);
define_global_rw_attr(sleep_wakeup_freq);
define_global_rw_attr(awake_ideal_freq);
#ifdef CONFIG_INTERACTION_HINTS
define_global_rw_attr(interactive_ideal_freq);
define_global_rw_attr(interactive_timeout_jiffies);
#endif
define_global_rw_attr(sample_rate_jiffies);
define_global_rw_attr(ramp_up_step);
define_global_rw_attr(ramp_down_step);
define_global_rw_attr(max_cpu_load);
define_global_rw_attr(min_cpu_load);

static struct attribute * asswax_attributes[] = {
#ifdef DEBUG
	&debug_mask_attr.attr,
#endif
	&up_rate_us_attr.attr,
	&down_rate_us_attr.attr,
	&sleep_ideal_freq_attr.attr,
	&sleep_wakeup_freq_attr.attr,
	&awake_ideal_freq_attr.attr,
#ifdef CONFIG_INTERACTION_HINTS
	&interactive_ideal_freq_attr.attr,
	&interactive_timeout_jiffies_attr.attr,
#endif
	&sample_rate_jiffies_attr.attr,
	&ramp_up_step_attr.attr,
	&ramp_down_step_attr.attr,
	&max_cpu_load_attr.attr,
	&min_cpu_load_attr.attr,
	NULL,
};

static struct attribute_group asswax_attr_group = {
	.attrs = asswax_attributes,
	.name = "asswax",
};

static int cpufreq_governor_asswax(struct cpufreq_policy *new_policy,
		unsigned int event)
{
	unsigned int cpu = new_policy->cpu;
	int rc;
	struct asswax_info_s *this_asswax = &per_cpu(asswax_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if ((!cpu_online(cpu)) || (!new_policy->cur))
			return -EINVAL;

		this_asswax->cur_policy = new_policy;

		this_asswax->enable = 1;

		asswax_update_min_max(this_asswax,new_policy,asswax_state);

		this_asswax->freq_table = cpufreq_frequency_get_table(cpu);
		if (!this_asswax->freq_table)
			printk(KERN_WARNING "Smartass: no frequency table for cpu %d?!\n",cpu);

		// Do not register the idle hook and create sysfs
		// entries if we have already done so.
		if (atomic_inc_return(&active_count) <= 1) {
			rc = sysfs_create_group(cpufreq_global_kobject,
						&asswax_attr_group);
			if (rc)
				return rc;

#ifdef CONFIG_INTERACTION_HINTS
			cpufreq_want_interact_hints(1);
#endif
			idle_notifier_register(&cpufreq_idle_nb);
			register_early_suspend(&asswax_power_suspend);
		}

		//if (this_asswax->cur_policy->cur < new_policy->max && !timer_pending(&this_asswax->timer))
		if (!timer_pending(&this_asswax->timer))
			reset_timer(cpu,this_asswax);

		break;

	case CPUFREQ_GOV_LIMITS:
		asswax_update_min_max(this_asswax,new_policy,asswax_state);

		if (this_asswax->cur_policy->cur > new_policy->max) {
			dprintk(ASSWAX_DEBUG_JUMPS,"SmartassI: jumping to new max freq: %d\n",new_policy->max);
			__cpufreq_driver_target(this_asswax->cur_policy,
						new_policy->max, CPUFREQ_RELATION_H);
		}
		else if (this_asswax->cur_policy->cur < new_policy->min) {
			dprintk(ASSWAX_DEBUG_JUMPS,"SmartassI: jumping to new min freq: %d\n",new_policy->min);
			__cpufreq_driver_target(this_asswax->cur_policy,
						new_policy->min, CPUFREQ_RELATION_L);
		}

		if (this_asswax->cur_policy->cur < new_policy->max && !timer_pending(&this_asswax->timer))
			reset_timer(cpu,this_asswax);

		break;

	case CPUFREQ_GOV_STOP:
		this_asswax->enable = 0;
		del_timer(&this_asswax->timer);
		flush_work(&freq_scale_work);
		this_asswax->idle_exit_time = 0;

		if (atomic_dec_return(&active_count) < 1) {
			sysfs_remove_group(cpufreq_global_kobject,
					   &asswax_attr_group);
#ifdef CONFIG_INTERACTION_HINTS
			cpufreq_want_interact_hints(0);
#endif
			idle_notifier_unregister(&cpufreq_idle_nb);
			unregister_early_suspend(&asswax_power_suspend);
		}
		break;

#ifdef CONFIG_INTERACTION_HINTS
	case CPUFREQ_GOV_INTERACT:
		asswax_state = 3;
		asswax_update_min_max_allcpus();
		break;

	case CPUFREQ_GOV_NOINTERACT:
		asswax_state = 2;
		break;
#endif
	}

	return 0;
}

static void asswax_suspend(int cpu, int suspend)
{
	struct asswax_info_s *this_asswax = &per_cpu(asswax_info, smp_processor_id());
	struct cpufreq_policy *policy = this_asswax->cur_policy;
	unsigned int new_freq;

	if (!this_asswax->enable)
		return;

	asswax_update_min_max(this_asswax,policy,suspend);
	if (!suspend) { // resume at max speed:
		new_freq = validate_freq(policy,sleep_wakeup_freq);

		dprintk(ASSWAX_DEBUG_JUMPS,"SmartassS: awaking at %d\n",new_freq);

		__cpufreq_driver_target(policy, new_freq,
					CPUFREQ_RELATION_L);
	} else {
		// to avoid wakeup issues with quick sleep/wakeup don't change actual frequency when entering sleep
		// to allow some time to settle down. Instead we just reset our statistics (and reset the timer).
		// Eventually, the timer will adjust the frequency if necessary.

		this_asswax->freq_change_time_in_idle =
			get_cpu_idle_time_us(cpu,&this_asswax->freq_change_time);

		dprintk(ASSWAX_DEBUG_JUMPS,"SmartassS: suspending at %d\n",policy->cur);
	}

	reset_timer(smp_processor_id(),this_asswax);
}

static void asswax_early_suspend(struct early_suspend *handler) {
	int i;
	if (asswax_state == 0 || sleep_ideal_freq==0) // disable behavior for sleep_ideal_freq==0
		return;
	asswax_state = 0;
	for_each_online_cpu(i)
		asswax_suspend(i,0);
}

static void asswax_late_resume(struct early_suspend *handler) {
	int i;
	if (asswax_state > 0) // already not suspended so nothing to do
		return;
	asswax_state = 1;
	for_each_online_cpu(i)
		asswax_suspend(i,1);
}

static struct early_suspend asswax_power_suspend = {
	.suspend = asswax_early_suspend,
	.resume = asswax_late_resume,
#ifdef CONFIG_MACH_HERO
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1,
#endif
};

static int __init cpufreq_asswax_init(void)
{
	unsigned int i;
	struct asswax_info_s *this_asswax;

	spin_lock_init(&cpumask_lock);

	/* Initalize per-cpu data: */
	for_each_possible_cpu(i) {
		this_asswax = &per_cpu(asswax_info, i);
		this_asswax->enable = 0;
		this_asswax->cur_policy = 0;
		this_asswax->ramp_dir = 0;
		this_asswax->time_in_idle = 0;
		this_asswax->idle_exit_time = 0;
		this_asswax->freq_change_time = 0;
		this_asswax->freq_change_time_in_idle = 0;
		this_asswax->cur_cpu_load = 0;
		// intialize timer:
		init_timer_deferrable(&this_asswax->timer);
		this_asswax->timer.function = cpufreq_asswax_timer;
		this_asswax->timer.data = i;
		work_cpumask_test_and_clear(i);
	}

	// Scale up is high priority
	up_wq = create_workqueue("kasswax_up");
	down_wq = create_workqueue("kasswax_down");
	if (!up_wq || !down_wq)
		return -ENOMEM;

	INIT_WORK(&freq_scale_work, cpufreq_asswax_freq_change_time_work);

	return cpufreq_register_governor(&cpufreq_gov_asswax);
}

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ASSWAX
fs_initcall(cpufreq_asswax_init);
#else
module_init(cpufreq_asswax_init);
#endif

static void __exit cpufreq_asswax_exit(void)
{
	end_interaction_timeout();
	cpufreq_unregister_governor(&cpufreq_gov_asswax);
	destroy_workqueue(up_wq);
	destroy_workqueue(down_wq);
}

module_exit(cpufreq_asswax_exit);

MODULE_AUTHOR ("godmachine81 rewrite- original author of Smartass and Brazilian Wax - Erasmux");
MODULE_DESCRIPTION ("'cpufreq_asswax' - A combination of Brazilian Wax and Smartass");
MODULE_LICENSE ("GPL");
