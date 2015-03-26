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

#include <glib.h>
#include <err.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <string.h>
#include <unistd.h>
#include "gpio.h"
#include "keypad.h"

#define VALIDATE(e) { if (e != NULL) errx(1, "Configuration file error: %s", e->message); }

guint g_key_file_get_hexinteger (GKeyFile *key_file,
				 const gchar *group_name,
				 const gchar *key,
				 GError **error);


guint g_key_file_get_hexinteger (GKeyFile *key_file,
				 const gchar *group_name,
				 const gchar *key,
				 GError **error)
{
	guint val=0;

	// Get raw value and exit if it's invalid
	char *raw_val = g_key_file_get_value(key_file, group_name, key, error);
	if (*error != NULL) return 0;

	// We have valid string, trying to parse hexadecimals
	int ret = sscanf(raw_val, "0x%x", &val);
	g_free(raw_val);

	if (ret != 1) {
		// Not sure if we should do it this way but it works...
		g_set_error (error, G_KEY_FILE_ERROR,
			     G_KEY_FILE_ERROR_INVALID_VALUE,
			     "Key file contains key '%s' in group '%s' "
			     "which has a value that cannot be interpreted.",
			     key, group_name);
	}
	return val;
}

int main(int argc, char *argv[])
{
	if (argc != 2) errx(1,"Usage %s CONFIG", argv[0]);

	GKeyFile *f = g_key_file_new();
	GError *e = NULL;
	g_key_file_load_from_file(f, argv[1], G_KEY_FILE_NONE, &e);
	VALIDATE(e);

	struct keypad *dev = g_new(struct keypad, 1);

	// Physical interface

	gint *gpio_row_ix = g_key_file_get_integer_list(f, "gpio", "rows", &dev->rows, &e);
	VALIDATE(e);

	gint *gpio_col_ix = g_key_file_get_integer_list(f, "gpio", "cols", &dev->cols, &e);
	VALIDATE(e);

	dev->debounce_ms = g_key_file_get_integer(f, "gpio", "debounce_ms", &e);
	VALIDATE(e);

	// Logical interface

	dev->keycode = g_key_file_get_integer_list(f, "input", "keycodes", &dev->keycodes, &e);
	VALIDATE(e);

	// Sanity checks
	if (dev->keycodes != dev->rows * dev->cols) {
		errx(3,"Keypad has %zd rows and %zd colums but %zd buttons. "
		     "Make sure keycodes has %zd elements",
		     dev->rows, dev->cols, dev->keycodes,
		     dev->rows*dev->cols);
	}

	// Linux uinput device

	struct uinput_user_dev uidev;
	bzero(&uidev, sizeof(uidev));

	char *name = g_key_file_get_string(f, "input", "name", &e);
	VALIDATE(e);
	memcpy(uidev.name, name, UINPUT_MAX_NAME_SIZE);
	g_free(name);

	uidev.id.bustype = g_key_file_get_hexinteger(f, "input", "bustype", &e);
	VALIDATE(e);

	uidev.id.vendor = g_key_file_get_hexinteger(f, "input", "vendor", &e);
	VALIDATE(e);

	uidev.id.product = g_key_file_get_hexinteger(f, "input", "product", &e);
	VALIDATE(e);

	uidev.id.version = g_key_file_get_integer(f, "input", "version", &e);
	VALIDATE(e);

	// Register uinput device

	char *uinput_dev = g_key_file_get_string(f, "input", "uinput", &e);
	VALIDATE(e);
	dev->uinput_fd = open(uinput_dev, O_WRONLY | O_NONBLOCK);
	if (dev->uinput_fd < 0) err(1,"Opening %s failed", uinput_dev);
	g_free(uinput_dev);

	if ( ioctl(dev->uinput_fd, UI_SET_EVBIT, EV_KEY) == -1 ||
	     ioctl(dev->uinput_fd, UI_SET_EVBIT, EV_SYN) == -1 ) {
		err(2, "Setting device flags failed");
	}

	for (gsize i=0; i<dev->keycodes; i++) {
		if (ioctl(dev->uinput_fd, UI_SET_KEYBIT,
			  dev->keycode[i]) == -1) {
			err(2, "Key registration failed");
		}
	}

	if (write(dev->uinput_fd, &uidev, sizeof(uidev)) != sizeof(uidev) ||
	    ioctl(dev->uinput_fd, UI_DEV_CREATE) == -1) {
		err(2, "Setting device flags failed");
	}

	// Open gpio pins

	FILE *export = fopen(GPIO_PATH "export", "w");
	if (export == NULL) err(2,"Unable to open %s", GPIO_PATH "export");

	dev->row = g_new(struct gpio, dev->rows);
	for (gsize i=0; i<dev->rows; i++) {
		gpio_open(export, dev->row+i, gpio_row_ix[i]);
	}
	g_free(gpio_row_ix);

	dev->col = g_new(struct gpio, dev->cols);
	for (gsize i=0; i<dev->cols; i++) {
		gpio_open(export, dev->col+i, gpio_col_ix[i]);
	}
	g_free(gpio_col_ix);

	fclose(export);

	keypad_setup(dev);
	keypad_loop(dev);

	return 0;
}
