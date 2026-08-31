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

#include <stdint.h>
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/osgbpb.h"
#include "oslib/taskwindow.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "file_instance.h"

#include "file_set.h"
#include "main.h"
#include "suite.h"
#include "textdump.h"

/**
 * The maximum length of a test file name.
 */

#define FILE_INSTANCE_NAME_LEN 256

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


	enum file_instance_status status;
};

/* Global variables. */


/* Static function prototypes. */

static void file_instance_update_file(struct suite_block *parent, struct file_instance_details *details, osgbpb_info *entry);

static osbool file_instance_scan_source(char *filename);
static osbool file_instance_scan_block(FILE *fh, int level);
static osbool file_instance_found_definition(FILE *fh);
static osbool file_instance_found_call(FILE *fh);
static osbool file_instance_task_window_ego(wimp_message *message);
static osbool file_instance_task_window_morio(wimp_message *message);
static osbool file_instance_task_window_output(wimp_message *message);

/**
 * Initialise the Test File code.
 */

void file_instance_initialise(void)
{
	event_add_message_handler(message_TASK_WINDOW_EGO, EVENT_MESSAGE_INCOMING, file_instance_task_window_ego);
	event_add_message_handler(message_TASK_WINDOW_MORIO, EVENT_MESSAGE_INCOMING, file_instance_task_window_morio);
	event_add_message_handler(message_TASK_WINDOW_OUTPUT, EVENT_MESSAGE_INCOMING, file_instance_task_window_output);
}

/**
 * Create a new file instance and link it to the supplied parent suite.
 *
 * \param *parent	Pointer to the parent suite.
 * \param *initial	Pointer to the file set which created the instance.
 * \param *name		Pointer to the name of the file.
 * \return		TRUE if successful; FALSE on error.
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

	new->name = suite_store_text(parent, name);
	if (new->name == TEXTDUMP_NULL) {
		file_instance_delete_instance(new);
		return NULL;
	}

	new->next = suite_store_file_instance(parent, new);

	debug_printf("Creating new file instance 0x%x in suite 0x%x for %s", new, parent, name);

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

	heap_free(instance);

	debug_printf("File instance deleted: 0x%x", instance);

	return next;
}

/**
 * Return the details for required for redrawing a display line of a Test File
 * instance.
 *
 * \param *instance	Pointer to the instance of interest.
 * \param *details	Pointer to a struct in which the details should be
 *			returned.
 * \return		TRUE if valid details were returned; else FALSE.
 */

osbool file_instance_get_line_details(struct file_instance_block *instance, struct file_instance_line_details *details)
{
	if (instance == NULL || details == NULL)
		return FALSE;

	details->name = instance->name;
	details->status = instance->status;

	return TRUE;
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


void file_instance_add_source_file(struct file_instance_block *instance, osgbpb_info *entry)
{
	file_instance_update_file(instance->parent, &(instance->source), entry);
}

void file_instance_add_executable_file(struct file_instance_block *instance, osgbpb_info *entry)
{
	file_instance_update_file(instance->parent, &(instance->executable), entry);
}

static void file_instance_update_file(struct suite_block *parent, struct file_instance_details *details, osgbpb_info *entry)
{
	if (parent == NULL || details == NULL || entry == NULL)
		return;

	details->name = suite_store_text(parent, entry->name);

	if ((entry->load_addr & 0xfff00000u) == 0xfff00000u) {
		details->timestamp = entry->exec_addr | ((uint64_t) (entry->load_addr & 0xffu) << 32);
	} else {
		details->timestamp = 0;
	}
}

void file_instance_validate_files(struct file_instance_block *instance)
{
	if (instance == NULL)
		return;

	switch (instance->status) {
	case FILE_INSTANCE_STATUS_UNKNOWN:
		if (instance->source.name != TEXTDUMP_NULL && instance->executable.name != TEXTDUMP_NULL)
			instance->status = FILE_INSTANCE_STATUS_READY_TO_RUN;
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
}





static osbool file_instance_scan_source(char *filename)
{
	FILE *fh = fopen(filename, "r");
	if (fh == NULL)
		return FALSE;

	osbool result = file_instance_scan_block(fh, 0);

	fclose(fh);

	return result;
}

static osbool file_instance_scan_block(FILE *fh, int level)
{
	if (fh == NULL)
		return FALSE;

	int c;

	/* Step past any leading white space. */

	while ((c = fgetc(fh)) != EOF && isspace(c));
	if (c == EOF)
		return TRUE;

	fseek(fh, -1, SEEK_CUR);

	/* Scan for text that we're interested in. */

	char *definition = "void ";
	char *call = "RUN_TEST(";

	char *test_definition = definition, *test_call = call;

	while ((c = fgetc(fh)) != EOF && c != '}') {
		if (level == 1 && test_definition == definition && *test_call == c) {
			/* We're matching a call line. */
			test_call++;
			if (*test_call == '\0')
				file_instance_found_call(fh);
		} else if (level == 0 && test_call == call && *test_definition == c) {
			/* We're matching a function definition line. */
			test_definition++;
			if (*test_definition == '\0')
				file_instance_found_definition(fh);
		} else if (c == '{') {
			/* We've moved into a new block. */
			file_instance_scan_block(fh, level + 1);
			test_definition = definition;
			test_call = call;
		} else if (c == ';') {
			/* The end of the current statement. */
			test_definition = definition;
			test_call = call;

			/* Skip past leading whitespace. */
			while ((c = fgetc(fh)) != EOF && isspace(c));
			if (c == EOF)
				break;

			fseek(fh, -1, SEEK_CUR);
		} else {
			/* All matches failed, so reset the searches. */
			test_definition = definition;
			test_call = call;
		}
	}

	return TRUE;
}

static osbool file_instance_found_definition(FILE *fh)
{
	char buffer[256], *b = buffer;

	int c = '\0';

	while ((c = fgetc(fh)) && c != '(' && c != ';')
		if (b < buffer + 255)
			*b++ = c;

	*b = '\0';

	if (c == ';')
		fseek(fh, -1, SEEK_CUR);
	else if (c == '(' && strcmp(buffer, "main") && strcmp(buffer, "setUp") && strcmp(buffer, "tearDown"))
		debug_printf("Found definition '%s'", buffer);

	return (c == '(') ? TRUE : FALSE;
}

static osbool file_instance_found_call(FILE *fh)
{
	char buffer[256], *b = buffer;

	int c = '\0';

	while ((c = fgetc(fh)) && c != ')' && c != ';')
		if (b < buffer + 255)
			*b++ = c;

	*b = '\0';

	if (c == ';')
		fseek(fh, -1, SEEK_CUR);
	else if (c == ')')
		debug_printf("Found call '%s'", buffer);

	return (c == ')') ? TRUE : FALSE;
}

osbool file_instance_execute(struct file_instance_block *instance)
{
//	if (instance == NULL)
		return FALSE;

//	char command[1024];

//	string_printf(command, 2014,
//			"TaskWindow \"Run %s\" -wimpslot 1024K -name \"Unit Test\" -quit -task &%08x -txt &%08x",
//			instance->absolute_file, main_task_handle, 0x1u
//	);

//	wimp_t child_task;

//	os_error *error = xwimp_start_task(command, &child_task);

//	debug_printf("Launched %s", command);
//	debug_printf("Result = 0x%x, Child = 0x%x", error, child_task);

//	return (error == NULL) ? TRUE : FALSE;
}




static osbool file_instance_task_window_ego(wimp_message *message)
{
	taskwindow_full_message_ego *ego = (taskwindow_full_message_ego *) message;

	debug_printf("Message_TaskWindowEgo, txt=0x%x", ego->txt);
	return TRUE;
}

static osbool file_instance_task_window_morio(wimp_message *message)
{
	debug_printf("Message_TaskWindowMorio");
	return TRUE;
}


static osbool file_instance_task_window_output(wimp_message *message)
{
	taskwindow_full_message_data *data = (taskwindow_full_message_data *) message;

	char buffer[256];

	string_copy(buffer, data->data, data->data_size);

	debug_printf("Message_TaskWindowOutput (%d): %s", data->data_size, buffer);
	return TRUE;
}
