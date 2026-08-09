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

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "test_file.h"

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
		break;

	case TEST_FILE_STATUS_ABSOLUTE:
		string_copy(entry->absolute_file, filename, TEST_FILE_NAME_LEN);
		entry->status |= TEST_FILE_STATUS_ABSOLUTE;
		break;

	default:
		break;
	}
}