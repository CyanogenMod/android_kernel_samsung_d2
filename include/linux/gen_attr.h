/*
 * linux/include/linux/gen_attr.h
 *
 * Copyright (c) 2013 Ryan Pennucci <decimalman@gmail.com>
 *
 * Generic attribute structures and handlers.  Generic attributes are intended
 * to reduce code required for typical show/store of int values, including ints
 * in arrays or structures.  Arbitrary get/set functions and post-set callbacks
 * are supported.
 *
 */
#ifndef _LINUX_GEN_ATTR_H
#define _LINUX_GEN_ATTR_H

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/string.h>

// This structure is mostly compatible with other atttribute structs
struct gen_attr {
	// Mostly standard xxx_attr struct (kobject type not guaranteed)
	struct attribute attr;
	ssize_t (*show)(struct kobject *, struct attribute *, char *);
	ssize_t (*store)(struct kobject *, struct attribute *, const char *, size_t);
	// Generic function information
	int min, max;			// min < val < max
	int cnt, stride;		// nr elements, bytes until next element
	int divisor;			// scale displayed/read value
	void *ptr;			// ptr to first element or target for get/set
	int (*get)(void *);		// NULL or called to get *ptr
	void (*set)(void *, int);	// NULL or called to set *ptr = int
	void (*cb)(void);		// NULL or called after setting
	char *fmt;			// NULL or per-element printf format
};

extern ssize_t gattr_generic_store(struct kobject *kobj, struct attribute *attr,
	const char *buf, size_t count);
extern ssize_t gattr_generic_show(struct kobject *kobj, struct attribute *attr,
	char *buf);

// The bare attribute from a gattr
#define gen_attr(g) ((gattr_##g).attr)
// A typed attribute pointer from a gattr
// Safe for most attr+show+store attr types
#define gen_tattr(type, g) ((struct type##_attribute *)&gattr_##g)
// A panic waiting to happen
#define gen_gattr(attr) ((struct gen_attr *)&attr)

#define __GENATTR(_name,_mode,_min,_max,_cnt,_stride,_div,_ptr,_set,_get,_cb,_fmt) { \
	.attr = {.name = __stringify(_name), .mode = _mode}, \
	.store = gattr_generic_store, .show = gattr_generic_show, \
	.min = _min, .max = _max, .cnt = _cnt, .stride = _stride, \
	.divisor = _div, .ptr = (void *) _ptr, .set = _set, .get = _get, \
	.cb = _cb, .fmt = _fmt, \
}

// single-int attr
#define __GATTR_NAME(var,name,min,max,cb) \
struct gen_attr gattr_##name = __GENATTR( \
	name, 0644, min, max, 1, 0, 0, &var, NULL, NULL, cb, NULL)

#define __GATTR(name,min,max,cb) __GATTR_NAME(name,name,min,max,cb)

// int-array attr
#define __GATTR_ARR_NAME(var,name,min,max,cb) \
struct gen_attr gattr_##name = __GENATTR( \
	name, 0644, min, max, (sizeof(name)/sizeof(int)), sizeof(int), 0, \
	var, NULL, NULL, cb, NULL)

#define __GATTR_ARR(name,min,max,cb) __GATTR_ARR_NAME(name,name,min,max,cb)

// Warning: untested from here down!

// single-struct attr
#define __GATTR_STR_NAME(name,elm,str,min,max,cb) \
struct gen_attr gattr_##name = __GENATTR( \
	name, 0644, min, max, 1, 0, 0, &str.elm, NULL, NULL, cb, NULL)

#define __GATTR_STR(name,str,min,max,cb) __GATTR_STR_NAME(name,name,str,min,max,cb)

// struct-array attr
#define __GATTR_STR_ARR_NAME(name,elm,str,min,max,cb) \
struct gen_attr gattr_##name = __GENATTR( \
	name, 0644, min, max, (sizeof(str)/sizeof(str[0])), sizeof(str[0]), 0, \
	&str.elm, NULL, NULL, cb, NULL)

#define __GATTR_STR_ARR(name,str,min,max,cb) __GATTR_STR_NAME(name,name,str,min,max,cb)

#endif
