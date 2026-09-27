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
 * \file: suite.c
 *
 * Test Suite implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>


/* Acorn C header files */

/* OSLib header files */

#include <oslib/os.h>

/* SF-Lib header files. */

#include <sflib/config.h>
#include <sflib/debug.h>
#include <sflib/errors.h>
#include <sflib/heap.h>
#include <sflib/string.h>

/* Application header files */

#include "suite.h"

#include "file_set.h"
#include "file_instance.h"
#include "project.h"
#include "textdump.h"
#include "window.h"

/* Structure definitions. */

struct suite_block {
	/**
	 * The name of the test suite.
	 */
	unsigned name;

	/**
	 * The name of the run path variable.
	 */
	unsigned run_path_variable;

	/**
	 * Did we set the run path variable at the start of the test?
	 */
	osbool we_set_run_path_variable;

	/**
	 * The memory allocation for tasks, in KB.
	 */
	int task_slot_size;

	/**
	 * Pointer to the details of the project contained in the suite.
	 */
	struct project_details *project;

	/**
	 * The textdump reference of the path to the suite folder.
	 */
	unsigned suite_folder;

	/**
	 * The textdump reference of the name of the source file folder within
	 * the suite folder.
	 */
	unsigned source_folder;

	/**
	 * The textdump reference of the name of the executable file folder
	 * within the suite folder.
	 */
	unsigned executable_folder;

	/**
	 * The window for the test suite.
	 */
	struct window_instance *window;

	/**
	 * The textdump instance for the suite to use.
	 */
	struct textdump_block *textdump;

	/**
	 * Pointer to the list of file sets associated with this suite.
	 */
	struct file_set_block *file_sets;

	/**
	 * Pointer to the list if file instances associated with this suite.
	 */
	struct file_instance_block *file_instances;

	/**
	 * Pointer to the file set currently on display in the window.
	 */
	struct file_set_block *current_file_set;

	/**
	 * Pointer to the next suite, or NULL.
	 */
	struct suite_block *next;
};

/* Global variables. */

/**
 * Pointer to the linked list of test suites.
 */

struct suite_block *suite_list = NULL;

/* Static function prototypes. */

static void suite_start_test_run(struct suite_block *instance, osbool full);
static void suite_close_handler(void *data);
static void suite_navigation_handler(enum window_navigation_target target, void *data);
static void suite_run_handler(osbool full, void *data);
static osbool suite_redraw_line_handler(int fold, int entry, struct window_line *content, void *data);
static void suite_object_has_log(int fold, void *data, osbool *this_log, osbool *any_log);
static void suite_object_open_log(int fold, void *data);
static osbool suite_object_save_log(int fold, char *filename, void *data);
static osbool suite_object_save_all_logs(char *filename, void *data);
static osbool suite_object_info_handler(int fold, struct file_dialogue_data *info, void *data);
static osbool suite_read_config_file(struct suite_block *instance);
static osbool suite_set_run_path_variable(struct suite_block *instance);
static void suite_unset_run_path_variable(struct suite_block *instance);

/* The Test Suite window definiton. */

static struct window_definition suite_window_definition = {
	.type = WINDOW_TYPE_SUITE,
	.callback_close = suite_close_handler,
	.callback_redraw = suite_redraw_line_handler,
	.callback_fileinfo = suite_object_info_handler,
	.callback_file_has_log = suite_object_has_log,
	.callback_open_log_viewer = suite_object_open_log,
	.callback_save_log = suite_object_save_log,
	.callback_save_all_logs = suite_object_save_all_logs,
	.callback_navigate = suite_navigation_handler,
	.callback_run = suite_run_handler
};

/**
 * Create a new Test Suite instance and link it in to the collection of
 * active instances.
 *
 * \param *folder	Pointer to the name of the folder holding the
 *			test suite files.
 * \return		TRUE if successful; FALSE on error.
 */

osbool suite_create_instance(char *folder)
{
	struct suite_block *new = heap_alloc(sizeof(struct suite_block));
	if (new == NULL)
		return FALSE;

	new->window = NULL;
	new->textdump = NULL;
	new->file_sets = NULL;
	new->file_instances = NULL;
	new->current_file_set = NULL;
	new->name = TEXTDUMP_NULL;
	new->run_path_variable = TEXTDUMP_NULL;
	new->we_set_run_path_variable = FALSE;
	new->task_slot_size = 1024;

	/* Set up the text dump to store strings for the suite. */

	new->textdump = textdump_create(TEXTDUMP_DEFAULT_ALLOCATION);
	if (new->textdump == NULL) {
		suite_delete_instance(new);
		return FALSE;
	}

	/* Set up the window for the suite. */

	new->window = window_create_instance(&suite_window_definition, folder, new);
	if (new->window == NULL) {
		suite_delete_instance(new);
		return FALSE;
	}

	/* Work out the project type. */

	new->project = project_get_definition(PROJECT_TYPE_UNITY_GCCSDK_SFTOOLS);
	debug_printf("Project type: 0x%x", new->project);

	/* Initialise the path and folder names. */

	new->suite_folder = textdump_store(new->textdump, folder);
	new->source_folder = textdump_store(new->textdump, "tests");
	new->executable_folder = textdump_store(new->textdump, "absolute");

	if (new->suite_folder == TEXTDUMP_NULL || new->source_folder == TEXTDUMP_NULL ||
			new->executable_folder == TEXTDUMP_NULL) {
		suite_delete_instance(new);
		return FALSE;
	}

	/* Link ourselves into the list of loaded test suites. */

	new->next = suite_list;
	suite_list = new;

	debug_printf("\\DCreating new suite 0x%x...", new);

	/* Start running the tests. */

	suite_start_test_run(new, TRUE);

	return TRUE;
}

/**
 * Delete a Test Suite instance and delink it from the collection of
 * active instances.
 *
 * \param *instance	Pointer to the instance to be deleted.
 */

void suite_delete_instance(struct suite_block *instance)
{
	if (instance == NULL)
		return;

	debug_printf("\\DDeleting test suite 0x%x", instance);

	/* Delete the window. */

	window_delete_instance(instance->window);
	instance->window = NULL;

	/* Delete the textdump. */

	textdump_destroy(instance->textdump);
	instance->textdump = NULL;

	/* Unlink the instance from the list of suites. */

	struct suite_block **list = &suite_list;

	while (*list != NULL && *list != instance)
		list = &((*list)->next);

	if (*list != NULL)
		*list = instance->next;

	/* Free the memory associated with the file sets. */

	while (instance->file_sets != NULL)
		instance->file_sets = file_set_delete_instance(instance->file_sets);

	/* Free the memory associated with the file instances. */

	while (instance->file_instances != NULL)
		instance->file_instances = file_instance_delete_instance(instance->file_instances);

	/* Free the suite memory itself. */

	heap_free(instance);
}

/**
 * Delete all active Test Suite instances and free all resources
 * associated with them.
 */

void suite_delete_all(void)
{
	while (suite_list != NULL)
		suite_delete_instance(suite_list);
}

/**
 * Start a suite scan and test run.
 *
 * \param *instance	Pointer to the suite of interest.
 * \param full		TRUE to perform a full run; FALSE to run only changed
 *			files.
 */

static void suite_start_test_run(struct suite_block *instance, osbool full)
{
	if (suite_read_config_file(instance) == FALSE)
		return;

	if (suite_set_run_path_variable(instance) == FALSE)
		return;

	instance->file_sets = file_set_create_instance(instance, instance->file_sets, full);

	/* Update the window for the new set. */

	instance->current_file_set = instance->file_sets;
	file_set_add_to_window(instance->current_file_set, instance->window);

	/* Just in case all of the files generated errors... */

	struct window_status_field *status = file_set_get_status(instance->current_file_set);
	debug_printf("\\REnding status 0x%x", status);
	if (status != NULL) {
		window_update_status_field(instance->window, status);
		suite_unset_run_path_variable(instance);
	}
}

/**
 * Store an item of text in the instance's text dump, returning the index of
 * the string.
 *
 * \param *instance	Poiinter to the Test Suite instance.
 * \param *text		Pointer to the text to be stored.
 * \return		The text dump offset, or TEXTDUMP_NULL.
 */

unsigned suite_store_text(struct suite_block *instance, char *text)
{
	if (instance == NULL)
		return TEXTDUMP_NULL;

	return textdump_store(instance->textdump, text);
}

/**
 * Return the textdump base for a suite instance.
 *
 * \param *instance	Pointer to the Test Suite instance.
 * \return		Pointer to the text dump, or NULL on failure.
 */

char *suite_get_textdump_base(struct suite_block *instance)
{
	return (instance == NULL) ? NULL : textdump_get_base(instance->textdump);
}

/**
 * Return details of the project type of a suite instance.
 *
 * \param *instance	Pointer to the test suite instance of interest.
 * \return		Pointer to the project definition, or NULL.
 */

struct project_details *suite_get_project_details(struct suite_block *instance)
{
	return (instance == NULL) ? NULL : instance->project;
}

/**
 * Test whether a file set is the first one stored in a suite instance.
 *
 * \param *instance	Pointer to the test suite instance.
 * \param *set		Pointer to the file set instance to be checked.
 * \return		TRUE if the file set is first in the instance.
 */

osbool suite_file_set_is_first(struct suite_block *instance, struct file_set_block *set)
{
	return (instance == NULL || instance->file_sets == set) ? TRUE : FALSE;
}

/**
 * Add a file instance reference to the linked list in its parent test suite.
 *
 * NB: This returns the existing head of the linked list of file instaces. It
 * is assumed that its caller will use this to link itself into the head of
 * the chain.
 *
 * \param *instance		Pointer to the Test Suite instance to be updated.
 * \param *file_instance	Pointer to the file instance to be added.
 * \return			Pointer to the file instance which was
 *				previously at the head of the chain.
 */

struct file_instance_block *suite_store_file_instance(struct suite_block *instance, struct file_instance_block *file_instance)
{
	if (instance == NULL || file_instance == NULL)
		return NULL;

	struct file_instance_block *next = instance->file_instances;
	instance->file_instances = file_instance;

	return next;
}

/**
 * Return a path to a specific folder within a test suite, writing it into the
 * supplied buffer.
 *
 * \param *instance	Pointer to the test suite to be queried.
 * \param *buffer	Pointer to the buffer to take the returned path.
 * \param length	The length of the supplied buffer, in bytes.
 * \param folder	The folder to be returned.
 * \param leafname	A textdump offset for a leafname to append to the path
 *			in the buffer, or TEXTDUMP_NULL for none.
 * \return		TRUE if successful; FALSE on failure.
 */

osbool suite_read_folder_path(struct suite_block *instance, char *buffer, size_t length,
		enum suite_folder folder, unsigned leafname)
{
	if (buffer == NULL || length == 0)
		return FALSE;

	*buffer = '\0';

	if (instance == NULL)
		return FALSE;

	char *textdump_base =  textdump_get_base(instance->textdump);
	if (textdump_base == NULL)
		return FALSE;

	/* Set the folder to either a constant string, or a textdump offset. */

	unsigned folder_offset = TEXTDUMP_NULL;
	char *folder_name = NULL;

	switch (folder) {
	case SUITE_FOLDER_SOURCE:
		folder_offset = instance->source_folder;
		break;
	case SUITE_FOLDER_EXECUTABLE:
		folder_offset = instance->executable_folder;
		break;
	case SUITE_FOLDER_CONFIG_FILE:
		folder_name = "Unify";
		break;
	default:
		return FALSE;
	}

	/* Turn textdump offsets into a string pointer and fail if necessary. */

	if (folder_name == NULL && folder_offset != TEXTDUMP_NULL)
		folder_name = textdump_base + folder_offset;

	if (folder_name == NULL)
		return FALSE;

	string_printf(buffer, length, "%s.%s%s%s",
			textdump_base + instance->suite_folder,
			folder_name,
			(leafname == TEXTDUMP_NULL) ? "" : ".",
			(leafname == TEXTDUMP_NULL) ? "" : textdump_base + leafname
	);

	return TRUE;
}

/**
 * Update the number of entries for a fold in the suite window, and force a
 * redraw.
 *
 * This should be called every time there's an update to a file set, to
 * ensure that the window contents remain up to date.
 *
 * \param *instance		Pointer to the test suite being updated.
 * \param *set			Pointer to the file set that the update relates
 *				to, which will be used to decide whether to
 *				apply the update.
 * \param id			A window object ID for the fold contents, if
 *				one has previously be allocated.
 * \param entries		The number of entries to be contained in the
 *				fold.
 */

void suite_update_window_fold(struct suite_block *instance, struct file_set_block *set, unsigned id, int entries)
{
	if (instance == NULL || instance->window == NULL)
		return;

	struct window_status_field *status = file_set_get_status(set);

	/* We should always call file_set_get_status(), as this updates the
	 * status of the file set so that it knows if it has finished --
	 * even if it isn't currently on display.
	 */

	if (set != instance->current_file_set)
		return;

	window_update_fold(instance->window, id, entries);

	if (status != NULL) {
		window_update_status_field(instance->window, status);
		suite_unset_run_path_variable(instance);
	}
}

/**
 * Handle close events from an instance window.
 *
 * \param *data		Pointer to our client data, which should be a
 *			pointer to an instance.
 */

static void suite_close_handler(void *data)
{
	struct suite_block *instance = data;
	if (instance == NULL)
		return;

	suite_delete_instance(instance);
}

/**
 * Handle navigation events from an instance window.
 *
 * \param target	The navigation target.
 * \param *data		Pointer to our client data, which should be a
 *			pointer to an instance.
 */

static void suite_navigation_handler(enum window_navigation_target target, void *data)
{
	struct suite_block *instance = data;
	if (instance == NULL)
		return;

	struct file_set_block *destination = NULL;

	switch (target) {
	case WINDOW_NAVIGATION_TARGET_BACK:
		destination = file_set_find_previous_object(instance->current_file_set);
		break;
	case WINDOW_NAVIGATION_TARGET_FORWARD:
		destination = file_set_find_next_object(instance->current_file_set, instance->file_sets);
		break;
	case WINDOW_NAVIGATION_TARGET_LATEST:
		destination = instance->file_sets;
		break;
	}

	if (destination == NULL)
		return;

	instance->current_file_set = destination;
	file_set_add_to_window(instance->current_file_set, instance->window);
}

/**
 * Handle run events from an instance window.
 *
 * \param full		TRUE if this should be a full run; FALSE for an incremental
 *			update.
 * \param *data		Pointer to our client data, which should be a
 *			pointer to an instance.
 */

static void suite_run_handler(osbool full, void *data)
{
	struct suite_block *instance = data;
	if (instance == NULL)
		return;

	suite_start_test_run(instance, full);

}

/**
 * Handle line redraw events from an instance window.
 *
 * \param fold		The index of the fold containing the line.
 * \param entry		The index of the entry within the fold, or -1.
 * \param *content	Pointer to a struct in which to return the line data.
 * \param *data		Pointer to our client data, which should be a
 *			pointer to an instance.
 * \return		TRUE if the line was valid; else FALSE.
 */

static osbool suite_redraw_line_handler(int fold, int entry, struct window_line *content, void *data)
{
	struct suite_block *instance = data;
	if (instance == NULL)
		return FALSE;

	char *textdump_base = textdump_get_base(instance->textdump);
	if (textdump_base == NULL)
		return FALSE;

	struct file_instance_line_details line_details;

	if (!file_set_get_line_details(instance->current_file_set, fold, entry, &line_details))
		return FALSE;

	/* Sort out the object name. */

	if (line_details.name == TEXTDUMP_NULL)
		return FALSE;

	content->text = textdump_base + line_details.name;

	/* Map the object status. */

	if (line_details.status == FILE_INSTANCE_STATUS_PASS) {
		content->status = WINDOW_STATUS_PASS;
	} else if (line_details.status == FILE_INSTANCE_STATUS_FAIL) {
		content->status = WINDOW_STATUS_FAIL;
	} else if (line_details.status == FILE_INSTANCE_STATUS_TEST_SKIPPED) {
		content->status = WINDOW_STATUS_SKIP;
	} else if (line_details.status == FILE_INSTANCE_STATUS_TEST_ERROR) {
		content->status = WINDOW_STATUS_ERROR;
	} else if (FILE_INSTANCE_STATUS_IS_ERROR(line_details.status)) {
		content->status = WINDOW_STATUS_ERROR;
	} else if (FILE_INSTANCE_STATUS_IS_IN_FLIGHT(line_details.status)) {
		content->status = WINDOW_STATUS_UNKNOWN;
	} else {
		content->status = WINDOW_STATUS_UNKNOWN;
	}

	/* Handle the bits which vary between folds and entries. */

	if (entry < 0) {
		content->count = line_details.count;
		content->total = line_details.total;
		content->faded = !line_details.is_new;
	} else {
		content->faded = FALSE;
	}

	return TRUE;
}

/**
 * Handle log presence request events from an instance window.
 *
 * \param fold		The index of the fold containing the line.
 * \param *data		Pointer to our client data, which should be a
 *			pointer to an instance.
 * \param *this_log	Pointer to a variable in which to return TRUE
 *			or FALSE for the specific entry log.
 * \param *any_logs	Pointer to a variable in which to return TRUE
 *			or FALSE for any log in the set.
 */

static void suite_object_has_log(int fold, void *data, osbool *this_log, osbool *any_log)
{
	struct suite_block *instance = data;
	if (instance == NULL)
		return;

	file_set_get_object_log_status(instance->current_file_set, fold, this_log, any_log);
}

/**
 * Handle log open request events from an instance window.
 *
 * \param fold		The index of the fold containing the line.
 * \param *data		Pointer to our client data, which should be a
 *			pointer to an instance.
 */

static void suite_object_open_log(int fold, void *data)
{
	struct suite_block *instance = data;
	if (instance == NULL)
		return;

	file_set_open_object_log(instance->current_file_set, fold);
}

/**
 * Handle log save request events from an instance window.
 *
 * \param fold		The index of the fold containing the line.
 * \param *filename	Pointer to the filename to save to.
 * \param *data		Pointer to our client data, which should be a
 *			pointer to an instance.
 */

static osbool suite_object_save_log(int fold, char *filename, void *data)
{
	struct suite_block *instance = data;
	if (instance == NULL)
		return FALSE;

	return file_set_save_object_log(instance->current_file_set, fold, filename);
}

/**
 * Handle all log save request events from an instance window.
 *
 * \param *filename	Pointer to the filename to save to.
 * \param *data		Pointer to our client data, which should be a
 *			pointer to an instance.
 */

static osbool suite_object_save_all_logs(char *filename, void *data)
{
	struct suite_block *instance = data;
	if (instance == NULL)
		return FALSE;

	return file_set_save_all_logs(instance->current_file_set, filename);
}

/**
 * Handle line redraw events from an instance window.
 *
 * \param fold		The index of the fold containing the line.
 * \param entry		The index of the entry within the fold, or -1.
 * \param *content	Pointer to a struct in which to return the line data.
 * \param *data		Pointer to our client data, which should be a
 *			pointer to an instance.
 * \return		TRUE if the line was valid; else FALSE.
 */

static osbool suite_object_info_handler(int fold, struct file_dialogue_data *info, void *data)
{
	struct suite_block *instance = data;
	if (instance == NULL)
		return FALSE;

	char *textdump_base = textdump_get_base(instance->textdump);
	if (textdump_base == NULL)
		return FALSE;

	struct file_instance_object_details object_details;

	if (!file_set_get_object_details(instance->current_file_set, fold, &object_details))
		return FALSE;

	info->name = (object_details.name != TEXTDUMP_NULL) ?
			textdump_base + object_details.name : NULL;
	info->run_timestamp = object_details.timestamp;

	info->source_filename = (object_details.source_filename != TEXTDUMP_NULL) ?
			textdump_base + object_details.source_filename : NULL;
	info->source_timestamp = object_details.source_timestamp;

	info->executable_filename = (object_details.executable_filename != TEXTDUMP_NULL) ?
			textdump_base + object_details.executable_filename : NULL;
	info->executable_timestamp = object_details.executable_timestamp;

	return TRUE;
}

/**
 * Look for a suite config file in the root folder, and parse it if found.
 * This will update the settings within the suite.
 *
 * \param *instance	Pointer to the suite to update.
 * \return		TRUE if successful; otherwise FALSE.
 */

static osbool suite_read_config_file(struct suite_block *instance)
{
	if (instance == NULL)
		return FALSE;

	char filename[256];
	if (!suite_read_folder_path(instance, filename, sizeof(filename), SUITE_FOLDER_CONFIG_FILE, TEXTDUMP_NULL))
		return FALSE;

	char section[sf_MAX_CONFIG_FILE_BUFFER], token[sf_MAX_CONFIG_FILE_BUFFER], value[sf_MAX_CONFIG_FILE_BUFFER];

	*section = '\0';
	*token = '\0';
	*value = '\0';

	/* Open the file and read it. Note that not opening the file is a valid
	 * thing, because there doesn't need to be a config file.
	 */

	osbool unexpected_tokens = FALSE;

	debug_printf("Reading config file %s", filename);

	FILE *fh = fopen(filename, "r");
	if (fh == NULL)
		return TRUE;

	while (config_read_token_pair(fh, token, value, section) != sf_CONFIG_READ_EOF) {
		if (string_nocase_strcmp(token, "SuiteName") == 0) {
			char *textdump_base = suite_get_textdump_base(instance);

			if (*value == '\0') {
				instance->name = TEXTDUMP_NULL;
			} else if (instance->name == TEXTDUMP_NULL ||
					strcmp(textdump_base + instance->name, value) != 0) {
				instance->name = suite_store_text(instance, value);
			}
		} else if (string_nocase_strcmp(token, "RunPathVar") == 0) {
			char *textdump_base = suite_get_textdump_base(instance);

			if (*value == '\0') {
				instance->run_path_variable = TEXTDUMP_NULL;
			} else if (instance->run_path_variable == TEXTDUMP_NULL ||
					strcmp(textdump_base + instance->run_path_variable, value) != 0) {
				instance->run_path_variable = suite_store_text(instance, value);
			}
		} else if (string_nocase_strcmp(token, "TaskMemory") == 0) {
			errno = 0;
			char *end = NULL;
			long result = strtol(value, &end, 10);

			if (errno == ERANGE) {
				error_msgs_report_error("ConfigBadMem");
				continue;
			} else if (end == value) {
				error_msgs_report_error("ConfigBadMem");
				continue;
			}

			if (result > 0 && result <= INT_MAX)
				instance->task_slot_size = result;
			else
				error_msgs_report_error("ConfigBadMem");
		} else {
			unexpected_tokens = TRUE;
		}
	}

	fclose(fh);

	if (unexpected_tokens)
		error_msgs_report_error("ConfigTokens");

	return TRUE;
}

/**
 * Set the run path variable for the suite.
 *
 * \param *instance	Pointer to the suite to update.
 * \return		TRUE if successful; else FALSE.
 */

static osbool suite_set_run_path_variable(struct suite_block *instance)
{
	if (instance == NULL)
		return FALSE;

	/* Make sure that we don't claim credit for this by mistake. */

	instance->we_set_run_path_variable = FALSE;

	if (instance->run_path_variable == TEXTDUMP_NULL)
		return TRUE;

	/* We have a variable to set, so check to see if it already exists. */

	char *var_name = suite_get_textdump_base(instance) + instance->run_path_variable;

	int var_len = 0;
	os_read_var_val_size(var_name, 0, os_VARTYPE_STRING, &var_len, NULL);

	if (var_len != 0)
		return TRUE;

	char folder[1024];
	if (suite_read_folder_path(instance, folder, sizeof(folder), SUITE_FOLDER_EXECUTABLE, TEXTDUMP_NULL) == FALSE)
		return FALSE;

	debug_printf("Setting %s to %s", var_name, folder);

	if (xos_set_var_val(var_name, (const byte *) folder, strlen(folder), 0, os_VARTYPE_STRING, NULL, NULL) != NULL)
		return FALSE;

	instance->we_set_run_path_variable = TRUE;

	return TRUE;
}

/**
 * Unset the run path variable for the suite.
 *
 * \param *instance	Pointer to the suite to update.
 */

static void suite_unset_run_path_variable(struct suite_block *instance)
{
	if (instance == NULL || instance->we_set_run_path_variable == FALSE)
		return;

	char *var_name = suite_get_textdump_base(instance) + instance->run_path_variable;
	xos_set_var_val(var_name, NULL, -1, 0, os_VARTYPE_STRING, NULL, NULL);

	debug_printf("Unset %s", var_name);

	instance->we_set_run_path_variable = FALSE;
}
