/* fake_mdnie.c -- hacks to expose video settings to Trickster et al.
 *
 * Copyright (c) 2013 Ryan Pennucci <decimalman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/sysfs.h>
#include <linux/gen_attr.h>

extern struct gen_attr gattr_mdnie_mcm;
extern struct gen_attr gattr_mdnie_mcm_temperature;

static struct attribute *hook_control_attrs[] = {
	&gen_attr(mdnie_mcm),
	&gen_attr(mdnie_mcm_temperature),
	NULL
};

static struct attribute_group hook_control = {
	.name = "hook_control",
	.attrs = hook_control_attrs,
};

static struct miscdevice mdnie_dev = {
	.name = "mdnie",
};

static int __init mdnie_init(void)
{
	int ret;
	ret = misc_register(&mdnie_dev);
	if (!ret)
		ret = sysfs_create_group(&mdnie_dev.this_device->kobj, &hook_control);
	return ret;
}

static void __exit mdnie_exit(void)
{
	misc_deregister(&mdnie_dev);
}

module_init(mdnie_init);
module_exit(mdnie_exit);

MODULE_AUTHOR("Ryan Pennucci <decimalman@gmail.com>");
MODULE_LICENSE("GPL");
