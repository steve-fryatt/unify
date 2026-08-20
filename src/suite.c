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

#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/osgbpb.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/general.h"
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

/**
 * The maximum length of a file path name.
 */

#define SUITE_MAX_PATH_LEN 256
#define SUITE_MAX_FOLDER_LEN 64

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






	struct file_instance_block *file_instances; // TODO -- Delete Me!!

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

static void suite_load(struct suite_block *instance);
static void suite_find_files(struct suite_block *instance, enum file_instance_status type);

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

	new->file_instances = NULL;
	new->window = NULL;
	new->textdump = NULL;
	new->file_sets = NULL;

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

	/* Load the first file set from the folder. */

	new->file_sets = file_set_create_instance(new, new->file_sets);

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

//	file_instance_delete_all(&(instance->file_instances)); -- TODO -- Delete Me!!

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

	return TRUE;
}


/**
 * Load a test suite from a folder on disc.
 *
 * \param *instance	Pointer to the instance to load
 */

static void suite_load(struct suite_block *instance) // TODO -- Delete Me!!!
{
	if (instance == NULL)
		return;

	suite_find_files(instance, FILE_INSTANCE_STATUS_SOURCE);
	suite_find_files(instance, FILE_INSTANCE_STATUS_ABSOLUTE);

	/* Run a test (TODO -- Remove this!) */

	if (instance->file_instances != NULL)
		file_instance_execute(instance->file_instances);
}

/**
 * Find a collection of files within a folder.
 *
 * \param
 */

static void suite_find_files(struct suite_block *instance, enum file_instance_status type)
{
	if (instance == NULL)
		return;

	/* Find the base for the filenames.
	 *
	 * *** This only remains valid until we shift the flex heap about! ***
	 */

	char *textdump_base =  textdump_get_base(instance->textdump);
	if (textdump_base == NULL)
		return;

	/* Configure for the target file. */

	char folder[SUITE_MAX_PATH_LEN];
	char *pattern = NULL;
	char *suffix = NULL;
	unsigned filetype = 0x0u;

	switch (type) {
	case FILE_INSTANCE_STATUS_SOURCE:
		string_printf(folder, SUITE_MAX_PATH_LEN, "%s.%s",
				textdump_base + instance->suite_folder,
				textdump_base + instance->source_folder
		);
		pattern = "*/c";
		suffix = "/c";
		filetype = osfile_TYPE_TEXT;
		break;

	case FILE_INSTANCE_STATUS_ABSOLUTE:
		string_printf(folder, SUITE_MAX_PATH_LEN, "%s.%s",
				textdump_base + instance->suite_folder,
				textdump_base + instance->executable_folder
		);
		filetype = osfile_TYPE_ABSOLUTE;
		break;

	default:
		return;
	}

	/* Read the files in the folder, and process any which match. */

	byte buffer[1024];
	char filename[SUITE_MAX_PATH_LEN];
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

				string_printf(filename, SUITE_MAX_PATH_LEN, "%s.%s", folder, entry->name);

				/* If there's a suffix to the name, remove it. */

				if (suffix != NULL) {
					int name_length = strlen(entry->name);
					int suffix_length = strlen(suffix);

					if ((name_length > suffix_length) && (string_nocase_strcmp(entry->name + name_length - suffix_length, suffix) == 0))
						*(entry->name + name_length - suffix_length) = '\0';
				}

				file_instance_include_entry(&(instance->file_instances), type, entry->name, filename);
			}


		}
	} while (error == NULL && context != -1);
}
