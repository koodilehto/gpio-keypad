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

#ifndef KEYPAD_H_
#define KEYPAD_H_

#include <glib.h>
#include "gpio.h"

struct keypad {
	gint debounce_ms;
	gsize rows;
	gsize cols;
	struct gpio *row;
	struct gpio *col;
	gsize keycodes;
	gint *keycode;
	int uinput_fd;
};

/**
 * Setups keypad hardware (directions, edges, etc.)
 */
void keypad_setup(struct keypad *dev);

/**
 * Processes keypad events continuously
 */
void keypad_loop(struct keypad *dev);

#endif /* KEYPAD_H_ */
