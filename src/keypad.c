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
#include <unistd.h>
#include <linux/uinput.h>
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
			for (gsize col=0; col<dev->cols; col++) {
				// Put all pins to floating mode except current column
				for (gsize other_col=0; other_col<dev->cols; other_col++) {
					gpio_write(dev->col[other_col].direction,
						   col == other_col ? "out\n" : "in\n");
				}
				
				// Scan
				for (gsize row=0; row<dev->rows; row++) {
					new_states[i++] = gpio_read(dev->row[row].value);
				}
			}

			// Output what happened
			gint changes = 0;
			struct input_event ev[dev->keycodes+1];	// All buttons + EV_SYN
			for (gsize i=0; i<dev->keycodes; i++) {
				if (new_states[i] != old_states[i]) {
					ev[changes].type = EV_KEY;
					ev[changes].code = dev->keycode[i];
					ev[changes].value = new_states[i];
					changes++;
					
					printf("%5d: key %d %s (%d bounces)\n", valid,
					       dev->keycode[i],
					       new_states[i] ? "down" : "up", bounces);
					old_states[i] = new_states[i];
				}
			}
			if (changes) {
				ev[changes].type = EV_SYN;
				ev[changes].code = SYN_REPORT;
				ev[changes].value = 0;
				gsize total = (changes+1) * sizeof(struct input_event);

				if (write(dev->uinput_fd, ev, total) != total) {
					err(3, "Linux uinput not okay");
				}
			} else {
				// Sometimes no change occurs (button bounces without changing state)
				printf("%5d: spurious bounce (%d bounces)\n", valid, bounces);
			}

			// Prepare for the next round
			bounces = -1;
			keypad_interrupt_enable(dev);
		}
	}
}
