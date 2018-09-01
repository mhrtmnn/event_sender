#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <signal.h>

/**
 * Compiler from buildroot toolchain automatically searches in the target's sysroot for headers
 * (include path is 'buildroot/output/host/arm-buildroot-linux-uclibcgnueabihf/sysroot/usr/include')
 */
#include <libevdev-1.0/libevdev/libevdev.h>


/***********************************************************************************************************************
* GLOBAL DATA
***********************************************************************************************************************/
static volatile int keep_running = 1;


/***********************************************************************************************************************
* HANDLERS
***********************************************************************************************************************/
void intHandler(int dummy) {
	keep_running = 0;
}


/***********************************************************************************************************************
* MAIN
***********************************************************************************************************************/
int main()
{
	int fd, rc, event_complete;
	int x_max, y_max;
	struct libevdev *evdev = NULL;

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
		exit(1);
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
		exit(1);
	}

	// get parameters
	x_max = libevdev_get_abs_maximum(evdev, ABS_X);
	y_max = libevdev_get_abs_maximum(evdev, ABS_Y);
	if (!x_max || !y_max) {
		fprintf(stderr, "Error getting abs max values\n");
		exit(1);
	}

	// main loop
	while (keep_running) {
		// Event group separation
		printf("--------------- EVENT ---------------\n");

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
					event_complete = (ev.code == SYN_REPORT) ? 1 : 0;
					break;
				case LIBEVDEV_READ_STATUS_SYNC:
					fprintf(stderr, "Dropped an event, resync required!\n");
					event_complete = 1;
					continue;
				case -EAGAIN:
					/* default case: do nothing */
					event_complete = 0;
					continue;
				default:
					fprintf(stderr, "Error in libevdev_next_event()!\n");
					event_complete = 1;
					continue;
			}

		// TODO: Pack partial event into a protobuf

		} while (!event_complete && keep_running);

		// Event group separation
		printf("\n");

		// TODO: send the protobuf with the complete event group via network

	}

	// cleanup
	printf("Graceful exit.\n");
	libevdev_free(evdev);
	close(fd);
}
