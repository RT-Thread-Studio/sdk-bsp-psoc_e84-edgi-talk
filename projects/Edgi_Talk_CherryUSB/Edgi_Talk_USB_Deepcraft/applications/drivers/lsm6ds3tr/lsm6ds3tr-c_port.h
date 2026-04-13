#ifndef APP_LSM6DS3TR_C_PORT_H
#define APP_LSM6DS3TR_C_PORT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float acc_mg[3];
    float gyro_mdps[3];
    float temp_degC;
    uint8_t valid_mask;
} lsm6ds3tr_sample_t;

#define LSM6DS3TR_SAMPLE_VALID_ACC  (1U << 0)
#define LSM6DS3TR_SAMPLE_VALID_GYRO (1U << 1)
#define LSM6DS3TR_SAMPLE_VALID_TEMP (1U << 2)

typedef struct
{
    int sample_rate_hz;
    int accel_range_g;
    int gyro_range_dps;
} lsm6ds3tr_port_config_t;

#define LSM6DS3TR_DEFAULT_RATE_HZ        50
#define LSM6DS3TR_DEFAULT_ACCEL_RANGE_G  2
#define LSM6DS3TR_DEFAULT_GYRO_RANGE_DPS 2000

int lsm6ds3tr_port_init(void);
bool lsm6ds3tr_port_is_ready(void);
int lsm6ds3tr_port_configure(const lsm6ds3tr_port_config_t *config);
int lsm6ds3tr_port_read_sample(lsm6ds3tr_sample_t *sample);

#endif
