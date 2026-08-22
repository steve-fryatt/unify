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

/* Acorn C header files */

/* OSLib header files */

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "suite.h"

#include "file_set.h"
#include "file_instance.h"
#include "textdump.h"
#include "window.h"

/**
 * The maximum length of the name of a test suite.
 */

#define SUITE_NAME_LEN 64

/* Structure definitions. */

struct suite_block {
	/**
	 * The name of the test suite.
	 */
	char name[SUITE_NAME_LEN];

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

static void suite_close_handler(void *data);
static osbool suite_redraw_line_handler(int line, struct window_line *content, void *data);

/* The Test Suite window definiton. */

static struct window_definition suite_window_definition = {
	.type = WINDOW_TYPE_SUITE,
	.callback_close = suite_close_handler,
	.callback_redraw = suite_redraw_line_handler
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

	/* Set up the text dump to store strings for the suite. */

	new->textdump = textdump_create(TEXTDUMP_DEFAULT_ALLOCATION);
	if (new->textdump == NULL) {
		suite_delete_instance(new);
		return FALSE;
	}

	/* Set up the window for the suite. */

	new->window = window_create_instance(&suite_window_definition, new);
	if (new->window == NULL) {
		suite_delete_instance(new);
		return FALSE;
	}

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

	/* Load the first file set from the folder. */

	new->file_sets = file_set_create_instance(new, new->file_sets);

	int objects = file_set_get_object_count(new->file_sets);
	window_set_extent(new->window, objects);

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

	debug_printf("Delete instance 0x%x", instance);

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


struct file_instance_block *suite_store_file_instance(struct suite_block *instance, struct file_instance_block *file_instance)
{
	if (instance == NULL || file_instance == NULL)
		return NULL;

	struct file_instance_block *next = instance->file_instances;
	instance->file_instances = file_instance;

	return next;
}

osbool suite_read_folder_path(struct suite_block *instance, char *buffer, size_t length, enum suite_folder folder)
{
	if (buffer == NULL || length == 0)
		return FALSE;

	*buffer = '\0';

	if (instance == NULL)
		return FALSE;

	char *textdump_base =  textdump_get_base(instance->textdump);
	if (textdump_base == NULL)
		return FALSE;

	unsigned folder_offset = TEXTDUMP_NULL;

	switch (folder) {
	case SUITE_FOLDER_SOURCE:
		folder_offset = instance->source_folder;
		break;
	case SUITE_FOLDER_EXECUTABLE:
		folder_offset = instance->executable_folder;
		break;
	default:
		return FALSE;
	}

	string_printf(buffer, length, "%s.%s",
			textdump_base + instance->suite_folder,
			textdump_base + folder_offset
	);

	return TRUE;
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

static osbool suite_redraw_line_handler(int line, struct window_line *content, void *data)
{
	struct suite_block *instance = data;
	if (instance == NULL)
		return FALSE;

	char *textdump_base = textdump_get_base(instance->textdump);
	if (textdump_base == NULL)
		return FALSE;

	unsigned text = file_set_get_object_name(instance->file_sets, line);
	if (text == TEXTDUMP_NULL)
		return FALSE;

	content->text = textdump_base + text;
	content->status = WINDOW_STATUS_FAIL;

	content->count = 0;
	content->total = 100;

	return TRUE;
}
