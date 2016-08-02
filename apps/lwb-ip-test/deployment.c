/*
 * Copyright (c) 2014, Uppsala University, Sweden.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Kasun Hewage <kasun.hewage@it.uu.se>
 *
 */

#include "contiki-conf.h"
#include "uip6/uip.h"
#include "deployment.h"
#include <string.h>

struct id_addr {
    uint16_t id;
    uint16_t addr[8];
};

static struct id_addr id_addr_list[] = { 
    {1, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x137c), UIP_HTONS(0x80a9)}}, 
    {2, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x116d), UIP_HTONS(0x5485)}},
    {3, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x116c), UIP_HTONS(0x2787)}},
    {4, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x137c), UIP_HTONS(0x1773)}},
    {5, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x1515), UIP_HTONS(0xf95c)}},
    {6, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x137c), UIP_HTONS(0x103c)}},
    {7, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x116d), UIP_HTONS(0x3543)}},
    {8, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x137b), UIP_HTONS(0xf36b)}},
    {9, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x116c), UIP_HTONS(0x5ee9)}},
   {10, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x116d), UIP_HTONS(0x3eb8)}},
   {11, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x137b), UIP_HTONS(0xc714)}},
   {12, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x137c), UIP_HTONS(0x35e2)}},
   {13, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x13b7), UIP_HTONS(0x762a)}},
   {14, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x13b7), UIP_HTONS(0xfd06)}},
   {15, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x137d), UIP_HTONS(0x2a0f)}},
   {16, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x13b7), UIP_HTONS(0x62fc)}},
   {17, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x13b7), UIP_HTONS(0x7e68)}},

   {18, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x1508), UIP_HTONS(0xa490)}},
   {19, {UIP_HTONS(0xaaaa), 0, 0, 0, UIP_HTONS(0x212), UIP_HTONS(0x7400), UIP_HTONS(0x13b7), UIP_HTONS(0xd423)}},
    {0, {0, 0, 0, 0, 0, 0 ,0, 0}}
};

/* The total number of nodes in the deployment */
#define N_NODES ((sizeof(id_addr_list)/sizeof(struct id_addr))-1)

/* Returns the node's node-id */
uint16_t
get_node_id()
{
  return node_id;
}

/* Returns the total number of nodes in the deployment */
uint16_t
get_n_nodes()
{
  return N_NODES;
}

uint16_t 
node_id_from_ipaddr(const uip_ipaddr_t *addr) {
    struct id_addr *curr = id_addr_list;
    while (curr->id != 0) {
        if (uip_ipaddr_cmp(addr, (uip_ipaddr_t*)&curr->addr)) {
            return curr->id;
        }
        curr++;
    }
    return 0;
}


void 
set_ipaddr_from_id(uip_ipaddr_t *ipaddr, uint16_t id){
    struct id_addr *curr = id_addr_list;
    while (curr->id != id) {
        curr++;
    }
    if (curr->id == id) {
        memcpy(ipaddr->u8, ((uip_ipaddr_t*)(curr->addr))->u8, 16);
    }
}


