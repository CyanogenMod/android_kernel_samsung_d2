/* lib/wmavg.c -- weighted moving averages
 * Copyright (C) 2014-2015, Ryan Pennucci <decimalman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/wmavg.h>

static struct kmem_cache *wmavg_cache __read_mostly;

unsigned long wmavg_sample_calc(struct wmavg_sample *s)
{
	u64 ret;
	if (unlikely(!s->value || !s->weight))
		return 0;

	ret = s->value;
	do_div(ret, s->weight);
	WARN_ONCE(ret > ULONG_MAX, "result overflow");

	return ret;
}

static void __wmavg_reduce(struct wmavg_sample *s, u64 new_weight)
{
	u64 new = s->value;
	do_div(new, s->weight);
	s->value = new * new_weight;
	s->weight = new_weight;
}

struct wmavg_average *wmavg_alloc(void)
{
	struct wmavg_average *a;

	if (unlikely(!wmavg_cache))
		return NULL;

	a = kzalloc(sizeof(struct wmavg_average), GFP_KERNEL);
	if (!a)
		return NULL;

	INIT_LIST_HEAD(&a->samples);

	return a;
}

void wmavg_free(struct wmavg_average *a)
{
	wmavg_clear(a);
	kfree(a);
	kmem_cache_shrink(wmavg_cache);
}

void wmavg_clear(struct wmavg_average *a)
{
	struct wmavg_sample_list *s, *tmp;
	list_for_each_entry_safe(s, tmp, &a->samples, list) {
		list_del(&s->list);
		kmem_cache_free(wmavg_cache, s);
	}
	a->__avg.value = a->__avg.weight = a->avg = 0;
}

#define inc_rb(v) do { if (++v == WMAVG_SAMPLES_PER_NODE) v = 0; } while (0)
#define dec_rb(v) do { v = (v ? v : WMAVG_SAMPLES_PER_NODE) - 1; } while (0)
static void __wmavg_trim(struct wmavg_average *a)
{
	struct wmavg_sample_list *l, *tmp;
	s64 rem;

	rem = a->__avg.weight - a->max_weight;
	if (unlikely(rem <= 0))
		return;

	list_for_each_entry_safe_reverse(l, tmp, &a->samples, list) {
		do {
			struct wmavg_sample *samp = &l->samples[l->tail];
			inc_rb(l->tail);

			a->__avg.value -= samp->value;
			a->__avg.weight -= samp->weight;
			rem -= samp->weight;

			if (rem > 0)
				continue;
			if (!rem)
				break;

			if (-rem > a->min_weight) {
				__wmavg_reduce(samp, -rem);
				a->__avg.value += samp->value;
				a->__avg.weight += samp->weight;
				dec_rb(l->tail);
				goto out;
			}
			break;
		} while (l->tail != l->head);
		if (l->tail == l->head) {
			list_del(&l->list);
			kmem_cache_free(wmavg_cache, l);
		}
		if (rem <= 0)
			break;
	}
out:
	a->avg = wmavg_sample_calc(&a->__avg);
}

static struct wmavg_sample_list *__wmavg_list_alloc(struct wmavg_average *a)
{
	struct wmavg_sample_list *l;

	l = kmem_cache_alloc(wmavg_cache, GFP_KERNEL);
	if (!l)
		return NULL;
	l->head = 0;
	l->tail = 0;
	list_add(&l->list, &a->samples);
	return l;
}

int wmavg_insert_sample(struct wmavg_average *a, struct wmavg_sample *s)
{
	struct wmavg_sample_list *l;

	if (s->weight < a->min_weight)
		return 0;

	if (unlikely(list_empty(&a->samples))) {
		l = __wmavg_list_alloc(a);
	} else {
		l = list_first_entry(&a->samples,
			struct wmavg_sample_list, list);
		if (l->head == l->tail)
			l = __wmavg_list_alloc(a);
	}
	if (!l)
		return -ENOMEM;

	l->samples[l->head] = *s;
	a->__avg.value += s->value;
	a->__avg.weight += s->weight;
	inc_rb(l->head);

	__wmavg_trim(a);

	return 0;
}

int wmavg_insert(struct wmavg_average *a,
                 unsigned long value,
		 unsigned long weight)
{
	struct wmavg_sample s;

	s.value = value;
	s.value *= weight;
	s.weight = weight;

	return wmavg_insert_sample(a, &s);
}

void wmavg_set_max_weight(struct wmavg_average *a, u64 max_weight)
{
	a->max_weight = max_weight;
	__wmavg_trim(a);
}


static int wmavg_init(void)
{
	wmavg_cache = KMEM_CACHE(wmavg_sample_list, 0);

	return 0;
}
late_initcall(wmavg_init);
