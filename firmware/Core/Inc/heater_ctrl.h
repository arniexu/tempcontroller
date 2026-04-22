#ifndef HEATER_CTRL_H
#define HEATER_CTRL_H

#include <stdbool.h>
#include <stdint.h>

void heater_ctrl_init(uint32_t window_ms);
void heater_ctrl_set_output_percent(float percent);
void heater_ctrl_update_1ms(void);
bool heater_ctrl_get_state(void);
void heater_ctrl_force_off(void);

#endif
