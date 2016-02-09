/*
 * Copyright (c) 2011, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/hw_random.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <mach/msm_iomap.h>
#include <mach/socinfo.h>

#define DRIVER_NAME "msm_rng"

/* Device specific register offsets */
#define PRNG_DATA_OUT_OFFSET    0x0000
#define PRNG_STATUS_OFFSET	0x0004
#define PRNG_LFSR_CFG_OFFSET	0x0100
#define PRNG_CONFIG_OFFSET	0x0104

/* Device specific register masks and config values */
#define PRNG_LFSR_CFG_MASK	0xFFFF0000
#define PRNG_LFSR_CFG_CLOCKS	0x0000DDDD
#define PRNG_CONFIG_MASK	0xFFFFFFFD
#define PRNG_HW_ENABLE		0x00000002

#define MAX_HW_FIFO_DEPTH 16                     /* FIFO is 16 words deep */
#define MAX_HW_FIFO_SIZE (MAX_HW_FIFO_DEPTH * 4) /* FIFO is 32 bits wide  */

#define FIFO_REFILL_MSECS (10)

#define RANDBUF_SIZE (4096)
#define RANDBUF_CNT (RANDBUF_SIZE/sizeof(unsigned long))

struct msm_rng_device {
	struct platform_device *pdev;
	void __iomem *base;
	struct clk *prng_clk;
};

static int __msm_rng_read(struct hwrng *rng, void *data, size_t max)
{
	struct msm_rng_device *msm_rng_dev;
	void __iomem *base;
	size_t currsize = 0;
	unsigned long *retdata = data;

	msm_rng_dev = (struct msm_rng_device *)rng->priv;
	base = msm_rng_dev->base;

	/* calculate max size bytes to transfer back to caller */
	//max = min_t(size_t, MAX_HW_FIFO_SIZE, max);
	//max &= ~3;

	/* enable PRNG clock */
	if (clk_enable(msm_rng_dev->prng_clk)) {
		dev_err(&msm_rng_dev->pdev->dev,
			"failed to enable clock in callback\n");
		return 0;
	}

	/* read random data from h/w */
	do {
		/* check status bit if data is available */
		if (!(readl_relaxed(base + PRNG_STATUS_OFFSET) & 1))
			break;	/* no data to read so just bail */

		/* read FIFO */
		*(retdata++) = readl_relaxed(base + PRNG_DATA_OUT_OFFSET);
		currsize += sizeof(unsigned long);
	} while (currsize < max);

	/* vote to turn off clock */
	clk_disable(msm_rng_dev->prng_clk);

	return currsize;
}

static int msm_rng_read(struct hwrng *rng, void *data, size_t max, bool wait);

static struct hwrng msm_rng = {
	.name = DRIVER_NAME,
	.read = msm_rng_read,
};

/* Implement arch_get_random_TYPE.  Precache data to avoid toggling the hwrng
 * clock every call.
 */
static unsigned long *randbuf;
static volatile int randbuf_head;
static atomic_t randbuf_tail;
static atomic_t bufwork_running = ATOMIC_INIT(1);
static void do_randbuf_fill(struct work_struct *work) {
	int cnt;
	struct msm_rng_device *msm_rng_dev;
	msm_rng_dev = (struct msm_rng_device *)msm_rng.priv;

	if (clk_prepare(msm_rng_dev->prng_clk))
		goto out;

again:
	cnt = atomic_read(&randbuf_tail) - randbuf_head - 1;
	if (cnt <= 0)
		cnt += RANDBUF_CNT;
	if (randbuf_head + cnt >= RANDBUF_CNT)
		cnt = RANDBUF_CNT - randbuf_head;
	if (cnt > 16)
		cnt = 16;

	cnt = __msm_rng_read(&msm_rng, randbuf + randbuf_head,
		cnt * sizeof(unsigned long));
	smp_wmb();
	randbuf_head = (randbuf_head + (cnt / sizeof(unsigned long)))
		% RANDBUF_CNT;

	if (cnt && atomic_read(&randbuf_tail) !=
		(randbuf_head + 1) % RANDBUF_CNT) {
		if (!schedule_timeout_interruptible(
			msecs_to_jiffies(FIFO_REFILL_MSECS)))
			goto again;
	}

	clk_unprepare(msm_rng_dev->prng_clk);
out:
	atomic_set(&bufwork_running, 0);
}
static DECLARE_WORK(randbuf_work, do_randbuf_fill);

static int msm_rng_read(struct hwrng *rng, void *data, size_t max, bool wait)
{
	int idx, new, cnt = 0;
	unsigned long *retdata = data;

	max &= ~(sizeof(unsigned long)-1);
	if (unlikely(!max))
		return 0;

	while (max) {
retry:
		smp_rmb();
		idx = atomic_read(&randbuf_tail);
		new = (idx + 1) % RANDBUF_CNT;
		if (idx == randbuf_head)
			goto empty;
		*retdata = randbuf[idx];
		if (idx != atomic_cmpxchg(&randbuf_tail, idx, new))
			goto retry;

		retdata++;
		max -= sizeof(unsigned long);
		cnt += sizeof(unsigned long);
		continue;

empty:
		if (!atomic_xchg(&bufwork_running, 1))
			schedule_work(&randbuf_work);
		if (!wait || !randbuf || schedule_timeout_interruptible(
			msecs_to_jiffies(FIFO_REFILL_MSECS)))
			return cnt;
	}
	new = new - randbuf_head - 1;
	if ((new <= 0 || new >= 128) &&
		!atomic_xchg(&bufwork_running, 1))
		schedule_work(&randbuf_work);
	return cnt;
}

int arch_get_random_long(unsigned long *v) {
	return msm_rng_read(&msm_rng, (void *)v, sizeof(unsigned long), 0);
}
EXPORT_SYMBOL(arch_get_random_long);
int arch_get_random_int(unsigned int *v) {
	return msm_rng_read(&msm_rng, (void *)v, sizeof(unsigned int), 0);
}
EXPORT_SYMBOL(arch_get_random_int);

static void msm_rng_read_randomness(struct work_struct *work) {
	char buf[256];
	int cnt;

	cnt = msm_rng_read(&msm_rng, buf, 256, 1);
	if (likely(cnt))
		add_device_randomness(buf, cnt);
}
static DECLARE_DELAYED_WORK(randomness_work, msm_rng_read_randomness);

/* Initialize the RNG: if the hw is already running, try to read some entropy
 * for the random core.  If not (or if reading fails), reset the hw.
 */
static int __devinit msm_rng_enable_hw(struct msm_rng_device *rng)
{
	unsigned long reg_val = 0;
	int ret = 0, cnt = 0;
	unsigned long buf[16];

	/* Enable the PRNG CLK */
	ret = clk_prepare_enable(rng->prng_clk);
	if (ret) {
		dev_err(&(rng->pdev)->dev,
				"failed to enable clock in probe\n");
		return -EPERM;
	}

	reg_val = readl_relaxed(rng->base + PRNG_CONFIG_OFFSET);
	if (reg_val & PRNG_HW_ENABLE) {
		for (cnt = 0; cnt < 16; cnt++) {
			if (!(readl_relaxed(rng->base + PRNG_STATUS_OFFSET) & 1))
				break;
			buf[cnt] = readl_relaxed(rng->base + PRNG_DATA_OUT_OFFSET);
		}
	}

	if (cnt) {
		printk(KERN_DEBUG "%s: adding %i words of randomness\n",
			__func__, cnt);
		add_device_randomness(buf, cnt * sizeof(unsigned long));
	}
	if (cnt < 16) {
		printk(KERN_DEBUG "%s: starting hwrng\n", __func__);
		reg_val = readl_relaxed(rng->base + PRNG_CONFIG_OFFSET);
		if (reg_val & PRNG_HW_ENABLE) {
			reg_val &= ~PRNG_HW_ENABLE;
			writel_relaxed(reg_val,
				rng->base + PRNG_CONFIG_OFFSET);
			mb();
		}
		reg_val = readl_relaxed(rng->base + PRNG_LFSR_CFG_OFFSET);
		if (reg_val & PRNG_LFSR_CFG_CLOCKS) {
			reg_val &= PRNG_LFSR_CFG_MASK;
			writel_relaxed(reg_val,
				rng->base + PRNG_LFSR_CFG_OFFSET);
			mb();
		}
		msleep(5);
		reg_val = readl_relaxed(rng->base + PRNG_LFSR_CFG_OFFSET);
		reg_val &= PRNG_LFSR_CFG_MASK;
		reg_val |= PRNG_LFSR_CFG_CLOCKS;
		writel_relaxed(reg_val, rng->base + PRNG_LFSR_CFG_OFFSET);
		mb();
		reg_val = readl_relaxed(rng->base + PRNG_CONFIG_OFFSET)
						& PRNG_CONFIG_MASK;
		reg_val |= PRNG_HW_ENABLE;
		writel_relaxed(reg_val, rng->base + PRNG_CONFIG_OFFSET);
		mb();
	}

	clk_disable_unprepare(rng->prng_clk);

	return 0;
}

static int __devinit msm_rng_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct msm_rng_device *msm_rng_dev = NULL;
	void __iomem *base = NULL;
	int error = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "invalid address\n");
		error = -EFAULT;
		goto err_exit;
	}

	msm_rng_dev = kzalloc(sizeof(msm_rng_dev), GFP_KERNEL);
	if (!msm_rng_dev) {
		dev_err(&pdev->dev, "cannot allocate memory\n");
		error = -ENOMEM;
		goto err_exit;
	}

	base = ioremap(res->start, resource_size(res));
	if (!base) {
		dev_err(&pdev->dev, "ioremap failed\n");
		error = -ENOMEM;
		goto err_iomap;
	}
	msm_rng_dev->base = base;

	/* create a handle for clock control */
	msm_rng_dev->prng_clk = clk_get(&pdev->dev, "core_clk");
	if (IS_ERR(msm_rng_dev->prng_clk)) {
		dev_err(&pdev->dev, "failed to register clock source\n");
		error = -EPERM;
		goto err_clk_get;
	}

	/* save away pdev and register driver data */
	msm_rng_dev->pdev = pdev;
	platform_set_drvdata(pdev, msm_rng_dev);

	/* Enable rng h/w */
	error = msm_rng_enable_hw(msm_rng_dev);

	if (error)
		goto rollback_clk;

	/* register with hwrng framework */
	msm_rng.priv = (unsigned long) msm_rng_dev;
	error = hwrng_register(&msm_rng);
	if (error) {
		dev_err(&pdev->dev, "failed to register hwrng\n");
		error = -EPERM;
		goto rollback_clk;
	}

#ifdef CONFIG_ARCH_RANDOM_HWRNG
	/* Init the arch_random bits */
	randbuf = kmalloc(RANDBUF_SIZE, GFP_KERNEL);
	if (randbuf) {
		schedule_work(&randbuf_work);
		schedule_delayed_work(&randomness_work, HZ / 4);
	} else {
		printk(KERN_WARNING "%s: can't allocate buffer\n", __func__);
	}
#endif

	return 0;

rollback_clk:
	clk_put(msm_rng_dev->prng_clk);
err_clk_get:
	iounmap(msm_rng_dev->base);
err_iomap:
	kfree(msm_rng_dev);
err_exit:
	return error;
}

static int __devexit msm_rng_remove(struct platform_device *pdev)
{
	struct msm_rng_device *msm_rng_dev = platform_get_drvdata(pdev);

	hwrng_unregister(&msm_rng);
	clk_put(msm_rng_dev->prng_clk);
	iounmap(msm_rng_dev->base);
	platform_set_drvdata(pdev, NULL);
	kfree(msm_rng_dev);
	return 0;
}

static struct of_device_id qrng_match[] = {
	{	.compatible = "qcom,msm-rng",
	},
	{}
};

static struct platform_driver rng_driver = {
	.probe      = msm_rng_probe,
	.remove     = __devexit_p(msm_rng_remove),
	.driver     = {
		.name   = DRIVER_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = qrng_match,
	}
};

static int __init msm_rng_init(void)
{
	return platform_driver_register(&rng_driver);
}

module_init(msm_rng_init);

static void __exit msm_rng_exit(void)
{
	platform_driver_unregister(&rng_driver);
}

module_exit(msm_rng_exit);

MODULE_AUTHOR("The Linux Foundation");
MODULE_DESCRIPTION("Qualcomm MSM Random Number Driver");
MODULE_LICENSE("GPL v2");
