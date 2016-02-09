#ifndef _HOTPLUG_MGMT_H
#define _HOTPLUG_MGMT_H

#include <linux/list.h>
#include <linux/cache.h>

enum hotplug_prio {
	HP_ALG_CPUFREQ,  // CPUFreq govs get max priority
	HP_ALG_KERNEL,   // In-kernel hotplug (Thalamus, etc.)
	HP_ALG_USER,     // Userspace hotplug (mpdecision, etc.)
	HP_ALG_FALLBACK, // Last-resort if user hotplug fails
	HP_ALG_NONE,     // No hotplug, for internal use
	HP_ALG_COUNT,
};

struct hotplug_alg {
	char *name;
	enum hotplug_prio prio;
	bool avail;
	void (*init_cb)(bool init);
	void (*tick_cb)(int cpu);
	struct list_head le;
};

extern void (*hotplug_alg_tick)(int) __read_mostly;

int hotplug_register_alg(struct hotplug_alg *c);
void hotplug_unregister_alg(struct hotplug_alg *c);
void hotplug_alg_available(struct hotplug_alg *c, bool avail);

#endif
