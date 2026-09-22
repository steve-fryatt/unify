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
 * \file: file_instance.c
 *
 * Test File implementation.
 */

/* ANSI C header files */

#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include <oslib/os.h>
#include <oslib/osgbpb.h>
#include <oslib/wimp.h>

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/event.h>
#include <sflib/heap.h>
#include <sflib/string.h>

/* Application header files */

#include "file_instance.h"

#include "date_time.h"
#include "file_set.h"
#include "flexutils.h"
#include "log.h"
#include "project.h"
#include "runner.h"
#include "suite.h"
#include "test_instance.h"
#include "textdump.h"
#include "window.h"

/**
 * The increments to allocate space for test objects.
 */

#define FILE_INSTANCE_ALLOCATION_UNIT 10

/**
 * The maximum length of a test file name.
 */

#define FILE_INSTANCE_NAME_LEN 256

/**
 * The maximum length of a runner command.
 */

#define FILE_INSTANCE_COMMAND_LEN (FILE_INSTANCE_NAME_LEN + 64)

/**
 * A suitable array index hasn't been found.
 */

#define FILE_INSTANCE_NOT_FOUND ((unsigned) 0xffffffffu)

/* Structure definitions. */

/**
 * Details of a file within a file instance.
 */

struct file_instance_details {
	/**
	 * Text dump offset to the name of the file.
	 */
	unsigned name;

	/**
	 * The size of the file as recorded in the instance.
	 */
	int size;

	/**
	 * The timestamp of the file as recorded in the instance.
	 */
	uint64_t timestamp;
};

/**
 * The definition of a file instance.
 */

struct file_instance_block {
	/**
	 * Pointer to the parent test suite.
	 */
	struct suite_block *parent;

	/**
	 * Pointer to the file set where this instance orifginated.
	 */
	struct file_set_block *initial;

	/**
	 * Pointer to the next file in the suite, or NULL.
	 */
	struct file_instance_block *next;

	/**
	 * The textdump reference of the base name of the file, with no
	 * suffixes.
	 */
	unsigned name;

	/**
	 * Details of the source file.
	 */
	struct file_instance_details source;

	/**
	 * Details of the executable file.
	 */
	struct file_instance_details executable;

	/**
	 * The window object associated with the file.
	 */
	unsigned window_object;

	/**
	 * The log data associated with the file.
	 */
	struct log_instance *log;

	/**
	 * The pass, fail or error status of the file instance.
	 */
	enum file_instance_status status;

	/**
	 * Flex block pointer to the list of tests in the file instance.
	 */
	struct test_instance_block *tests;

	/**
	 * The space allocated to tests in the test list.
	 */
	size_t test_space;

	/**
	 * The number of tests in the test list.
	 */
	size_t test_count;
};

/* Global variables. */


/* Static function prototypes. */

static struct file_instance_block *file_instance_clone_instance(struct file_set_block *initial, struct file_instance_block *template, osbool use_source);
static void file_instance_store_file(struct suite_block *parent, struct file_instance_details *details, osgbpb_info *entry);
static void file_instance_found_definition(struct file_instance_block *instance, char *name, int line);
static void file_instance_found_call(struct file_instance_block *instance, char *name, int line);
static void file_instance_scan_log(struct file_instance_block *instance);
static void file_instance_found_test_result(struct file_instance_block *instance, char *name, int line, enum project_outcome outcome);
static void file_instance_found_summary(struct file_instance_block *instance, int tests, int passed, int failed, int skipped);
static void file_instance_found_overall_result(struct file_instance_block *instance, enum project_outcome outcome);
static unsigned file_instance_find_test(struct file_instance_block *instance, char *name);
static unsigned file_instance_add_test(struct file_instance_block *instance);

/**
 * Create a new file instance and link it to the supplied parent suite.
 *
 * \param *parent	Pointer to the parent suite.
 * \param *initial	Pointer to the file set which created the instance.
 * \param *name		Pointer to the name of the file.
 * \return		Pointer to the new file instance, or NULL on error.
 */

struct file_instance_block *file_instance_create_instance(struct suite_block *parent, struct file_set_block *initial, char *name)
{
	if (parent == NULL || initial == NULL || name == NULL)
		return NULL;

	struct file_instance_block *new = heap_alloc(sizeof(struct file_instance_block));
	if (new == NULL)
		return NULL;

	new->parent = parent;
	new->initial = initial;
	new->status = FILE_INSTANCE_STATUS_UNKNOWN;
	new->source.name = TEXTDUMP_NULL;
	new->executable.name = TEXTDUMP_NULL;
	new->window_object = WINDOW_NULL_FOLD;
	new->tests = NULL;
	new->test_space = FILE_INSTANCE_ALLOCATION_UNIT;
	new->test_count = 0;
	new->log = NULL;

	new->name = suite_store_text(parent, name);
	if (new->name == TEXTDUMP_NULL) {
		file_instance_delete_instance(new);
		return NULL;
	}

	if (!flexutils_allocate((void **) &(new->tests), sizeof(struct test_instance_block), new->test_space)) {
		heap_free(new);
		return NULL;
	}

	new->next = suite_store_file_instance(parent, new);

	debug_printf("Creating new file instance 0x%x in suite 0x%x for %s", new, parent, name);

	return new;
}

/**
 * Clone an existing file instance and link it to the same parent suite.
 *
 * NB: Cloning does not copy tests across from the template instance. It is
 * assumed that if we're cloning a new instance, enough has changed that the
 * tests will need to be re-scanned.
 *
 * \param *initial	Pointer to the file set which created the instance.
 * \param *template	Pointer to the file instance which is to be used as a
 *			template for the clone.
 * \param use_source	TRUE if the details of the source file should be
 *			cloned as part of the operation; otherwise FALSE.
 * \return		Pointer to the new file instance, or NULL on error.
 */

static struct file_instance_block *file_instance_clone_instance(struct file_set_block *initial, struct file_instance_block *template, osbool use_source)
{
	if (initial == NULL || template == NULL)
		return NULL;

	struct file_instance_block *new = heap_alloc(sizeof(struct file_instance_block));
	if (new == NULL)
		return NULL;

	new->parent = template->parent;
	new->initial = initial;
	new->status = FILE_INSTANCE_STATUS_UNKNOWN;
	new->source.name = (use_source == TRUE) ? template->source.name : TEXTDUMP_NULL;
	new->source.size = (use_source == TRUE) ? template->source.size : 0;
	new->source.timestamp = (use_source == TRUE) ? template->source.timestamp : 0;
	new->executable.name = TEXTDUMP_NULL;
	new->executable.size = 0;
	new->executable.timestamp = 0;
	new->window_object = template->window_object;
	new->tests = NULL;
	new->test_space = FILE_INSTANCE_ALLOCATION_UNIT;
	new->test_count = 0;
	new->log = NULL;

	new->name = template->name;

	if (!flexutils_allocate((void **) &(new->tests), sizeof(struct test_instance_block), new->test_space)) {
		heap_free(new);
		return NULL;
	}

	new->next = suite_store_file_instance(template->parent, new);

	debug_printf("Cloning new file instance 0x%x in suite 0x%x for %s",
			new, template->parent, suite_get_textdump_base(template->parent) + new->name);

	return new;
}

/**
 * Delete a Test File instance.
 *
 * NB: It is left up to the caller to do something sensible with any linked
 * list references. Currently this is called by the suite when cleaning up on
 * deletion, and that just picks its way down the list removing items as it
 * goes.
 *
 * \param *instance	Pointer to the instance to be deleted.
 * \return		Pointer to the next file instance known to the instance,
 *			or NULL if there wasn't one.
 */

struct file_instance_block *file_instance_delete_instance(struct file_instance_block *instance)
{
	if (instance == NULL)
		return NULL;

	struct file_instance_block *next = instance->next;

	/* Free the memory associated with the instance. */

	if (instance->log != NULL)
		log_delete_instance(instance->log);

	flexutils_free((void **) &(instance->tests));

	heap_free(instance);

	debug_printf("File instance deleted: 0x%x", instance);

	return next;
}

/**
 * Given a window instance, request that a file instance adds itself to the
 * windiow contents.
 *
 * \param *instance	Pointer to the instance to add.
 * \param *window	Pointer to the window instance to take the file.
 */

void file_instance_add_to_window(struct file_instance_block *instance, struct window_instance *window)
{
	if (instance == NULL || window == NULL)
		return;

	instance->window_object = window_add_new_fold(
			window,
			instance->window_object,
			instance->test_count
	);
}

/**
 * Return the details for required for redrawing a display line of a Test File
 * instance.
 *
 * \param *instance	Pointer to the instance of interest.
 * \param *set		Pointer to the file set instance requesting the details.
 * \param test		The index of the test of interest, or -1 for the file.
 * \param *details	Pointer to a struct in which the details should be
 *			returned.
 * \return		TRUE if valid details were returned; else FALSE.
 */

osbool file_instance_get_line_details(struct file_instance_block *instance, struct file_set_block *set, int test,
		struct file_instance_line_details *details)
{
	if (instance == NULL || details == NULL)
		return FALSE;

	if (test < 0) {
		details->name = instance->name;
		details->status = instance->status;
		details->total = instance->test_count;
		details->count = 0;
	} else if (test < instance->test_count) {
		if (test_instance_get_line_details(&(instance->tests[test]), details) == FALSE)
			return FALSE;
	} else {
		return FALSE;
	}
	details->is_new = (instance->initial == set) ? TRUE : FALSE;

	return TRUE;
}

/**
 * Return details of a file instance.
 * \param *instance	Pointer to the instance of interest.
 * \param *details	Pointer to a struct in which the details should be
 *			returned.
 * \return		TRUE if valid details were returned; else FALSE.
 */

osbool file_instance_get_object_details(struct file_instance_block *instance, struct file_instance_object_details *details)
{
	if (instance == NULL || details == NULL)
		return FALSE;

	details->name = instance->name;
	details->timestamp = file_set_get_timestamp(instance->initial);
	details->status = instance->status;
	details->source_filename = instance->source.name;
	details->source_timestamp = instance->source.timestamp;
	details->executable_filename = instance->executable.name;
	details->executable_timestamp = instance->executable.timestamp;

	return TRUE;
}

/**
 * Report whether a file instance has a source file identified.
 *
 * \param *instance	Pointer to the instance of interest.
 * \return		TRUE if a source file is specified; otherwise FALSE.
 */

osbool file_instance_has_source(struct file_instance_block *instance)
{
	return (instance == NULL || instance->source.name == TEXTDUMP_NULL) ? FALSE : TRUE;
}

/**
 * Report whether a file instance has a log associated with it.
 *
 * \param *instance	Pointer to the instance of interest.
 * \return		TRUE if a log is available; otherwise FALSE.
 */

osbool file_instance_has_log(struct file_instance_block *instance)
{
	return (instance == NULL || instance->log == NULL) ? FALSE : TRUE;
}

/**
 * Open the log for a file instance.
 *
 * \param *instance	Pointer to the instance of intest.
 * \return		TRUE if the log was opened; else FALSE.
 */

osbool file_instance_open_log(struct file_instance_block *instance)
{
	if (instance == NULL || instance->log == NULL)
		return FALSE;

	log_open_window(instance->log);

	return TRUE;
}

/**
 * Write the log file for an instance to a file handle.
 *
 * \param *instance	Pointer to the instance of interest.
 * \param *file		The file handle to write to.
 * \param header	TRUE to write a header for the file; else FALSE.
 * \return		TRUE if the log was written; else FALSE.
 */

osbool file_instance_save_log(struct file_instance_block *instance, FILE *file, osbool header)
{
	if (instance == NULL || instance->log == NULL || file == NULL)
		return FALSE;

	/* If required, write a header block for the log.*/

	if (header == TRUE) {
		if (ftell(file) > 0) {
			/* Separate the log from any previous ones. */

			if (fputs("\n", file) == EOF)
				return FALSE;
		}

		/* Write the file name. */

		char *textbase = suite_get_textdump_base(instance->parent);

		if (fprintf(file, "# File: %s\n", textbase + instance->name) < 0)
			return FALSE;

		/* Write the run timestamp. */

		uint64_t date = file_set_get_timestamp(instance->initial);
		char buffer[64];
		date_time_write_standard_string(date, buffer, sizeof(buffer));

		if (fprintf(file, "# Date: %s\n", buffer) < 0)
			return FALSE;

		/* Blank line following the header. */

		if (fputs("\n", file) == EOF)
			return FALSE;
	}

	/* Write out the log contents. */

	return log_write_to_file(instance->log, file);
}

/**
 * Compare the details of an object found on disc with those stored in a
 * file instance.
 *
 * \param *instance	Pointer to the instance to be checked.
 * \param *clean_name	Pointer to a string containing the base name of the
 *			object with any suffix removed.
 * \param *entry	Pointer to the data for the object returned from OS_GBPB.
 * \return		TRUE if the object matches; else FALSE.
 */

osbool file_instance_compare_object(struct file_instance_block *instance, char *clean_name, osgbpb_info *entry)
{
	if (instance == NULL)
		return FALSE;

	char *textdump_base = suite_get_textdump_base(instance->parent);
	if (textdump_base == NULL || instance->name == TEXTDUMP_NULL)
		return FALSE;

	debug_printf("Comparing against %s", textdump_base + instance->name);

	if (string_nocase_strcmp(clean_name, textdump_base + instance->name) == 0)
		return TRUE;

	return FALSE;
}

/**
 * Add the details of a source file to a file instance, returning a pointer to
 * the (possibly new) instance.
 *
 * For a new instance, the file will be added with only some basic sanity
 * checks. If this is an updated instance, then if the file details appear to
 * have changed, the instance will be cloned and a pointer to the clone
 * returned.
 *
 * \param *instance	Pointer to the file instance in question.
 * \param *set		Pointer to the file set which is being constructed.
 * \param *entry	Pointer to the OS_GBPB data for the file to be added.
 * \return		A pointer to the instance, which will either be the same
 *			one originally supplied or a new clone.
 */

struct file_instance_block *file_instance_add_source_file(struct file_instance_block *instance, struct file_set_block *set, osgbpb_info *entry)
{
	if (instance == NULL)
		return NULL;

	/* If this is a new instance, do some error checks. */

	if (instance->initial == set) {
		/* If there's already an error, bail out. */

		if (instance->status != FILE_INSTANCE_STATUS_UNKNOWN)
			return instance;

		/* There shouldn't be a file already! */

		if (instance->source.name != TEXTDUMP_NULL) {
			instance->status = FILE_INSTANCE_STATUS_ERROR_DUPLICATE_SOURCE;
			return instance;
		}
	}

	/* Check the file details. The names should match, we hope! If the file appears
	 * to have changed, create a new instance.
	 */

	uint64_t timestamp = date_time_read_osgbpb_timestamp(entry);

	if (instance->source.name != TEXTDUMP_NULL) {
		debug_printf("Testing the source file details...");
		debug_printf("Existing time: %" PRId64 " New time: %"PRId64, instance->source.timestamp, timestamp);
		if (instance->source.size == entry->size && instance->source.timestamp == timestamp)
			return instance;

		instance = file_instance_clone_instance(set, instance, FALSE);
	}

	file_instance_store_file(instance->parent, &(instance->source), entry);

	return instance;
}

/**
 * Add the details of an executable file to a file instance, returning a pointer
 * to the (possibly new) instance.
 *
 * For a new instance, the file will be added with only some basic sanity
 * checks. If this is an updated instance, then if the file details appear to
 * have changed, the instance will be cloned and a pointer to the clone
 * returned.
 *
 * \param *instance	Pointer to the file instance in question.
 * \param *set		Pointer to the file set which is being constructed.
 * \param *entry	Pointer to the OS_GBPB data for the file to be added.
 * \return		A pointer to the instance, which will either be the same
 *			one originally supplied or a new clone.
 */

struct file_instance_block *file_instance_add_executable_file(struct file_instance_block *instance, struct file_set_block *set, osgbpb_info *entry)
{
	if (instance == NULL)
		return NULL;

	/* If this is a new instance, do some error checks. */

	if (instance->initial == set) {
		/* If there's already an error, bail out. */

		if (instance->status != FILE_INSTANCE_STATUS_UNKNOWN)
			return instance;

		/* There shouldn't be a file already! */

		if (instance->executable.name != TEXTDUMP_NULL) {
			instance->status = FILE_INSTANCE_STATUS_ERROR_DUPLICATE_SOURCE;
			return instance;
		}
	}

	/* Check the file details. The names should match, we hope! If the file appears
	 * to have changed, create a new instance.
	 */

	uint64_t timestamp = date_time_read_osgbpb_timestamp(entry);

	if (instance->executable.name != TEXTDUMP_NULL) {
		debug_printf("Testing the executable file details...");
		if (instance->executable.size == entry->size && instance->executable.timestamp == timestamp)
			return instance;

		instance = file_instance_clone_instance(set, instance, TRUE);
	}

	file_instance_store_file(instance->parent, &(instance->executable), entry);

	return instance;
}

/**
 * Store a file's details within a file instance.
 *
 * \param *parent	Pointer to the parent suite.
 * \param *details	Pointer to the file details within the file instance
 *			which are to be updated.
 * \param *entry	Pointer to the OS_GBPB data for the file to be added.
 */

static void file_instance_store_file(struct suite_block *parent, struct file_instance_details *details, osgbpb_info *entry)
{
	if (parent == NULL || details == NULL || entry == NULL)
		return;

	details->name = suite_store_text(parent, entry->name);
	details->size = entry->size;

	if ((entry->load_addr & 0xfff00000u) == 0xfff00000u) {
		details->timestamp = entry->exec_addr | ((uint64_t) (entry->load_addr & 0xffu) << 32);
	} else {
		details->timestamp = 0;
	}
}

/**
 * Perform some pre-flight validation on a new file instance.
 *
 * \param *instance	Pointer to the file instance to be validated.
 * \param *set		Pointer to the file set block requesting the validation.
 * \return		TRUE if the file instance is new to this file set;
 *			otherwise FALSE.
 */

osbool file_instance_validate_files(struct file_instance_block *instance, struct file_set_block *set)
{
	if (instance == NULL)
		return FALSE;

	/* Check whether the instance belongs to the calling file set. If it doesn't,
	 * then it isn't new and doesn't require validation.
	 */

	if (instance->initial != set)
		return FALSE;

	/* Validate the file instance. */

	switch (instance->status) {
	case FILE_INSTANCE_STATUS_UNKNOWN:
		if (instance->source.name != TEXTDUMP_NULL && instance->executable.name != TEXTDUMP_NULL)
			instance->status = FILE_INSTANCE_STATUS_READY_TO_SCAN;
		else if (instance->source.name == TEXTDUMP_NULL && instance->executable.name == TEXTDUMP_NULL)
			instance->status = FILE_INSTANCE_STATUS_ERROR_NO_FILES;
		else if (instance->source.name == TEXTDUMP_NULL)
			instance->status = FILE_INSTANCE_STATUS_ERROR_NO_SOURCE;
		else if (instance->executable.name == TEXTDUMP_NULL)
			instance->status = FILE_INSTANCE_STATUS_ERROR_NO_EXECUTABLE;
		else
			instance->status = FILE_INSTANCE_STATUS_ERROR_BAD_FILES;
		break;

	default:
		break;
	}

	return TRUE;
}

/**
 * Scan the source file associated with a file instance, so that the tests
 * defined within it can be added to the file instance.
 *
 * This calls the source scan functions provided by the project type associated
 * with the parent test suite.
 *
 * \param *instance	Pointer to the file instance to be scanned.
 */

void file_instance_scan_source(struct file_instance_block *instance)
{
	if (instance == NULL || instance->status != FILE_INSTANCE_STATUS_READY_TO_SCAN)
		return;

	struct project_source_callbacks callbacks = {
		.owner = instance,
		.found_definition = file_instance_found_definition,
		.found_call = file_instance_found_call
	};

	struct project_details *project = suite_get_project_details(instance->parent);
	if (project == NULL || project->source_decoder == NULL) {
		instance->status = FILE_INSTANCE_STATUS_ERROR_FAILED_TO_SCAN_SOURCE;
		return;
	}

	/* Get the filename of the source. */

	char filename[FILE_INSTANCE_NAME_LEN];
	if (!suite_read_folder_path(instance->parent, filename, FILE_INSTANCE_NAME_LEN,
			SUITE_FOLDER_SOURCE, instance->source.name)) {
		instance->status = FILE_INSTANCE_STATUS_ERROR_FAILED_TO_SCAN_SOURCE;
		return;
	}

	/* Scan the file. */

	FILE *fh = fopen(filename, "r");
	if (fh == NULL) {
		instance->status = FILE_INSTANCE_STATUS_ERROR_FAILED_TO_SCAN_SOURCE;
		return;
	}

	if (project->source_decoder(fh, &callbacks) == TRUE)
		instance->status = FILE_INSTANCE_STATUS_READY_TO_RUN;
	else
		instance->status = FILE_INSTANCE_STATUS_ERROR_FAILED_TO_SCAN_SOURCE;

	fclose(fh);
}

/**
 * Handle callbacks from the project source scanner, reporting that a test
 * function definition has been identified.
 *
 * \param *instance	Pointer to the associated file instance.
 * \param *name		Pointer to the name of the identified test function.
 * \param line		The line number from the source file at which the
 *			definition was located, or -1 if this isn't known.
 */

static void file_instance_found_definition(struct file_instance_block *instance, char *name, int line)
{
	debug_printf("We've found a definition of %s at line %d", name, line);

	/* Find the test or create a new one. */

	unsigned test = file_instance_find_test(instance, name);

	if (test == FILE_INSTANCE_NOT_FOUND) {
		instance->status = FILE_INSTANCE_STATUS_ERROR_BAD_TESTS;
		return;
	}

	if (test_instance_add_location(&(instance->tests[test]), TEST_INSTANCE_LOCATION_DEFINITION, -1) == FALSE)
		instance->status = FILE_INSTANCE_STATUS_ERROR_BAD_TESTS;

	debug_printf("This has become test %u in the file.", test);
}

/**
 * Handle callbacks from the project source scanner, reporting that a test
 * function call has been identified.
 *
 * If the project doesn't have separate definitions and calls, a parser may call
 * this immediately after calling file_instance_found_definition().
 *
 * \param *instance	Pointer to the associated file instance.
 * \param *name		Pointer to the name of the identified test function.
 * \param line		The line number from the source file at which the
 *			call was located, or -1 if this isn't known.
 */

static void file_instance_found_call(struct file_instance_block *instance, char *name, int line)
{
	debug_printf("We've found a call to %s at line %d", name, line);

	/* Find the test or create a new one. */

	unsigned test = file_instance_find_test(instance, name);

	if (test == FILE_INSTANCE_NOT_FOUND) {
		instance->status = FILE_INSTANCE_STATUS_ERROR_BAD_TESTS;
		return;
	}

	if (test_instance_add_location(&(instance->tests[test]), TEST_INSTANCE_LOCATION_CALL, -1) == FALSE)
		instance->status = FILE_INSTANCE_STATUS_ERROR_BAD_TESTS;

	debug_printf("This has become test %u in the file.", test);
}

/**
 * Attempt to queue a file instance for execution.
 *
 * \param *instance	Pointer to the file instance to be executed.
 */

void file_instance_execute(struct file_instance_block *instance)
{
	if (instance == NULL || instance->status != FILE_INSTANCE_STATUS_READY_TO_RUN)
		return;

	/* Check that we have a file to run. */

	if (instance->executable.name == TEXTDUMP_NULL) {
		instance->status = FILE_INSTANCE_STATUS_ERROR_FAILED_TO_QUEUE;
		return;
	}

	/* Get the filename of the executable. */

	char filename[FILE_INSTANCE_NAME_LEN];
	if (!suite_read_folder_path(instance->parent, filename, FILE_INSTANCE_NAME_LEN,
			SUITE_FOLDER_EXECUTABLE, instance->executable.name)) {
		instance->status = FILE_INSTANCE_STATUS_ERROR_FAILED_TO_QUEUE;
		return;
	}

	/* Write the command. */

	char command[FILE_INSTANCE_COMMAND_LEN];

	// TODO -- The command probably should come from the suite type.

	string_printf(command, FILE_INSTANCE_COMMAND_LEN, "Run %s", filename);

	if (runner_add_task(command, instance))
		instance->status = FILE_INSTANCE_STATUS_IN_QUEUE;
	else
		instance->status = FILE_INSTANCE_STATUS_ERROR_FAILED_TO_QUEUE;
}

/**
 * Accept TaskWindow output from the runner and add it to the log for a
 * file instance. If a log doesn't exist, it will be created.
 *
 * \param *instance	Pointer to the file instance to be updated.
 * \param *content	Pointer to the new log content. This does not need
 *			to be zero-terminated.
 * \param length	The length of the content, in bytes.
 */

void file_instance_take_log_content(struct file_instance_block *instance, char *content, size_t length)
{
	if (instance == NULL || content == NULL)
		return;

	if (instance->log == NULL) {
		uint64_t timestamp = file_set_get_timestamp(instance->initial);

		char date[DATE_TIME_LEN];
		date_time_write_standard_string(timestamp, date, DATE_TIME_LEN);

		char *textbase = suite_get_textdump_base(instance->parent);
		if (textbase == NULL || instance->name == TEXTDUMP_NULL)
			return;

		char title[FILE_INSTANCE_NAME_LEN + DATE_TIME_LEN + 16];
		string_printf(title, sizeof(title), "%s (at %s)", textbase + instance->name, date);

		instance->log = log_create_instance(title);
	}

	if (instance->log != NULL)
		log_add_text(instance->log, content, length);
}

/**
 * Called by the runner if the attempt to launch the executable in TaskWindow
 * failed.
 *
 * \param *instance		Pointer to the instance affected.
 */

void file_instance_execution_falied(struct file_instance_block *instance)
{
	if (instance == NULL)
		return;

	instance->status = FILE_INSTANCE_STATUS_ERROR_FALIED_TO_EXECUTE;
}

/**
 * Called by the runner when the task has completed execution.
 *
 * \param *instance		Pointer to the instance affected.
 */

void file_instance_execution_finished(struct file_instance_block *instance)
{
	if (instance == NULL)
		return;

	if (instance->log != NULL) {
		instance->status = FILE_INSTANCE_STATUS_EXECUTED;

		log_finish_text(instance->log);
		file_instance_scan_log(instance);
	} else {
		instance->status = FILE_INSTANCE_STATUS_ERROR_NO_OUTPUT;
	}
}

/**
 * Scan the log associated with a file instance, so that the test results
 * contained within it can be added to the file instance.
 *
 * This calls the log scan functions provided by the project type associated
 * with the parent test suite.
 *
 * \param *instance	Pointer to the file instance to be scanned.
 */

static void file_instance_scan_log(struct file_instance_block *instance)
{
	if (instance == NULL || instance->status != FILE_INSTANCE_STATUS_EXECUTED)
		return;

	struct project_log_callbacks callbacks = {
		.owner = instance,
		.found_test_result = file_instance_found_test_result,
		.found_summary = file_instance_found_summary,
		.found_overall_result = file_instance_found_overall_result
	};

	struct project_details *project = suite_get_project_details(instance->parent);
	if (project == NULL || project->log_decoder == NULL) {
		instance->status = FILE_INSTANCE_STATUS_ERROR_FAILED_TO_SCAN_LOG;
		return;
	}

	/* Scan the log. */

	if (project->log_decoder(instance->log, &callbacks) == TRUE)
		instance->status = FILE_INSTANCE_STATUS_READY_TO_REPORT;
	else
		instance->status = FILE_INSTANCE_STATUS_ERROR_FAILED_TO_SCAN_LOG;
}

/**
 * TODO
 */

static void file_instance_found_test_result(struct file_instance_block *instance, char *name, int line, enum project_outcome outcome)
{
	debug_printf("Found test result: name=%s, line=%d, outcome=%d", name, line, outcome);
}

/**
 * TODO
 */

static void file_instance_found_summary(struct file_instance_block *instance, int tests, int passed, int failed, int skipped)
{
	debug_printf("Found summary: pass=%d, fail=%d, skip=%d, total=%d", passed, failed, skipped, tests);
}

/**
 * TODO
 */

static void file_instance_found_overall_result(struct file_instance_block *instance, enum project_outcome outcome)
{
	debug_printf("Found overall result: %d", outcome);
}

/**
 * Given a test function name, locate a matching test within the file instance.
 * This returns an index into the tests flex array. If no match was found, the
 * returned index will contain a newly-created test record.
 *
 * \param *instance	Pointer to the file instance to be searched.
 * \param *name		Pointer to the function name to be searched for.
 * \return		The index of the record, or FILE_INSTANCE_NOT_FOUND if
 *			for some reason no match could be found and no new
 *			record could be created.
 */

static unsigned file_instance_find_test(struct file_instance_block *instance, char *name)
{
	if (instance == NULL || instance->tests == NULL)
		return FILE_INSTANCE_NOT_FOUND;

	for (unsigned i = 0; i < instance->test_count; i++) {
		if (test_instance_compare_test(&(instance->tests[i]), instance->parent, name) == TRUE)
			return i;
	}

	unsigned new = file_instance_add_test(instance);
	if (new == FILE_INSTANCE_NOT_FOUND)
		return FILE_INSTANCE_NOT_FOUND;

	/* Store the name here, because if it shifts the flex heap that would
	 * break the pointer to the test instance if we did it in the called
	 * function!
	 */

	unsigned test_name = suite_store_text(instance->parent, name);

	test_instance_populate_new_test(&(instance->tests[new]), test_name);

	return new;
}

/**
 * Add a new test instance record into the tests array of a file instance.
 *
 * \param *instance	Pointer to the file instance in which to create the
 *			new test.
 * \return		The index into the array of the new test, or
 *			FILE_INSTANCE_NOT_FOUND if the operation failed.
 */

static unsigned file_instance_add_test(struct file_instance_block *instance)
{
	if (instance == NULL)
		return FILE_INSTANCE_NOT_FOUND;

	if (instance->test_count >= instance->test_space) {
		debug_printf("We need more space...");
		size_t new_space = instance->test_space;

		while (new_space <= instance->test_count)
			new_space += FILE_INSTANCE_ALLOCATION_UNIT;

		if (flexutils_resize((void **) &(instance->tests), sizeof(struct test_instance_block), new_space))
			instance->test_space = new_space;
		debug_printf("Space increased to %u units", instance->test_space);
	}

	if (instance->test_count >= instance->test_space)
		return FILE_INSTANCE_NOT_FOUND;

	return instance->test_count++;
}
