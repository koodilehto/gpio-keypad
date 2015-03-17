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

const int debounce_ms = 10;

int open_gpio(const char *pathname);
char take_contents(const int fd);

int open_gpio(const char *pathname)
{
	int dev_fd = open(pathname, O_NOCTTY|O_RDONLY);
	if (dev_fd != -1) return dev_fd;
	err(2,"Unable to open GPIO %s", pathname);
}

char take_contents(const int fd)
{
	uint8_t buf[3];

	// Starting from beginning
	off_t seek_ret = lseek(fd,0,SEEK_SET);
	if (seek_ret != 0) {
		err(3,"Seek failed");
	}

	int got = read(fd,buf,sizeof(buf));
	if (got != 2) {
		err(3,"Lukuvirhe gpio:sta");
	}
	
	return buf[0];
}

int main(int argc, char *argv[])
{
	struct pollfd fds[] = {
		{open_gpio("/sys/class/gpio/gpio22/value"),POLLPRI,0},
		{open_gpio("/sys/class/gpio/gpio27/value"),POLLPRI,0},
		{open_gpio("/sys/class/gpio/gpio26/value"),POLLPRI,0},
		{open_gpio("/sys/class/gpio/gpio24/value"),POLLPRI,0},
	};
	const int fds_n = sizeof(fds) / sizeof(struct pollfd);

	int sample = 0;
	int valid = 0;
	bool stable = true;

	// Process messages forever
	while (true) {
		const int ret = poll(fds, fds_n, stable ? -1 : debounce_ms);
		if (ret == -1) {
			err(5,"Error while polling");
		} else if (ret == 0) {
			stable = true;
			valid++;
		} else {
			stable = false;
			sample++;
			printf("Unstaabeli konstaapeli\n");
		}
		
		char state;
		int row;

		for (int i=0; i<fds_n; i++) {
			if (fds[i].revents & POLLPRI) {
				row = i;
				state = take_contents(fds[i].fd);
				break;
			}
		}
		
		if (stable) {
			printf("mittaus %d, kelpo %d, rivi %d, arvo %c\n", sample, valid, row, state);
		}
	}
}
