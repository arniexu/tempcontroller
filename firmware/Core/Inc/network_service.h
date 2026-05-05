#ifndef NETWORK_SERVICE_H
#define NETWORK_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
	bool ready;
	bool link_up;
	bool full_duplex;
	uint16_t speed_mbps;
} network_status_t;

void network_service_init(void);
void network_service_process(void);
bool network_service_is_ready(void);
bool network_service_get_status(network_status_t *status);

#endif
