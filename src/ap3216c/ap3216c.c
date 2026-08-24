#include <linux/i2c.h>

#include "ap3216c.h"

static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    
}

static int ap3216c_remove(struct i2c_client *client) {
    return 0;
}
/* Platform Data ID table */
static const struct i2c_device_id ap3216c_id[] = {
    { .name = "ap3216c", .driver_data = NULL },
    { }
};

/* Device Tree ID table */
static const struct of_device_id ap3216c_DT_id[] = {
    { .compatible = "smartChai,ap3216c"},
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