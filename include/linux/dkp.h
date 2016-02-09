/*
 * linux/include/linux/dkp.h
 *
 * Copyright (c) 2013 Ryan Pennucci <decimalman@gmail.com>
 *
 * A kobject is placed in /sys/kernel/dkp.  It is intended to collect
 * attributes that don't logically fit elsewhere, but should still be
 * available.
 */

#ifndef _LINUX_DKP_H
#define _LINUX_DKP_H

#include <linux/gen_attr.h>
#include <linux/sysfs.h>

extern struct kobject *dkp_global_kobject;

extern void _dkp_register(struct attribute *gattr);
#define dkp_register(attr) \
	_dkp_register(&gen_attr(attr))

static inline void _dkp_unregister(struct attribute *attr) {
        sysfs_remove_file(dkp_global_kobject, attr);
}
#define dkp_unregister(attr) \
	_dkp_unregister(&gen_attr(attr))

#endif
