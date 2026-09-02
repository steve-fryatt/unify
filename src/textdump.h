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

#ifndef UNIFY_TEXTDUMP
#define UNIFY_TEXTDUMP

#include <oslib/types.h>

struct textdump_block;

/**
 * 'NULL' value for use with the unsigned flex block offsets.
 */

#define TEXTDUMP_NULL 0xffffffff

/**
 * Use the default allocation block.
 */
#define TEXTDUMP_DEFAULT_ALLOCATION (0)

/**
 * Initialise a text storage block.
 *
 * \param allocation	The allocation block size, or 0 for the default.
 * \return		The block handle, or NULL on failure.
 */

struct textdump_block *textdump_create(unsigned allocation);


/**
 * Destroy a text dump, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void textdump_destroy(struct textdump_block *handle);


/**
 * Return the offset base for a text block. The returned value is only guaranteed
 * to be correct unitl the Flex heap is altered.
 *
 * \param			The block handle.
 * \return			The block base, or NULL on error.
 */

char *textdump_get_base(struct textdump_block *text);


/**
 * Store a text string in the text dump, allocating new memory if required,
 * and returning the offset to the stored string.
 *
 * \param *handle		The handle of the text dump to take the string.
 * \param *text			The text to be stored.
 * \return			Offset if successful; TEXTDUMP_NULL on failure.
 */

unsigned textdump_store(struct textdump_block *handle, char *text);

#endif
