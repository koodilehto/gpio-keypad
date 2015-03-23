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

#include <glib.h>
#include <err.h>
#include <stdio.h>


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
	guint val;

	// Get raw value and exit if it's invalid
	char *raw_val = g_key_file_get_value(key_file, group_name, key, error);
	if (*error != NULL) return 0;

	// We have valid string, trying to parse hexadecimals
	if (sscanf(raw_val, "0x%x", &val) == 1) return val;

	// Not sure if we should do it this way but it works...
	g_set_error (error, G_KEY_FILE_ERROR,
		     G_KEY_FILE_ERROR_INVALID_VALUE,
		     "Key file contains key '%s' in group '%s' "
		     "which has a value that cannot be interpreted.",
		     key, group_name);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 2) errx(1,"Usage %s CONFIG", argv[0]);

	GKeyFile *f = g_key_file_new();
	GError *error = NULL;
	g_key_file_load_from_file(f, argv[1], G_KEY_FILE_NONE, &error);
	VALIDATE(error);

	// Physical interface

	gsize gpio_rows_n;
	gint *gpio_rows = g_key_file_get_integer_list(f, "gpio", "rows", &gpio_rows_n, &error);
	VALIDATE(error);

	gsize gpio_cols_n;
	gint *gpio_cols = g_key_file_get_integer_list(f, "gpio", "cols", &gpio_cols_n, &error);
	VALIDATE(error);

	guint debounce_ms = g_key_file_get_integer(f, "gpio", "debounce_ms", &error);
	VALIDATE(error);

	// Logical interface

	char *name = g_key_file_get_string(f, "input", "name", &error);
	VALIDATE(error);

	gsize keycodes_n;
	gint *keycodes = g_key_file_get_integer_list(f, "input", "keycodes", &keycodes_n, &error);
	VALIDATE(error);

	guint bustype = g_key_file_get_hexinteger(f, "input", "bustype", &error);
	VALIDATE(error);

	guint vendor = g_key_file_get_hexinteger(f, "input", "vendor", &error);
	VALIDATE(error);

	guint product = g_key_file_get_hexinteger(f, "input", "product", &error);
	VALIDATE(error);

	guint version = g_key_file_get_integer(f, "input", "version", &error);
	VALIDATE(error);
	
	// Viriviri

	printf("Nimi: %s\nArvo: %d\n", name, debounce_ms);
	for (gsize i=0; i<keycodes_n; i++) {
		printf("koodi %d\n",keycodes[i]);
	}

	for (gsize i=0; i<gpio_rows_n; i++) {
		printf("koodia %d\n",gpio_rows[i]);
	}

	for (gsize i=0; i<gpio_cols_n; i++) {
		printf("koodit %d\n",gpio_cols[i]);
	}

	return 0;
}
