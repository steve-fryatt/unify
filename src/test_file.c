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
 * \file: test_file.c
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

#include "test_file.h"

#include "main.h"

/**
 * The maximum length of a test file name.
 */

#define TEST_FILE_NAME_LEN 256

/* Structure definitions. */

struct test_file_block {
	/**
	 * The base name of the file, without any suffixes.
	 */
	char filename[TEST_FILE_NAME_LEN];

	enum test_file_status status;

	char source_file[TEST_FILE_NAME_LEN];

	char absolute_file[TEST_FILE_NAME_LEN];


	/**
	 * Pointer to the next file in the suite, or NULL.
	 */
	struct test_file_block *next;
};

/* Global variables. */


/* Static function prototypes. */

static struct test_file_block *test_file_create_instance(struct test_file_block **list, char *name);
static void test_file_delete_instance(struct test_file_block **list, struct test_file_block *instance);

static osbool test_file_scan_source(char *filename);
static osbool test_file_scan_block(FILE *fh, int level);
static osbool test_file_found_definition(FILE *fh);
static osbool test_file_found_call(FILE *fh);
static osbool test_file_task_window_ego(wimp_message *message);
static osbool test_file_task_window_morio(wimp_message *message);
static osbool test_file_task_window_output(wimp_message *message);

/**
 * Initialise the Test File code.
 */

void test_file_initialise(void)
{
	event_add_message_handler(message_TASK_WINDOW_EGO, EVENT_MESSAGE_INCOMING, test_file_task_window_ego);
	event_add_message_handler(message_TASK_WINDOW_MORIO, EVENT_MESSAGE_INCOMING, test_file_task_window_morio);
	event_add_message_handler(message_TASK_WINDOW_OUTPUT, EVENT_MESSAGE_INCOMING, test_file_task_window_output);
}

/**
 * Create a new Test File instance and link it in to the supplied
 * collection of active instances.
 *
 * \param *folder	Pointer to the name of the folder holding the
 *			test suite files.
 * \return		TRUE if successful; FALSE on error.
 */

static struct test_file_block *test_file_create_instance(struct test_file_block **list, char *name)
{
	if (list == NULL || name == NULL)
		return NULL;

	struct test_file_block *new = heap_alloc(sizeof(struct test_file_block));
	if (new == NULL)
		return NULL;

	new->status = TEST_FILE_STATUS_NONE;
	string_copy(new->filename, name, TEST_FILE_NAME_LEN);
	*new->source_file = '\0';
	*new->absolute_file = '\0';

	new->next = *list;
	*list = new;

	return new;
}

/**
 * Delete a Test File instance and delink it from the collection of
 * active instances.
 *
 * \param *instance	Pointer to the instance to be deleted.
 */

static void test_file_delete_instance(struct test_file_block **list, struct test_file_block *instance)
{
	if (list == NULL || instance == NULL)
		return;

	/* Unlink the instance from the list of suites. */

	while (*list != NULL && *list != instance)
		list = &((*list)->next);

	if (*list != NULL)
		*list = instance->next;

	/* Free the memory associated with the instance. */

	heap_free(instance);
}

/**
 * Delete all active Test Suite instances and free all resources
 * associated with them.
 */

void test_file_delete_all(struct test_file_block **list)
{
	while (*list != NULL)
		test_file_delete_instance(list, *list);
}

void test_file_include_entry(struct test_file_block **list, enum test_file_status type, char* name, char *filename)
{
	if (list == NULL || name == NULL || filename == NULL)
		return;

	debug_printf("Process name=%s, type=%d, filename=%s", name, type, filename);

	struct test_file_block *entry = *list;

	while (entry != NULL && string_nocase_strcmp(entry->filename, name) != 0) {
		debug_printf("Searching against = %s", entry->filename);
		entry = entry->next;
	}

	debug_printf("Existing entry = 0x%x", entry);

	if (entry == NULL)
		entry = test_file_create_instance(list, name);

	debug_printf("Entry to update = 0x%x", entry);

	if (entry == NULL)
		return;

	switch (type) {
	case TEST_FILE_STATUS_SOURCE:
		string_copy(entry->source_file, filename, TEST_FILE_NAME_LEN);
		entry->status |= TEST_FILE_STATUS_SOURCE;
		test_file_scan_source(filename);
		break;

	case TEST_FILE_STATUS_ABSOLUTE:
		string_copy(entry->absolute_file, filename, TEST_FILE_NAME_LEN);
		entry->status |= TEST_FILE_STATUS_ABSOLUTE;
		break;

	default:
		break;
	}
}


static osbool test_file_scan_source(char *filename)
{
	FILE *fh = fopen(filename, "r");
	if (fh == NULL)
		return FALSE;

	osbool result = test_file_scan_block(fh, 0);

	fclose(fh);

	return result;
}

static osbool test_file_scan_block(FILE *fh, int level)
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
				test_file_found_call(fh);
		} else if (level == 0 && test_call == call && *test_definition == c) {
			/* We're matching a function definition line. */
			test_definition++;
			if (*test_definition == '\0')
				test_file_found_definition(fh);
		} else if (c == '{') {
			/* We've moved into a new block. */
			test_file_scan_block(fh, level + 1);
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

static osbool test_file_found_definition(FILE *fh)
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

static osbool test_file_found_call(FILE *fh)
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

osbool test_file_execute(struct test_file_block *instance)
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




static osbool test_file_task_window_ego(wimp_message *message)
{
	taskwindow_full_message_ego *ego = (taskwindow_full_message_ego *) message;

	debug_printf("Message_TaskWindowEgo, txt=0x%x", ego->txt);
	return TRUE;
}

static osbool test_file_task_window_morio(wimp_message *message)
{
	debug_printf("Message_TaskWindowMorio");
	return TRUE;
}


static osbool test_file_task_window_output(wimp_message *message)
{
	taskwindow_full_message_data *data = (taskwindow_full_message_data *) message;

	char buffer[256];

	string_copy(buffer, data->data, data->data_size);

	debug_printf("Message_TaskWindowOutput (%d): %s", data->data_size, buffer);
	return TRUE;
}
