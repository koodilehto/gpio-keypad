/* -*- mode: c; c-file-style: "linux"; compile-command: "scons -C ../.." -*-
 *  vi: set shiftwidth=8 tabstop=8 noexpandtab:
 *
 *  Copyright 2015 Joel Lehtonen
 *  
 *  This program is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU Affero General Public License
 *  as published by the Free Software Foundation, either version 3 of
 *  the License, or (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *  
 *  You should have received a copy of the GNU Affero General Public
 *  License along with this program.  If not, see
 *  <http://www.gnu.org/licenses/>.
 */

#include <err.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

const int debounce_ms = 40;

int gpio_open(const char *pathname, int flags);
char gpio_read(const int fd);
void gpio_write(const int fd, const char *value);

int gpio_open(const char *pathname, int flags)
{
	int dev_fd = open(pathname, O_NOCTTY | flags);
	if (dev_fd != -1) return dev_fd;
	err(2,"Unable to open GPIO %s", pathname);
}

char gpio_read(const int fd)
{
	uint8_t buf[2];

	// Starting from beginning
	if (lseek(fd, 0, SEEK_SET) != 0) {
		err(3,"Seek failed");
	}

	// Read only two bytes; binary value and newline
	if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
		err(3,"Error while reading from GPIO pin");
	}
	
	// Return binary value ('0' or '1')
	return buf[0];
}

void gpio_write(const int fd, const char *value)
{
	size_t len = strlen(value);
	if (write(fd, value, len) == len) return;
	err(3,"Error while writing to GPIO pin");
}

#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))
#define FOR_EACH(i,array) for (int i=0; i<ARRAY_LENGTH(array); i++)

int main(int argc, char *argv[])
{
	struct pollfd row_pollfds[] = {
		{gpio_open("/sys/class/gpio/gpio22/value", O_RDONLY), POLLPRI, 0},
		{gpio_open("/sys/class/gpio/gpio27/value", O_RDONLY), POLLPRI, 0},
		{gpio_open("/sys/class/gpio/gpio26/value", O_RDONLY), POLLPRI, 0},
		{gpio_open("/sys/class/gpio/gpio24/value", O_RDONLY), POLLPRI, 0},
	};

	const int row_edge_fds[] = {
		gpio_open("/sys/class/gpio/gpio22/edge", O_WRONLY),
		gpio_open("/sys/class/gpio/gpio27/edge", O_WRONLY),
		gpio_open("/sys/class/gpio/gpio26/edge", O_WRONLY),
		gpio_open("/sys/class/gpio/gpio24/edge", O_WRONLY),
	};

	const int col_dir_fds[] = {
		gpio_open("/sys/class/gpio/gpio23/direction", O_WRONLY),
		gpio_open("/sys/class/gpio/gpio21/direction", O_WRONLY),
		gpio_open("/sys/class/gpio/gpio25/direction", O_WRONLY)
	};

	int bounces = -1;
	int valid = 0;
	bool stable = true;

	// Process messages forever
	while (true) {
		// Listen for changes
		const int ret = poll(row_pollfds, ARRAY_LENGTH(row_pollfds), stable ? -1 : debounce_ms);
		if (ret == -1) {
			err(5,"Error while polling");
		} else if (ret == 0) {
			// Timeout reached; considering stable
			stable = true;
		} else {
			// Bounced before timeout, not stable
			stable = false;
			bounces++;

			// Clean poll state
			FOR_EACH(i, row_pollfds) {
				if (row_pollfds[i].revents & POLLPRI) {
					gpio_read(row_pollfds[i].fd);
					break;
				}
			}
		}

		if (stable) {
			// Turn off interrupts
			FOR_EACH(col, row_edge_fds) {
				gpio_write(row_edge_fds[col], "none\n");
			}

			// The state of buttons are stable, scan. 
			printf("\n%5d: ",valid++);
			FOR_EACH(col, col_dir_fds) {
				// Put all pins to floating mode except current column
				FOR_EACH(other_col, col_dir_fds) {
					gpio_write(col_dir_fds[other_col], col == other_col ? "out\n" : "in\n");
				}
				
				// Scan
				FOR_EACH(row, row_pollfds) {
					char value = gpio_read(row_pollfds[row].fd);
					printf("%c", value);
				}
			}
			printf(" bounces: %d\n",bounces);
			bounces = -1;
			
			// Reset directions
			FOR_EACH(col, col_dir_fds) {
				gpio_write(col_dir_fds[col], "out\n");
			}

			// Turn on interrupts
			FOR_EACH(col, row_edge_fds) {
				gpio_write(row_edge_fds[col], "both\n");
			}
		}
	}
}
