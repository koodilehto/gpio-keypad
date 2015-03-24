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

#include "keypad.h"

static void keypad_interrupt_enable(struct keypad *dev);

static void keypad_interrupt_enable(struct keypad *dev)
{
	// Reset directions so all lines are active for interrupts
	for (gsize i=0; i<dev->cols; i++) {
		gpio_write(dev->col[i].direction, "out\n");
	}

	// Turn interrupts on
	for (gsize i=0; i<dev->rows; i++) {
		gpio_write(dev->row[i].edge, "both\n");
	}
}

void keypad_setup(struct keypad *dev)
{
	// Rows to input
	for (gsize i=0; i<dev->rows; i++) {
		gpio_write(dev->row[i].direction, "in\n");
	}

	// Column are driven open-source
	for (gsize i=0; i<dev->cols; i++) {
		gpio_write(dev->col[i].value, "1\n");
	}

	keypad_interrupt_enable(dev);
}

void keypad_loop(struct keypad *dev)
{
}
