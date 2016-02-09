/* arch/arm/kernel/hotplug_mgmt.c
 *
 * Copyright (c) 2013-2014, Ryan Pennucci <decimalman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * at your option) any later version.
 */

/* hotplug_mgmt.c: support for on-the-fly hotplug algorithm selection
 *
 * On init, hotplug algoritms register themselves here, providing a (de)init
 * callback, and (optionally) a per-tick callback.  Each algorithm may notify
 * us that is (un)available.
 *
 * We track algorithms' availability, and (de)init them as availability
 * permits, guaranteeing that there is only one active hotplug algorithm
 * running at any given time.
 */

#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/errno.h>
#include <linux/hotplug_mgmt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/cache.h>

static struct hotplug_alg hp_none_alg = {
	.name = "none",
	.prio = HP_ALG_NONE,
	.avail = 1,
	.init_cb = NULL,
	.tick_cb = NULL,
	.le = LIST_HEAD_INIT(hp_none_alg.le),
};
static LIST_HEAD(hp_algs);
static struct hotplug_alg *active_alg = &hp_none_alg;
static DEFINE_MUTEX(alg_lock);

void (*hotplug_alg_tick)(int) __read_mostly;

/* Handle transition from one alg to another.  Called with alg_lock held.
 */
static void transition_alg(struct hotplug_alg *a) {
	if (a == active_alg)
		return;

	printk(KERN_DEBUG "%s: switching alg %s(%i) to %s(%i)\n",
		__func__, active_alg->name, active_alg->prio,
		a->name, a->prio);

	hotplug_alg_tick = NULL;
	barrier();
	if (active_alg->init_cb)
		active_alg->init_cb(0);

	if (a->init_cb)
		a->init_cb(1);
	barrier();
	hotplug_alg_tick = a->tick_cb;
	active_alg = a;
}

/* Change alg availability.  If need be, transition to another alg,
 * guaranteeing that the active alg is the lowest-priority alg available.
 */
static void __hotplug_alg_available(struct hotplug_alg *a, bool avail) {
	if (a->avail == avail)
		return;

	a->avail = avail;

	if (a->prio > active_alg->prio)
		return;

	if (avail && a->prio < active_alg->prio) {
		transition_alg(a);
	} else if (!avail && a == active_alg) {
		struct list_head *le;
		struct hotplug_alg *ha;
		list_for_each(le, &hp_algs) {
			ha = list_entry(le, struct hotplug_alg, le);
			if (ha->avail) {
				transition_alg(ha);
				break;
			}
		}
	}
}

void hotplug_alg_available(struct hotplug_alg *a, bool avail) {
	mutex_lock(&alg_lock);
	__hotplug_alg_available(a, avail);
	mutex_unlock(&alg_lock);
}
EXPORT_SYMBOL_GPL(hotplug_alg_available);

/* Sanity-check the alg and insert it into an appropriate position in the
 * algs list.
 */
int hotplug_register_alg(struct hotplug_alg *a) {
	struct list_head *le;
	struct hotplug_alg *ha;

	if (a->prio >= HP_ALG_NONE)
		return -EINVAL;

	if (!a->name)
		a->name = "<undefined>";

	mutex_lock(&alg_lock);

	list_for_each(le, &hp_algs) {
		ha = list_entry(le, struct hotplug_alg, le);
		if (a->prio <= ha->prio) {
			list_add_tail(&a->le, &ha->le);
			printk(KERN_DEBUG "%s: registered %s to prio %i\n",
				__func__, a->name, a->prio);
			goto out;
		}
	}

out:
	mutex_unlock(&alg_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(hotplug_register_alg);

/* Almost no-op.  Make sure the alg isn't available and remove it.
 */
void hotplug_unregister_alg(struct hotplug_alg *a) {
	mutex_lock(&alg_lock);

	__hotplug_alg_available(a, 0);
	list_del(&a->le);

	mutex_unlock(&alg_lock);
}
EXPORT_SYMBOL_GPL(hotplug_unregister_alg);

static int __init hotplug_mgmt_init(void) {
	list_add(&hp_none_alg.le, &hp_algs);
	return 0;
}
early_initcall(hotplug_mgmt_init);
