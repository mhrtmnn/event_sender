#ifndef _avahi_handling
#define _avahi_handling


/*******************************************************************************
* PROTOTYPES
*******************************************************************************/

/**
 * Use Avahi to find ip addr and port for a given service name
 *
 * return: 0 on success, <0 on error
 */
int avahi_find_host_addr(char*, char **, unsigned *);


#endif /* _avahi_handling */