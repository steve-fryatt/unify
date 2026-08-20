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
 * \file: file_set.c
 *
 * File Set implementation.
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/osgbpb.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/general.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "file_set.h"

#include "file_instance.h"
#include "flexutils.h"
#include "suite.h"
#include "textdump.h"

/**
 * The increments to allocate space for file objects.
 */

#define FILE_SET_ALLOCATION_UNIT 10

/**
 * The maximum length of a file path name.
 */

#define FILE_SET_MAX_PATH_LEN 256

/* Structure definitions. */

/**
 * A File Set instance block.
 */

struct file_set_block {
	/**
	 * Pointer to the parent test suite.
	 */
	struct suite_block *parent;

	/**
	 * Pointer to the preceding file set.
	 *
	 * These step back in time/date order, with new sets added to the front
	 * of the list as new attempts are made to run the parent test suite.
	 */
	struct file_set_block *previous;

	/**
	 * Flex block pointer to the list of objects in the file set.
	 */
	struct file_instance_block **objects;

	size_t object_space;
	size_t object_count;
};

/* Global variables. */


/* Static function prototypes. */

static osbool file_set_add_object(struct file_set_block *instance, struct file_instance_block *object);
static void file_set_find_objects(struct file_set_block *instance, enum file_instance_status type);

/**
 * Create a new file set instance, by scanning the parent suite and creating a
 * list of file objects.
 *
 * \param *parent		Pointer to the parent test suite.
 * \param *previous		Pointer to the previous file set in the parent
 *				test suite, or NULL if this is the first.
 * \return			Pointer to the new file set, or NULL on failure.
 */

struct file_set_block *file_set_create_instance(struct suite_block *parent, struct file_set_block *previous)
{
	struct file_set_block *new = heap_alloc(sizeof(struct file_set_block));
	if (new == NULL)
		return NULL;

	new->objects = NULL;
	new->object_space = FILE_SET_ALLOCATION_UNIT;
	new->object_count = 0;

	if (!flexutils_allocate((void **) &(new->objects), sizeof(struct file_instance_block *), new->object_space)) {
		heap_free(new);
		return NULL;
	}

	new->parent = parent;
	new->previous = previous;

	debug_printf("Creating new file set 0x%x in suite 0x%x...", new, parent);

	/* Find the files. */

	file_set_find_objects(new, FILE_INSTANCE_STATUS_SOURCE);
	file_set_find_objects(new, FILE_INSTANCE_STATUS_ABSOLUTE);

	debug_printf("New file set done!");

	return new;
}

/**
 * Destroy a file set instance.
 *
 * NB: It is left up to the caller to do something sensible with any linked
 * list references. Currently this is called by the suite when cleaning up on
 * deletion, and that just picks its way down the list removing items as it
 * goes.
 *
 * \param *instance		Pointer to the file set instance to be destroyed.
 * \return			Pointer to the previous file set known to the
 *				instance, or NULL if there wasn't one.
 */

struct file_set_block *file_set_delete_instance(struct file_set_block *instance)
{
	if (instance == NULL)
		return NULL;

	struct file_set_block *previous = instance->previous;

	flexutils_free((void **) &(instance->objects));

	heap_free(instance);

	debug_printf("File set deleted, previous was 0x%x", previous);

	return previous;
}

static osbool file_set_add_object(struct file_set_block *instance, struct file_instance_block *object)
{
	if (instance == NULL)
		return FALSE;

	if (instance->object_count >= instance->object_space) {
		size_t new_space = instance->object_space;

		while (new_space <= instance->object_count)
			new_space += FILE_SET_ALLOCATION_UNIT;

		if (flexutils_resize((void **) &(instance->objects), sizeof(struct file_set_object *), new_space))
			instance->object_space = new_space;
	}

	if (instance->object_count >= instance->object_space)
		return FALSE;

	instance->objects[instance->object_count++] = object;

	return TRUE;
}

size_t file_set_get_object_count(struct file_set_block *instance)
{
	if (instance == NULL)
		return 0;

	return instance->object_count;
}

unsigned file_set_get_object_name(struct file_set_block *instance, int i)
{
	if (instance == NULL || instance->objects == NULL || i < 0 || i >= instance->object_count)
		return TEXTDUMP_NULL;

	return file_instance_get_name(instance->objects[i]);
}







/**
 * Find a collection of files within a folder.
 *
 * \param
 */

static void file_set_find_objects(struct file_set_block *instance, enum file_instance_status type)
{
	if (instance == NULL)
		return;

	/* Configure for the target file. */

	char folder[FILE_SET_MAX_PATH_LEN];
	char *pattern = NULL;
	char *suffix = NULL;
	unsigned filetype = 0x0u;

	switch (type) {
	case FILE_INSTANCE_STATUS_SOURCE:
		suite_read_folder_path(instance->parent, folder, FILE_SET_MAX_PATH_LEN, SUITE_FOLDER_SOURCE);
		pattern = "*/c";
		suffix = "/c";
		filetype = osfile_TYPE_TEXT;
		break;

	case FILE_INSTANCE_STATUS_ABSOLUTE:
		suite_read_folder_path(instance->parent, folder, FILE_SET_MAX_PATH_LEN, SUITE_FOLDER_EXECUTABLE);
		filetype = osfile_TYPE_ABSOLUTE;
		break;

	default:
		return;
	}

	debug_printf("Scanning folder: %s", folder);

	/* Read the files in the folder, and process any which match. */

	byte buffer[1024];
	char filename[FILE_SET_MAX_PATH_LEN];
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

				string_printf(filename, FILE_SET_MAX_PATH_LEN, "%s.%s", folder, entry->name);

				/* If there's a suffix to the name, remove it. */

				if (suffix != NULL) {
					int name_length = strlen(entry->name);
					int suffix_length = strlen(suffix);

					if ((name_length > suffix_length) && (string_nocase_strcmp(entry->name + name_length - suffix_length, suffix) == 0))
						*(entry->name + name_length - suffix_length) = '\0';
				}

				debug_printf("Found %s at %s", entry->name, filename);

	//			file_instance_include_entry(&(instance->file_instances), type, entry->name, filename);

				// TODO Find object (in previous and current).
				// If not found,

				struct file_instance_block *file = file_instance_create_instance(instance->parent, entry->name);

				// Merge flags as required.
				// If required, add in as an object.

				file_set_add_object(instance, file);

			}


		}
	} while (error == NULL && context != -1);
}

static struct file_set_block *file_set_find_object(struct file_set_block *instance, char *name)
{
	if (instance == NULL || instance->objects == NULL)
		return NULL;

	for (int i = 0; i < instance->object_count; i++) {
		// TODO -- Match against the file instance.
	}
}