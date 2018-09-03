#include<arpa/inet.h>
#include<stdio.h> /* fprintf */
#include<string.h> /* strerror */
#include <unistd.h> /* close */

#include "network_handling.h"
#include "avahi_handling.h"

/***********************************************************************************************************************
* GLOBAL DATA
***********************************************************************************************************************/
int g_sock = 0;
struct sockaddr_in g_si_other = {0};

/***********************************************************************************************************************
* MACROS/DEFINES
***********************************************************************************************************************/
#define IP_TP "10.10.0.102"
#define PORT_TP 8888


/***********************************************************************************************************************
* HELPER FUNC
***********************************************************************************************************************/
static int get_port()
{
	return PORT_TP;
}

static int get_ip(char *dst)
{
	// use avahi to find server ip addr
	main_ava();

	strcpy(dst, IP_TP);

	return 0;
}


/***********************************************************************************************************************
* IMPLEMENTATION OF EXPORTED FUNCTIONS
***********************************************************************************************************************/
int init_nw()
{
	int err;
	char dst_ip[15];
	struct sockaddr_in si_other = {0};

	// sanity check
	if (g_sock) {
		fprintf(stderr, "Error, already initialized\n");
		return -1;
	}

	// create udp socket
	g_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_sock < 0) {
        fprintf(stderr, "Could not create socket (%s)\n", strerror(-g_sock));
		return -1;
    }

	// setup dst address
	si_other.sin_family = AF_INET;
    si_other.sin_port = htons(get_port());

	// retrieve the dst ip addr
	err = get_ip(dst_ip);
	if (err < 0) {
        fprintf(stderr, "Could not retrieve destination IP\n");
		return -1;
    }

	// convert string into binary addr representation, returns 0 on error
    err = inet_aton(dst_ip, &si_other.sin_addr);
	if (err == 0) {
        fprintf(stderr, "Could not convert IP (inet_aton)\n");
		return -1;
    }

	// save addr in global var for use in sendto()
	memcpy(&g_si_other, &si_other, sizeof(g_si_other));

	return 0;
}

void teardown_nw()
{
	if (g_sock)
		close(g_sock);
}

int nw_send(uint8_t *buffer, unsigned buf_len)
{
    int err;

	// send buffer to specified address
	err = sendto(g_sock, buffer, buf_len , 0 /*flags*/, (struct sockaddr *) &g_si_other, sizeof(g_si_other));
	if (err < 0) {
        fprintf(stderr, "Could not send packet %d (%s)\n", err, strerror(-err));
		return -1;
	}

	fprintf(stderr, "Sent!\n");

	return 0;
}
