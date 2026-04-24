#ifndef BSP_KEY_H
#define BSP_KEY_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BSP_KEY_SET = 0,
    BSP_KEY_UP,
    BSP_KEY_DOWN,
    BSP_KEY_BACK,
    BSP_KEY_COUNT
} bsp_key_id_t;

void bsp_key_init(void);
bool bsp_key_get_state(bsp_key_id_t key);
void bsp_key_mock_set_state(bsp_key_id_t key, bool pressed);

#endif
