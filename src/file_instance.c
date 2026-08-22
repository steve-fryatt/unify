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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/os.h"
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

	enum file_instance_status status;

	char source_file[FILE_INSTANCE_NAME_LEN];

	char absolute_file[FILE_INSTANCE_NAME_LEN];
};

/* Global variables. */


/* Static function prototypes. */

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
	new->status = FILE_INSTANCE_STATUS_NONE;
	*new->source_file = '\0';
	*new->absolute_file = '\0';

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

unsigned file_instance_get_name(struct file_instance_block *instance)
{
	if (instance == NULL)
		return TEXTDUMP_NULL;

	return instance->name;
}


#if 0


void file_instance_include_entry(struct file_instance_block **list, enum file_instance_status type, char* name, char *filename)
{
	if (list == NULL || name == NULL || filename == NULL)
		return;

	debug_printf("Process name=%s, type=%d, filename=%s", name, type, filename);

	struct file_instance_block *entry = *list;

	while (entry != NULL && string_nocase_strcmp(entry->filename, name) != 0) {
		debug_printf("Searching against = %s", entry->filename);
		entry = entry->next;
	}

	debug_printf("Existing entry = 0x%x", entry);

	if (entry == NULL)
		entry = file_instance_create_instance(list, name);

	debug_printf("Entry to update = 0x%x", entry);

	if (entry == NULL)
		return;

	switch (type) {
	case FILE_INSTANCE_STATUS_SOURCE:
		string_copy(entry->source_file, filename, FILE_INSTANCE_NAME_LEN);
		entry->status |= FILE_INSTANCE_STATUS_SOURCE;
		file_instance_scan_source(filename);
		break;

	case FILE_INSTANCE_STATUS_ABSOLUTE:
		string_copy(entry->absolute_file, filename, FILE_INSTANCE_NAME_LEN);
		entry->status |= FILE_INSTANCE_STATUS_ABSOLUTE;
		break;

	default:
		break;
	}
}
#endif

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
	if (instance == NULL)
		return FALSE;

	char command[1024];

	string_printf(command, 2014,
			"TaskWindow \"Run %s\" -wimpslot 1024K -name \"Unit Test\" -quit -task &%08x -txt &%08x",
			instance->absolute_file, main_task_handle, 0x1u
	);

	wimp_t child_task;

	os_error *error = xwimp_start_task(command, &child_task);

	debug_printf("Launched %s", command);
	debug_printf("Result = 0x%x, Child = 0x%x", error, child_task);

	return (error == NULL) ? TRUE : FALSE;
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
