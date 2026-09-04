#ifndef AP3216C_H
#define AP3216C_H

/*User space available definitions*/

/*Kernel space available definitions*/
#ifdef __KERNEL__

/* System Register Table*/
#define AP3216C_SYSTEM_CONFIGURATION_REG        0x00
#define AP3216C_INTERRUPT_STATUS_REG            0x01
#define AP3216C_CLEAR_INT_MANNER_REG            0x02
#define AP3216C_IR_DATA_LOW_REG                 0x0A
#define AP3216C_IR_DATA_HIGH_REG                0x0B
#define AP3216C_ALS_DATA_LOW_REG                0x0C
#define AP3216C_ALS_DATA_HIGH_REG               0x0D
#define AP3216C_PS_DATA_LOW_REG                 0x0E
#define AP3216C_PS_DATA_HIGH_REG                0x0F

/* ALS Register Table*/
#define AP3216C_ALS_CONFIGURATION_REG           0x10
#define AP3216C_ALS_CALIBRATION_REG             0x19
#define AP3216C_ALS_LOW_THRESHOLD_LOW_REG       0x1A
#define AP3216C_ALS_LOW_THRESHOLD_HIGH_REG      0x1B
#define AP3216C_ALS_HIGH_THRESHOLD_LOW_REG      0x1C
#define AP3216C_ALS_HIGH_THRESHOLD_HIGH_REG     0x1D

/* PS Register Table*/
#define AP3216C_PS_CONFIGURATION_REG            0x20
#define AP3216C_PS_LED_DRIVER_REG               0x21
#define AP3216C_PS_INT_FORM_REG                 0x22
#define AP3216C_PS_MEAN_TIME_REG                0x23
#define AP3216C_PS_LED_WAITING_TIME_REG         0x24
#define AP3216C_PS_CALIBRATION_LOW_REG          0x28
#define AP3216C_PS_CALIBRATION_HIGH_REG         0x29
#define AP3216C_PS_LOW_THRESHOLD_LOW_REG        0x2A    /* bit 2:0 */
#define AP3216C_PS_LOW_THRESHOLD_HIGH_REG       0x2B    /* bit 10:3 */
#define AP3216C_PS_HIGH_THRESHOLD_LOW_REG       0x2C    /* bit 2:0 */
#define AP3216C_PS_HIGH_THRESHOLD_HIGH_REG      0x2D    /* bit 10:3 */

/* mask/value for mode*/
#define AP3216C_MODE_MASK                       GENMASK(2, 0)
#define AP3216C_ALS_CONF_PERSIST_MASK           GENMASK(3, 0)
#define AP3216C_ALS_CONF_RANGE_MASK             GENMASK(5, 4)
#define AP3216C_IR_DATA_LOW_MASK                GENMASK(1, 0)
#define AP3216C_PS_DATA_LOW_MASK                GENMASK(3, 0)
#define AP3216C_PS_DATA_HIGH_MASK               GENMASK(5, 0)
#define AP3216C_PS_THRESHOLD_LOW_MASK           GENMASK(1, 0)
#define AP3216C_PS_IROF_MASK                    BIT(6)
#define AP3216C_PS_OBJ_MASK                     BIT(7)
#define AP3216C_ALS_INT_MASK                    BIT(0)
#define AP3216C_PS_INT_MASK                     BIT(1)
#define AP3216C_CLEAR_INT_MANNER_MASK           BIT(0)

enum ap3216c_channel_addr {
    AP3216C_CHANNEL_ALS,
    AP3216C_CHANNEL_PS,
    AP3216C_CHANNEL_IR,
};

struct ap3216c_dev {
    struct i2c_client *client;  /* i2c device model */
    struct mutex lock;    /* lock for the device */
    s64 irq_timestamp;        /* timestamp for the interrupt */
};

#define AP3216_DEVICE_ID 0x00

#endif /* __KERNEL__ */

#endif /* AP3216C_H */