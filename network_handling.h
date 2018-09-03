#ifndef _network_handling
#define _network_handling

#include <stdint.h>


/*******************************************************************************
* PROTOTYPES
*******************************************************************************/

/**
 * Initialize the network subsystem
 *
 * return: void
 */
int init_nw();

/**
 * Teardown the network subsystem
 *
 * return: void
 */
void teardown_nw();

/**
 * Send a given buffer of given length over the network to a server
 *
 * return: 0 on success, <0 on error
 */
int nw_send(uint8_t *, unsigned);


#endif /* _network_handling */