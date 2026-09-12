/* Copyright 2026, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Unify:
 *
 *   http://www.stevefryatt.org.uk/risc-os/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: file_dialogue.c
 *
 * File Dialogue implementation.
 */

/* ANSI C header files */

#include <stddef.h>
#include <stdint.h>

/* Acorn C header files */

/* OSLib header files */

#include <oslib/wimp.h>

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/icons.h>
#include <sflib/ihelp.h>
#include <sflib/templates.h>

/* Application header files */

#include "file_dialogue.h"

#include "date_time.h"

/**
 * The window icons.
 */

#define FILE_DIALOGUE_ICON_NAME ((wimp_i) 0)
#define FILE_DIALOGUE_ICON_SOURCE_FILENAME ((wimp_i) 6)
#define FILE_DIALOGUE_ICON_SOURCE_TIMESTAMP ((wimp_i) 8)
#define FILE_DIALOGUE_ICON_STATUS ((wimp_i) 10)
#define FILE_DIALOGUE_ICON_EXEC_FILENAME ((wimp_i) 12)
#define FILE_DIALOGUE_ICON_EXEC_TIMESTAMP ((wimp_i) 14)
#define FILE_DIALOGUE_ICON_TIMESTAMP ((wimp_i) 16)

/* Structure definitions. */


/* Global variables. */

static wimp_w file_dialogue_window = NULL;

/* Static function prototypes. */

/**
 * Initialise the file dialogue.
 */

void file_dialogue_initialise(void)
{
	file_dialogue_window = templates_create_window("FileInfo");
	templates_link_menu_dialogue("FileInfo", file_dialogue_window);
	ihelp_add_window(file_dialogue_window, "FileInfo", NULL);
}

/**
 * Populate the details of the file information dialogue.
 *
 * \param *data		Pointer to a structure containing the details to be
 *			populated.
 */

void file_dialogue_populate(struct file_dialogue_data *data)
{
	if (data == NULL)
		return;

	icons_strncpy(file_dialogue_window, FILE_DIALOGUE_ICON_NAME,
			(data->name != NULL) ? data->name : "");

	icons_strncpy(file_dialogue_window, FILE_DIALOGUE_ICON_STATUS, ""); // TODO - Populate this field.

	date_time_write_to_icon(data->run_timestamp, file_dialogue_window, FILE_DIALOGUE_ICON_TIMESTAMP);

	icons_strncpy(file_dialogue_window, FILE_DIALOGUE_ICON_SOURCE_FILENAME,
			(data->source_filename != NULL) ? data->source_filename : "");

	date_time_write_to_icon(data->source_timestamp, file_dialogue_window, FILE_DIALOGUE_ICON_SOURCE_TIMESTAMP);

	icons_strncpy(file_dialogue_window, FILE_DIALOGUE_ICON_EXEC_FILENAME,
			(data->executable_filename != NULL) ? data->executable_filename : "");

	date_time_write_to_icon(data->executable_timestamp, file_dialogue_window, FILE_DIALOGUE_ICON_EXEC_TIMESTAMP);
}
