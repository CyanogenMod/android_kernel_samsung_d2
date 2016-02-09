/* Copyright (c) 2009-2012, The Linux Foundation. All rights reserved.
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
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_i2c.h>

MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.2");
MODULE_ALIAS("platform:i2c_qup");

/* QUP Registers */
enum {
	QUP_CONFIG              = 0x0,
	QUP_STATE               = 0x4,
	QUP_IO_MODE             = 0x8,
	QUP_SW_RESET            = 0xC,
	QUP_OPERATIONAL         = 0x18,
	QUP_ERROR_FLAGS         = 0x1C,
	QUP_ERROR_FLAGS_EN      = 0x20,
	QUP_MX_READ_CNT         = 0x208,
	QUP_MX_INPUT_CNT        = 0x200,
	QUP_MX_WR_CNT           = 0x100,
	QUP_OUT_DEBUG           = 0x108,
	QUP_OUT_FIFO_CNT        = 0x10C,
	QUP_OUT_FIFO_BASE       = 0x110,
	QUP_IN_READ_CUR         = 0x20C,
	QUP_IN_DEBUG            = 0x210,
	QUP_IN_FIFO_CNT         = 0x214,
	QUP_IN_FIFO_BASE        = 0x218,
	QUP_I2C_CLK_CTL         = 0x400,
	QUP_I2C_STATUS          = 0x404,
};

/* QUP States and reset values */
enum {
	QUP_RESET_STATE         = 0,
	QUP_RUN_STATE           = 1U,
	QUP_STATE_MASK          = 3U,
	QUP_PAUSE_STATE         = 3U,
	QUP_STATE_VALID         = 1U << 2,
	QUP_I2C_MAST_GEN        = 1U << 4,
	QUP_OPERATIONAL_RESET   = 0xFF0,
	QUP_I2C_STATUS_RESET    = 0xFFFFFC,
};

/* QUP OPERATIONAL FLAGS */
enum {
	QUP_OUT_SVC_FLAG        = 1U << 8,
	QUP_IN_SVC_FLAG         = 1U << 9,
	QUP_MX_INPUT_DONE       = 1U << 11,
};

/* I2C mini core related values */
enum {
	I2C_MINI_CORE           = 2U << 8,
	I2C_N_VAL               = 0xF,

};

/* Packing Unpacking words in FIFOs , and IO modes*/
enum {
	QUP_WR_BLK_MODE  = 1U << 10,
	QUP_RD_BLK_MODE  = 1U << 12,
	QUP_UNPACK_EN    = 1U << 14,
	QUP_PACK_EN      = 1U << 15,
};

/* QUP tags */
enum {
	QUP_OUT_NOP   = 0,
	QUP_OUT_START = 1U << 8,
	QUP_OUT_DATA  = 2U << 8,
	QUP_OUT_STOP  = 3U << 8,
	QUP_OUT_REC   = 4U << 8,
	QUP_IN_DATA   = 5U << 8,
	QUP_IN_STOP   = 6U << 8,
	QUP_IN_NACK   = 7U << 8,
};

/* Status, Error flags */
enum {
	I2C_STATUS_WR_BUFFER_FULL  = 1U << 0,
	I2C_STATUS_BUS_ACTIVE      = 1U << 8,
	I2C_STATUS_BUS_MASTER      = 1U << 9,
	I2C_STATUS_ERROR_MASK      = 0x38000FC,
	QUP_I2C_NACK_FLAG          = 1U << 3,
	QUP_IN_NOT_EMPTY           = 1U << 5,
	QUP_STATUS_ERROR_FLAGS     = 0x7C,
};

/* Master status clock states */
enum {
	I2C_CLK_RESET_BUSIDLE_STATE = 0,
	I2C_CLK_FORCED_LOW_STATE    = 5,
};

#define QUP_MAX_CLK_STATE_RETRIES	300
#define DEFAULT_CLK_RATE		(19200000)
#define I2C_STATUS_CLK_STATE		13
#define QUP_OUT_FIFO_NOT_EMPTY		0x10

static char const * const i2c_rsrcs[] = { "i2c_clk", "i2c_sda" };

static struct gpiomux_setting recovery_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

struct i2c_qup_dev {
	struct device                *dev;
	void __iomem                 *base;		/* virtual */
	void __iomem                 *gsbi;		/* virtual */
	int                          err_irq;
	struct clk                   *clk;
	struct clk                   *pclk;
	struct i2c_adapter           adapter;

	struct i2c_msg               *msg;
	int                          pos;
	int                          cnt;
	int                          err;
	int                          mode;
	int                          clk_ctl;
	int                          one_bit_t;
	int                          out_fifo_sz;
	int                          in_fifo_sz;
	int                          out_blk_sz;
	int                          in_blk_sz;
	int                          wr_sz;
	struct msm_i2c_platform_data *pdata;
	void                         *complete;
	int                          i2c_gpios[ARRAY_SIZE(i2c_rsrcs)];
};

static irqreturn_t i2c_qup_interrupt(int irq, void *devid)
{
	struct i2c_qup_dev *dev = devid;
	uint32_t reg;

	if (unlikely(pm_runtime_status_suspended(dev->dev)))
		return IRQ_NONE;

	if (unlikely(!dev->msg || !dev->complete)) {
		/* Clear Error interrupt if it's a level triggered interrupt*/
		writel_relaxed(QUP_RESET_STATE, dev->base + QUP_STATE);
		/* Ensure that state is written before ISR exits */
		wmb();
		return IRQ_HANDLED;
	}

	reg = readl_relaxed(dev->base + QUP_I2C_STATUS);
	if (unlikely(reg & I2C_STATUS_ERROR_MASK)) {
		dev_err(dev->dev, "QUP: I2C status flags :0x%x, irq:%d\n",
			reg, irq);
		dev->err = reg;
		/* Clear Error interrupt if it's a level triggered interrupt*/
		writel_relaxed(QUP_RESET_STATE, dev->base + QUP_STATE);
		goto intr_done;
	}

	reg = readl_relaxed(dev->base + QUP_ERROR_FLAGS);
	if (unlikely(reg & 0x7F)) {
		dev_err(dev->dev, "QUP: QUP status flags :0x%x\n", reg);
		dev->err = -reg;
		/* Clear Error interrupt if it's a level triggered interrupt*/
		writel_relaxed((reg & QUP_STATUS_ERROR_FLAGS),
			dev->base + QUP_ERROR_FLAGS);
		goto intr_done;
	}

	reg = readl_relaxed(dev->base + QUP_OPERATIONAL);
	if (reg & QUP_OUT_SVC_FLAG)
		writel_relaxed(QUP_OUT_SVC_FLAG, dev->base + QUP_OPERATIONAL);
	if (dev->msg->flags == I2C_M_RD) {
		if (reg & (QUP_MX_INPUT_DONE | QUP_IN_SVC_FLAG)) {
			writel_relaxed(QUP_IN_SVC_FLAG, dev->base +
				QUP_OPERATIONAL);
		} else {
			return IRQ_HANDLED;
		}
	}

intr_done:
	/* Ensure that any state changes are acknowledged before ISR exits */
	smp_wmb();
	complete(dev->complete);
	return IRQ_HANDLED;
}

static inline int i2c_qup_test_state(struct i2c_qup_dev *dev,
                                     uint32_t req_state)
{
	uint32_t state = readl_relaxed(dev->base + QUP_STATE);

	if (!(state & QUP_STATE_VALID))
		return 0;

	if (req_state == QUP_STATE_VALID)
		return 1;
	else if (req_state == QUP_I2C_MAST_GEN)
		return state & QUP_I2C_MAST_GEN;
	else
		return req_state == (state & QUP_STATE_MASK);
}

static int i2c_qup_poll_state(struct i2c_qup_dev *dev, uint32_t req_state)
{
	int i;

	for (i = 0; i < 500; i++) {
		if (i2c_qup_test_state(dev, req_state))
			return 0;
		udelay(1);
	}

	return -ETIMEDOUT;
}

static int i2c_qup_update_state(struct i2c_qup_dev *dev, uint32_t state)
{
	if (i2c_qup_test_state(dev, state))
		return 0;

	if (i2c_qup_poll_state(dev, QUP_STATE_VALID))
		return -EIO;

	writel_relaxed(state, dev->base + QUP_STATE);

	if (i2c_qup_poll_state(dev, state))
		return -EIO;

	return 0;
}

/*
 * Before calling i2c_qup_config_core_on_en(), please make sure that QuPE core
 * is in RESET state.
 */
static inline void i2c_qup_config_core_on_en(struct i2c_qup_dev *dev)
{
	uint32_t status;

	status = readl_relaxed(dev->base + QUP_CONFIG);
	status |= BIT(13);
	writel_relaxed(status, dev->base + QUP_CONFIG);
	wmb();
}

static void i2c_qup_pwr_mgmt(struct i2c_qup_dev *dev, unsigned int state)
{
	if (state) {
		clk_prepare_enable(dev->clk);
		if (dev->pclk)
			clk_prepare_enable(dev->pclk);
		writel_relaxed(1, dev->base + QUP_SW_RESET);
	} else {
		i2c_qup_update_state(dev, QUP_RESET_STATE);
		clk_disable_unprepare(dev->clk);
		i2c_qup_config_core_on_en(dev);
		if (dev->pclk)
			clk_disable_unprepare(dev->pclk);
	}
}

static int i2c_qup_poll_writeready(struct i2c_qup_dev *dev, int rem)
{
	unsigned int retry = dev->one_bit_t * dev->out_fifo_sz * 9;

	while (retry--) {
		uint32_t status = readl_relaxed(dev->base + QUP_I2C_STATUS);

		if (!(status & I2C_STATUS_WR_BUFFER_FULL)) {
			if (((dev->msg->flags & I2C_M_RD) || (rem == 0)) &&
					!(status & I2C_STATUS_BUS_ACTIVE))
				return 0;
			else if ((dev->msg->flags == 0) && (rem > 0))
				return 0;
		}
		udelay(dev->one_bit_t);
	}

	return -ETIMEDOUT;
}

static int i2c_qup_poll_clock_ready(struct i2c_qup_dev *dev)
{
	uint32_t retries = 0;
	uint32_t op_flgs = -1, clk_state = -1;

	/*
	 * Wait for the clock state to transition to either IDLE or FORCED
	 * LOW.  This will usually happen within one cycle of the i2c clock.
	 */

	while (retries++ < QUP_MAX_CLK_STATE_RETRIES) {
		uint32_t status = readl_relaxed(dev->base + QUP_I2C_STATUS);
		clk_state = (status >> I2C_STATUS_CLK_STATE) & 0x7;
		/* Read the operational register */
		op_flgs = readl_relaxed(dev->base +
			QUP_OPERATIONAL) & QUP_OUT_FIFO_NOT_EMPTY;

		/*
		 * In very corner case when slave do clock stretching and
		 * output fifo will have 1 block of data space empty at
		 * the same time.  So i2c qup will get output service
		 * interrupt and as it doesn't have more data to be written.
		 * This can lead to issue where output fifo is not empty.
		*/
		if (op_flgs == 0 &&
			(clk_state == I2C_CLK_RESET_BUSIDLE_STATE ||
			clk_state == I2C_CLK_FORCED_LOW_STATE)){
			dev_dbg(dev->dev, "clk_state 0x%x op_flgs [%x]\n",
				clk_state, op_flgs);
			return 0;
		}

		/* 1-bit delay before we check again */
		udelay(dev->one_bit_t);
	}

	dev_err(dev->dev, "Error waiting for clk ready clk_state: 0x%x op_flgs: 0x%x\n",
		clk_state, op_flgs);
	return -ETIMEDOUT;
}

static inline int i2c_qup_request_gpios(struct i2c_qup_dev *dev)
{
	int i;
	int result = 0;

	for (i = 0; i < ARRAY_SIZE(i2c_rsrcs); i++) {
		if (dev->i2c_gpios[i] >= 0) {
			result = gpio_request(dev->i2c_gpios[i], i2c_rsrcs[i]);
			if (result) {
				dev_err(dev->dev,
					"gpio_request for pin %d failed\
					with error %d\n", dev->i2c_gpios[i],
					result);
				goto error;
			}
		}
	}
	return 0;

error:
	for (; --i >= 0;) {
		if (dev->i2c_gpios[i] >= 0)
			gpio_free(dev->i2c_gpios[i]);
	}
	return result;
}

static inline void i2c_qup_free_gpios(struct i2c_qup_dev *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(i2c_rsrcs); i++) {
		if (dev->i2c_gpios[i] >= 0)
			gpio_free(dev->i2c_gpios[i]);
	}
}

static void i2c_qup_issue_read(struct i2c_qup_dev *dev, struct i2c_msg *msg,
                           int *idx, uint32_t carry_over)
{
	uint16_t addr = (msg->addr << 1) | 1;
	/* QUP limit 256 bytes per read. By HW design, 0 in the 8-bit field
	 * is treated as 256 byte read.
	 */
	uint16_t rd_len = dev->cnt & 0xff;

	if (*idx % 4) {
		writel_relaxed(carry_over | ((QUP_OUT_START | addr) << 16),
			dev->base + QUP_OUT_FIFO_BASE);
		writel_relaxed((QUP_OUT_REC | rd_len),
			dev->base + QUP_OUT_FIFO_BASE);
	} else {
		writel_relaxed(((QUP_OUT_REC | rd_len) << 16) | QUP_OUT_START |
			addr, dev->base + QUP_OUT_FIFO_BASE);
	}
	*idx += 4;
}

static void i2c_qup_issue_write(struct i2c_qup_dev *dev, struct i2c_msg *msg,
                            int rem, int *idx, uint32_t *carry_over)
{
	int entries = dev->cnt;
	int empty_sl = dev->wr_sz - ((*idx) >> 1);
	int i = 0;
	uint32_t val = 0;
	uint32_t last_entry = 0;
	uint16_t addr = msg->addr << 1;

	if (dev->pos == 0) {
		if (*idx % 4) {
			writel_relaxed(*carry_over | ((QUP_OUT_START |
							addr) << 16),
					dev->base + QUP_OUT_FIFO_BASE);
		} else {
			val = QUP_OUT_START | addr;
		}
		*idx += 2;
		i++;
		entries++;
	} else {
		/* Avoid setp time issue by adding 1 NOP when number of bytes
		 * are more than FIFO/BLOCK size. setup time issue can't appear
		 * otherwise since next byte to be written will always be ready
		 */
		val = (QUP_OUT_NOP | 1);
		*idx += 2;
		i++;
		entries++;
	}
	if (entries > empty_sl)
		entries = empty_sl;

	for (; i < (entries - 1); i++) {
		if (*idx % 4) {
			writel_relaxed(val | ((QUP_OUT_DATA |
				msg->buf[dev->pos]) << 16),
				dev->base + QUP_OUT_FIFO_BASE);
		} else {
			val = QUP_OUT_DATA | msg->buf[dev->pos];
		}
		(*idx) += 2;
		dev->pos++;
	}
	if (dev->pos < (msg->len - 1))
		last_entry = QUP_OUT_DATA;
	else if (rem > 1) /* not last array entry */
		last_entry = QUP_OUT_DATA;
	else
		last_entry = QUP_OUT_STOP;
	if ((*idx % 4) == 0) {
		/*
		 * If read-start and read-command end up in different fifos, it
		 * may result in extra-byte being read due to extra-read cycle.
		 * Avoid that by inserting NOP as the last entry of fifo only
		 * if write command(s) leave 1 space in fifo.
		 */
		if (rem > 1) {
			struct i2c_msg *next = msg + 1;
			if (next->addr == msg->addr && (next->flags & I2C_M_RD)
					&& *idx == ((dev->wr_sz*2) - 4)) {
				writel_relaxed(((last_entry |
					msg->buf[dev->pos]) |
					((1 | QUP_OUT_NOP) << 16)),
					dev->base + QUP_OUT_FIFO_BASE);
				*idx += 2;
			} else if (next->flags == 0 && dev->pos == msg->len - 1
					&& *idx < (dev->wr_sz*2) &&
					(next->addr != msg->addr)) {
				/* Last byte of an intermittent write */
				writel_relaxed((QUP_OUT_STOP |
					msg->buf[dev->pos]),
					dev->base + QUP_OUT_FIFO_BASE);
				*idx += 2;
			} else {
				*carry_over = (last_entry | msg->buf[dev->pos]);
			}
		} else {
			writel_relaxed((last_entry | msg->buf[dev->pos]),
				dev->base + QUP_OUT_FIFO_BASE);
		}
	} else {
		writel_relaxed(val | ((last_entry | msg->buf[dev->pos]) << 16),
			dev->base + QUP_OUT_FIFO_BASE);
	}

	*idx += 2;
	dev->pos++;
	dev->cnt = msg->len - dev->pos;
}

static void i2c_qup_set_read_mode(struct i2c_qup_dev *dev, int rd_len)
{
	uint32_t wr_mode = QUP_PACK_EN | QUP_UNPACK_EN;

	if (rd_len > 256)
		rd_len = 256;

	if (dev->wr_sz < dev->out_fifo_sz)
		wr_mode |= QUP_WR_BLK_MODE;

	if (rd_len > dev->in_fifo_sz) {
		wr_mode |= QUP_RD_BLK_MODE;
		writel_relaxed(wr_mode, dev->base + QUP_IO_MODE);
		writel_relaxed(rd_len, dev->base + QUP_MX_INPUT_CNT);
	} else {
		writel_relaxed(wr_mode, dev->base + QUP_IO_MODE);
		writel_relaxed(rd_len, dev->base + QUP_MX_READ_CNT);
	}
}

static int i2c_qup_set_wr_mode(struct i2c_qup_dev *dev, int rem)
{
	int total_len = 0;
	int ret = 0;
	int len = dev->msg->len;
	struct i2c_msg *next = NULL;
	if (rem > 1)
		next = dev->msg + 1;
	while (rem > 1 && next->flags == 0 && (next->addr == dev->msg->addr)) {
		len += next->len + 1;
		next = next + 1;
		rem--;
	}
	if (len >= (dev->out_fifo_sz - 1)) {
		total_len = len + 1 + (len/(dev->out_blk_sz-1));

		writel_relaxed(QUP_WR_BLK_MODE | QUP_PACK_EN | QUP_UNPACK_EN,
			dev->base + QUP_IO_MODE);
		dev->wr_sz = dev->out_blk_sz;
	} else {
		writel_relaxed(QUP_PACK_EN | QUP_UNPACK_EN,
			dev->base + QUP_IO_MODE);
	}

	if (rem > 1) {
		if (next->addr == dev->msg->addr &&
			next->flags == I2C_M_RD) {
			i2c_qup_set_read_mode(dev, next->len);
			/* make sure read start & read command are in 1 blk */
			if ((total_len % dev->out_blk_sz) ==
				(dev->out_blk_sz - 1))
				total_len += 3;
			else
				total_len += 2;
		}
	}
	/* WRITE COUNT register valid/used only in block mode */
	if (dev->wr_sz == dev->out_blk_sz)
		writel_relaxed(total_len, dev->base + QUP_MX_WR_CNT);
	return ret;
}


static void i2c_qup_recover_bus_busy(struct i2c_qup_dev *dev)
{
	int i;
	int gpio_clk;
	int gpio_dat;
	bool gpio_clk_status = false;
	uint32_t status = readl_relaxed(dev->base + QUP_I2C_STATUS);
	struct gpiomux_setting old_gpio_setting[ARRAY_SIZE(i2c_rsrcs)];

	if (likely(!(status & (I2C_STATUS_BUS_ACTIVE | I2C_STATUS_BUS_MASTER))))
		return;

	if (i2c_qup_request_gpios(dev)) {
		dev_err(dev->dev, "Request GPIOs failed\n");
		return;
	}

	gpio_clk = dev->i2c_gpios[0];
	gpio_dat = dev->i2c_gpios[1];

	if ((gpio_clk == -1) && (gpio_dat == -1)) {
		dev_err(dev->dev, "Recovery failed due to undefined GPIO's\n");
		return;
	}

	disable_irq(dev->err_irq);
	for (i = 0; i < ARRAY_SIZE(i2c_rsrcs); i++) {
		if (msm_gpiomux_write(dev->i2c_gpios[i], GPIOMUX_ACTIVE,
				&recovery_config, &old_gpio_setting[i])) {
			dev_err(dev->dev, "GPIO pins have no active setting\n");
			goto recovery_end;
		}
	}

	dev_warn(dev->dev, "i2c_scl: %d, i2c_sda: %d\n",
		 gpio_get_value(gpio_clk), gpio_get_value(gpio_dat));

	for (i = 0; i < 9; i++) {
		if (gpio_get_value(gpio_dat) && gpio_clk_status)
			break;
		gpio_direction_output(gpio_clk, 0);
		udelay(5);
		gpio_direction_output(gpio_dat, 0);
		udelay(5);
		gpio_direction_input(gpio_clk);
		udelay(5);
		if (!gpio_get_value(gpio_clk))
			udelay(20);
		if (!gpio_get_value(gpio_clk))
			usleep_range(10000, 10000);
		gpio_clk_status = gpio_get_value(gpio_clk);
		gpio_direction_input(gpio_dat);
		udelay(5);
	}

	/* Configure ALT funciton to QUP I2C*/
	for (i = 0; i < ARRAY_SIZE(i2c_rsrcs); i++) {
		msm_gpiomux_write(dev->i2c_gpios[i], GPIOMUX_ACTIVE,
				&old_gpio_setting[i], NULL);
	}

	udelay(10);

	status = readl_relaxed(dev->base + QUP_I2C_STATUS);
	if (!(status & I2C_STATUS_BUS_ACTIVE)) {
		dev_info(dev->dev, "Bus busy cleared after %d clock cycles, "
			 "status %x\n",
			 i, status);
		goto recovery_end;
	}

	dev_warn(dev->dev, "Bus still busy, status %x\n", status);

recovery_end:
	i2c_qup_free_gpios(dev);
	enable_irq(dev->err_irq);
}

static int i2c_qup_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[],
                        int num)
{
	DECLARE_COMPLETION_ONSTACK(complete);
	struct i2c_qup_dev *dev = i2c_get_adapdata(adap);
	int ret = -EIO;
	int rem = num;

	if (unlikely(pm_runtime_get_sync(dev->dev) < 0))
		return -EIO;

	if (unlikely(!i2c_qup_test_state(dev, QUP_RESET_STATE))) {
		dev_warn(dev->dev, "not reset yet, trying again\n");
		writel_relaxed(1, dev->base + QUP_SW_RESET);
		if (i2c_qup_poll_state(dev, QUP_RESET_STATE)) {
			dev_err(dev->dev, "QUP Busy:Trying to recover\n");
			goto out_err;
		}
	}

	dev->complete = &complete;
	dev->pos = 0;

	enable_irq(dev->err_irq);

	/* Initialize QUP registers */
	writel_relaxed(0, dev->base + QUP_CONFIG);
	writel_relaxed(QUP_OPERATIONAL_RESET, dev->base + QUP_OPERATIONAL);
	writel_relaxed(QUP_STATUS_ERROR_FLAGS, dev->base + QUP_ERROR_FLAGS_EN);

	writel_relaxed(I2C_MINI_CORE | I2C_N_VAL, dev->base + QUP_CONFIG);

	/* Initialize I2C mini core registers */
	writel_relaxed(0, dev->base + QUP_I2C_CLK_CTL);
	writel_relaxed(QUP_I2C_STATUS_RESET, dev->base + QUP_I2C_STATUS);

	while (rem) {
		bool filled = false;

		dev->cnt = msgs->len - dev->pos;
		dev->msg = msgs;

		dev->wr_sz = dev->out_fifo_sz;
		dev->err = 0;

		if (i2c_qup_poll_state(dev, QUP_I2C_MAST_GEN)) {
			ret = -EIO;
			goto out_err;
		}

		/* HW limits Read upto 256 bytes in 1 read without stop */
		if (dev->msg->flags & I2C_M_RD) {
			i2c_qup_set_read_mode(dev, dev->cnt);
			if (dev->cnt > 256)
				dev->cnt = 256;
		} else {
			ret = i2c_qup_set_wr_mode(dev, rem);
			if (ret != 0)
				goto out_err;
			/* Don't fill block till we get interrupt */
			if (dev->wr_sz == dev->out_blk_sz)
				filled = true;
		}

		ret = i2c_qup_update_state(dev, QUP_RUN_STATE);
		if (ret < 0)
			goto out_err;

		writel_relaxed(dev->clk_ctl, dev->base + QUP_I2C_CLK_CTL);
		/* CLK_CTL register is not in the same 1K region as other QUP
		 * registers. Ensure that clock control is written before
		 * programming other QUP registers
		 */
		wmb();

		do {
			int idx = 0;
			uint32_t carry_over = 0;

			/* Transition to PAUSE state only possible from RUN */
			ret = i2c_qup_update_state(dev, QUP_PAUSE_STATE);
			if (ret < 0) {
				goto out_err;
			}

			/* This operation is Write, check the next operation
			 * and decide mode
			 */
			while (filled == false) {
				if ((msgs->flags & I2C_M_RD))
					i2c_qup_issue_read(dev, msgs, &idx,
							carry_over);
				else
					i2c_qup_issue_write(dev, msgs, rem,
							&idx, &carry_over);
				if (idx >= (dev->wr_sz << 1))
					filled = true;
				/* Start new message */
				if (filled == false) {
					if (msgs->flags & I2C_M_RD)
							filled = true;
					else if (rem > 1) {
						/* Only combine operations with
						 * same address
						 */
						struct i2c_msg *next = msgs + 1;
						if (next->addr != msgs->addr)
							filled = true;
						else {
							rem--;
							msgs++;
							dev->msg = msgs;
							dev->pos = 0;
							dev->cnt = msgs->len;
							if (msgs->len > 256)
								dev->cnt = 256;
						}
					} else
						filled = true;
				}
			}
			ret = i2c_qup_update_state(dev, QUP_RUN_STATE);
			if (ret < 0) {
				goto out_err;
			}
			dev_dbg(dev->dev, "idx:%d, rem:%d, num:%d, mode:%d\n",
				idx, rem, num, dev->mode);

			if (unlikely(!wait_for_completion_timeout(&complete,
					usecs_to_jiffies(dev->one_bit_t *
					dev->out_fifo_sz * 16) + 1))) {
				uint32_t istatus = readl_relaxed(dev->base +
							QUP_I2C_STATUS);
				uint32_t qstatus = readl_relaxed(dev->base +
							QUP_ERROR_FLAGS);
				uint32_t op_flgs = readl_relaxed(dev->base +
							QUP_OPERATIONAL);

				/*
				 * Dont wait for 1 sec if i2c sees the bus
				 * active and controller is not master.
				 * A slave has pulled line low. Try to recover
				 */
				if (!(istatus & I2C_STATUS_BUS_ACTIVE) ||
					(istatus & I2C_STATUS_BUS_MASTER)) {
					if (wait_for_completion_timeout(
							&complete, HZ))
						goto timeout_err;
				}
				i2c_qup_recover_bus_busy(dev);
				dev_err(dev->dev,
					"Transaction timed out, SL-AD = 0x%x\n",
					dev->msg->addr);

				dev_err(dev->dev, "I2C Status: %x\n", istatus);
				dev_err(dev->dev, "QUP Status: %x\n", qstatus);
				dev_err(dev->dev, "OP Flags: %x\n", op_flgs);
				writel_relaxed(1, dev->base + QUP_SW_RESET);
				/* Make sure that the write has gone through
				 * before returning from the function
				 */
				wmb();
				ret = -ETIMEDOUT;
				goto out_err;
			}
timeout_err:
			if (unlikely(dev->err)) {
				if (dev->err > 0 &&
					dev->err & QUP_I2C_NACK_FLAG) {
					dev_err(dev->dev,
					"I2C slave addr:0x%x not connected\n",
					dev->msg->addr);
					dev->err = ENOTCONN;
				} else if (dev->err < 0) {
					dev_err(dev->dev,
					"QUP data xfer error %d\n", dev->err);
					ret = dev->err;
					goto out_err;
				} else if (dev->err > 0) {
					/*
					 * ISR returns +ve error if error code
					 * is I2C related, e.g. unexpected start
					 * So you may call recover-bus-busy when
					 * this error happens
					 */
					i2c_qup_recover_bus_busy(dev);
				}
				ret = -dev->err;
				goto out_err;
			}
			if (dev->msg->flags & I2C_M_RD) {
				int i;
				uint32_t dval = 0;
				for (i = 0; dev->pos < dev->msg->len; i++,
						dev->pos++) {
					uint32_t rd_status =
						readl_relaxed(dev->base
							+ QUP_OPERATIONAL);
					if (i % 2 == 0) {
						if ((rd_status &
							QUP_IN_NOT_EMPTY) == 0)
							break;
						dval = readl_relaxed(dev->base +
							QUP_IN_FIFO_BASE);
						dev->msg->buf[dev->pos] =
							dval & 0xFF;
					} else
						dev->msg->buf[dev->pos] =
							((dval & 0xFF0000) >>
							 16);
				}
				dev->cnt -= i;
			} else
				filled = false; /* refill output FIFO */
			dev_dbg(dev->dev, "pos:%d, len:%d, cnt:%d\n",
					dev->pos, msgs->len, dev->cnt);
		} while (dev->cnt > 0);
		if (msgs->len == dev->pos) {
			rem--;
			msgs++;
			dev->pos = 0;
		}
		if (rem) {
			ret = i2c_qup_poll_clock_ready(dev);
			if (ret < 0) {
				goto out_err;
			}
			ret = i2c_qup_update_state(dev, QUP_RESET_STATE);
			if (ret < 0) {
				goto out_err;
			}
		}
		/* Wait for I2C bus to be idle */
		ret = i2c_qup_poll_writeready(dev, rem);
		if (ret) {
			dev_err(dev->dev,
				"Error waiting for write ready\n");
			goto out_err;
		}
	}

	ret = num;
out_err:
	writel_relaxed(QUP_RESET_STATE, dev->base + QUP_STATE);
	disable_irq(dev->err_irq);
	dev->complete = NULL;
	dev->msg = NULL;
	pm_runtime_mark_last_busy(dev->dev);
	pm_runtime_put_sync_autosuspend(dev->dev);
	return ret;
}

static void __devinit i2c_qup_init_clk_ctl(struct i2c_qup_dev *dev)
{
	int fs_div;
	int hs_div;
	uint32_t fifo_reg;

	if (dev->gsbi) {
		writel_relaxed(0x2 << 4, dev->gsbi);
		wmb();
	}

	fs_div = ((dev->pdata->src_clk_rate
			/ dev->pdata->clk_freq) / 2) - 3;
	hs_div = 3;
	dev->clk_ctl = ((hs_div & 0x7) << 8) | (fs_div & 0xff);
	fifo_reg = readl_relaxed(dev->base + QUP_IO_MODE);
	if (fifo_reg & 0x3)
		dev->out_blk_sz = (fifo_reg & 0x3) * 16;
	else
		dev->out_blk_sz = 16;
	if (fifo_reg & 0x60)
		dev->in_blk_sz = ((fifo_reg & 0x60) >> 5) * 16;
	else
		dev->in_blk_sz = 16;
	/*
	 * The block/fifo size w.r.t. 'actual data' is 1/2 due to 'tag'
	 * associated with each byte written/received
	 */
	dev->out_blk_sz /= 2;
	dev->in_blk_sz /= 2;
	dev->out_fifo_sz = dev->out_blk_sz *
				(2 << ((fifo_reg & 0x1C) >> 2));
	dev->in_fifo_sz = dev->in_blk_sz *
				(2 << ((fifo_reg & 0x380) >> 7));
	dev_dbg(dev->dev, "QUP IN:bl:%d, ff:%d, OUT:bl:%d, ff:%d\n",
			dev->in_blk_sz, dev->in_fifo_sz,
			dev->out_blk_sz, dev->out_fifo_sz);
}

static u32 i2c_qup_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
}

static const struct i2c_algorithm i2c_qup_algo = {
	.master_xfer	= i2c_qup_xfer,
	.functionality	= i2c_qup_func,
};

static int __devinit i2c_qup_probe(struct platform_device *pdev)
{
	struct i2c_qup_dev	*dev;
	struct resource		*qup_mem, *gsbi_mem, *qup_io, *gsbi_io, *res;
	struct resource		*err_irq;
	struct clk		*clk, *pclk;
	int ret = 0;
	int i;
	struct msm_i2c_platform_data *pdata;

	gsbi_mem = NULL;
	dev_dbg(&pdev->dev, "i2c_qup_probe\n");

	if (pdev->dev.of_node) {
		struct device_node *node = pdev->dev.of_node;
		pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;
		ret = of_property_read_u32(node, "qcom,i2c-bus-freq",
					&pdata->clk_freq);
		if (ret)
			goto get_res_failed;
		ret = of_property_read_u32(node, "cell-index", &pdev->id);
		if (ret)
			goto get_res_failed;
		/* Optional property */
		of_property_read_u32(node, "qcom,i2c-src-freq",
					&pdata->src_clk_rate);
	} else {
		pdata = pdev->dev.platform_data;
	}

	if (!pdata) {
		dev_err(&pdev->dev, "platform data not initialized\n");
		return -ENOSYS;
	}

	qup_mem = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"qup_phys_addr");
	if (!qup_mem) {
		dev_err(&pdev->dev, "no qup mem resource?\n");
		ret = -ENODEV;
		goto get_res_failed;
	}

	err_irq = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
						"qup_err_intr");
	if (!err_irq) {
		dev_err(&pdev->dev, "no error irq resource?\n");
		ret = -ENODEV;
		goto get_res_failed;
	}

	qup_io = request_mem_region(qup_mem->start, resource_size(qup_mem),
					pdev->name);
	if (!qup_io) {
		dev_err(&pdev->dev, "QUP region already claimed\n");
		ret = -EBUSY;
		goto get_res_failed;
	}

	if (!pdata->use_gsbi_shared_mode) {
		gsbi_mem = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"gsbi_qup_i2c_addr");
		if (!gsbi_mem) {
			dev_dbg(&pdev->dev, "Assume BLSP\n");
			/*
			 * BLSP core does not need protocol programming so this
			 * resource is not expected
			 */
			goto blsp_core_init;
		}
		gsbi_io = request_mem_region(gsbi_mem->start,
						resource_size(gsbi_mem),
						pdev->name);
		if (!gsbi_io) {
			dev_err(&pdev->dev, "GSBI region already claimed\n");
			ret = -EBUSY;
			goto err_res_failed;
		}
	}

blsp_core_init:
	clk = clk_get(&pdev->dev, "core_clk");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "Could not get core_clk\n");
		ret = PTR_ERR(clk);
		goto err_clk_get_failed;
	}

	pclk = clk_get(&pdev->dev, "iface_clk");
	if (IS_ERR(pclk)) {
		dev_err(&pdev->dev, "Could not get iface_clk\n");
		ret = PTR_ERR(pclk);
		clk_put(clk);
		goto err_clk_get_failed;
	}

	if (pdata->clk_freq <= 0 ||
			pdata->clk_freq > 2000000) {
		dev_err(&pdev->dev, "clock frequency not supported\n");
		ret = -EIO;
		goto err_config_failed;
	}

	dev = kzalloc(sizeof(struct i2c_qup_dev), GFP_KERNEL);
	if (!dev) {
		ret = -ENOMEM;
		goto err_alloc_dev_failed;
	}

	dev->dev = &pdev->dev;
	dev->err_irq = err_irq->start;
	dev->clk = clk;
	if (!pdata->keep_ahb_clk_on)
		dev->pclk = pclk;
	dev->base = ioremap(qup_mem->start, resource_size(qup_mem));
	if (!dev->base) {
		ret = -ENOMEM;
		goto err_ioremap_failed;
	}

	/* Configure GSBI block to use I2C functionality */
	if (gsbi_mem) {
		dev->gsbi = ioremap(gsbi_mem->start, resource_size(gsbi_mem));
		if (!dev->gsbi) {
			ret = -ENOMEM;
			goto err_gsbi_failed;
		}
	}

	for (i = 0; i < ARRAY_SIZE(i2c_rsrcs); i++) {
		res = platform_get_resource_byname(pdev, IORESOURCE_IO,
						   i2c_rsrcs[i]);
		dev->i2c_gpios[i] = res ? res->start : -1;
	}

	platform_set_drvdata(pdev, dev);

	dev->one_bit_t = DIV_ROUND_UP(USEC_PER_SEC, pdata->clk_freq);
	dev->pdata = pdata;

	/*
	 * If bootloaders leave a pending interrupt on certain GSBI's,
	 * then we reset the core before registering for interrupts.
	 */

	if (dev->pdata->src_clk_rate > 0)
		clk_set_rate(dev->clk, dev->pdata->src_clk_rate);
	else
		dev->pdata->src_clk_rate = 19200000;

	clk_prepare_enable(dev->clk);
	clk_prepare_enable(pclk);
	writel_relaxed(1, dev->base + QUP_SW_RESET);
	if (i2c_qup_poll_state(dev, QUP_STATE_VALID))
		goto err_reset_failed;
	i2c_qup_init_clk_ctl(dev);
	clk_disable_unprepare(dev->clk);
	ret = i2c_qup_update_state(dev, QUP_RESET_STATE);
	if (ret)
		dev_err(dev->dev, "i2c_qup_update_state failed\n");
	else
		i2c_qup_config_core_on_en(dev);
	if (dev->pclk)
		clk_disable_unprepare(dev->pclk);

	/*
	 * We use num_irqs to also indicate if we got 3 interrupts or just 1.
	 * If we have just 1, we use err_irq as the general purpose irq
	 * and handle the changes in ISR accordingly
	 * Per Hardware guidelines, if we have 3 interrupts, they are always
	 * edge triggering, and if we have 1, it's always level-triggering
	 *
	 * XXX on 8960, num_irqs == 1.
	 */
	ret = request_irq(dev->err_irq, i2c_qup_interrupt,
			IRQF_TRIGGER_HIGH, "qup_err_intr", dev);
	if (unlikely(ret)) {
		dev_err(&pdev->dev, "request_err_irq failed\n");
		goto err_request_irq_failed;
	}
	disable_irq(dev->err_irq);
	i2c_set_adapdata(&dev->adapter, dev);
	dev->adapter.algo = &i2c_qup_algo;
	strlcpy(dev->adapter.name,
		"QUP I2C adapter",
		sizeof(dev->adapter.name));
	dev->adapter.nr = pdev->id;

	ret = i2c_add_numbered_adapter(&dev->adapter);
	if (ret) {
		dev_err(&pdev->dev, "i2c_add_adapter failed\n");
		free_irq(dev->err_irq, dev);
	} else {
		if (dev->dev->of_node) {
			dev->adapter.dev.of_node = pdev->dev.of_node;
			of_i2c_register_devices(&dev->adapter);
		}

		pm_runtime_set_autosuspend_delay(&pdev->dev, MSEC_PER_SEC);
		pm_runtime_use_autosuspend(&pdev->dev);
		pm_runtime_enable(&pdev->dev);
		return 0;
	}

err_request_irq_failed:
	if (dev->gsbi)
		iounmap(dev->gsbi);
err_reset_failed:
	clk_disable_unprepare(dev->clk);
	clk_disable_unprepare(pclk);
err_gsbi_failed:
	iounmap(dev->base);
err_ioremap_failed:
	kfree(dev);
err_alloc_dev_failed:
err_config_failed:
	clk_put(clk);
	clk_put(pclk);
err_clk_get_failed:
	if (gsbi_mem)
		release_mem_region(gsbi_mem->start, resource_size(gsbi_mem));
err_res_failed:
	release_mem_region(qup_mem->start, resource_size(qup_mem));
get_res_failed:
	if (pdev->dev.of_node)
		kfree(pdata);
	return ret;
}

static int __devexit i2c_qup_remove(struct platform_device *pdev)
{
	struct i2c_qup_dev	*dev = platform_get_drvdata(pdev);
	struct resource		*qup_mem, *gsbi_mem;

	i2c_del_adapter(&dev->adapter);
	if (unlikely(!pm_runtime_status_suspended(dev->dev)))
		i2c_qup_pwr_mgmt(dev, 0);
	platform_set_drvdata(pdev, NULL);
	free_irq(dev->err_irq, dev);
	if (dev->pclk)
		clk_put(dev->pclk);
	clk_put(dev->clk);
	if (dev->gsbi)
		iounmap(dev->gsbi);
	iounmap(dev->base);

	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);

	if (!(dev->pdata->use_gsbi_shared_mode)) {
		gsbi_mem = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"gsbi_qup_i2c_addr");
		release_mem_region(gsbi_mem->start, resource_size(gsbi_mem));
	}
	qup_mem = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"qup_phys_addr");
	release_mem_region(qup_mem->start, resource_size(qup_mem));
	if (dev->dev->of_node)
		kfree(dev->pdata);
	kfree(dev);
	return 0;
}

#ifdef CONFIG_PM
static int i2c_qup_pm_suspend_runtime(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct i2c_qup_dev *dev = platform_get_drvdata(pdev);
	i2c_qup_pwr_mgmt(dev, 0);
	return 0;
}

static int i2c_qup_pm_resume_runtime(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct i2c_qup_dev *dev = platform_get_drvdata(pdev);
	i2c_qup_pwr_mgmt(dev, 1);
	return 0;
}

#ifdef CONFIG_PM_RUNTIME
/*
 * i2c must remain available while entering suspend, but we don't want it
 * enabled while suspended.  With autosuspend enabled, we have an implicit
 * usage_count.  We release it during suspend and reacquire it during resume.
 */
static int i2c_qup_suspend(struct device *device)
{
	unsigned long flags;

	spin_lock_irqsave(&device->power.lock, flags);
	device->power.use_autosuspend = 0;
	spin_unlock_irqrestore(&device->power.lock, flags);

	pm_runtime_put_sync_suspend(device);

	return 0;
}

static int i2c_qup_resume(struct device *device)
{
	unsigned long flags;

	pm_runtime_get_noresume(device);

	// Since we don't want to resume, we can't just call use_autosuspend()
	spin_lock_irqsave(&device->power.lock, flags);
	device->power.use_autosuspend = 1;
	spin_unlock_irqrestore(&device->power.lock, flags);

	return 0;
}
#endif /* CONFIG_PM_RUNTIME */

static const struct dev_pm_ops i2c_qup_dev_pm_ops = {
#ifdef CONFIG_PM_RUNTIME
	SET_SYSTEM_SLEEP_PM_OPS(
		i2c_qup_suspend,
		i2c_qup_resume
	)
	SET_RUNTIME_PM_OPS(
		i2c_qup_pm_suspend_runtime,
		i2c_qup_pm_resume_runtime,
		NULL
	)
#else
	SET_SYSTEM_SLEEP_PM_OPS(
		i2c_qup_pm_suspend_runtime,
		i2c_qup_pm_resume_runtime
	)
#endif
};
#endif /* CONFIG_PM */

static struct of_device_id i2c_qup_dt_match[] = {
	{
		.compatible = "qcom,i2c-qup",
	},
	{}
};

static struct platform_driver i2c_qup_driver = {
	.probe		= i2c_qup_probe,
	.remove		= __devexit_p(i2c_qup_remove),
	.driver		= {
		.name	= "qup_i2c",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &i2c_qup_dev_pm_ops,
#endif
		.of_match_table = i2c_qup_dt_match,
	},
};

/* QUP may be needed to bring up other drivers */
static int __init i2c_qup_init_driver(void)
{
	return platform_driver_register(&i2c_qup_driver);
}
arch_initcall(i2c_qup_init_driver);

static void __exit i2c_qup_exit_driver(void)
{
	platform_driver_unregister(&i2c_qup_driver);
}
module_exit(i2c_qup_exit_driver);

