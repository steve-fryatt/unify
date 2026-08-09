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
 * \file: test_suite.c
 *
 * Test Suite implementation.
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/osgbpb.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/general.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "test_suite.h"

#include "test_file.h"

/**
 * The maximum length of the name of a test suite.
 */

#define TEST_SUITE_NAME_LEN 64

/**
 * The maximum length of a file path name.
 */

#define TEST_SUITE_MAX_PATH_LEN 256
#define TEST_SUITE_MAX_FOLDER_LEN 64

/* Structure definitions. */

struct test_suite_block {
	/**
	 * The name of the test suite.
	 */
	char name[TEST_SUITE_NAME_LEN];

	/**
	 * The path to the suite folder.
	 */
	char folder[TEST_SUITE_MAX_PATH_LEN];

	char source_folder[TEST_SUITE_MAX_FOLDER_LEN];
	char executable_folder[TEST_SUITE_MAX_FOLDER_LEN];

	struct test_file_block *test_files;

	/**
	 * Pointer to the next suite, or NULL.
	 */
	struct test_suite_block	*next;
};

/* Global variables. */

/**
 * Pointer to the linked list of test suites.
 */

struct test_suite_block *test_suite_list = NULL;

/* Static function prototypes. */

static void test_suite_load(struct test_suite_block *instance);
static void test_suite_find_files(struct test_suite_block *instance, enum test_file_status type);

/**
 * Create a new Test Suite instance and link it in to the collection of
 * active instances.
 *
 * \param *folder	Pointer to the name of the folder holding the
 *			test suite files.
 * \return		TRUE if successful; FALSE on error.
 */

osbool test_suite_create_instance(char *folder)
{
	struct test_suite_block *new = heap_alloc(sizeof(struct test_suite_block));
	if (new == NULL)
		return FALSE;

	string_copy(new->folder, folder, TEST_SUITE_MAX_PATH_LEN);
	string_copy(new->source_folder, "tests", TEST_SUITE_MAX_FOLDER_LEN);
	string_copy(new->executable_folder, "absolute", TEST_SUITE_MAX_FOLDER_LEN);

	new->test_files = NULL;

	new->next = test_suite_list;
	test_suite_list = new;

	test_suite_load(new);

	return TRUE;
}

/**
 * Delete a Test Suite instance and delink it from the collection of
 * active instances.
 *
 * \param *instance	Pointer to the instance to be deleted.
 */

void test_suite_delete_instance(struct test_suite_block *instance)
{
	if (instance == NULL)
		return;

	/* Unlink the instance from the list of suites. */

	struct test_suite_block **list = &test_suite_list;

	while (*list != NULL && *list != instance)
		list = &((*list)->next);

	if (*list != NULL)
		*list = instance->next;

	/* Free the memory associated with the instance. */

	test_file_delete_all(&(instance->test_files));

	heap_free(instance);
}

/**
 * Delete all active Test Suite instances and free all resources
 * associated with them.
 */

void test_suite_delete_all(void)
{
	while (test_suite_list != NULL)
		test_suite_delete_instance(test_suite_list);
}

/**
 * Load a test suite from a folder on disc.
 *
 * \param *instance	Pointer to the instance to load
 */

static void test_suite_load(struct test_suite_block *instance)
{
	if (instance == NULL)
		return;

	test_suite_find_files(instance, TEST_FILE_STATUS_SOURCE);
	test_suite_find_files(instance, TEST_FILE_STATUS_ABSOLUTE);
}

/**
 * Find a collection of files within a folder.
 *
 * \param
 */

static void test_suite_find_files(struct test_suite_block *instance, enum test_file_status type)
{
	if (instance == NULL)
		return;

	/* Configure for the target file. */

	char folder[TEST_SUITE_MAX_PATH_LEN];
	char *pattern = NULL;
	char *suffix = NULL;
	unsigned filetype = 0x0u;

	switch (type) {
	case TEST_FILE_STATUS_SOURCE:
		string_printf(folder, TEST_SUITE_MAX_PATH_LEN, "%s.%s", instance->folder, instance->source_folder);
		pattern = "*/c";
		suffix = "/c";
		filetype = osfile_TYPE_TEXT;
		break;

	case TEST_FILE_STATUS_ABSOLUTE:
		string_printf(folder, TEST_SUITE_MAX_PATH_LEN, "%s.%s", instance->folder, instance->executable_folder);
		filetype = osfile_TYPE_ABSOLUTE;
		break;

	default:
		return;
	}

	/* Read the files in the folder, and process any which match. */

	byte buffer[1024];
	char filename[TEST_SUITE_MAX_PATH_LEN];
	int count = 0, context = 0;
	os_error *error = NULL;

	do {
		error = xosgbpb_dir_entries_info(folder, (osgbpb_info_list *) buffer, 100, context, 1024, pattern, &count, &context);

		if (error == NULL && count > 0) {
			byte *buffer_offset = buffer;

			for (int i = 0; i < count; i++) {
				osgbpb_info *entry = (osgbpb_info *) buffer_offset;
				buffer_offset += WORDALIGN(21 + strlen(entry->name));

				/* We're only interested in files. */

				if (entry->obj_type != fileswitch_IS_FILE)
					continue;

				/* We don't care about untyped files. */

				if ((entry->load_addr & 0xfff00000u) != 0xfff00000u)
					continue;

				/* Check if this is the correct type; ignore if not. */

				if (((entry->load_addr & osfile_FILE_TYPE) >> osfile_FILE_TYPE_SHIFT) != filetype)
					continue;

				/* Assemble the full pathname of the file. */

				string_printf(filename, TEST_SUITE_MAX_PATH_LEN, "%s.%s", folder, entry->name);

				/* If there's a suffix to the name, remove it. */

				if (suffix != NULL) {
					int name_length = strlen(entry->name);
					int suffix_length = strlen(suffix);

					if ((name_length > suffix_length) && (string_nocase_strcmp(entry->name + name_length - suffix_length, suffix) == 0))
						*(entry->name + name_length - suffix_length) = '\0';
				}

				test_file_include_entry(&(instance->test_files), type, entry->name, filename);
			}


		}
	} while (error == NULL && context != -1);
}
