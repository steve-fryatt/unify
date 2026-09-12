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
#include <stddef.h>
#include <stdint.h>

/* Acorn C header files */

/* OSLib header files */

#include <oslib/os.h>
#include <oslib/osfile.h>
#include <oslib/osgbpb.h>
#include <oslib/wimp.h>

/* SF-Lib header files. */

#include <sflib/debug.h>
#include <sflib/errors.h>
#include <sflib/general.h>
#include <sflib/heap.h>
#include <sflib/string.h>

/* Application header files */

#include "file_set.h"

#include "date_time.h"
#include "file_instance.h"
#include "flexutils.h"
#include "suite.h"
#include "textdump.h"
#include "window.h"

/**
 * The increments to allocate space for file objects.
 */

#define FILE_SET_ALLOCATION_UNIT 10

/**
 * The maximum length of a file path name.
 */

#define FILE_SET_MAX_PATH_LEN 256

/**
 * OS_GBPB Buffer size.
 */

#define FILE_SET_OS_GBPB_BUFFER_SIZE 1024

/**
 * The maximum number of objects to read on a call to OS_GBPB.
 */

#define FILE_SET_OS_GBPB_MAX_READ 100

/**
 * A suitable array index hasn't been found.
 */

#define FILE_SET_NOT_FOUND ((unsigned) 0xffffffffu)

/**
 * The types of file which exist in a set.
 */

enum file_set_type {
	FILE_SET_TYPE_UNKNOWN,
	FILE_SET_TYPE_SOURCE,
	FILE_SET_TYPE_EXECUTABLE
};

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
	 * The timestamp when the file set was created.
	 */
	uint64_t timestamp;

	/**
	 * Flex block pointer to the list of objects in the file set.
	 */
	struct file_instance_block **objects;

	/**
	 * The space allocated to objects in the object list.
	 */
	size_t object_space;

	/**
	 * The number of objects in the object list.
	 */
	size_t object_count;
};

/* Global variables. */


/* Static function prototypes. */

static unsigned file_set_add_object(struct file_set_block *instance, struct file_instance_block *object);
static void file_set_find_objects(struct file_set_block *instance, enum file_set_type type, osbool all_new);
static unsigned file_set_find_object(struct file_set_block *instance, char *clean_name, osgbpb_info *entry);


/**
 * Create a new file set instance, by scanning the parent suite and creating a
 * list of file objects.
 *
 * \param *parent		Pointer to the parent test suite.
 * \param *previous		Pointer to the previous file set in the parent
 *				test suite, or NULL if this is the first.
 * \param full			TRUE if the new instance should be a full run;
 *				otherwise it will just contain incremental changes.
 * \return			Pointer to the new file set, or NULL on failure.
 */

struct file_set_block *file_set_create_instance(struct suite_block *parent, struct file_set_block *previous, osbool full)
{
	/* Allocate the instance memory and fill the data. */

	struct file_set_block *new = heap_alloc(sizeof(struct file_set_block));
	if (new == NULL)
		return NULL;

	new->objects = NULL;
	new->object_space = FILE_SET_ALLOCATION_UNIT;
	new->object_count = 0;
	new->timestamp = date_time_read_current_time();

	if (!flexutils_allocate((void **) &(new->objects), sizeof(struct file_instance_block *), new->object_space)) {
		heap_free(new);
		return NULL;
	}

	new->parent = parent;
	new->previous = previous;

	debug_printf("\\KCreating new file set 0x%x in suite 0x%x...", new, parent);

	/* Find the files, first searching for sources and then executables. */

	file_set_find_objects(new, FILE_SET_TYPE_SOURCE, full);
	file_set_find_objects(new, FILE_SET_TYPE_EXECUTABLE, full);

	debug_printf("\\kNew file set done!");
	char timebuf[128];
	date_time_write_standard_string(new->timestamp, timebuf, 128);

	debug_printf("File set created at %s", timebuf);

	/* Do some initial validation on the files that we found. */

	for (int i = 0; i < new->object_count; i++)
		file_instance_validate_files(new->objects[i]);

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

/**
 * Add a file instance to the list of instances owned by a file set.
 *
 * \param *instance		Pointer to the file set instance to take
 *				the object.
 * \param *object		Pointer to the object to be added.
 * \return			The index of the new object, or FILE_SET_NOT_FOUND.
 */

static unsigned file_set_add_object(struct file_set_block *instance, struct file_instance_block *object)
{
	if (instance == NULL)
		return FILE_SET_NOT_FOUND;

	if (instance->object_count >= instance->object_space) {
		size_t new_space = instance->object_space;

		while (new_space <= instance->object_count)
			new_space += FILE_SET_ALLOCATION_UNIT;

		if (flexutils_resize((void **) &(instance->objects), sizeof(struct file_instance_block *), new_space))
			instance->object_space = new_space;
	}

	if (instance->object_count >= instance->object_space)
		return FILE_SET_NOT_FOUND;

	instance->objects[instance->object_count] = object;

	return instance->object_count++;
}

/**
 * Given a file set and a window instance, add the connetns of the file set
 * to the window.
 *
 * \param *instance		Pointer to the file set to be added.
 * \param *window		The window to add the file set to.
 */

void file_set_add_to_window(struct file_set_block *instance, struct window_instance *window)
{
	if (instance == NULL || window == NULL)
		return;

	enum window_content_relation relation = WINDOW_CONTENT_RELATION_NONE;

	if (instance->previous == NULL)
		relation |= WINDOW_CONTENT_RELATION_LAST;

	if (suite_file_set_is_first(instance->parent, instance))
		relation |= WINDOW_CONTENT_RELATION_FIRST;

	window_start_new_content(window, instance->timestamp, relation);

	for (int i = 0; i < instance->object_count; i++)
		file_instance_add_to_window(instance->objects[i], window);

	window_finish_new_content(window);
}

/**
 * Given a file set instance, find the previous instance in the timeline.
 *
 * \param *instance		Pointer to the file set to start from.
 * \return			Pointer to the previous instance, or NULL.
 */

struct file_set_block *file_set_find_previous_object(struct file_set_block *instance)
{
	if (instance == NULL)
		return NULL;

	return instance->previous;
}

/**
 * Given a file set instance, find the next instance in the timeline.
 *
 * \param *instance		Pointer to the file set to start from.
 * \param *list			Poiuter to the head of the linked list of file
 * \return			Pointer to the next instance, or NULL.
 */

struct file_set_block *file_set_find_next_object(struct file_set_block *instance, struct file_set_block *list)
{
	if (instance == NULL || list == NULL)
		return NULL;

	while (list != NULL && list->previous != instance)
		list = list->previous;

	if (list == NULL || list->previous != instance)
		return NULL;

	return list;
}

/**
 * Return the details of a file instance required for redraw, for a specific
 * line from within the file set.
 *
 * \param *instance		Pointer to the file set instance of interest.
 * \param line			The line number from which to retiurn details.
 * \param *details		Pointer to a structure in memory to hold the
 *				returned details.
 * \return			TRUE if successful; FALSE on error.
 */

osbool file_set_get_line_details(struct file_set_block *instance, int line, struct file_instance_line_details *details)
{
	if (instance == NULL || instance->objects == NULL)
		return FALSE;

	if (line < 0 || line >= instance->object_count)
		return FALSE;

	return file_instance_get_line_details(instance->objects[line], instance, details);
}

/**
 * Return the details of a file instance, for a specific line from within the
 * file set.
 *
 * \param *instance		Pointer to the file set instance of interest.
 * \param line			The line number from which to retiurn details.
 * \param *details		Pointer to a structure in memory to hold the
 *				returned details.
 * \return			TRUE if successful; FALSE on error.
 */

osbool file_set_get_object_details(struct file_set_block *instance, int line,
		struct file_instance_object_details *details)
{
	if (instance == NULL || instance->objects == NULL)
		return FALSE;

	if (line < 0 || line >= instance->object_count)
		return FALSE;

	return file_instance_get_object_details(instance->objects[line], details);
}

/**
 * Return the timestamp for a file set instance.
 *
 * \param *instance		Pointer to the file set instance of interest.
 * \return			The timestamp of the file set.
 */

uint64_t file_set_get_timestamp(struct file_set_block *instance)
{
	return (instance == NULL) ? 0 : instance->timestamp;
}

/**
 * Find a collection of files within a folder.
 *
 * NB: There is an expectation that this routine will be called in a specific
 * sequence of FILE_SET_TYPE_SOURCE folled by FILE_SET_TYPE_EXECUTABLE. If this
 * is not done, the results will be undefined.
 *
 * \param *instance		Pointer to the file set instance to be populated.
 * \param type			The type of file to be searched for.
 * \param all_new		TRUE if all objects should be created new.
 */

static void file_set_find_objects(struct file_set_block *instance, enum file_set_type type, osbool all_new)
{
	if (instance == NULL)
		return;

	/* Configure for the target file. */

	char folder[FILE_SET_MAX_PATH_LEN];
	char *pattern = NULL;
	char *suffix = NULL;
	unsigned filetype = 0x0u;

	switch (type) {
	case FILE_SET_TYPE_SOURCE:
		suite_read_folder_path(instance->parent, folder, FILE_SET_MAX_PATH_LEN, SUITE_FOLDER_SOURCE);
		pattern = "*/c";
		suffix = "/c";
		filetype = osfile_TYPE_TEXT;
		break;

	case FILE_SET_TYPE_EXECUTABLE:
		suite_read_folder_path(instance->parent, folder, FILE_SET_MAX_PATH_LEN, SUITE_FOLDER_EXECUTABLE);
		filetype = osfile_TYPE_ABSOLUTE;
		break;

	default:
		return;
	}

	debug_printf("\\CScanning folder: %s", folder);

	/* Read the files in the folder, and process any which match. */

	byte buffer[FILE_SET_OS_GBPB_BUFFER_SIZE];
	int count = 0, context = 0;
	os_error *error = NULL;

	char *clean_name = NULL;
	char clean_name_buffer[FILE_SET_MAX_PATH_LEN];

	do {
		error = xosgbpb_dir_entries_info(
				folder,
				(osgbpb_info_list *) buffer,
				FILE_SET_OS_GBPB_MAX_READ,
				context,
				FILE_SET_OS_GBPB_BUFFER_SIZE,
				pattern,
				&count,
				&context
		);

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

				/* Start by assuming that the name is clean as it is. */

				clean_name = entry->name;

				/* Then, if there's a suffix to the name, copy the name out without suffix. */

				if (suffix != NULL) {
					int name_length = strlen(entry->name);
					int suffix_length = strlen(suffix);

					if ((name_length > suffix_length) && (string_nocase_strcmp(entry->name + name_length - suffix_length, suffix) == 0)) {
						int i = 0;

						for (; i < (name_length - suffix_length) && i < (FILE_SET_MAX_PATH_LEN - 1); i++)
							clean_name_buffer[i] = entry->name[i];

						clean_name_buffer[i] = '\0';
						clean_name = clean_name_buffer;
					}
				}

				debug_printf("Found %s at %s.%s", clean_name, folder, entry->name);

				/* For executables, check to see if we already have a corresponding source file. */

				unsigned file_index = FILE_SET_NOT_FOUND;

				if (type == FILE_SET_TYPE_EXECUTABLE)
					file_index = file_set_find_object(instance, clean_name, entry);

				/* Now look for a previous instance */

				if (file_index == FILE_SET_NOT_FOUND && instance->previous != NULL && all_new == FALSE) {
					debug_printf("Looking for a previous instance...");
					unsigned index = file_set_find_object(instance->previous, clean_name, entry);

					/* Use this old file instance for now, unless it has a source file and we're
					 * searching for executables (because if we are, and we get here, there can't
					 * have been a source file found).
					 */

					if (index != FILE_SET_NOT_FOUND &&
							!(type == FILE_SET_TYPE_EXECUTABLE &&
							file_instance_has_source(instance->previous->objects[index]))) {
						file_index = file_set_add_object(instance, instance->previous->objects[index]);
						debug_printf("Found pre-existing file instance to re-use.");
					}
				}

				/* If the file doesn't exist, create a new instance. */

				if (file_index == FILE_SET_NOT_FOUND) {
					struct file_instance_block *file = file_instance_create_instance(instance->parent, instance, clean_name);
					debug_printf("Creating new file instance 0x%x", file);
					if (file != NULL)
						file_index = file_set_add_object(instance, file);
				} else {
					debug_printf("Reusing existing file instance 0x%x", instance->objects[file_index]);
				}

				/* Now try to add the file to the instance that we found. This may update
				 * the instance if the current file is found to be incompatible with the
				 * existing file details.
				 */

				switch (type) {
				case FILE_SET_TYPE_SOURCE:
					instance->objects[file_index] = file_instance_add_source_file(instance->objects[file_index], instance, entry);
					break;
				case FILE_SET_TYPE_EXECUTABLE:
					instance->objects[file_index] = file_instance_add_executable_file(instance->objects[file_index], instance, entry);
					break;
				default:
					break;
				}
			}
		}
	} while (error == NULL && context != -1);
}

/**
 * Given some file details read from the disc, see if we already have a file
 * instance in our collection which might match it and return the index into
 * the objects array,
 *
 * \param *instance		Pointer to the file set instance to be searched.
 * \return			The index of the match, or FILE_SET_NOT_FOUND.
 *
 */

static unsigned file_set_find_object(struct file_set_block *instance, char *clean_name, osgbpb_info *entry)
{
	if (instance == NULL || instance->objects == NULL)
		return FILE_SET_NOT_FOUND;

	for (unsigned i = 0; i < instance->object_count; i++) {
		if (file_instance_compare_object(instance->objects[i], clean_name, entry))
			return i;
	}

	return FILE_SET_NOT_FOUND;
}
