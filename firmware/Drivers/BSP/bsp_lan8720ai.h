#ifndef BSP_LAN8720AI_H
#define BSP_LAN8720AI_H

#include <stdbool.h>
#include <stdint.h>

typedef bool (*bsp_lan8720ai_mdio_read_fn)(uint8_t phy_addr, uint8_t reg_addr, uint16_t *value);
typedef bool (*bsp_lan8720ai_mdio_write_fn)(uint8_t phy_addr, uint8_t reg_addr, uint16_t value);
typedef void (*bsp_lan8720ai_delay_ms_fn)(uint32_t delay_ms);

typedef enum
{
    BSP_LAN8720AI_SPEED_10M = 10,
    BSP_LAN8720AI_SPEED_100M = 100
} bsp_lan8720ai_speed_t;

typedef struct
{
    bool link_up;
    bool full_duplex;
    bsp_lan8720ai_speed_t speed_mbps;
    bool auto_negotiation_complete;
} bsp_lan8720ai_link_state_t;

void bsp_lan8720ai_set_phy_addr(uint8_t phy_addr);
uint8_t bsp_lan8720ai_get_phy_addr(void);

void bsp_lan8720ai_set_mdio_read(bsp_lan8720ai_mdio_read_fn read_fn);
void bsp_lan8720ai_set_mdio_write(bsp_lan8720ai_mdio_write_fn write_fn);
void bsp_lan8720ai_set_delay(bsp_lan8720ai_delay_ms_fn delay_fn);

bool bsp_lan8720ai_read_reg(uint8_t reg_addr, uint16_t *value);
bool bsp_lan8720ai_write_reg(uint8_t reg_addr, uint16_t value);

bool bsp_lan8720ai_probe(void);
bool bsp_lan8720ai_soft_reset(uint32_t max_polls);
bool bsp_lan8720ai_start_auto_negotiation(void);
bool bsp_lan8720ai_wait_auto_negotiation(uint32_t max_polls, uint32_t poll_delay_ms);
bool bsp_lan8720ai_get_link_state(bsp_lan8720ai_link_state_t *state);
bool bsp_lan8720ai_init(uint32_t reset_polls, uint32_t aneg_polls, uint32_t poll_delay_ms);

#endif
