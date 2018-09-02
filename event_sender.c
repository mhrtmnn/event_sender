#include <stdio.h>
#include <stdlib.h> /* exit */
#include <fcntl.h> /* open */
#include <unistd.h> /* close() */
#include <signal.h> /* signal */
#include <string.h> /* strerror, strcmp */
#include <errno.h> /* err codes */

#include "event_sender.h"
#include "protobuf_handling.h"

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
int send_update(NunchukUpdate *nun_protobuf)
{
		uint8_t *buf = pack_nunchuk_protobuf(nun_protobuf);
		// udp send buf

		return 0;
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
			const char *s_ev_type, *s_ev_code;

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
					s_ev_type = libevdev_event_type_get_name(ev.type);
					s_ev_code = libevdev_event_code_get_name(ev.type, ev.code);
					printf("Event: %s %s %d\n", s_ev_type, s_ev_code, ev.value);
					event_complete = (ev.code == SYN_REPORT) ? true : false;
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

			// TODO: update the status structure

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
}
