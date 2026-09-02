/* Copyright 2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: flexutils.c
 *
 * Extensions to the Flex implementation.
 */

/* ANSII Header Files. */

#include <stdlib.h>
#include <stddef.h>

/* Acorn C Header files. */

#include <flex.h>

/* OSLib Header files. */

#include <oslib/types.h>

/* SFLib Header files. */

#include <sflib/debug.h>

/* Application header files. */

#include "flexutils.h"

/**
 * The minimum block to allocate.
 */

#define FLEXUTILS_MIN_BLOCK 4

/* Static function prototypes. */

static osbool flexutils_get_block_size(void **anchor, size_t block, size_t *size);


/**
 * Initialise a NULL flex anchor with the minimum amount of memory
 * necessary to allow an allocation to take place. If the allocation
 * fails, the anchor is set to NULL.
 *
 * \param **anchor		The flex anchor to be initialised.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool flexutils_initialise(void **anchor)
{
	if (anchor == NULL  || *anchor != NULL)
		return FALSE;

	if (flex_alloc((flex_ptr) anchor, FLEXUTILS_MIN_BLOCK) == 0) {
		*anchor = NULL;
		return FALSE;
	}

	return TRUE;
}


/**
 * Free a non-NULL flex anchor, and set the anchor to NULL.
 *
 * \param **anchor		The flex anchor to be freed.
 */

void flexutils_free(void **anchor)
{
	if (anchor == NULL || *anchor == NULL)
		return;

	flex_free((flex_ptr) anchor);
}


/**
 * Allocate memory to a flex block for a given number of objects. The anchor
 * must be NULL on entry.
 *
 * \param **anchor		The flex anchor to be allocated.
 * \param block_size		The size of a single object in the block.
 * \param new_size		The number of blocks required.
 * \return			TRUE if successful; FALSE on an error.
 */

osbool flexutils_allocate(void **anchor, size_t block_size, size_t new_size)
{
	if (anchor == NULL || *anchor != NULL)
		return FALSE;

	if (flex_alloc((flex_ptr) anchor, new_size * block_size) == 0)
		return FALSE;

	return TRUE;
}

/**
 * Resize a flex block to a new number of objects.
 *
 * \param **anchor		The flex anchor to be resized.
 * \param block_size		The size of a single object in the block.
 * \param new_size		The number of blocks required.
 * \return			TRUE if successful; FALSE on an error.
 */

osbool flexutils_resize(void **anchor, size_t block_size, size_t new_size)
{
	size_t	blocks;

	if (anchor == NULL || *anchor == NULL)
		return FALSE;

	/* Check that the flex block is an expected size, and that the entry to delete is within it. */

	if (!flexutils_get_block_size(anchor, block_size, &blocks))
		return FALSE;

	/* Try to resize the block. */

	if (flex_extend((flex_ptr) anchor, new_size * block_size) == 0)
		return FALSE;

	return TRUE;
}

/**
 * Delete an object from within a flex block, shuffling any objects above
 * it down to fill the gap.
 *
 * \param **anchor		The flex anchor to be shrunk.
 * \param block_size		The size of a single object in the block.
 * \param entry			The entry to be deleted.
 * \return			TRUE if successful; FALSE on an error.
 */

osbool flexutils_delete_object(void **anchor, size_t block_size, int entry)
{
	size_t	blocks;

	if (anchor == NULL || *anchor == NULL)
		return FALSE;

	/* Check that the flex block is an expected size, and that the entry to delete is within it. */

	if (!flexutils_get_block_size(anchor, block_size, &blocks))
		return FALSE;

	if (entry < 0 || entry >= blocks)
		return FALSE;

	/* Try to delete the entry. */

	if (flex_midextend((flex_ptr) anchor, (entry + 1) * block_size, -block_size) == 0)
		return FALSE;

	return TRUE;
}


/**
 * Request the number of objects that will fit into a block array in the
 * specified flex block. The call will fail if the flex block size does
 * not correspond to an exact number of objects.
 *
 * \param **anchor		The flex block to be measured.
 * \param block			The size of a single object.
 * \param *size			Pointer to a variable to take the number
 *				of objects which will fit into the block.
 * \return			TRUE if successful; FALSE on an error.
 */

static osbool flexutils_get_block_size(void **anchor, size_t block, size_t *size)
{
	size_t	bytes;

	if (anchor == NULL || *anchor == NULL || size == NULL)
		return FALSE;

	bytes = flex_size((flex_ptr) anchor);
	*size = bytes / block;

	if (((*size * block) != bytes) && (bytes != FLEXUTILS_MIN_BLOCK)) {
		*size = 0;
		return FALSE;
	}

	return TRUE;
}
