/*
 * mms_ts.c - Touchscreen driver for Melfas MMS-series touch controllers
 *
 * Copyright (C) 2011 Google Inc.
 * Author: Dima Zavin <dima@android.com>
 *         Simon Wilson <simonwilson@google.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/cpufreq.h>
#include <linux/workqueue.h>

#include <linux/platform_data/mms_ts.h>

#define MAX_FINGERS		10
#define MAX_WIDTH		30
#define MAX_PRESSURE		4095
#define MAX_ANGLE		90
#define MIN_ANGLE		-90

#define MMS_INPUT_EVENT_PKT_SZ	0x0F
#define MMS_INPUT_EVENT0	0x10
#define FINGER_EVENT_SZ	8

int touch_is_pressed;
EXPORT_SYMBOL(touch_is_pressed);

struct mms_ts_info {
	struct i2c_client	*client;
	struct input_dev	*input_dev;
	unsigned int		finger_state;

	int	irq;
	bool	enabled;
	bool	ta_status;
	bool	noise_mode;

	struct mms_ts_platform_data *pdata;
	struct early_suspend early_suspend;
	struct tsp_callbacks callbacks;
	struct delayed_work finish_resume;
	char phys[32];
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mms_ts_early_suspend(struct early_suspend *h);
static void mms_ts_late_resume(struct early_suspend *h);
static void mms_ts_finish_resume(struct work_struct *work);
#endif

static void release_all_fingers(struct mms_ts_info *info)
{
	int i;

	for (i = 0; i < MAX_FINGERS; i++) {
		if (!(info->finger_state & (1 << i)))
			continue;
		input_mt_slot(info->input_dev, i);
		input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER,
					   false);
	}
	input_sync(info->input_dev);
	info->finger_state = touch_is_pressed = 0;
#ifdef CONFIG_INTERACTION_HINTS
	cpufreq_set_interactivity(0, INTERACT_ID_TOUCHSCREEN);
#endif
}

static void mms_set_noise_mode(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;

	if (!info->enabled)
		return;

	dev_notice(&client->dev, "ta/noise update: ta%s noise%s\n",
		info->ta_status ? "+" : "-",
		info->noise_mode ? "+" : "-");
	// This should work fine, but I don't have mfg docs to know for sure.
	i2c_smbus_write_byte_data(info->client,
		info->noise_mode ? 0x30 : 0x33,
		info->ta_status ? 0x1 : 0x2);
}

static inline void mms_pwr_on_reset(struct mms_ts_info *info)
{
	struct i2c_adapter *adapter = to_i2c_adapter(info->client->dev.parent);

	i2c_lock_adapter(adapter);
	if (info->pdata->mux_fw_flash)
		info->pdata->mux_fw_flash(true);

	info->pdata->vdd_on(0);
	msleep(50);
	info->pdata->vdd_on(1);
	msleep(50);

	if (info->pdata->mux_fw_flash)
		info->pdata->mux_fw_flash(false);
	i2c_unlock_adapter(adapter);

	msleep(70);
}

static void reset_mms_ts(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;

	if (!info->enabled)
		return;

	dev_notice(&client->dev, "%s++\n", __func__);
	disable_irq_nosync(info->irq);
	info->enabled = false;
	release_all_fingers(info);

	mms_pwr_on_reset(info);
	enable_irq(info->irq);
	info->enabled = true;

	mms_set_noise_mode(info);

	dev_notice(&client->dev, "%s--\n", __func__);
}

/* Toggle TSP mode so it will send another interrupt. */
static void mms_reset_irq(struct mms_ts_info *info)
{
	int ret = i2c_smbus_write_byte_data(info->client, 0, 0);
	if (!ret)
		ret = i2c_smbus_write_byte_data(info->client, 0, 1);
	if (ret)
		reset_mms_ts(info);
}


static void melfas_ta_cb(struct tsp_callbacks *cb, bool ta_status)
{
	struct mms_ts_info *info =
			container_of(cb, struct mms_ts_info, callbacks);

	info->ta_status = ta_status;
	if (info->enabled)
		mms_set_noise_mode(info);
}

static irqreturn_t mms_ts_interrupt(int irq, void *dev_id)
{
	struct mms_ts_info *info = dev_id;
	struct i2c_client *client = info->client;
	u8 buf[MAX_FINGERS*FINGER_EVENT_SZ];
	int i;
	struct input_event_list ev_list[] = {
		{ EV_ABS, ABS_MT_SLOT },
		{ EV_ABS, ABS_MT_TRACKING_ID },
		{ EV_ABS, ABS_MT_WIDTH_MAJOR },
		{ EV_ABS, ABS_MT_POSITION_X },
		{ EV_ABS, ABS_MT_POSITION_Y },
		{ EV_ABS, ABS_MT_TOUCH_MAJOR },
		{ EV_ABS, ABS_MT_TOUCH_MINOR },
		{ EV_ABS, ABS_MT_ANGLE },
		{ EV_ABS, ABS_MT_PALM },
		{ EV_CNT },
	};
	/* Since the QUP driver can't merge operations past a read, we can safely
	 * merge the smbus & i2c reads into a single block and avoid locking
	 * and reinitializing.
	 */
	u8 reg0 = MMS_INPUT_EVENT_PKT_SZ, reg1 = MMS_INPUT_EVENT0;
	struct i2c_msg msg[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.buf	= &reg0,
			.len	= 1,
		}, {
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.buf	= (char *)&msg[3].len,
			.len	= 1,
		}, {
			.addr   = client->addr,
			.flags  = 0,
			.buf    = &reg1,
			.len    = 1,
		}, {
			.addr   = client->addr,
			.flags  = I2C_M_RD,
			.buf    = buf,
			.len	= 0,
		},
	};

	i = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (unlikely(i != ARRAY_SIZE(msg) || !msg[3].len)) {
		dev_err(&client->dev,
			"failed to read %d bytes of touch data (%d)\n",
			msg[3].len, i);
		mms_reset_irq(info);
		goto out;
	}

	if (unlikely(buf[0] == 0x0F)) { /* ESD */
		dev_dbg(&client->dev, "ESD DETECT.... reset!!\n");
		reset_mms_ts(info);
		goto out;
	}

	if (unlikely(buf[0] == 0x0E)) { /* NOISE MODE */
		dev_dbg(&client->dev, "[TSP] noise mode enter!!\n");
		info->noise_mode = 1;
		mms_set_noise_mode(info);
		goto out;
	}

	for (i = 0; i < msg[3].len; i += FINGER_EVENT_SZ) {
		u8 *tmp = &buf[i];
		int id = (tmp[0] & 0xf) - 1;
		if (unlikely(id >= MAX_FINGERS)) {
			dev_notice(&client->dev, "finger id error [%d]\n", id);
			reset_mms_ts(info);
			goto out;
		}

		if ((tmp[0] & 0x80) == 0) {
			if (!(info->finger_state & (1 << id)))
				continue;
			input_mt_slot(info->input_dev, id);
			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER, false);
			info->finger_state &= ~(1 << id);
			continue;
		}

		ev_list[0].value = id;
		ev_list[1].value = input_mt_get_value(&info->input_dev->mt[id],
			ABS_MT_TRACKING_ID);
		if (ev_list[1].value < 0)
			ev_list[1].value = input_mt_new_trkid(info->input_dev);
		ev_list[2].value = tmp[4];
		ev_list[3].value = tmp[2] | ((tmp[1] & 0xf) << 8);
		ev_list[4].value = tmp[3] | ((tmp[1] & 0xf0) << 4);
		ev_list[5].value = tmp[6];
		ev_list[6].value = tmp[7];
		ev_list[7].value = tmp[5] > 127 ? (tmp[5] - 256) : tmp[5];
		ev_list[8].value = (buf[0] & 0x10) >> 4;
		input_event_list(info->input_dev, ev_list);
		info->finger_state |= 1 << id;
	}

	input_sync(info->input_dev);
	touch_is_pressed = !!info->finger_state;
#ifdef CONFIG_INTERACTION_HINTS
	cpufreq_set_interactivity(touch_is_pressed, INTERACT_ID_TOUCHSCREEN);
#endif

out:
	return IRQ_HANDLED;
}

static int __devinit mms_ts_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct mms_ts_info *info;
	struct input_dev *input_dev;
	int ret = 0;
	int max_x, max_y;
	void (*register_cb)(struct tsp_callbacks *) = NULL;
	touch_is_pressed = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	info = kzalloc(sizeof(struct mms_ts_info), GFP_KERNEL);
	if (!info) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "Failed to allocate memory for input device\n");
		ret = -ENOMEM;
		goto err_input_alloc;
	}

	info->client = client;
	info->input_dev = input_dev;
	info->pdata = client->dev.platform_data;
	info->irq = -1;
	if (info->pdata) {
		max_x = info->pdata->max_x;
		max_y = info->pdata->max_y;
		register_cb = info->pdata->register_cb;
	} else {
		max_x = 720;
		max_y = 1280;
	}

	i2c_set_clientdata(client, info);

	info->callbacks.inform_charger = melfas_ta_cb;
	if (register_cb)
		register_cb(&info->callbacks);

	input_mt_init_slots(input_dev, MAX_FINGERS);

	snprintf(info->phys, sizeof(info->phys),
		 "%s/input0", dev_name(&client->dev));
	input_dev->name = "sec_touchscreen"; /*= "Melfas MMSxxx Touchscreen";*/
	input_dev->phys = info->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, MAX_WIDTH, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_TOUCH_MAJOR, 0, MAX_PRESSURE, 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_TOUCH_MINOR, 0, MAX_PRESSURE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_ANGLE, MIN_ANGLE, MAX_ANGLE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PALM, 0, 1, 0, 0);
	input_set_drvdata(input_dev, info);

	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&client->dev, "failed to register input dev (%d)\n", ret);
		goto err_reg_input_dev;
	}

	ret = request_threaded_irq(client->irq, NULL, mms_ts_interrupt,
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				   "mms_ts", info);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to register interrupt\n");
		goto err_reg_input_dev;
	}
	info->irq = client->irq;
	info->enabled = true;

	mms_reset_irq(info);

#ifdef CONFIG_HAS_EARLYSUSPEND
	info->early_suspend.suspend = mms_ts_early_suspend;
	info->early_suspend.resume = mms_ts_late_resume;
	INIT_DELAYED_WORK(&info->finish_resume, mms_ts_finish_resume);
	register_early_suspend(&info->early_suspend);
#endif

	return 0;

err_reg_input_dev:
	input_free_device(input_dev);
err_input_alloc:
	kfree(info);
err_alloc:
	return ret;
}

static int __devexit mms_ts_remove(struct i2c_client *client)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);

	if (info->irq >= 0)
		free_irq(info->irq, info);
	input_unregister_device(info->input_dev);
	kfree(info);

	return 0;
}

#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static int mms_ts_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mms_ts_info *info = i2c_get_clientdata(client);

	cancel_delayed_work_sync(&info->finish_resume);

	mutex_lock(&info->input_dev->mutex);

	if (info->enabled) {
		disable_irq(info->irq);
		info->pdata->vdd_on(0);
		info->enabled = false;
		release_all_fingers(info);
	}

	mutex_unlock(&info->input_dev->mutex);
	return 0;
}

static int mms_ts_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mms_ts_info *info = i2c_get_clientdata(client);

	if (info->input_dev->users && !info->enabled) {
		info->pdata->vdd_on(1);
		schedule_delayed_work(&info->finish_resume,
			msecs_to_jiffies(120));
	}

	return 0;
}

static void mms_ts_finish_resume(struct work_struct *work) {
	struct mms_ts_info *info =
		container_of(work, struct mms_ts_info, finish_resume.work);
	mutex_lock(&info->input_dev->mutex);

	if (!info->enabled) {
		mms_set_noise_mode(info);
		enable_irq(info->irq);
		mms_reset_irq(info);
		info->enabled = true;
	}

	mutex_unlock(&info->input_dev->mutex);
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mms_ts_early_suspend(struct early_suspend *h)
{
	struct mms_ts_info *info;
	info = container_of(h, struct mms_ts_info, early_suspend);
	mms_ts_suspend(&info->client->dev);
}

static void mms_ts_late_resume(struct early_suspend *h)
{
	struct mms_ts_info *info;
	info = container_of(h, struct mms_ts_info, early_suspend);
	mms_ts_resume(&info->client->dev);
}
#endif

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static const struct dev_pm_ops mms_ts_pm_ops = {
	.suspend	= mms_ts_suspend,
	.resume		= mms_ts_resume,
};
#endif

static const struct i2c_device_id mms_ts_id[] = {
	{ "mms_ts", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mms_ts_id);

static struct i2c_driver mms_ts_driver = {
	.probe		= mms_ts_probe,
	.remove		= __devexit_p(mms_ts_remove),
	.driver = {
		.name = "mms_ts",
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
		.pm	= &mms_ts_pm_ops,
#endif
	},
	.id_table	= mms_ts_id,
};

static int __init mms_ts_init(void)
{
#ifdef CONFIG_BATTERY_SEC
	if (poweroff_charging) {
		pr_info("%s : LPM Charging Mode!!\n", __func__);
		return 0;
	}
#endif

	return i2c_add_driver(&mms_ts_driver);
}

static void __exit mms_ts_exit(void)
{
	i2c_del_driver(&mms_ts_driver);
}

module_init(mms_ts_init);
module_exit(mms_ts_exit);

/* Module information */
MODULE_DESCRIPTION("Touchscreen driver for Melfas MMS-series controllers");
MODULE_LICENSE("GPL");
