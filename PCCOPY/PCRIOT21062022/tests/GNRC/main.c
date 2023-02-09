/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example application for demonstrating the RIOT network stack
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "shell.h"
#include "pm_layered.h"
#include "msg.h"
#include "board.h"
#include "net/gnrc/rpl.h"
#include "net/gnrc/rpl/structs.h"
#include "net/gnrc/rpl/dodag.h"
#include "periph/gpio.h"
#include "shell.h"
#include "net/gnrc/netif.h"
#include "net/netif.h"
#include "net/gnrc/netapi.h"


// #define PHYSICAL_NETIF 5
#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

extern int udp_cmd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "udp", "send data over UDP and listen on UDP ports", udp_cmd },
    { NULL, NULL, NULL }
};

static void _gnrc_netapi_set_all(netopt_state_t state)
{
    gnrc_netif_t* netif = NULL;
    while ((netif = gnrc_netif_iter(netif))) {
        /* retry if busy */
        while (gnrc_netapi_set(netif->pid, NETOPT_STATE, 0,
                               &state, sizeof(state)) == -EBUSY) {}
    }
}

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    //board_antenna_config(RFCTL_ANTENNA_EXT);    
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT network stack example application111111111111111");
    
    // gnrc_netif_t *netif = gnrc_netif_get_by_pid(PHYSICAL_NETIF);
    // if(netif == NULL) {
    //     printf("Is at86rf2xx on pid %d?", PHYSICAL_NETIF);
    //     return 1;
    // }
    puts("ready for radio off");

    xtimer_sleep(15);

    netopt_state_t devstate = NETOPT_STATE_SLEEP;
    _gnrc_netapi_set_all(devstate);
    
    puts("Radio is sleeping...");

    xtimer_sleep(3);
    // gnrc_rpl_init(6);
    pm_unblock(1);
    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
