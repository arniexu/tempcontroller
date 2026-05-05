#include "network_service.h"

#include "app_config.h"

#if (APP_USE_ETHERNET == 1U)

#include "bsp_lan8720ai.h"
#include "debug_log.h"
#include "scheduler.h"

#if defined(STM32F4xx) || defined(STM32F407xx)
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"
#include "lwip/pbuf.h"
#include "lwip/ip4_addr.h"
#include "stm32f1xx_hal.h"
#include <string.h>

#ifndef BSP_ETH_AUTONEG_TIMEOUT_POLLS
#define BSP_ETH_AUTONEG_TIMEOUT_POLLS   (5000U)
#endif

#define NETWORK_PHY_RESET_POLLS         (200U)
#define NETWORK_LINK_CHECK_PERIOD_MS    (1000U)
#define NETWORK_TX_TIMEOUT_MS           (100U)

typedef struct
{
    ETH_BufferTypeDef list;
    uint8_t *dma_buf;
} rx_chain_node_t;

static ETH_HandleTypeDef g_eth;
static ETH_DMADescTypeDef g_tx_desc[ETH_TX_DESC_CNT];
static ETH_DMADescTypeDef g_rx_desc[ETH_RX_DESC_CNT];
static uint8_t g_tx_buffer[ETH_MAX_PACKET_SIZE];
static uint8_t g_rx_dma_buffer[ETH_RX_DESC_CNT][ETH_MAX_PACKET_SIZE];
static rx_chain_node_t g_rx_nodes[ETH_RX_DESC_CNT];
static uint32_t g_rx_alloc_index = 0U;

static struct netif g_netif;
static bool g_netif_added = false;
static bool g_network_ready = false;
static bool g_link_up = false;
static bool g_full_duplex = false;
static uint16_t g_speed_mbps = 0U;
static uint32_t g_last_link_check_ms = 0U;

static uint8_t g_mac_addr[6] = {0x02U, 0x80U, 0xE1U, 0x00U, 0x00U, 0x01U};

static ETH_TxPacketConfigTypeDef g_tx_packet_cfg;

static rx_chain_node_t *find_rx_node(uint8_t *dma_buf)
{
    uint32_t i;

    for (i = 0U; i < ETH_RX_DESC_CNT; ++i)
    {
        if (g_rx_nodes[i].dma_buf == dma_buf)
        {
            return &g_rx_nodes[i];
        }
    }

    return NULL;
}

static bool mdio_read_cb(uint8_t phy_addr, uint8_t reg_addr, uint16_t *value)
{
    uint32_t reg_value = 0U;

    if (value == NULL)
    {
        return false;
    }

    if (HAL_ETH_ReadPHYRegister(&g_eth, phy_addr, reg_addr, &reg_value) != HAL_OK)
    {
        return false;
    }

    *value = (uint16_t)reg_value;
    return true;
}

static bool mdio_write_cb(uint8_t phy_addr, uint8_t reg_addr, uint16_t value)
{
    return (HAL_ETH_WritePHYRegister(&g_eth, phy_addr, reg_addr, value) == HAL_OK);
}

static void delay_cb(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
    if (buff == NULL)
    {
        return;
    }

    *buff = g_rx_dma_buffer[g_rx_alloc_index];
    g_rx_alloc_index = (g_rx_alloc_index + 1U) % ETH_RX_DESC_CNT;
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t length)
{
    rx_chain_node_t *node;

    if ((pStart == NULL) || (pEnd == NULL) || (buff == NULL))
    {
        return;
    }

    node = find_rx_node(buff);
    if (node == NULL)
    {
        return;
    }

    node->list.buffer = buff;
    node->list.len = (uint32_t)length;
    node->list.next = NULL;

    if (*pStart == NULL)
    {
        *pStart = &node->list;
        *pEnd = &node->list;
    }
    else
    {
        ((ETH_BufferTypeDef *)(*pEnd))->next = &node->list;
        *pEnd = &node->list;
    }
}

void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
    (void)buff;
}

void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    GPIO_InitTypeDef gpio = {0};

    if ((heth == NULL) || (heth->Instance != ETH))
    {
        return;
    }

    __HAL_RCC_ETH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    /* RMII: PA1 REF_CLK, PA2 MDIO, PA7 CRS_DV */
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    gpio.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* RMII: PC1 MDC, PC4 RXD0, PC5 RXD1 */
    gpio.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* RMII: PB11 TX_EN, PB12 TXD0, PB13 TXD1 */
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
    HAL_GPIO_Init(GPIOB, &gpio);
}

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    ETH_BufferTypeDef tx_buffer = {0};
    struct pbuf *q;
    uint32_t frame_len = 0U;

    (void)netif;

    for (q = p; q != NULL; q = q->next)
    {
        if ((frame_len + q->len) > sizeof(g_tx_buffer))
        {
            return ERR_MEM;
        }
        (void)memcpy(&g_tx_buffer[frame_len], q->payload, q->len);
        frame_len += q->len;
    }

    tx_buffer.buffer = g_tx_buffer;
    tx_buffer.len = frame_len;
    tx_buffer.next = NULL;

    g_tx_packet_cfg.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
    g_tx_packet_cfg.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
    g_tx_packet_cfg.CRCPadCtrl = ETH_CRC_PAD_INSERT;
    g_tx_packet_cfg.Length = frame_len;
    g_tx_packet_cfg.TxBuffer = &tx_buffer;
    g_tx_packet_cfg.pData = NULL;

    if (HAL_ETH_Transmit(&g_eth, &g_tx_packet_cfg, NETWORK_TX_TIMEOUT_MS) != HAL_OK)
    {
        return ERR_IF;
    }

    return ERR_OK;
}

static void low_level_init(struct netif *netif)
{
    uint32_t i;
    bsp_lan8720ai_link_state_t link_state;

    g_eth.Instance = ETH;
    g_eth.Init.MACAddr = g_mac_addr;
    g_eth.Init.MediaInterface = HAL_ETH_RMII_MODE;
    g_eth.Init.TxDesc = g_tx_desc;
    g_eth.Init.RxDesc = g_rx_desc;
    g_eth.Init.RxBuffLen = ETH_MAX_PACKET_SIZE;

    for (i = 0U; i < ETH_RX_DESC_CNT; ++i)
    {
        g_rx_nodes[i].dma_buf = g_rx_dma_buffer[i];
        g_rx_nodes[i].list.buffer = g_rx_dma_buffer[i];
        g_rx_nodes[i].list.len = 0U;
        g_rx_nodes[i].list.next = NULL;
    }

    if (HAL_ETH_Init(&g_eth) != HAL_OK)
    {
        debug_log_error("NET", "HAL_ETH_Init failed");
        return;
    }

    bsp_lan8720ai_set_phy_addr((uint8_t)BSP_ETH_PHY_ADDR);
    bsp_lan8720ai_set_mdio_read(mdio_read_cb);
    bsp_lan8720ai_set_mdio_write(mdio_write_cb);
    bsp_lan8720ai_set_delay(delay_cb);

    if (!bsp_lan8720ai_init(NETWORK_PHY_RESET_POLLS, BSP_ETH_AUTONEG_TIMEOUT_POLLS, 1U))
    {
        debug_log_warn("NET", "LAN8720AI init failed");
    }

    if (HAL_ETH_Start(&g_eth) != HAL_OK)
    {
        debug_log_error("NET", "HAL_ETH_Start failed");
        return;
    }

    netif->hwaddr_len = ETH_HWADDR_LEN;
    (void)memcpy(netif->hwaddr, g_mac_addr, ETH_HWADDR_LEN);
    netif->mtu = 1500U;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    if (bsp_lan8720ai_get_link_state(&link_state) && link_state.link_up)
    {
        netif_set_link_up(netif);
        netif->flags |= NETIF_FLAG_LINK_UP;
        g_link_up = true;
        g_full_duplex = link_state.full_duplex;
        g_speed_mbps = (uint16_t)link_state.speed_mbps;
    }
    else
    {
        netif_set_link_down(netif);
        netif->flags &= (uint8_t)(~NETIF_FLAG_LINK_UP);
        g_link_up = false;
        g_full_duplex = false;
        g_speed_mbps = 0U;
    }
}

static err_t ethernet_netif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->name[0] = 'e';
    netif->name[1] = '0';
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    low_level_init(netif);
    return ERR_OK;
}

static void ethernetif_input(struct netif *netif)
{
    void *rx_chain = NULL;
    ETH_BufferTypeDef *eth_buf;
    struct pbuf *p;
    struct pbuf *q;
    uint16_t total_len = 0U;
    uint16_t copied = 0U;

    if (HAL_ETH_ReadData(&g_eth, &rx_chain) != HAL_OK)
    {
        return;
    }

    if (rx_chain == NULL)
    {
        return;
    }

    eth_buf = (ETH_BufferTypeDef *)rx_chain;
    while (eth_buf != NULL)
    {
        if ((total_len + eth_buf->len) < total_len)
        {
            return;
        }
        total_len = (uint16_t)(total_len + (uint16_t)eth_buf->len);
        eth_buf = eth_buf->next;
    }

    p = pbuf_alloc(PBUF_RAW, total_len, PBUF_POOL);
    if (p == NULL)
    {
        return;
    }

    eth_buf = (ETH_BufferTypeDef *)rx_chain;
    for (q = p; (q != NULL) && (eth_buf != NULL); q = q->next)
    {
        uint16_t to_copy = q->len;
        uint16_t copied_this_q = 0U;

        while ((eth_buf != NULL) && (copied_this_q < to_copy))
        {
            uint16_t seg_len = (uint16_t)eth_buf->len;
            uint16_t remaining_q = (uint16_t)(to_copy - copied_this_q);
            uint16_t chunk = (seg_len < remaining_q) ? seg_len : remaining_q;

            (void)memcpy((uint8_t *)q->payload + copied_this_q, eth_buf->buffer, chunk);
            copied += chunk;
            copied_this_q += chunk;

            if (chunk == seg_len)
            {
                eth_buf = eth_buf->next;
            }
            else
            {
                eth_buf->buffer += chunk;
                eth_buf->len -= chunk;
            }
        }
    }

    if (copied != total_len)
    {
        pbuf_free(p);
        return;
    }

    if (netif->input(p, netif) != ERR_OK)
    {
        pbuf_free(p);
    }
}

static void update_link_state(struct netif *netif)
{
    bsp_lan8720ai_link_state_t link_state;

    if (!bsp_lan8720ai_get_link_state(&link_state))
    {
        return;
    }

    if (link_state.link_up)
    {
        netif_set_link_up(netif);
        netif_set_up(netif);
        g_link_up = true;
        g_full_duplex = link_state.full_duplex;
        g_speed_mbps = (uint16_t)link_state.speed_mbps;
    }
    else
    {
        netif_set_link_down(netif);
        netif_set_down(netif);
        g_link_up = false;
        g_full_duplex = false;
        g_speed_mbps = 0U;
    }
}

static void init_lwip_stack(void)
{
    ip4_addr_t ipaddr;
    ip4_addr_t netmask;
    ip4_addr_t gw;

    IP4_ADDR(&ipaddr, 192U, 168U, 1U, 50U);
    IP4_ADDR(&netmask, 255U, 255U, 255U, 0U);
    IP4_ADDR(&gw, 192U, 168U, 1U, 1U);

    lwip_init();

    if (netif_add(&g_netif, &ipaddr, &netmask, &gw, NULL, ethernet_netif_init, ethernet_input) == NULL)
    {
        debug_log_error("NET", "netif_add failed");
        return;
    }

    g_netif_added = true;
    netif_set_default(&g_netif);
    netif_set_up(&g_netif);
    update_link_state(&g_netif);
    g_network_ready = true;
    debug_log_info("NET", "lwip stack up");
}

u32_t sys_now(void)
{
    return (u32_t)scheduler_now_ms();
}

void network_service_init(void)
{
    init_lwip_stack();
    g_last_link_check_ms = scheduler_now_ms();
}

void network_service_process(void)
{
    uint32_t now_ms;

    if (!g_netif_added)
    {
        return;
    }

    ethernetif_input(&g_netif);
    sys_check_timeouts();

    now_ms = scheduler_now_ms();
    if ((now_ms - g_last_link_check_ms) >= NETWORK_LINK_CHECK_PERIOD_MS)
    {
        g_last_link_check_ms = now_ms;
        update_link_state(&g_netif);
    }
}

bool network_service_is_ready(void)
{
    return g_network_ready;
}

bool network_service_get_status(network_status_t *status)
{
    if (status == 0)
    {
        return false;
    }

    status->ready = g_network_ready;
    status->link_up = g_link_up;
    status->full_duplex = g_full_duplex;
    status->speed_mbps = g_speed_mbps;
    return true;
}

#else

void network_service_init(void)
{
}

void network_service_process(void)
{
}

bool network_service_is_ready(void)
{
    return false;
}

bool network_service_get_status(network_status_t *status)
{
    if (status == 0)
    {
        return false;
    }

    status->ready = false;
    status->link_up = false;
    status->full_duplex = false;
    status->speed_mbps = 0U;
    return true;
}

#endif

#else

void network_service_init(void)
{
}

void network_service_process(void)
{
}

bool network_service_is_ready(void)
{
    return false;
}

bool network_service_get_status(network_status_t *status)
{
    if (status == 0)
    {
        return false;
    }

    status->ready = false;
    status->link_up = false;
    status->full_duplex = false;
    status->speed_mbps = 0U;
    return true;
}

#endif
