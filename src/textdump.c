/* Copyright 2026, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Unify:
 *
 *   http://www.stevefryatt.org.uk/risc-os
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
 * \file: textdump.c
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

/* SFLib Header files. */

#include "sflib/heap.h"

/* OSLib Header files. */

#include "oslib/types.h"

/* Application header files. */

#include "textdump.h"

#include "flexutils.h"


#define TEXTDUMP_ALLOCATION 1024						/**< The default allocation block size.					*/

struct textdump_block {
	char			*text;						/**< The general text string dump.					*/
	unsigned		free;						/**< Offset to the first free character in the text dump.		*/
	unsigned		size;						/**< The current claimed size of the text dump.				*/
	unsigned		allocation;					/**< The allocation block size of the text dump.			*/
};


/**
 * Initialise a text storage block.
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \return			The block handle, or NULL on failure.
 */

struct textdump_block *textdump_create(unsigned allocation)
{
	struct textdump_block	*new;

	new = heap_alloc(sizeof(struct textdump_block));
	if (new == NULL)
		return NULL;

	new->allocation = (allocation == TEXTDUMP_DEFAULT_ALLOCATION) ? TEXTDUMP_ALLOCATION : allocation;

	new->text = NULL;
	new->free = 0;
	new->size = new->allocation;

	if (!flexutils_allocate((void **) &(new->text), sizeof(char), new->allocation)) {
		heap_free(new);
		return NULL;
	}

	return new;
}


/**
 * Destroy a text dump, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void textdump_destroy(struct textdump_block *handle)
{
	if (handle == NULL)
		return;

	if (handle->text != NULL)
		flexutils_free((void **) &(handle->text));

	heap_free(handle);
}


/**
 * Return the offset base for a text block. The returned value is only guaranteed
 * to be correct unitl the Flex heap is altered.
 *
 * \param handle		The block handle.
 * \return			The block base, or NULL on error.
 */

char *textdump_get_base(struct textdump_block *handle)
{
	if (handle == NULL)
		return NULL;

	return handle->text;
}


/**
 * Store a text string in the text dump, allocating new memory if required,
 * and returning the offset to the stored string.
 *
 * \param *handle		The handle of the text dump to take the string.
 * \param *text			The text to be stored.
 * \return			Offset if successful; TEXTDUMP_NULL on failure.
 */

unsigned textdump_store(struct textdump_block *handle, char *text)
{
	int		length, blocks, i;
	unsigned	offset;

	if (handle == NULL || text == NULL)
		return TEXTDUMP_NULL;

	length = strlen(text) + 1;

	if ((handle->free + length) > handle->size) {
		for (blocks = 1; (handle->free + length) > (handle->size + blocks * handle->allocation); blocks++);

		if (!flexutils_resize((void **) &(handle->text), sizeof(char), handle->size + (blocks * handle->allocation)))
			return TEXTDUMP_NULL;

		handle->size += blocks * handle->allocation;
	}

	offset = handle->free;

	/* Copy the string and then convert non-printing characters to '.' */

	strcpy(handle->text + handle->free, text);

	for (i = 0; *(handle->text + handle->free + i) > 0; i++) {
		if (*(handle->text + handle->free + i) < ' ')
			*(handle->text + handle->free + i) = '.';
	}

	handle->free += length;

	return offset;
}
