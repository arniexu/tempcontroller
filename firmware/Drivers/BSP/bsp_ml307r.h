#ifndef BSP_ML307R_H
#define BSP_ML307R_H

#include <stdbool.h>

void bsp_ml307r_init(void);

bool bsp_ml307r_send_raw(const char *text);
bool bsp_ml307r_send_cmd(const char *cmd);
bool bsp_ml307r_read_line(char *buf, unsigned int buf_size);
bool bsp_ml307r_wait_for(const char *token, unsigned int max_polls);

bool bsp_ml307r_check_alive(void);
bool bsp_ml307r_disable_echo(void);
bool bsp_ml307r_query_csq(int *rssi, int *ber);

#endif
