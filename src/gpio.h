/* -*- mode: c; c-file-style: "linux"; compile-command: "scons -C .." -*-
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

#ifndef GPIO_H_
#define GPIO_H_

#include <glib.h>
#include <stdbool.h>

#define GPIO_PATH "/sys/class/gpio/"

struct gpio {
	int value;
	int edge;
	int direction;
};

/**
 * Open given GPIO pin and populate gpio structure. Dies if pin is not
 * accessible. */
void gpio_open(FILE* export, struct gpio *gpio, gint value);

/**
 * Read value in given GPIO pin.
 */
bool gpio_read(const int fd);

/**
 * Write attribute to given GPIO pin. You can use this with value,
 * edge and direction.
 */
void gpio_write(const int fd, const char *value);

#endif /* GPIO_H_ */
