#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#define NO_SYS                          1
#define SYS_LIGHTWEIGHT_PROT            0
#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0

#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (16 * 1024)
#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_TCP_PCB                8
#define MEMP_NUM_TCP_PCB_LISTEN         4
#define MEMP_NUM_TCP_SEG                16
#define MEMP_NUM_SYS_TIMEOUT            8
#define PBUF_POOL_SIZE                  16
#define PBUF_POOL_BUFSIZE               1536

#define LWIP_TCP                        1
#define LWIP_UDP                        1
#define LWIP_ICMP                       1
#define LWIP_DHCP                       0
#define LWIP_AUTOIP                     0
#define LWIP_DNS                        1

#define LWIP_ETHERNET                   1
#define LWIP_ARP                        1
#define ETHARP_SUPPORT_STATIC_ENTRIES   1

#define IP_FORWARD                      0
#define IP_REASSEMBLY                   1
#define IP_FRAG                         1

#define LWIP_IPV4                       1
#define LWIP_IPV6                       0

#define CHECKSUM_GEN_IP                 0
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0

#define LWIP_PROVIDE_ERRNO              1

#endif
