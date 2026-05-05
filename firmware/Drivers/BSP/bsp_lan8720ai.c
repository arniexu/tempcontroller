#include "bsp_lan8720ai.h"

#ifndef APP_ETH_PHY_LAN8720AI
#define APP_ETH_PHY_LAN8720AI (0U)
#endif

#if (APP_ETH_PHY_LAN8720AI == 1U)

#include <stddef.h>

#ifndef BSP_ETH_PHY_ADDR
#define BSP_ETH_PHY_ADDR (0x00U)
#endif

#define LAN8720_REG_BMCR                 (0x00U)
#define LAN8720_REG_BMSR                 (0x01U)
#define LAN8720_REG_PHYID1               (0x02U)
#define LAN8720_REG_PHYID2               (0x03U)
#define LAN8720_REG_SCSR                 (0x1FU)

#define LAN8720_BMCR_RESET               (0x8000U)
#define LAN8720_BMCR_AUTONEG_ENABLE      (0x1000U)
#define LAN8720_BMCR_RESTART_AUTONEG     (0x0200U)

#define LAN8720_BMSR_AUTONEG_COMPLETE    (0x0020U)
#define LAN8720_BMSR_LINK_STATUS         (0x0004U)

#define LAN8720_PHYID1_EXPECTED          (0x0007U)
#define LAN8720_PHYID2_MASK              (0xFFF0U)
#define LAN8720_PHYID2_EXPECTED          (0xC0F0U)

#define LAN8720_SCSR_SPEED_DUPLEX_MASK   (0x001CU)
#define LAN8720_SCSR_10_HALF             (0x0004U)
#define LAN8720_SCSR_10_FULL             (0x0008U)
#define LAN8720_SCSR_100_HALF            (0x0014U)
#define LAN8720_SCSR_100_FULL            (0x0018U)

static uint8_t s_phy_addr = (uint8_t)BSP_ETH_PHY_ADDR;
static bsp_lan8720ai_mdio_read_fn s_mdio_read = NULL;
static bsp_lan8720ai_mdio_write_fn s_mdio_write = NULL;
static bsp_lan8720ai_delay_ms_fn s_delay_ms = NULL;

static bool read_reg(uint8_t reg_addr, uint16_t *value)
{
    if ((s_mdio_read == NULL) || (value == NULL))
    {
        return false;
    }

    return s_mdio_read(s_phy_addr, reg_addr, value);
}

static bool write_reg(uint8_t reg_addr, uint16_t value)
{
    if (s_mdio_write == NULL)
    {
        return false;
    }

    return s_mdio_write(s_phy_addr, reg_addr, value);
}

void bsp_lan8720ai_set_phy_addr(uint8_t phy_addr)
{
    s_phy_addr = phy_addr;
}

uint8_t bsp_lan8720ai_get_phy_addr(void)
{
    return s_phy_addr;
}

void bsp_lan8720ai_set_mdio_read(bsp_lan8720ai_mdio_read_fn read_fn)
{
    s_mdio_read = read_fn;
}

void bsp_lan8720ai_set_mdio_write(bsp_lan8720ai_mdio_write_fn write_fn)
{
    s_mdio_write = write_fn;
}

void bsp_lan8720ai_set_delay(bsp_lan8720ai_delay_ms_fn delay_fn)
{
    s_delay_ms = delay_fn;
}

bool bsp_lan8720ai_read_reg(uint8_t reg_addr, uint16_t *value)
{
    return read_reg(reg_addr, value);
}

bool bsp_lan8720ai_write_reg(uint8_t reg_addr, uint16_t value)
{
    return write_reg(reg_addr, value);
}

bool bsp_lan8720ai_probe(void)
{
    uint16_t id1 = 0U;
    uint16_t id2 = 0U;

    if (!read_reg(LAN8720_REG_PHYID1, &id1))
    {
        return false;
    }

    if (!read_reg(LAN8720_REG_PHYID2, &id2))
    {
        return false;
    }

    if (id1 != LAN8720_PHYID1_EXPECTED)
    {
        return false;
    }

    return ((id2 & LAN8720_PHYID2_MASK) == LAN8720_PHYID2_EXPECTED);
}

bool bsp_lan8720ai_soft_reset(uint32_t max_polls)
{
    uint16_t bmcr = 0U;
    uint32_t polls = 0U;

    if (!read_reg(LAN8720_REG_BMCR, &bmcr))
    {
        return false;
    }

    bmcr |= LAN8720_BMCR_RESET;
    if (!write_reg(LAN8720_REG_BMCR, bmcr))
    {
        return false;
    }

    while (polls < max_polls)
    {
        if (!read_reg(LAN8720_REG_BMCR, &bmcr))
        {
            return false;
        }

        if ((bmcr & LAN8720_BMCR_RESET) == 0U)
        {
            return true;
        }

        if (s_delay_ms != NULL)
        {
            s_delay_ms(1U);
        }
        polls++;
    }

    return false;
}

bool bsp_lan8720ai_start_auto_negotiation(void)
{
    uint16_t bmcr = 0U;

    if (!read_reg(LAN8720_REG_BMCR, &bmcr))
    {
        return false;
    }

    bmcr |= (uint16_t)(LAN8720_BMCR_AUTONEG_ENABLE | LAN8720_BMCR_RESTART_AUTONEG);
    return write_reg(LAN8720_REG_BMCR, bmcr);
}

bool bsp_lan8720ai_wait_auto_negotiation(uint32_t max_polls, uint32_t poll_delay_ms)
{
    uint16_t bmsr = 0U;
    uint32_t polls = 0U;

    while (polls < max_polls)
    {
        if (!read_reg(LAN8720_REG_BMSR, &bmsr))
        {
            return false;
        }

        if ((bmsr & LAN8720_BMSR_AUTONEG_COMPLETE) != 0U)
        {
            return true;
        }

        if ((poll_delay_ms > 0U) && (s_delay_ms != NULL))
        {
            s_delay_ms(poll_delay_ms);
        }
        polls++;
    }

    return false;
}

bool bsp_lan8720ai_get_link_state(bsp_lan8720ai_link_state_t *state)
{
    uint16_t bmsr = 0U;
    uint16_t scsr = 0U;
    uint16_t speed_duplex = 0U;

    if (state == NULL)
    {
        return false;
    }

    if (!read_reg(LAN8720_REG_BMSR, &bmsr))
    {
        return false;
    }

    /* BMSR link bit is latch-low, read twice for current status. */
    if (!read_reg(LAN8720_REG_BMSR, &bmsr))
    {
        return false;
    }

    state->link_up = ((bmsr & LAN8720_BMSR_LINK_STATUS) != 0U);
    state->auto_negotiation_complete = ((bmsr & LAN8720_BMSR_AUTONEG_COMPLETE) != 0U);
    state->speed_mbps = BSP_LAN8720AI_SPEED_10M;
    state->full_duplex = false;

    if (!state->link_up)
    {
        return true;
    }

    if (!read_reg(LAN8720_REG_SCSR, &scsr))
    {
        return false;
    }

    speed_duplex = (uint16_t)(scsr & LAN8720_SCSR_SPEED_DUPLEX_MASK);

    if ((speed_duplex == LAN8720_SCSR_100_FULL) || (speed_duplex == LAN8720_SCSR_100_HALF))
    {
        state->speed_mbps = BSP_LAN8720AI_SPEED_100M;
    }
    else
    {
        state->speed_mbps = BSP_LAN8720AI_SPEED_10M;
    }

    state->full_duplex = ((speed_duplex == LAN8720_SCSR_100_FULL) ||
                          (speed_duplex == LAN8720_SCSR_10_FULL));

    return true;
}

bool bsp_lan8720ai_init(uint32_t reset_polls, uint32_t aneg_polls, uint32_t poll_delay_ms)
{
    if (!bsp_lan8720ai_probe())
    {
        return false;
    }

    if (!bsp_lan8720ai_soft_reset(reset_polls))
    {
        return false;
    }

    if (!bsp_lan8720ai_start_auto_negotiation())
    {
        return false;
    }

    return bsp_lan8720ai_wait_auto_negotiation(aneg_polls, poll_delay_ms);
}

#endif
