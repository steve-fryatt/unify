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
 * \file: file_set.h
 *
 * File Set interface.
 *
 * A File Set is a collection of source and executable files found within a
 * Test Suite at a point in time when an attempt was made to run the tests.
 */

#ifndef UNIFY_FILE_SET
#define UNIFY_FILE_SET

#include <stddef.h>
#include <stdint.h>

/**
 * Details of a File Set instance for external parties.
 */

struct file_set_details {
	/**
	 * The number of objects in the set.
	 */
	int object_count;

	/**
	 * The time when the set was created.
	 */
	uint64_t timestamp;
};

/**
 * A File Set instance.
 */

struct file_set_block;

#include "suite.h"

/**
 * Create a new file set instance, by scanning the parent suite and creating a
 * list of file objects.
 *
 * \param *parent		Pointer to the parent test suite.
 * \param *previous		Pointer to the previous file set in the parent
 *				test suite, or NULL if this is the first.
 * \return			Pointer to the new file set, or NULL on failure.
 */

struct file_set_block *file_set_create_instance(struct suite_block *parent, struct file_set_block *previous);

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

struct file_set_block *file_set_delete_instance(struct file_set_block *instance);

/**
 * Return the details of a file set.
 *
 * \param *instance		Pointer to the file set instance of interest.
 * \param *details		Pointer to a structure in memory to hold the
 *				returned details.
 * \return			TRUE if successful; FALSE on error.
 */

size_t file_set_get_details(struct file_set_block *instance, struct file_set_details *details);

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

osbool file_set_get_object_line_details(struct file_set_block *instance, int line, struct file_instance_line_details *details);

#endif
