/* drivers/staging/android/persist_entropy.c
 * Copyright (C) 2014, Ryan Pennucci <decimalman@gmail.com>
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
 */
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/persistent_ram.h>
#include <linux/jiffies.h>
#include <linux/random.h>

// Bytes of entropy to save across a reboot
#define SAVE_SIZE (1024)

static int __devinit persist_entropy_probe(struct platform_device *pdev)
{
	struct persistent_ram_zone *prz;
	size_t old_size;
	char *old_buf;

	prz = persistent_ram_init_ringbuffer(&pdev->dev, false);
	if (IS_ERR(prz))
		return PTR_ERR(prz);

	platform_set_drvdata(pdev, prz);

	old_size = persistent_ram_old_size(prz);
	if (!old_size) {
		pr_warn("%s: no saved entropy!\n", __func__);
		return 0;
	}

	old_buf = persistent_ram_old(prz);
	pr_info("%s: restoring %i bytes of entropy\n",
		__func__, old_size);
	add_device_randomness(old_buf, old_size);
	memset(old_buf, 0, old_size);
	persistent_ram_free_old(prz);

	return 0;
}

static void __devexit persist_entropy_shutdown(struct platform_device *pdev)
{
	struct persistent_ram_zone *prz = platform_get_drvdata(pdev);
	char *buf = kmalloc(SAVE_SIZE, GFP_KERNEL);
	if (!buf)
		return;

	pr_info("%s: saving %i bytes of entropy\n", __func__, SAVE_SIZE);
	get_random_bytes(buf, SAVE_SIZE);
	persistent_ram_write(prz, buf, SAVE_SIZE);

	kfree(buf);
	return;
}

static struct platform_driver persist_entropy_driver = {
	.driver		= {
		.name	= "persist_entropy",
	},
	.probe = persist_entropy_probe,
	.shutdown = persist_entropy_shutdown,
};

static int __init persist_entropy_init(void)
{
	return platform_driver_register(&persist_entropy_driver);
}

core_initcall(persist_entropy_init);
