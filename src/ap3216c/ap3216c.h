#ifndef AP3216C_H
#define AP3216C_H

/*User space available definitions*/

/*Kernel space available definitions*/
#ifdef __KERNEL__

/* System Register Table*/
#define AP3216C_SYSTEM_CONFIGURATION_REG        0x00
#define AP3216C_INTERRUPT_STATUS_REG            0x01
#define AP3216C_CLEAR_INT_REG                   0x02
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

enum ap3216c_channel_addr {
    AP3216C_CHANNEL_ALS,
    AP3216C_CHANNEL_PS,
    AP3216C_CHANNEL_IR,
};

struct ap3216c_dev {
    struct i2c_client *client;  /* i2c device model */
    struct mutex mixer_placeholder;    /* placeholder for the mixer */

    int irq;                    /* interrupt */
    u8 mode;                    /* working mode */
    u16 als_data;               /* ambient light sensor data */
    u16 ps_data;                /* proximity sensor data */
    u16 ir_data;                /* infrared sensor data */
};

#define AP3216_DEVICE_ID 0x00
static const struct ap3216c_chip_info ap3216c_chip = {
    .id = AP3216_DEVICE_ID,
};

#endif /* __KERNEL__ */

#endif /* AP3216C_H */