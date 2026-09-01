#include <linux/i2c.h>

#include "ap3216c.h"

static int ap3216c_update_field(struct ap3216c_dev *ddata, int reg, const int mask, int value) {
    
    int ret;
    u8 reg_value;
    struct i2c_client *client = ddata->client;

    /* check if the value is within the mask */
    if (!FIELD_FIT(mask, value))
        return -EINVAL;

    mutex_lock(&ddata->lock);
    ret = i2c_smbus_read_byte_data(client, reg);
    if(ret < 0) {
        goto out_unlock;
    }

    reg_value = (ret & ~mask) | FIELD_PREP(mask, value);
    ret = i2c_smbus_write_byte_data(client, reg, reg_value);
    if(ret < 0) {
        goto out_unlock;
    }
    
    ret = 0;
out_unlock:
    mutex_unlock(&ddata->lock);
    return ret;
}

static int read_als_data(struct ap3216c_dev *ddata, int *val) {
    
    struct i2c_client *client = ddata->client;
    int als_data_low;
    int als_data_high;
    
    als_data_low = i2c_smbus_read_byte_data(client, AP3216C_ALS_DATA_LOW_REG);
    if(als_data_low < 0) {
        return als_data_low;
    }
    
    als_data_high = i2c_smbus_read_byte_data(client, AP3216C_ALS_DATA_HIGH_REG);
    if(als_data_high < 0) {
        return als_data_high;
    }

    *val = (als_data_high << 8) | als_data_low;
    return 0;
}

static int read_als_scale_microlux(struct ap3216c_dev *ddata, int *val, int *val2) {

    struct i2c_client *client = ddata->client;
    int ret;

    ret = i2c_smbus_read_byte_data(client, AP3216C_ALS_CONFIGURATION_REG);
    if(ret < 0) {
        return ret;
    }

    // ret should be 0, 1, 2, 3
    ret = FIELD_GET(AP3216C_ALS_CONF_RANGE_MASK, ret);
    
    *val = 0;
    *val2 = ap3216c_als_scale_microlux[ret];
    return 0;
}

static int write_als_scale_microlux(struct ap3216c_dev *ddata, int val, int val2) {
    
    if (val != 0)
        return -EINVAL;

    for (int i = 0; i < ARRAY_SIZE(ap3216c_als_scale_microlux); i++) {
        if (val2 == ap3216c_als_scale_microlux[i])
            return ap3216c_update_field(ddata, AP3216C_ALS_CONFIGURATION_REG,
                AP3216C_ALS_CONF_RANGE_MASK, i);
    }
    return -EINVAL;
}

static int write_als_threshold(struct ap3216c_dev *ddata, enum iio_event_direction dir,
                                int val, int val2) {

    u8 lower_byte = val & 0xff;
    u8 higher_byte = (val >> 8) & 0xff;
    u8 low_reg, high_reg;
    int ret;
    if(val2 != 0 || val < 0 || val > 0xffff)
        return -EINVAL;

    lower_byte = val & 0xff;
    higher_byte = (val >> 8) & 0xff;
    if(dir == IIO_EV_DIR_RISING) {
        low_reg = AP3216C_ALS_HIGH_THRESHOLD_LOW_REG;
        high_reg = AP3216C_ALS_HIGH_THRESHOLD_HIGH_REG;
    }else if(dir == IIO_EV_DIR_FALLING) {
        low_reg = AP3216C_ALS_LOW_THRESHOLD_LOW_REG;
        high_reg = AP3216C_ALS_LOW_THRESHOLD_HIGH_REG;
    }else {
        return -EINVAL;
    }

    mutex_lock(&ddata->lock);
    ret = i2c_smbus_write_byte_data(ddata->client, low_reg, lower_byte);
    if(ret < 0) {
        goto out_unlock;
    }
    ret = i2c_smbus_write_byte_data(ddata->client, high_reg, higher_byte);
    if(ret < 0) {
        goto out_unlock;
    }
    /* write success */
    ret = 0;
out_unlock:
    mutex_unlock(&ddata->lock);
    return ret;
}

static int ap3216c_write_event_value(struct iio_dev *indio_dev,
        const struct iio_chan_spec *chan, enum iio_event_type type,
        enum iio_event_direction dir, enum iio_event_info info, int val, int val2) {

    struct ap3216c_dev *ddata = iio_priv(indio_dev);
    switch(chan->address) {
        case AP3216C_CHANNEL_ALS:
            switch(type) {
                case IIO_EV_TYPE_THRESH:
                    if(info == IIO_EV_INFO_VALUE)
                        return write_als_threshold(ddata, dir, val, val2);
            }
    }
    return -EINVAL;
}

static int read_als_threshold(struct ap3216c_dev *ddata,
                                enum iio_event_direction dir,
                                int *val, int *val2)
{
    u8 low_reg, high_reg;
    int lower_byte, higher_byte;
    int ret = 0;

    switch (dir) {
        case IIO_EV_DIR_RISING:
            low_reg = AP3216C_ALS_HIGH_THRESHOLD_LOW_REG;
            high_reg = AP3216C_ALS_HIGH_THRESHOLD_HIGH_REG;
        break;

        case IIO_EV_DIR_FALLING:
            low_reg = AP3216C_ALS_LOW_THRESHOLD_LOW_REG;
            high_reg = AP3216C_ALS_LOW_THRESHOLD_HIGH_REG;
        break;

        default:
            return -EINVAL;
    }

    mutex_lock(&ddata->lock);

    lower_byte = i2c_smbus_read_byte_data(ddata->client, low_reg);
    if (lower_byte < 0) {
        ret = lower_byte;
        goto out_unlock;
    }

    higher_byte = i2c_smbus_read_byte_data(ddata->client, high_reg);
    if (higher_byte < 0) {
        ret = higher_byte;
        goto out_unlock;
    }

    *val = (higher_byte << 8) | lower_byte;
    *val2 = 0;

out_unlock:
    mutex_unlock(&ddata->lock);
    return ret;
}

static int ap3216c_read_event_value(struct iio_dev *indio_dev,
        const struct iio_chan_spec *chan, enum iio_event_type type,
        enum iio_event_direction dir, enum iio_event_info info, int *val, int *val2) {
    
    struct ap3216c_dev *ddata = iio_priv(indio_dev);
    int ret;
    switch(chan->address) {
        case AP3216C_CHANNEL_ALS:
            switch(type) {
                case IIO_EV_TYPE_THRESH:
                    if(info == IIO_EV_INFO_VALUE){
                        ret = read_als_threshold(ddata, dir, val, val2);
                        if(ret < 0)
                            return ret;
                        else
                            return IIO_VAL_INT;
                    }
            }
    }
    return -EINVAL;
}

static int ap3216c_read_raw(struct iio_dev *indio_dev,
        struct iio_chan_spec const *chan, int *val, int *val2, long mask) {

    int ret;
    struct ap3216c_dev *ddata = iio_priv(indio_dev);
        switch(chan->address) {
            case AP3216C_CHANNEL_ALS:
                switch(mask) {
                    case IIO_CHAN_INFO_RAW:
                        ret = read_als_data(ddata, val);
                        if(ret < 0) {
                            return ret;
                        }
                        return IIO_VAL_INT;
                    case IIO_CHAN_INFO_SCALE:
                        ret = read_als_scale_microlux(ddata, val, val2);
                        if(ret < 0) {
                            return ret;
                        }
                        return IIO_VAL_INT_PLUS_MICRO;
                    default:
                        return -EINVAL;
                }
                return IIO_VAL_INT;
            case AP3216C_CHANNEL_PS:
                // read_ps(ddata);
                return IIO_VAL_INT;
            case AP3216C_CHANNEL_IR:
                // read_ir(ddata);
                return IIO_VAL_INT;
            default:
                return -EINVAL;
        }
    return -EINVAL;
}

static int ap3216c_write_raw(struct iio_dev *indio_dev,
        struct iio_chan_spec const *chan, int val, int val2, long mask) {
    
    int ret;
    struct ap3216c_dev *ddata = iio_priv(indio_dev);
        switch(chan->address) {
            case AP3216C_CHANNEL_ALS:
                switch(mask) {
                    case IIO_CHAN_INFO_SCALE:
                        return write_als_scale_microlux(ddata, val, val2);
                }
                return -EINVAL;
            case AP3216C_CHANNEL_PS:
                // write_ps(ddata);
                return -EINVAL;
            case AP3216C_CHANNEL_IR:
                // write_ir(ddata);
                return -EINVAL;
            default:
                return -EINVAL;
        }
    return -EINVAL;
}

static int ap3216c_write_raw_get_fmt(struct iio_dev *indio_dev,
        struct iio_chan_spec const *chan, long mask) {
    
    struct ap3216c_dev *ddata = iio_priv(indio_dev);
    switch(chan->address) {
        case AP3216C_CHANNEL_ALS:
            switch(mask) {
                case IIO_CHAN_INFO_SCALE:
                    return IIO_VAL_INT_PLUS_MICRO;
                case IIO_CHAN_INFO_RAW:
                    return IIO_VAL_INT;
                default:
                    return -EINVAL;
            }
    }
    return -EINVAL;
}

/* attribute mode: show and store the mode */
static ssize_t ap3216c_show_mode(struct device *dev,
        struct device_attribute *attr, char *buf) {
    
    struct iio_dev *indio_dev = dev_to_iio_dev(dev);
    struct ap3216c_dev *ddata = iio_priv(indio_dev);
    struct i2c_client *client = ddata->client;
    int ret;

    ret = i2c_smbus_read_byte_data(client, AP3216C_SYSTEM_CONFIGURATION_REG);
    if(ret < 0) {
        return ret;
    }

    ret &= AP3216C_MODE_MASK;
    return sysfs_emit(buf, "%d\n", ret);
}

static ssize_t ap3216c_store_mode(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count) {
    
    struct iio_dev *indio_dev = dev_to_iio_dev(dev);
    struct ap3216c_dev *ddata = iio_priv(indio_dev);
    u8 mode;
    int ret;

    ret = kstrtou8(buf, 0, &mode);
    if(ret < 0) {
        return ret;
    }

    ret = ap3216c_update_field(ddata, AP3216C_SYSTEM_CONFIGURATION_REG,
                    AP3216C_MODE_MASK, mode);
    if(ret < 0) {
        return ret;
    }

    return count;
}

static IIO_DEVICE_ATTR(mode, 0644, ap3216c_show_mode, ap3216c_store_mode, 0); /* attribute mode: show and store the mode */

/* attribute persistence: show and store ALS persistence time */
static ssize_t ap3216c_show_als_persistence(struct device *dev,
        struct device_attribute *attr, char *buf) {
    
    struct iio_dev *indio_dev = dev_to_iio_dev(dev);
    struct ap3216c_dev *ddata = iio_priv(indio_dev);
    struct i2c_client *client = ddata->client;
    int ret;

    ret = i2c_smbus_read_byte_data(client, AP3216C_ALS_CONFIGURATION_REG);
    if(ret < 0) {
        return ret;
    }

    ret &= AP3216C_ALS_CONF_PERSIST_MASK;
    return sysfs_emit(buf, "%d\n", ret);
}

static ssize_t ap3216c_store_als_persistence(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count) {
    
    struct iio_dev *indio_dev = dev_to_iio_dev(dev);
    struct ap3216c_dev *ddata = iio_priv(indio_dev);
    u8 als_persistence;
    int ret;

    ret = kstrtou8(buf, 0, &als_persistence);
    if(ret < 0) {
        return ret;
    }

    for (int i = 0; i < ARRAY_SIZE(ap3216c_als_persistence); i++) {
        if (als_persistence == ap3216c_als_persistence[i].value){
            ret = ap3216c_update_field(ddata, AP3216C_ALS_CONFIGURATION_REG,
                AP3216C_ALS_CONF_PERSIST_MASK, ap3216c_als_persistence[i].reg_code);
            if(ret < 0) {
                return ret;
            }
            return count;
        }
    }
    return -EINVAL;
}

static IIO_DEVICE_ATTR(als_persistence, 0644, ap3216c_show_als_persistence, ap3216c_store_als_persistence, 0);/* attribute persistence: show and store ALS persistence time */

/*hard interrupt handler*/
static irqreturn_t ap3216c_irq_handler(int irq, void *dev_id) {
    /* placeholder for hard interrupt handler*/
    struct iio_dev *indio_dev = dev_id;
    struct ap3216c_dev *ddata = iio_priv(indio_dev);
    ddata->irq_timestamp = iio_get_time_ns(indio_dev);
    return IRQ_WAKE_THREAD;
}

/* thread interrupt handler */
static irqreturn_t ap3216c_irq_thread_fn(int irq, void *dev_id) {
    struct iio_dev *indio_dev = dev_id;
    struct ap3216c_dev *ddata = iio_priv(indio_dev);
    int irq_status, ret;

    mutex_lock(&ddata->lock);
    irq_status = i2c_smbus_read_byte_data(ddata->client, AP3216C_INTERRUPT_STATUS_REG);
    if(irq_status < 0) {
        mutex_unlock(&ddata->lock);
        dev_err_ratelimited(&ddata->client->dev,
            "failed to read interrupt status: %d\n",
            irq_status);
        return IRQ_HANDLED;
    }

    /* clear the interrupt status register AP3216C_INTERRUPT_STATUS_REG.
     * There are two ways to clear the interrupt status:
     * 1. Write 1 to the interrupt status register AP3216C_CLEAR_INT_REG. Then write
     * mask to the interrupt status register AP3216C_INTERRUPT_STATUS_REG.
     * 2. Read the data register(AP3216C_ALS_DATA_*_REG or AP3216C_PS_DATA_*_REG) if
     * the AP3216C_CLEAR_INT_REG is 0, the interrupt status will be cleared automatically.
    */
    /* We use No.1 method to clear the interrupt status */
    ret = i2c_smbus_write_byte_data(ddata->client, AP3216C_CLEAR_INT_REG, 1);
    if(irq_status & AP3216C_ALS_INT_MASK) {
        ret |=i2c_smbus_write_byte_data(ddata->client, AP3216C_INTERRUPT_STATUS_REG, AP3216C_ALS_INT_MASK);
        iio_push_event(indio_dev,
            IIO_UNMOD_EVENT_CODE(IIO_LIGHT,
                            0,
                            IIO_EV_TYPE_THRESH,
                            IIO_EV_DIR_EITHER),
                            ddata->irq_timestamp);
    }else if(irq_status & AP3216C_PS_INT_MASK) {
        i2c_smbus_write_byte_data(ddata->client, AP3216C_INTERRUPT_STATUS_REG, AP3216C_PS_INT_MASK);
        /* placeholder for PS interrupt handler*/
    }

    if(ret) {
        dev_err_ratelimited(&ddata->client->dev,
            "failed to clear interrupt status: %d\n",
            ret);
    }

    mutex_unlock(&ddata->lock);
    return IRQ_HANDLED;
}

static struct attribute *ap3216c_attributes[] = {
    &iio_dev_attr_mode.dev_attr.attr,
    &iio_dev_attr_als_persistence.dev_attr.attr,
    NULL,
};

static struct attribute_group ap3216c_attr_group = {
    .attrs = ap3216c_attributes,
};

static const struct iio_info ap3216c_info = {
    .attrs = &ap3216c_attr_group,
    .write_raw_get_fmt = ap3216c_write_raw_get_fmt,/* only be used to set the format of the raw_data() function */
    .read_raw = ap3216c_read_raw,
    .write_raw = ap3216c_write_raw,
    .write_event_value = ap3216c_write_event_value,
    .read_event_value = ap3216c_read_event_value,
};

static const struct iio_event_spec ap3216c_event_spec[] = {
    {
        .type = IIO_EV_TYPE_THRESH,
        .dir = IIO_EV_DIR_RISING,
        .mask_separate = BIT(IIO_EV_INFO_VALUE),
    },
    {
        .type = IIO_EV_TYPE_THRESH,
        .dir = IIO_EV_DIR_FALLING,
        .mask_separate = BIT(IIO_EV_INFO_VALUE),
    },
};

static const struct iio_chan_spec ap3216c_channels[] = {
    {
        .type = IIO_LIGHT,
        .channel = 0,
        .address = AP3216C_CHANNEL_ALS,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
        .event_spec = ap3216c_event_spec,
        .num_event_specs = ARRAY_SIZE(ap3216c_event_spec),
    },
    {
        .type = IIO_PROXIMITY,
        .channel = 0,
        .address = AP3216C_CHANNEL_PS,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    },
    {
        .type = IIO_INTENSITY,
        .channel = 0,
        .modified = 1,
        .channel2 = IIO_MOD_LIGHT_IR,
        .address = AP3216C_CHANNEL_IR,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    },
};

static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    struct device *dev = &client->dev;
    if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(dev, "I2C adapter doesn't support I2C functionality\n");
        return -EOPNOTSUPP;
    }

    int ret;
    struct ap3216c_dev *ddata;
    struct iio_dev *indio_dev;
    /* Use IIO device api to request IIO dev object and request extra memory for the device private data */
    indio_dev = devm_iio_device_alloc(dev, sizeof(struct ap3216c_dev));
    if(!indio_dev) {
        dev_err(dev, "Failed to allocate IIO devicememory\n");
        return -ENOMEM;
    }

    /* initialize the device private data */
    ddata = iio_priv(indio_dev);
    ddata->client = client;

    /* initialize the IIO device */
    indio_dev->info = &ap3216c_info;
    indio_dev->name = "ap3216c";
    indio_dev->modes = INDIO_DIRECT_MODE;
    indio_dev->channels = ap3216c_channels;
    indio_dev->num_channels = ARRAY_SIZE(ap3216c_channels);
    indio_dev->dev.parent = dev;

    /* register the device private data to the client */
    i2c_set_clientdata(client, indio_dev);
    mutex_init(&ddata->lock);

    /* register the IIO device */
    ret = devm_iio_device_register(dev, indio_dev);
    if (ret)
        return dev_err_probe(dev, ret, "failed to register IIO device\n");
    
    ret = devm_request_threaded_irq(dev, client->irq, ap3216c_irq_handler, ap3216c_irq_thread_fn,
                                    IRQF_ONESHOT, "ap3216c", indio_dev);
    if (ret)
        return dev_err_probe(dev, ret, "failed to request IRQ\n");

    return 0;
}

static int ap3216c_remove(struct i2c_client *client) {
    return 0;
}
/* Platform Data ID table */
static const struct i2c_device_id ap3216c_id[] = {
    { .name = "ap3216c", .driver_data = AP3216_DEVICE_ID },
    { }
};

/* Device Tree ID table */
static const struct of_device_id ap3216c_DT_id[] = {
    { .compatible = "smartchai,ap3216c", .data = &ap3216c_chip},
    { }
};

static struct i2c_driver ap3216c_driver = {
    .driver = {
        .name = "ap3216c",
        .of_match_table = ap3216c_DT_id,
    },
    .probe = ap3216c_probe,
    .remove = ap3216c_remove,
    .id_table = ap3216c_id,
};

module_i2c_driver(ap3216c_driver);

MODULE_AUTHOR("Jin Xie");
MODULE_DESCRIPTION("AP3216C Light, Proximity, and Infrared Sensor Driver");
MODULE_LICENSE("GPL v2");