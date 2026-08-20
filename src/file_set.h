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

struct file_set_block *file_set_delete_instance(struct file_set_block *instance);

unsigned file_set_get_object_name(struct file_set_block *instance, int i);

#endif
