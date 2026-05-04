#ifndef LWCELL_HDR_OPTS_H
#define LWCELL_HDR_OPTS_H

/*
 * Project-specific LwCELL options.
 * This file is only configuration and does not force library compilation.
 */

#if !__DOXYGEN__
#define LWCELL_CFG_DBG                  LWCELL_DBG_OFF
#define LWCELL_CFG_DBG_TYPES_ON         (LWCELL_DBG_TYPE_TRACE | LWCELL_DBG_TYPE_STATE)

#define LWCELL_CFG_INPUT_USE_PROCESS    1
#define LWCELL_CFG_AT_ECHO              0

#define LWCELL_CFG_NETWORK              1
#define LWCELL_CFG_CONN                 1
#define LWCELL_CFG_NETCONN              1
#define LWCELL_CFG_MQTT                 1
#define LWCELL_CFG_SMS                  1
#define LWCELL_CFG_CALL                 1
#define LWCELL_CFG_PHONEBOOK            1
#define LWCELL_CFG_USSD                 1

#define LWCELL_CFG_CONN_MAX_DATA_LEN    512
#define LWCELL_CFG_CONN_MIN_DATA_LEN    64
#define LWCELL_CFG_MAX_CONNS            1
#define LWCELL_CFG_RCV_BUFF_SIZE        512

#define LWCELL_CFG_RESET_ON_INIT        0
#define LWCELL_CFG_RESET_ON_DEVICE_PRESENT 0

/* Keep default allocator for now to minimize integration work. */
#define LWCELL_CFG_MEM_CUSTOM           0
#endif

#endif
