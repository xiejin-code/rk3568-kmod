#include <linux/i2c.h>

#include "ap3216c.h"

static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    struct device *dev = &client->dev;
    if(i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(dev, "I2C adapter doesn't support I2C functionality\n");
        return -EOPNOTSUPP;
    }

    struct ap3216c_dev *ddata;
    /* Allocate memory for the device private data */
    ddata = devm_kzalloc(dev, sizeof(struct ap3216c_dev, GFP_KERNEL));
    if(!ddata) {
        dev_err(dev, "Failed to allocate memory\n");
        return -ENOMEM;
    }

    /* initialize the device private data */
    ddata->client = client;

    /* register the device private data to the client */
    i2c_set_clientdata(client, ddata);
    mutex_init(&ddata->mixer_placeholder);
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

MODULE_AUTHOR("Jin Xie")
MODULE_DESCRIPTION("AP3216C Light, Proximity, and Infrared Sensor Driver")
MODULE_LICENSE("GPL v2");