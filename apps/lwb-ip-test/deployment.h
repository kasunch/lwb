#ifndef DEPLOYMENT_H
#define DEPLOYMENT_H

#include "contiki-conf.h"
#include "node-id.h"
#include "uip6/uip.h"


uint16_t get_n_nodes();
uint16_t get_node_id();
uint16_t node_id_from_ipaddr(const uip_ipaddr_t *addr);
void set_ipaddr_from_id(uip_ipaddr_t *ipaddr, uint16_t id);

#endif // DEPLOYMENT_H
