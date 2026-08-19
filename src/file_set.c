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
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "file_set.h"

#include "flexutils.h"
#include "test_suite.h"
#include "textdump.h"


/* Structure definitions. */

struct file_set_object {
	unsigned name;
};

/**
 * A File Set instance block.
 */

struct file_set_block {
	/**
	 * Pointer to the parent test suite.
	 */
	struct test_suite_block *parent;

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
	struct file_set_object *objects;

	size_t object_count;
};

/* Global variables. */


/* Static function prototypes. */


/**
 * Create a new file set instance, by scanning the parent suite and creating a
 * list of file objects.
 *
 * \param *parent		Pointer to the parent test suite.
 * \param *previous		Pointer to the previous file set in the parent
 *				test suite, or NULL if this is the first.
 * \return			Pointer to the new file set, or NULL on failure.
 */

struct file_set_block *file_set_create_instance(struct test_suite_block *parent, struct file_set_block *previous)
{
	struct file_set_block *new = heap_alloc(sizeof(struct file_set_block));
	if (new == NULL)
		return NULL;

	new->objects = NULL;
	new->object_count = 0;

	if (!flexutils_allocate((void **) &(new->objects), sizeof(struct file_set_object), 10)) {
		heap_free(new);
		return NULL;
	}

	new->parent = parent;
	new->previous = previous;

	debug_printf("New file set created...");

	/* What follows is for text purposes only... */

	new->object_count = 10;

	for (int i = 0; i < new->object_count; i++) {
		char b[64];
		string_printf(b, 64, "Object %d", i + 1);
		new->objects[i].name = test_suite_store_text(new->parent, b);
	}

	return new;
}

/**
 * Destroy a file set instance.
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

	debug_printf("File set deteled, previous was 0x%x", previous);

	return previous;
}

unsigned file_set_get_object_name(struct file_set_block *instance, int i)
{
	if (instance == NULL || instance->objects == NULL || i < 0 || i > instance->object_count)
		return TEXTDUMP_NULL;

	return instance->objects[i].name;
}