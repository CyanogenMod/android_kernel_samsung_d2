/*
 * include/linux/wmavg.h -- weighted moving averages
 * Copyright 2014-2015, Ryan Pennucci <decimalman@gmail.com>
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

/*
 * Weighted moving averages can be useful.  This implementation attempts to
 * provide a sane interface to manipulate them.
 *
 * Allocate a struct wmavg_average with wmavg_alloc().  Feed it with
 * wmavg_insert().  Get the average with wmavg_average->avg.  Do your own
 * locking.
 *
 * NB: While samples are 64-bit internally, values larger than 32-bit are
 *     likely to overflow.  Sum(value * weight) must be less than ULLONG_MAX.
 *     Signed values are not supported.
 */

#ifndef _WMAVG_H
#define _WMAVG_H

#include <linux/list.h>

#define WMAVG_ALLOC_SHIFT 10

/*
 * struct wmavg_sample:
 * For ease of calculation, value actually stores (value * weight).
 */
struct wmavg_sample {
	u64 value;
	u64 weight;
};

unsigned long wmavg_sample_calc(struct wmavg_sample *s);

/*
 * struct wmavg_sample_list:
 * Several wmavg_samples and a list_head.  When possible, acts as a ring buffer
 * to avoid allocations altogether.
 */
#define WMAVG_SAMPLES_PER_NODE \
	(((1<<WMAVG_ALLOC_SHIFT) - sizeof(struct list_head) - \
	sizeof(int) * 2) / sizeof(struct wmavg_sample))
struct wmavg_sample_list {
	struct list_head    list;
	int                 head, tail;
	struct wmavg_sample samples[WMAVG_SAMPLES_PER_NODE];
};

struct wmavg_average {
	unsigned long       avg;        // the average; updated every insert
	unsigned long       min_weight; // ignore insignificant samples
	u64                 max_weight; // maximum weight of all samples
	struct wmavg_sample __avg;      // accumulators for the average
	struct list_head    samples;    // list of wmavg_sample_list
};

struct wmavg_average *wmavg_alloc(void);
void wmavg_free(struct wmavg_average *a);
void wmavg_clear(struct wmavg_average *a);
int wmavg_insert_sample(struct wmavg_average *a, struct wmavg_sample *s);
int wmavg_insert(struct wmavg_average *a,
                 unsigned long value,
                 unsigned long weight);

void wmavg_set_max_weight(struct wmavg_average *avg, u64 max_weight);
static inline void wmavg_set_min_weight(struct wmavg_average *a,
                                        unsigned long min_weight)
{
	a->min_weight = min_weight;
}

#endif
