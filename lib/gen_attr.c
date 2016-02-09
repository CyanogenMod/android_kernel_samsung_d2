/*
 * gen_attr.c: generic attribute handlers
 *
 * Copyright (c) 2013 Ryan Pennucci <decimalman@gmail.com>
 */
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/gen_attr.h>
#include <linux/export.h>

/* Pro tip: don't touch kobj.  It's probably not a kobject. */
ssize_t gattr_generic_store(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t count) {
	int i, l, t, r;
	int *v;
	void *p;
	struct gen_attr *gattr = (struct gen_attr *)attr;
	if (gattr->store && gattr->store != gattr_generic_store)
		return gattr->store(kobj, attr, buf, count);
	v = kmalloc(gattr->cnt * sizeof(int), GFP_KERNEL);
	if (!v)
		return -ENOMEM;
	for (i = 0, t = 0; i < gattr->cnt; i++) {
		r = sscanf(buf + t, " %i%n", &v[i], &l);
		if (!r || v[i] < gattr->min || v[i] > gattr->max) break;
		if (gattr->divisor > 1)
			v[i] *= gattr->divisor;
		t += l;
	}
	while (buf[t] == ' ' && t < count) t++;
	if (i != gattr->cnt || t < count - 1) {
		count = -EINVAL;
		goto out;
	}
	p = gattr->ptr;
	for (i = 0; i < gattr->cnt; i++) {
		if (gattr->set)
			gattr->set(p, v[i]);
		else
			*((int *)p) = v[i];
		p += gattr->stride;
	}
	if (gattr->cb)
		gattr->cb();
out:
	kfree(v);
	return count;
}
EXPORT_SYMBOL(gattr_generic_store);

ssize_t gattr_generic_show(struct kobject *kobj, struct attribute *attr, char *buf) {
	char *fmt;
	int i, l, v;
	void *p;
	struct gen_attr *gattr = (struct gen_attr *)attr;
	if (gattr->show && gattr->show != gattr_generic_show)
		return gattr->show(kobj, attr, buf);
	fmt = gattr->fmt ? : "%i";
	p = gattr->ptr;
	for (i = 0, l = 0; i < gattr->cnt; i++) {
		if (gattr->get)
			v = gattr->get(p);
		else
			v = *((int *)p);
		if (gattr->divisor > 1)
			v /= gattr->divisor;
		l += sprintf(buf + l, fmt, v);
		buf[l++] = ' ';
		p += gattr->stride;
	}
	buf[l-1] = '\n';
	return l;
}
EXPORT_SYMBOL(gattr_generic_show);
