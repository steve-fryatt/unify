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
 * \file: log_window.c
 *
 * Log Window implementation.
 */

/* ANSI C header files */

#include <stddef.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include <oslib/osspriteop.h>
#include <oslib/wimp.h>

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/errors.h>
#include <sflib/event.h>
#include <sflib/heap.h>
#include <sflib/string.h>
#include <sflib/templates.h>
#include <sflib/windows.h>

/* Application header files */

#include "log_window.h"

/* Definitions. */

/**
 * A Log Window instance data.
 */

struct log_window_block {
	/**
	 * The window definition.
	 */
	struct log_window_definition *definition;

	/**
	 * The client data pointer/
	 */
	void *client_data;

	/**
	 * The handle of the log window.
	 */
	wimp_w window_handle;

	/**
	 * The title of the window.
	 */
	char window_title[];
};

/* Global variables. */

static wimp_window *log_window_definition = NULL;

/* Static function prototypes. */

static void log_window_close_handler(wimp_close *close);

/**
 * Initialise the log window implementation
 *
 * \param *sprites	Pointer to the user sprite area.
 */

void log_window_initialise(osspriteop_area *sprites)
{
	log_window_definition = templates_load_window("Log");
	log_window_definition->sprite_area = sprites;
	log_window_definition->icon_count = 0;

	// TODO -- for now, don't worry about redraw loops.

	log_window_definition->flags |= wimp_WINDOW_AUTO_REDRAW;
}

/**
 * Create a new Log Window instance.
 *
 * \param *definition	Pointer to the client window definition.
 * \param *title	Pointer to the title to use for the window.
 * \param *data		Pointer to the client data.
 * \return		Pointer to the new instance, or NULL on failure.
 */

struct log_window_block *log_window_create_instance(struct log_window_definition *definition, char *title, void *data)
{
	if (/*definition == NULL || */title == NULL)
		return NULL;

	/* Allocate memory for the instance structure. */

	size_t title_len = strlen(title) + 1;

	struct log_window_block *new = heap_alloc(sizeof(struct log_window_block) + title_len);
	if (new == NULL)
		return NULL;

	/* Initialise the data. */

	new->definition = definition;
	new->client_data = data;
	new->window_handle = NULL;

	string_copy(new->window_title, title, title_len);

	/* Create the window. */

	log_window_definition->title_data.indirected_text.text = new->window_title;
	log_window_definition->title_data.indirected_text.size = title_len;

	os_error *error = xwimp_create_window(log_window_definition, &(new->window_handle));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);

		new->window_handle = NULL;
		log_window_delete_instance(new);
		return NULL;
	}

//	ihelp_add_window(instance->handle, "LogWindow", NULL);

	event_add_window_user_data(new->window_handle, new);
//	event_add_window_menu(instance->handle, log_window_menu);
	event_add_window_close_event(new->window_handle, log_window_close_handler);
//	event_add_window_menu_prepare(instance->handle, log_menu_prepare_handler);
//	event_add_window_menu_warning(instance->handle, log_menu_warning_handler);
//	event_add_window_redraw_event(instance->handle, log_redraw_handler);

	/* Open the window. */

//	log_set_window_extent(instance);
	windows_open(new->window_handle);

	debug_printf("Created new log window instance 0x%x", new);

	return new;
}

/**
 * Delete a Log Window instance.
 *
 * \param *instance	Pointer to the instance to be deleted.
 */

void log_window_delete_instance(struct log_window_block *instance)
{
	if (instance == NULL)
		return;

	if (instance->window_handle != NULL) {
//		ihelp_remove_window(instance->window_handle);
		event_delete_window(instance->window_handle);
		wimp_delete_window(instance->window_handle);
	}

	debug_printf("Deleting log window instance 0x%x", instance);

	heap_free(instance);
}


/**
 * Handle Close events on a Log Window instance window.
 *
 * \param *close		The Wimp Close data block.
 */

static void log_window_close_handler(wimp_close *close)
{
	struct log_window_block *instance = event_get_window_user_data(close->w);

	if (instance == NULL)
		return;

	/* Notify the client. */

	if (instance->definition != NULL && instance->definition->callback_close != NULL)
		instance->definition->callback_close(instance->client_data);

	/* Delete ourselves. */

	log_window_delete_instance(instance);
}
