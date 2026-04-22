#ifndef TEMP_MANAGER_H
#define TEMP_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float t1;
    float t2;
    float t3;
    float t_ctrl;
    uint8_t valid_mask;
    bool sensor_degraded;
    bool sensor_fault;
} temp_snapshot_t;

void temp_manager_init(void);
void temp_manager_update(void);
const temp_snapshot_t *temp_manager_get_snapshot(void);

#endif
