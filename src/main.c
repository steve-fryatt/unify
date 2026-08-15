/* Copyright 2026, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Unify:
 *
 *   http://www.stevefryatt.org.uk/risc-os
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
 * \file: main.c
 */

/* ANSI C header files */

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/wimp.h"

/* SF-Lib header files */

#include "sflib/config.h"
#include "sflib/dataxfer.h"
#include "sflib/event.h"
#include "sflib/errors.h"
#include "sflib/heap.h"
#include "sflib/ihelp.h"
#include "sflib/msgs.h"
#include "sflib/resources.h"
#include "sflib/string.h"
#include "sflib/tasks.h"
#include "sflib/templates.h"
#include "sflib/url.h"

/* Application header files */

#include "main.h"

#include "iconbar.h"
#include "test_suite.h"
#include "test_file.h"
#include "window.h"

/**
 * The size of buffer allocated to resource filename processing.
 */

#define MAIN_FILENAME_BUFFER_LEN 1024

/**
 * The size of buffer allocated to the task name.
 */

#define MAIN_TASKNAME_BUFFER_LEN 64

/* Declare the global variables that are used. */

static struct dataxfer_memory main_memory_handlers;

/* Cross file global variables */

/**
 * The Wimp task handle for the Unify application.
 */

wimp_t main_task_handle;

/**
 * Set TRUE for the application to quit at the next oppoprtunity.
 */

osbool main_quit_flag = FALSE;

/* Static Function Prototypes */

static void main_poll_loop(void);
static void main_initialise(void);
static osbool main_message_quit(wimp_message *message);
static osbool main_message_prequit(wimp_message *message);


/**
 * Main code entry point.
 */

int main(void)
{
	main_initialise();

	main_poll_loop();

	msgs_terminate();
	test_suite_delete_all();

	wimp_close_down(main_task_handle);

	return 0;
}


/**
 * Wimp Poll loop.
 */

static void main_poll_loop(void)
{
	wimp_poll_flags	flags;
	wimp_event_no	reason;
	wimp_block	blk;
	os_t		next_poll;

	next_poll = os_read_monotonic_time();

	while (!main_quit_flag) {
		flags = (next_poll != 0) ? 0 : wimp_MASK_NULL;

		reason = wimp_poll_idle(flags, &blk, next_poll, NULL);

		if (!event_process_event(reason, &blk, 0, &next_poll)) {
			switch (reason) {
			case wimp_OPEN_WINDOW_REQUEST:
				wimp_open_window(&(blk.open));
				break;

			case wimp_CLOSE_WINDOW_REQUEST:
				wimp_close_window(blk.close.w);
				break;

			case wimp_KEY_PRESSED:
				wimp_process_key(blk.key.c);
				break;
			}
		}
	}
}


/**
 * Application initialisation.
 */

static void main_initialise(void)
{
	static char task_name[MAIN_TASKNAME_BUFFER_LEN];
	char resources[MAIN_FILENAME_BUFFER_LEN], res_temp[MAIN_FILENAME_BUFFER_LEN];

	hourglass_on();

	/* Initialise the resources. */

	string_copy(resources, "<Unify$Dir>.Resources", MAIN_FILENAME_BUFFER_LEN);
	if (!resources_initialise_paths(resources, MAIN_FILENAME_BUFFER_LEN, "Unify$Language", "UK"))
		error_report_fatal("Failed to initialise resources.");

	/* Load the messages file. */

	if (!resources_find_file(resources, res_temp, MAIN_FILENAME_BUFFER_LEN, "Messages", osfile_TYPE_TEXT))
		error_report_fatal("Failed to locate suitable Messages file.");

	msgs_initialise(res_temp);

	/* Initialise the error message system. */

	error_initialise("TaskName:Unify", "TaskSpr:!application", NULL);

	/* Initialise with the Wimp. */

	msgs_lookup("TaskName:Unify", task_name, MAIN_TASKNAME_BUFFER_LEN);
	main_task_handle = wimp_initialise(wimp_VERSION_RO38, task_name, NULL, NULL);

	event_add_message_handler(message_QUIT, EVENT_MESSAGE_INCOMING, main_message_quit);
	event_add_message_handler(message_PRE_QUIT, EVENT_MESSAGE_INCOMING, main_message_prequit);

	/* Initialise the flex heap. */

	flex_init(task_name, 0, 0);
	heap_initialise();

	/* Load the configuration. */

	config_initialise(task_name, "Unify", "<Unify$Dir>", NULL);

	config_load();

	/* Set up the dataxfer module's memory handlers, to use SFHeap. */

	main_memory_handlers.alloc = heap_alloc;
	main_memory_handlers.realloc = heap_extend;
	main_memory_handlers.free = heap_free;

	/* Load the menu structure. */

	if (!resources_find_file(resources, res_temp, MAIN_FILENAME_BUFFER_LEN, "Menus", osfile_TYPE_DATA))
		error_msgs_param_report_fatal("BadResource", "Menus", NULL, NULL, NULL);

	templates_load_menus(res_temp);

	/* Load the window templates. */

	if (!resources_find_file(resources, res_temp, MAIN_FILENAME_BUFFER_LEN, "Templates", osfile_TYPE_TEMPLATE))
		error_msgs_param_report_fatal("BadResource", "Templates", NULL, NULL, NULL);

	templates_open(res_temp);

	/* Initialise the individual modules. */

	dataxfer_initialise(main_task_handle, &main_memory_handlers);
	ihelp_initialise();
	url_initialise();

	window_initialise();
	iconbar_initialise();
	test_file_initialise();

	templates_close();

	/* Tidy up and finish initialisation. */

	hourglass_off();
}


/**
 * Handle incoming Message_Quit.
 */

static osbool main_message_quit(wimp_message *message)
{
	main_quit_flag = TRUE;

	return TRUE;
}


/**
 * Handle incoming Message_PreQuit.
 *
 * \param *message              The message data to be handled.
 * \return                      TRUE to claim the message; FALSE to pass it on.
 */

static osbool main_message_prequit(wimp_message *message)
{
	if (!main_check_for_unsaved_data())
		return TRUE;

	message->your_ref = message->my_ref;
	wimp_send_message(wimp_USER_MESSAGE_ACKNOWLEDGE, message, message->sender);

	return TRUE;
}


/**
 * Check for unsaved data, and ask the user if they wish to discard the changes.
 *
 * \return			TRUE if the user wishes to save the data.
 */

osbool main_check_for_unsaved_data(void)
{
	osbool modified = FALSE;

	/* Return true if anything needs rescuing. */

	return modified;
}
