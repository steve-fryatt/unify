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
 * \file: flexutils.h
 *
 * Extensions to the Flex implementation
 */

#ifndef UNIFY_FLEXUTILS
#define UNIFY_FLEXUTILS

#include <stdlib.h>
#include <stddef.h>

#include <oslib/types.h>
#include <flex.h>


/**
 * Initialise a flex anchor with the minimum amount of memory necessary
 * to allow an allocation to take place. If the allocation fails, the
 * anchor is set to NULL.
 *
 * \param **anchor		The flex anchor to be initialised.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool flexutils_initialise(void **anchor);


/**
 * Free a non-NULL flex anchor, and set the anchor to NULL.
 *
 * \param **anchor		The flex anchor to be freed.
 */

void flexutils_free(void **anchor);


/**
 * Allocate memory to a flex block for a given number of objects. The anchor
 * must be NULL on entry.
 *
 * \param **anchor		The flex anchor to be allocated.
 * \param block_size		The size of a single object in the block.
 * \param new_size		The number of blocks required.
 * \return			TRUE if successful; FALSE on an error.
 */

osbool flexutils_allocate(void **anchor, size_t block_size, size_t new_size);


/**
 * Resize a flex block to a new number of objects.
 *
 * \param **anchor		The flex anchor to be resized.
 * \param block_size		The size of a single object in the block.
 * \param new_size		The number of blocks required.
 * \return			TRUE if successful; FALSE on an error.
 */

osbool flexutils_resize(void **anchor, size_t block_size, size_t new_size);


/**
 * Delete an object from within a flex block, shuffling any objects above
 * it down to fill the gap.
 *
 * \param **anchor		The flex anchor to be shrunk.
 * \param block_size		The size of a single object in the block.
 * \param entry			The entry to be deleted.
 * \return			TRUE if successful; FALSE on an error.
 */

osbool flexutils_delete_object(void **anchor, size_t block_size, int entry);

#endif
