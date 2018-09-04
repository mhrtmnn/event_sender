#include <stdlib.h> /* malloc */
#include <stdio.h> /* stderr */
#include <errno.h> /* err codes */
#include <string.h> /* strcpy */

#include "event_sender.h"
#include "protobuf_handling.h"


/***********************************************************************************************************************
* GLOBAL DATA
***********************************************************************************************************************/

// global buffer used to pack protobuf into
uint8_t g_proto_pack_buff[MAX_UNPACK_BUF_SIZE];

	//TODO: new_nunchuk_protobuf can only be called once, i.e.
	// there can only be one active protobuf at once.
	// Also concurrent unpacks will suffer from synchronization issues.
	// Maybe make unpack buffers associated to a given protobuf.


/***********************************************************************************************************************
* HELPER FUNC
***********************************************************************************************************************/

/**
 * Pack a given nunchuk_update protobuf into a pre-allocated buffer.
 * The size of the buffer is verified.
 *
 * return: 0 on success, <0 on error
 */
static int __pack_nunchuk_protobuf(NunchukUpdate *msg, uint8_t *buf, unsigned buf_len)
{
	int rc;
	unsigned len; // Length of serialized data

	// check length of provided buffer before serializing the protobuf
	len = nunchuk_update__get_packed_size(msg);
	if (buf_len != len) {
		fprintf(stderr, "Buffer has wrong length (is %d, should be %d)!\n", buf_len, len);
		return -EINVAL;
	}

	// serialize the protobuf
	rc = nunchuk_update__pack(msg, buf);
	if (rc != len) {
		fprintf(stderr, "Failed to pack protobuf\n");
		return -EINVAL;
	}

	return 0;
}


/***********************************************************************************************************************
* IMPLEMENTATION OF EXPORTED FUNCTIONS
***********************************************************************************************************************/
NunchukUpdate *new_nunchuk_protobuf(void)
{
	NunchukUpdate			*nun_update;// Main msg
	NunchukUpdate__ButInfo	*nun_but;	// Nested msg 1
	NunchukUpdate__JoyInfo	*nun_joy;	// Nested msg 2
	char *str;							// string in Main msg

	// allocate memory
	nun_update 	= malloc(sizeof(*nun_update));
	nun_but		= malloc(sizeof(*nun_but));
	nun_joy		= malloc(sizeof(*nun_joy));
	str			= calloc(MAX_STR_LEN, sizeof(char)); // zero initialized
	if (!nun_update || !nun_but || !nun_joy || !str) {
		fprintf(stderr, "Failed allocate memory\n");
		return NULL;
	}

	// init message
	nunchuk_update__init(nun_update);
	nunchuk_update__but_info__init(nun_but);
	nunchuk_update__joy_info__init(nun_joy);

	// connect str buffer
	nun_update->query = str;

	// connect inner to outer messages
	nun_update->buttons = nun_but;
	nun_update->joystick = nun_joy;

	return nun_update;
}

void free_nunchuk_protobuf(NunchukUpdate *msg)
{
	if (!msg)
		return;

	if (msg->buttons)
		free(msg->buttons);
	if (msg->joystick)
		free(msg->joystick);
	if (msg->query)
		free(msg->query);

	free(msg);
}

void fill_nunchuk_protobuf(nun_stat_t *stat, NunchukUpdate *msg)
{
	strcpy(msg->query, "HelloWorld!");

	msg->buttons->but_c = (stat->but_c == BUT_KEEP)?
		NUNCHUK_UPDATE__BUT_INFO__BUT_STATES__KEEP:
		(stat->but_c == BUT_DOWN)?
		NUNCHUK_UPDATE__BUT_INFO__BUT_STATES__DOWN:
		NUNCHUK_UPDATE__BUT_INFO__BUT_STATES__UP;

	msg->buttons->but_z = (stat->but_z == BUT_KEEP)?
		NUNCHUK_UPDATE__BUT_INFO__BUT_STATES__KEEP:
		(stat->but_z == BUT_DOWN)?
		NUNCHUK_UPDATE__BUT_INFO__BUT_STATES__DOWN:
		NUNCHUK_UPDATE__BUT_INFO__BUT_STATES__UP;


	msg->joystick->joy_x = stat->joy_x;
	msg->joystick->joy_y = stat->joy_y;
}

void fill_stats_from_nunchuk_protobuf(NunchukUpdate *msg, nun_stat_t *stat)
{
	stat->but_c = (msg->buttons->but_c == NUNCHUK_UPDATE__BUT_INFO__BUT_STATES__KEEP)?
		BUT_KEEP:
		(msg->buttons->but_c == NUNCHUK_UPDATE__BUT_INFO__BUT_STATES__DOWN)?
		BUT_DOWN:
		BUT_UP;

	stat->but_z = (msg->buttons->but_z == NUNCHUK_UPDATE__BUT_INFO__BUT_STATES__KEEP)?
		BUT_KEEP:
		(msg->buttons->but_z == NUNCHUK_UPDATE__BUT_INFO__BUT_STATES__DOWN)?
		BUT_DOWN:
		BUT_UP;

	stat->joy_x = msg->joystick->joy_x;
	stat->joy_y = msg->joystick->joy_y;
}

/**
 * wrapper for __pack_nunchuk_protobuf, uses a global unpack buffer that is returned.
 * By this, the user does not need to determine the size of the buffer and preallocate it himself.
 */
int pack_nunchuk_protobuf(NunchukUpdate *msg, uint8_t **buf, unsigned *buflen)
{
	int len = nunchuk_update__get_packed_size(msg);

	// return buffer and its current length via argument ptrs
	*buflen = len;
	*buf = g_proto_pack_buff;

	/* dbg */
	// printf("Packing probuf, size %d\n", len);

	return __pack_nunchuk_protobuf(msg, g_proto_pack_buff, len);
}

int unpack_nunchuk_protobuf(uint8_t *buf, unsigned len, nun_stat_t *stat)
{
	// de-serialize the buffer again, protobuf is allocated by unpack func
	NunchukUpdate *new_msg = nunchuk_update__unpack(NULL, len, buf); // use default allocator
	if (!new_msg) {
		fprintf(stderr, "Failed to unpack protobuf\n");
		return -EINVAL;
	}

	// copy content to 'stat' struct
	fill_stats_from_nunchuk_protobuf(new_msg, stat);

	// free the allocated protobuf
	nunchuk_update__free_unpacked(new_msg, NULL); // use default allocator

	return 0;
}
