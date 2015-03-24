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
#include <poll.h>
#include <stdio.h>
#include <string.h>
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
	struct pollfd *polled = g_new(struct pollfd, dev->rows);
	for (gsize i=0; i<dev->rows; i++) {
		polled[i].fd = dev->row[i].value;
		polled[i].events = POLLPRI; // GPIO state change
		polled[i].revents = 0;
	}

	int bounces = -1;
	int valid = 0;
	bool stable = true;

	bool old_states[dev->keycodes];
	bool new_states[dev->keycodes];

	// Initial state has no keys pressed
	bzero(old_states, dev->keycodes*sizeof(bool));

	// Process messages forever
	while (true) {
		// Listen for changes
		const int ret = poll(polled, dev->rows, stable ? -1 : dev->debounce_ms);
		if (ret == -1) {
			err(5,"Error while polling");
		} else if (ret == 0) {
			// Timeout reached; considering stable
			stable = true;
			valid++;
		} else {
			// Bounced before timeout, not stable
			stable = false;
			bounces++;

			// Clean poll state
			for (gsize i=0; i<dev->rows; i++) {
				if (polled[i].revents & POLLPRI) {
					gpio_read(dev->row[i].value);
				}
			}
		}

		if (stable) {
			// Turn off interrupts
			for (gsize i=0; i<dev->rows; i++) {
				gpio_write(dev->row[i].edge, "none\n");
			}

			// The state of buttons are stable, scan. 
			int i=0;
			for (gsize col=0; i<dev->cols; i++) {
				// Put all pins to floating mode except current column
				for (gsize other_col=0; i<dev->cols; i++) {
					gpio_write(dev->col[other_col].direction,
						   col == other_col ? "out\n" : "in\n");
				}
				
				// Scan
				for (gsize row=0; i<dev->rows; row++) {
					new_states[i++] = gpio_read(dev->row[row].value);
				}
			}

			// Output what happened
			bool spurious = true;
			for (gsize i=0; i<dev->keycodes; i++) {
				if (new_states[i] != old_states[i]) {
					printf("%5d: key %c %s (%d bounces)\n", valid,
					       dev->keycode[i],
					       new_states[i] ? "down" : "up", bounces);
					old_states[i] = new_states[i];
					spurious = false;
				}
			}
			if (spurious) {
				// Sometimes no change occurs (button bounces without changing state)
				printf("%5d: spurious bounce (%d bounces)\n", valid, bounces);
			}

			// Prepare for the next round
			bounces = -1;
			keypad_interrupt_enable(dev);
		}
	}
}
