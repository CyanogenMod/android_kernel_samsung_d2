/* drivers/char/isaac.c
 * Implementation (c) 2014 Ryan Pennucci
 * Bob Jenkins has placed ISAAC in the public domain.
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
/* I'm not qualified to make changes to the ISAAC algorithms, but I have
 * substantially re-implemented the algorithms to be more suitable for the
 * kernel.
 *	- RP
 */
/* TODO:
 *  - Fix seeding
 */

#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/cpumask.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#ifdef MODULE
#include <linux/cdev.h>
#include <linux/device.h>
#endif

/* FIXME: do this right */
#define ISAAC_DEV MKDEV(235, 11)

#define RANDSIZL (CONFIG_ISAAC_SHIFT)
#define RANDSIZ  (1<<RANDSIZL)
#define RANDSIZB (RANDSIZ<<2)
struct isaac_ctx {
	u32		mem[RANDSIZ];
	union {
		u32	res[RANDSIZ];
		u8	bytes[RANDSIZB];
	};
	u32		a, b, c;
	u32		rem;
	struct semaphore sem;
};

/* Private per-CPU seeds.  These are used to initialize per-fd contexts, as
 * well as small reads, get_random_bytes, etc.
 */
static DEFINE_PER_CPU(struct isaac_ctx, cpu_seeds);

/* Generate random bytes. */
#ifndef CONFIG_ISAAC_PLUS
/* Bob Jenkins' ISAAC algorithm.
 */
#define SHR(x,s) (x>>s)
#define SHL(x,s) (x<<s)
#define IND(x) (*(u32 *)((u8 *)(mm) + ((x) & ((RANDSIZ-1)<<2))))
#define STEP(mix) {				\
	x = *m;					\
	a = (a ^ (mix)) + *(m2++);		\
	*(m++) = y = IND(x) + a + b;		\
	*(r++) = b = IND(y >> RANDSIZL) + x;	\
}
#else
/* Jean-Philippe Aumasson's ISAAC+ algorithm, almost.  ISAAC's IND() macro is
 * used, since it's equivalent (and generates better code) for RANDSIZL < 16.
 */
#define SHR(x,s) ror32(x,s)
#define SHL(x,s) rol32(x,s)
#define IND(x) (*(u32 *)((u8 *)(mm) + ((x) & ((RANDSIZ-1)<<2))))
#define STEP(mix) {				\
	x = *m;					\
	a = (a ^ (mix)) + *(m2++);		\
	*(m++) = y = IND(x) + a ^ b;		\
	*(r++) = b = IND(y >> RANDSIZL) ^ a + x;\
}
#endif

/* Generate random bytes from an isaac_ctx into an arbitrary buffer of type
 * u32 buf[RANDSIZ]; ctx->res is handy.
 *
 * NB: isaac() does not necessarily write to ctx->res; it doesn't make sense
 * for it to update ctx->rem.  Caller must update ctx->rem iff buf == ctx->res.
 */
static void isaac(struct isaac_ctx *ctx, u32 *buf)
{
	register u32 a, b, x, y, *m, *mm, *m2, *r, *mend;
	mm = ctx->mem;
	r = buf;
	a = ctx->a;
	b = ctx->b + (++ctx->c);
	for (m = mm, mend = m2 = m + (RANDSIZ/2); m < mend; ) {
		STEP(SHL(a,13));
		STEP(SHR(a,6));
		STEP(SHL(a,2));
		STEP(SHR(a,16));
	}
	for (m2 = mm; m2 < mend; ) {
		STEP(SHL(a,13));
		STEP(SHR(a,6));
		STEP(SHL(a,2));
		STEP(SHR(a,16));
	}
	ctx->a = a;
	ctx->b = b;
}

#define MIX()				\
{					\
	a ^= b << 11;	d += a; b += c;	\
	b ^= c >> 2;	e += b; c += d;	\
	c ^= d << 8;	f += c; d += e;	\
	d ^= e >> 16;	g += d; e += f;	\
	e ^= f << 10;	h += e; f += g;	\
	f ^= g >> 4;	a += f; g += h;	\
	g ^= h << 8;	b += g; h += a;	\
	h ^= a >> 9;	c += h; a += b;	\
}
#define LOAD(t)				\
{					\
	a += t[i    ]; b += t[i + 1];	\
	c += t[i + 2]; d += t[i + 3];	\
	e += t[i + 4]; f += t[i + 5];	\
	g += t[i + 6]; h += t[i + 7];	\
}
#define STORE()				\
{					\
	m[i    ] = a; m[i + 1] = b;	\
	m[i + 2] = c; m[i + 3] = d;	\
	m[i + 4] = e; m[i + 5] = f;	\
	m[i + 6] = g; m[i + 7] = h;	\
}
static void isaac_init(struct isaac_ctx *ctx)
{
	int i;
	u32 a, b, c, d, e, f, g, h;
	u32 *m, *r;

	m = ctx->mem;
	r = ctx->res;
	ctx->a = ctx->b = ctx->c = 0;
	ctx->rem = 0;
	a = b = c = d = e = f = g = h = 0x9e3779b9;

	for (i = 5; --i; ) {
		MIX();
	}

	isaac(&get_cpu_var(cpu_seeds), r);
	put_cpu_var(cpu_seeds);

	for (i = 0; i < RANDSIZ; i += 8) {
		LOAD(r);
		MIX();
		STORE();
	}
	for (i = 0; i < RANDSIZ; i += 8) {
		LOAD(m);
		MIX();
		STORE();
	}
}

/* Seed a new context with a specific seed.  During initialization, we seed
 * each cpu_seed using the previous seed.
 */
static void isaac_init_seed(struct isaac_ctx *seed, struct isaac_ctx *ctx)
{
	int i;
	u32 a, b, c, d, e, f, g, h;
	u32 *m, *r;

	m = ctx->mem;
	r = ctx->res;
	ctx->a = ctx->b = ctx->c = 0;
	ctx->rem = 0;
	a = b = c = d = e = f = g = h = 0x9e3779b9;

	for (i = 5; --i; ) {
		MIX();
	}

	if (seed) {
		isaac(seed, r);
		for (i = 0; i < RANDSIZ; i += 8) {
			LOAD(r);
			MIX();
			STORE();
		}
		for (i = 0; i < RANDSIZ; i += 8) {
			LOAD(m);
			MIX();
			STORE();
		}
		isaac(ctx, r);
	} else {
#ifndef CONFIG_ISAAC_RANDOM
		/* FIXME: Get an actual seed here. */
		pr_warn("initializing without a seed!\n");
#else
		isaac_extract_seed(r, RANDSIZ);
#endif
		for (i = 0; i < RANDSIZ; i += 8) {
			LOAD(r);
			MIX();
			STORE();
		}
		for (i = 9; --i; )
			isaac(ctx, ctx->res);
	}
}

/* Allocate a new context, with its semaphore locked */
static struct isaac_ctx *isaac_alloc(void)
{
	struct isaac_ctx *ctx = kmalloc(sizeof(struct isaac_ctx), GFP_KERNEL);
	if (!ctx)
		return NULL;

	isaac_init(ctx);
	sema_init(&ctx->sem, 0);
	return ctx;
}

static int isaac_open(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static int isaac_release(struct inode *inode, struct file *filp)
{
	if (filp->private_data)
		kfree(filp->private_data);
	return 0;
}

/* isaac_read returns data strictly in the order it was generated.  There's
 * nothing to be gained with the tricks isaac_copyout uses.
 */
static ssize_t isaac_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos)
{
	struct isaac_ctx *ctx;
	size_t cnt;
	ssize_t ret = count;

	if (unlikely(!count))
		return ret;

	if (filp->private_data) {
		ctx = filp->private_data;
		if (down_interruptible(&ctx->sem))
			return -ERESTARTSYS;
	} else if (unlikely(count > 1024)) {
		/* initialized with sem locked */
		ctx = filp->private_data = isaac_alloc();
	} else {
		/* Hack: Fault pages now.  Once we get_cpu(), we can't fault them. */
		if (unlikely(put_user((char)0, buf)))
			return -EFAULT;
		ctx = &get_cpu_var(cpu_seeds);
	}

	/* Copy out any existing bytes */
	if (ctx->rem) {
		cnt = min_t(size_t, count, ctx->rem * sizeof(ctx->res[0]));
		if (copy_to_user(buf, ctx->bytes +
				      (ctx->rem * sizeof(ctx->res[0])) - cnt,
				 cnt)) {
			ret = -EFAULT;
			goto out;
		}
		ctx->rem -= DIV_ROUND_UP(cnt, sizeof(ctx->res[0]));
		buf += cnt;
		count -= cnt;
	}
	/* Generate more bytes as needed */
	if (count) {
		do {
			isaac(ctx, ctx->res);
			cnt = min_t(size_t, count, RANDSIZB);
			if (copy_to_user(buf, ctx->bytes + RANDSIZB - cnt, cnt)) {
				ret = -EFAULT;
				goto out;
			}
			buf += cnt;
			count -= cnt;
		} while (count);
		ctx->rem = RANDSIZ - DIV_ROUND_UP(cnt, sizeof(ctx->res[0]));
	}

out:
	if (filp->private_data)
		up(&ctx->sem);
	else
		put_cpu_var(cpu_seeds);

	return (ssize_t)ret;
}

ssize_t random_write(struct file *file, const char __user *buffer,
		     size_t count, loff_t *ppos);
long random_ioctl(struct file *f, unsigned int cmd, unsigned long arg);
const struct file_operations isaac_fops = {
	.open = isaac_open,
	.release = isaac_release,
	.read = isaac_read,
	.write = random_write,
	.unlocked_ioctl = random_ioctl,
};

#ifdef CONFIG_ISAAC_RANDOM
/* As much as possible, we generate data directly into the output buffer.  This
 * often saves a memcpy, and occasionally shaves off an iteration of isaac().
 */
static inline void isaac_copyout(struct isaac_ctx *ctx, void *buf, size_t len)
{
	if (unlikely(len > RANDSIZB)) {
		while (len > RANDSIZB) {
			isaac(ctx, buf);
			len -= RANDSIZB;
			buf += RANDSIZB;
		}
	}
	while (len) {
		size_t cnt = ctx->rem * sizeof(ctx->res[0]);
		if (!cnt) {
			isaac(ctx, ctx->res);
			cnt = RANDSIZB;
			ctx->rem = RANDSIZ;
		}
		cnt = min_t(size_t, len, cnt);
		memcpy(buf, ctx->bytes + (ctx->rem * sizeof(ctx->res[0])) - cnt,
			cnt);
		ctx->rem -= DIV_ROUND_UP(cnt, sizeof(ctx->res[0]));
		len -= cnt;
		buf += cnt;
	}
}

/* isaac_copyout wrappers. */
unsigned int get_random_int(void)
{
	unsigned int r;
	struct isaac_ctx *ctx = &get_cpu_var(cpu_seeds);

	if (unlikely(!ctx->rem)) {
		isaac(ctx, ctx->res);
		ctx->rem = RANDSIZ;
	}

	r = ctx->res[--ctx->rem];

	put_cpu_var(cpu_seeds);
	return r;
}
void get_random_bytes(void *buf, int nbytes)
{
	isaac_copyout(&get_cpu_var(cpu_seeds), buf, nbytes);
	put_cpu_var(cpu_seeds);
}
EXPORT_SYMBOL(get_random_bytes);
#endif

#ifdef MODULE
/* FIXME */
static struct class *isaac_class;
static struct cdev isaac_cdev;
static struct device *isaac_device;
static int __init register_device(void)
{
	int ret;

	isaac_class = class_create(THIS_MODULE, "fastrng");
	if (IS_ERR(isaac_class)) {
		pr_warn("failed to register class\n");
		return PTR_ERR(isaac_class);
	}

	cdev_init(&isaac_cdev, &isaac_fops);
	isaac_cdev.owner = THIS_MODULE;
	ret = cdev_add(&isaac_cdev, ISAAC_DEV, 1);
	if (ret < 0) {
		pr_warn("failed to add cdev\n");
		goto out_destroy;
	}

	ret = register_chrdev_region(ISAAC_DEV, 1, "/dev/isaac");
	if (ret < 0) {
		pr_warn("failed to allocate major/minor\n");
		goto out_remove;
	}

	isaac_device = device_create(isaac_class, NULL, ISAAC_DEV,
		NULL, "isaac");
	if (IS_ERR(isaac_device)) {
		pr_warn("failed to create device\n");
		ret = PTR_ERR(isaac_device);
		goto out_unregister;
	}

	return 0;

out_unregister:
	unregister_chrdev_region(ISAAC_DEV, 1);
out_remove:
	cdev_del(&isaac_cdev);
out_destroy:
	class_destroy(isaac_class);

	return ret;
}

static void __exit unregister_device(void)
{
	unregister_chrdev_region(ISAAC_DEV, 1);
	cdev_del(&isaac_cdev);
	device_destroy(isaac_class, ISAAC_DEV);
	class_destroy(isaac_class);
}
module_exit(unregister_device);
#endif

/* TODO:
 *  - How much locking do we need here?
 *  - How can we capture a seed?
 */
static int __init isaac_seeds_init(void)
{
	int i, j;
	struct isaac_ctx *seed, *ctx;

	i = 0;
	j = num_possible_cpus() - 1;

	ctx = &get_cpu_var(cpu_seeds);
	isaac_init_seed(NULL, ctx);

	do {
		seed = ctx;
		ctx = &per_cpu(cpu_seeds, j);
		isaac_init_seed(seed, ctx);
	} while (--j >= 0);

	put_cpu_var(cpu_seeds);
#ifdef MODULE
	return register_device();
#else
	return 0;
#endif
}
core_initcall(isaac_seeds_init);

MODULE_DESCRIPTION("ISAAC PRNG");
MODULE_LICENSE("GPL");
