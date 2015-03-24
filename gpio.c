/* -*- mode: c; c-file-style: "linux"; compile-command: "scons" -*-
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
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>
#include "gpio.h"

static int gpio_open_one(char *fmt, gint value, int flags);

static int gpio_open_one(char *fmt, gint value, int flags)
{
	// Build file name
	char pathname[64]; // Buffer long enough to hold sysfs path
	int written = snprintf(pathname, sizeof(pathname), fmt, value);
	if (written < 0) errx(2,"Unknown string formatting error");
	if (written >= sizeof(pathname)) errx(2,"String formatting error, too long line");

	// Open file
	int dev_fd = open(pathname, O_NOCTTY | flags);
	if (dev_fd != -1) return dev_fd;
	err(2,"Unable to open GPIO %s", pathname);
}

void gpio_open(struct gpio *gpio, gint value)
{
	gpio->value = gpio_open_one("/sys/class/gpio/gpio%d/value", value, O_RDWR);
	gpio->edge = gpio_open_one("/sys/class/gpio/gpio%d/edge", value, O_WRONLY);
	gpio->direction = gpio_open_one("/sys/class/gpio/gpio%d/direction", value, O_WRONLY);
}

bool gpio_read(const int fd)
{
	guint8 buf[2];

	// Starting from beginning
	if (lseek(fd, 0, SEEK_SET) != 0) {
		err(3,"Seek failed");
	}

	// Read only two bytes; binary value and newline
	if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
		err(3,"Error while reading from GPIO pin");
	}
	
	// Convert '0' and '1' to boolean and return
	return buf[0] == '1';
}

void gpio_write(const int fd, const char *value)
{
	size_t len = strlen(value);
	if (write(fd, value, len) == len) return;
	err(3,"Error while writing to GPIO pin");
}
