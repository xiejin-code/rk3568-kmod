#include <linux/i2c.h>

#include "ap3216c.h"

static int ap3216c_update_field(struct ap3216c_dev *ddata, int reg, int mask, int value) {
    
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

static int ap3216c_write_raw(struct iio_dev *indio_dev,
        struct iio_chan_spec const *chan, int val, int val2, long mask) {
    
    int ret;
    struct ap3216c_dev *ddata = iio_priv(indio_dev);
        switch(chan->address) {
            case AP3216C_CHANNEL_ALS:
                switch(mask) {
                    case IIO_CHAN_INFO_SCALE:
                        return write_als_scale_microlux(ddata, val, val2);
                    default:
                        return -EINVAL;
                }
                return IIO_VAL_INT;
            case AP3216C_CHANNEL_PS:
                // write_ps(ddata);
                return EINVAL;
            case AP3216C_CHANNEL_IR:
                // write_ir(ddata);
                return EINVAL;
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
    unsigned int mode;
    int ret;

    ret = kstrtouint(buf, 0, &mode);
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

static struct attribute *ap3216c_attributes[] = {
    &iio_dev_attr_mode.dev_attr.attr,
    NULL,
};

static struct attribute_group ap3216c_attr_group = {
    .attrs = ap3216c_attributes,
};

static const struct iio_info ap3216c_info = {
    .attrs = &ap3216c_attr_group,
    .read_raw = ap3216c_read_raw,
    .write_raw_get_fmt = ap3216c_write_raw_get_fmt,
};

static const struct iio_chan_spec ap3216c_channels[] = {
    {
        .type = IIO_LIGHT,
        .address = AP3216C_CHANNEL_ALS,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
    },
    {
        .type = IIO_PROXIMITY,
        .address = AP3216C_CHANNEL_PS,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    },
    {
        .type = IIO_INTENSITY,
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
    /* Use IIO device api to allocate memory for the device private data */
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
    // indio_dev->event_attrs = ap3216c_event_attrs;
    // indio_dev->num_event_attrs = ARRAY_SIZE(ap3216c_event_attrs);

    /* register the device private data to the client */
    i2c_set_clientdata(client, indio_dev);
    mutex_init(&ddata->lock);
    /* register the IIO device */
    ret = devm_iio_device_register(dev, indio_dev);
    if (ret)
        return dev_err_probe(dev, ret, "failed to register IIO device\n");

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
    { .compatible = "smartChai,ap3216c", .data = &ap3216c_chip},
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