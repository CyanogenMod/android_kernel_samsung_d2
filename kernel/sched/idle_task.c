#include "sched.h"

#include <linux/export.h>
#include <linux/ktime.h>

/*
 * idle-task scheduling class.
 *
 * (NOTE: these are not related to SCHED_IDLE tasks which are
 *  handled in sched_fair.c)
 */

#ifdef CONFIG_SMP
static int
select_task_rq_idle(struct task_struct *p, int sd_flag, int flags)
{
	return task_cpu(p); /* IDLE tasks as never migrated */
}
#endif /* CONFIG_SMP */
static void bump_idle_stat(struct rq *rq, struct task_struct *idle)
{
	u64 delta = rq->clock_task - idle->se.exec_start;
	if (unlikely((s64)delta < 0))
		return;

	idle->se.sum_exec_runtime += delta;
	idle->se.exec_start = rq->clock_task;
}
/*
 * Idle tasks are unconditionally rescheduled:
 */
static void check_preempt_curr_idle(struct rq *rq, struct task_struct *p, int flags)
{
	bump_idle_stat(rq, p);
	resched_task(rq->idle);
}

static struct task_struct *pick_next_task_idle(struct rq *rq)
{
	rq->idle->se.exec_start = rq->clock_task;
	schedstat_inc(rq, sched_goidle);
	return rq->idle;
}

/*
 * It is not legal to sleep in the idle task - print a warning
 * message if some code attempts to do it:
 */
static void
dequeue_task_idle(struct rq *rq, struct task_struct *p, int flags)
{
	raw_spin_unlock_irq(&rq->lock);
	printk(KERN_ERR "bad: scheduling from the idle thread!\n");
	dump_stack();
	raw_spin_lock_irq(&rq->lock);
}

static void put_prev_task_idle(struct rq *rq, struct task_struct *prev)
{
	bump_idle_stat(rq, prev);
}

static void task_tick_idle(struct rq *rq, struct task_struct *curr, int queued)
{
	bump_idle_stat(rq, curr);
}

static void set_curr_task_idle(struct rq *rq)
{
	rq->idle->se.exec_start = rq->clock_task;
}

static void switched_to_idle(struct rq *rq, struct task_struct *p)
{
	BUG();
}

static void
prio_changed_idle(struct rq *rq, struct task_struct *p, int oldprio)
{
	BUG();
}

static unsigned int get_rr_interval_idle(struct rq *rq, struct task_struct *task)
{
	return 0;
}

ktime_t get_idle_ktime(unsigned int cpu) {
	if (unlikely(cpu > NR_CPUS)) {
		WARN_ONCE(1, KERN_ERR "Called with illegal cpu %i", cpu);
		return (ktime_t) { .tv64 = 0 };
	}
	return (ktime_t) { .tv64 = cpu_rq(cpu)->idle->se.sum_exec_runtime };
}
EXPORT_SYMBOL(get_idle_ktime);

/*
 * Simple, special scheduling class for the per-CPU idle tasks:
 */
const struct sched_class idle_sched_class = {
	/* .next is NULL */
	/* no enqueue/yield_task for idle tasks */

	/* dequeue is not valid, we print a debug message there: */
	.dequeue_task		= dequeue_task_idle,

	.check_preempt_curr	= check_preempt_curr_idle,

	.pick_next_task		= pick_next_task_idle,
	.put_prev_task		= put_prev_task_idle,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_idle,
#endif

	.set_curr_task          = set_curr_task_idle,
	.task_tick		= task_tick_idle,

	.get_rr_interval	= get_rr_interval_idle,

	.prio_changed		= prio_changed_idle,
	.switched_to		= switched_to_idle,
};
