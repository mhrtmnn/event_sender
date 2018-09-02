#ifndef _network_handling
#define _network_handling

#include <stdint.h>

/*******************************************************************************
* MACROS
*******************************************************************************/

/*******************************************************************************
* PROTOTYPES
*******************************************************************************/

/**
 * Send a given buffer of given length over the network to a server
 *
 * return: 0 on success, <0 on error
 */
int nw_send(uint8_t *, unsigned);

#endif /* _network_handling */