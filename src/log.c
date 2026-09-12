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
 * \file: log.c
 *
 * Log storage and display implementation.
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include <oslib/os.h>
#include <oslib/wimp.h>

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/errors.h>
#include <sflib/event.h>
#include <sflib/heap.h>
#include <sflib/ihelp.h>
#include <sflib/templates.h>
#include <sflib/windows.h>

/* Application header files */

#include "log.h"

/* Structure definitions. */

/**
 * A log instance.
 */

struct log_instance {

	wimp_w handle;
};

/* Global variables. */

/**
 * Definition of the log window.
 */

static wimp_window *log_window_definition = NULL;

/**
 * The window menu.
 */

static wimp_menu *log_window_menu = NULL;

/* Static function prototypes. */

static void log_close_handler(wimp_close *close);

/**
 * Initialise the log implementation.
 *
 * \param task_handle		The handle of the task.
 */

void log_initialise(void)
{
	log_window_definition = templates_load_window("Log");

	/* Set up the log window menu and its dialogues. */

//	log_window_menu = templates_get_menu("LogWindowMenu");
//	ihelp_add_menu(log_window_menu, "LogMenu");
}

/**
 * Create a new log instance.
 *
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct log_instance *log_create_instance(void)
{
	/* Allocate the instance memory. */

	struct log_instance *instance = heap_alloc(sizeof(struct log_instance));
	if (instance == NULL)
		return NULL;

	instance->handle = NULL;

	/* Allocate the flex blocks. */

//	if (!flexutils_allocate((void **) &(instance->known_objects), sizeof(struct window_object), instance->object_space)) {
//		window_delete_instance(instance);
//		return NULL;
//	}

//	if (!flexutils_allocate((void **) &(instance->active_folds), sizeof(struct window_fold), instance->fold_space)) {
//		window_delete_instance(instance);
//		return NULL;
//	}

	debug_printf("Log 0x%x created...", instance);

	return instance;
}

/**
 * Destroy a log instance.
 *
 * \param *instance		The instance to be deleted.
 */

void log_delete_instance(struct log_instance *instance)
{
	if (instance == NULL)
		return;

	/* Delete the windows. */

	if (instance->handle != NULL) {
		ihelp_remove_window(instance->handle);
		event_delete_window(instance->handle);
		wimp_delete_window(instance->handle);
	}

	/* Free the memory used. */

//	flexutils_free((void **) &(instance->known_objects));
//	flexutils_free((void **) &(instance->active_folds));

	debug_printf("Log 0x%x deleted...", instance);

	heap_free(instance);
}

/**
 * Open (or re-open) a window for a log instance.
 *
 * \param *instance		The instance for which to open the window.
 */

void log_open_window(struct log_instance *instance)
{
	if (instance == NULL)
		return;

	if (instance->handle != NULL) {
		windows_open(instance->handle);
		return;
	}
	/* Create the new window. */

	os_error *error = xwimp_create_window(log_window_definition, &(instance->handle));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return;
	}

	ihelp_add_window(instance->handle, "LogWindow", NULL);

	event_add_window_user_data(instance->handle, instance);
//	event_add_window_menu(instance->handle, window_menu);
//	event_add_window_open_event(instance->handle, window_open_handler);
	event_add_window_close_event(instance->handle, log_close_handler);
//	event_add_window_mouse_event(instance->handle, window_click_handler);
//	event_add_window_menu_prepare(instance->handle, window_menu_prepare);
//	event_add_window_menu_warning(instance->handle, window_menu_warning);
//	event_add_window_menu_selection(instance->handle, window_menu_selection);
//	event_add_window_menu_close(instance->handle, window_menu_close);
//	event_add_window_redraw_event(instance->handle, window_redraw_handler);
//	event_add_window_scroll_event(instance->handle, window_scroll_handler);

	/* Open the windows. */

//	wimp_window_state window = { .w = instance->handle };
//	wimp_get_window_state(&window);
//	window_recalculate_columns(instance, (wimp_open *) &window);
//	window.next = wimp_TOP;
//	wimp_open_window((wimp_open *) &window);

	windows_open(instance->handle);

//	windows_open_nested_as_toolbar(instance->pane_handle, instance->handle, instance->pane_size, FALSE);
//	window_position_toolbar_icons((wimp_open *) &window, instance);
}







/**
 * Handle Close events on an instance window.
 *
 * \param *close		The Wimp Close data block.
 */

static void log_close_handler(wimp_close *close)
{
	struct log_instance *instance = event_get_window_user_data(close->w);

	if (instance == NULL || instance->handle == NULL)
		return;

	ihelp_remove_window(instance->handle);
	event_delete_window(instance->handle);
	wimp_delete_window(instance->handle);

	instance->handle = NULL;
}
