/* simple_gpu_algorithm.c:
 * Dummy parameter wrapper for simple GPU governor.
 */

#include <linux/kernel.h>
#include <linux/module.h>

extern int simple_gpu_active;
extern int simple_gpu_default_laziness __read_mostly;
extern int simple_gpu_ramp_up_threshold __read_mostly;
extern void simple_gpu_activate(void);

static int simple_set_activate(const char *val, const struct kernel_param *kp)
{
	int ret = param_set_int(val, kp);
	simple_gpu_active = !!simple_gpu_active;
	simple_gpu_activate();
	return ret;
}

static const struct kernel_param_ops activate_ops = {
	.set = simple_set_activate,
	.get = param_get_int,
};

module_param_cb(simple_gpu_activate, &activate_ops, &simple_gpu_active, 0644);
MODULE_PARM_DESC(simple_gpu_activate, "enable simple GPU governor");

module_param_cb(simple_laziness, &param_ops_int, &simple_gpu_default_laziness, 0644);
MODULE_PARM_DESC(simple_laziness, "delay before changing frequency");

module_param_cb(simple_ramp_threshold, &param_ops_int, &simple_gpu_ramp_up_threshold, 0644);
MODULE_PARM_DESC(simple_ramp_threshold, "usage required to increase frequency");


