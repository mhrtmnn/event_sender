#include <stdio.h>
#include <stdlib.h> /* exit */
#include <fcntl.h> /* open */
#include <unistd.h> /* close() */
#include <signal.h> /* signal */
#include <string.h> /* strerror, strcmp */
#include <errno.h> /* err codes */

#include "event_sender.h"
#include "protobuf_handling.h"
#include "network_handling.h"

/**
 * Compiler from buildroot toolchain automatically searches in the target's sysroot for headers and libs.
 * (include/lib path is 'buildroot/output/host/arm-buildroot-linux-uclibcgnueabihf/sysroot/usr/{include/lib}')
 */
#include <libevdev-1.0/libevdev/libevdev.h>


/***********************************************************************************************************************
* GLOBAL DATA
***********************************************************************************************************************/
static volatile bool keep_running = true;


/***********************************************************************************************************************
* HANDLERS
***********************************************************************************************************************/
void intHandler(int dummy) {
	keep_running = false;
}


/***********************************************************************************************************************
* HELPER FUNC
***********************************************************************************************************************/
void print_nun_stat(nun_stat_t *nun_stat)
{
	printf(">>> Buttons:  But-C=%d, But-Z=%d\n", nun_stat->but_c, nun_stat->but_z);
	printf(">>> Joystick: Joy-X=%d, Joy-Y=%d\n", nun_stat->joy_x, nun_stat->joy_y);
}

int unpack_buffer(uint8_t *buffer, unsigned length)
{
	nun_stat_t nun_stat;

	int err = unpack_nunchuk_protobuf(buffer, length, &nun_stat);
	if (err) {
		fprintf(stderr, "Error unpacking protobuf!");
		return err;
	}

	print_nun_stat(&nun_stat);

	return 0;
}

int send_update(NunchukUpdate *nun_protobuf)
{
		int err;
		unsigned length;
		uint8_t *buffer;

		err = pack_nunchuk_protobuf(nun_protobuf, &buffer, &length);
		if (err) {
			fprintf(stderr, "Failed to pack protobuf (%s)\n", strerror(-err));
			return err;
		}

		/* debug */ unpack_buffer(buffer, length);

		// send out buf
		return nw_send(buffer, length);
}


/***********************************************************************************************************************
* MAIN
***********************************************************************************************************************/
int main()
{
	bool event_complete;
	int fd, rc, x_max, y_max;
	struct libevdev *evdev = NULL;
	NunchukUpdate *nun_protobuf;

	// init the protobuf used to send nunchuk data
	nun_protobuf = new_nunchuk_protobuf();
	if (!nun_protobuf) {
		fprintf(stderr, "Error allocating protobuf!\n");
		exit(EXIT_FAILURE);
	}

	// signalling for interrupting main loop
	signal(SIGINT, intHandler);

	/**
	 * INFO:
	 * - fopen returns a C standard FILE* which enables buffered IO, using fscanf etc
	 * - open (syscall) returns an OS dependent fd, that enables non blocking IO using lseek/read/write etc syscalls
	 */

	// init libevdev
	fd = open("/dev/input/event0", O_RDONLY|O_NONBLOCK);
	rc = libevdev_new_from_fd(fd, &evdev);
	if (rc < 0) {
		fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
		exit(EXIT_FAILURE);
	}

	// display the input device
	printf("Input device name: \"%s\"\n", libevdev_get_name(evdev));
	printf("Input device ID: bus %#x vendor %#x product %#x\n",
		libevdev_get_id_bustype(evdev),
		libevdev_get_id_vendor(evdev),
		libevdev_get_id_product(evdev));

	// check the input device
	if (strcmp(libevdev_get_name(evdev), "Wii Nunchuk")) {
		fprintf(stderr, "This is not the device you are looking for ...\n");
		exit(EXIT_FAILURE);
	}

	// get parameters
	x_max = libevdev_get_abs_maximum(evdev, ABS_X);
	y_max = libevdev_get_abs_maximum(evdev, ABS_Y);
	if (!x_max || !y_max) {
		fprintf(stderr, "Error getting abs max values\n");
		exit(EXIT_FAILURE);
	}

	// main loop
	while (keep_running) {
		// Event group separation
		printf("--------------- EVENT ---------------\n");

		// init status struct with neutral values
		nun_stat_t nun_status = {JOY_NO_CHANGE, JOY_NO_CHANGE, BUT_KEEP, BUT_KEEP};

		do {
			struct input_event ev;

			/**
			 * libevdev_next_event() returns:
			 * - EAGAIN if there is no new event,
			 * - LIBEVDEV_READ_STATUS_SYNC if an event was dropped and a resync with the device is necessary
			 * - LIBEVDEV_READ_STATUS_SUCCESS if the event was read successfully.
			 *
			 * Event groups are separated by an event of type EV_SYN and code SYN_REPORT.
			 */
			rc = libevdev_next_event(evdev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
			switch (rc) {
				case LIBEVDEV_READ_STATUS_SUCCESS:
					printf("Event: %s %s %d\n",
						libevdev_event_type_get_name(ev.type),
						libevdev_event_code_get_name(ev.type, ev.code),
						ev.value);
					event_complete = false;
					break;
				case LIBEVDEV_READ_STATUS_SYNC:
					fprintf(stderr, "Dropped an event, resync required!\n");
					event_complete = true;
					continue;
				case -EAGAIN:
					/* default case: do nothing */
					event_complete = false;
					continue;
				default:
					fprintf(stderr, "Error in libevdev_next_event()!\n");
					event_complete = true;
					continue;
			}

			// update nun_status
			if (ev.type == EV_KEY) {
				/**
				 * Event to nun_stat mapping:
				 * 0 <-> BUT_UP
				 * 1 <-> BUT_DOWN
				 * no event <-> BUT_KEEP
				 */
				switch (ev.code) {
					case BTN_C:
						nun_status.but_c = ev.value;
						break;
					case BTN_Z:
						nun_status.but_z = ev.value;
						break;
					default:
						fprintf(stderr, "Unexpected event code! (%d)\n", ev.code);
						break;
				}
			} else if (ev.type == EV_ABS) {
				switch (ev.code) {
					case ABS_X:
						nun_status.joy_x = ev.value;
						break;
					case ABS_Y:
						nun_status.joy_y = ev.value;
						break;
					default:
						fprintf(stderr, "Unexpected event code! (%d)\n", ev.code);
						break;
				}
			} else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
				// indicates that input_sync() was called in the kernel, i.e. event group is complete
				event_complete = true;
			} else {
				fprintf(stderr, "Unexpected event type! (%d)\n", ev.type);
			}

		} while (!event_complete && keep_running);

		// Event group separation
		printf("\n");

		// send out the protobuf with the complete event
		fill_nunchuk_protobuf(&nun_status, nun_protobuf);
		send_update(nun_protobuf);
	}

	// cleanup
	printf("Graceful exit.\n");
	free_nunchuk_protobuf(nun_protobuf);
	libevdev_free(evdev);
	close(fd);
	return EXIT_SUCCESS;
}
