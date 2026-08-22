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
 * \file: file_instance.h
 *
 * Test File interface.
 */

#ifndef UNIFY_FILE_INSTANCE
#define UNIFY_FILE_INSTANCE

enum file_instance_status {
	FILE_INSTANCE_STATUS_NONE = 0,
	FILE_INSTANCE_STATUS_SOURCE = 1,	/**< The test file has source.		*/
	FILE_INSTANCE_STATUS_ABSOLUTE = 2	/**< The test file has an executable.	*/
};

struct file_instance_block;

#include "suite.h"
#include "file_set.h"

/**
 * Initialise the Test File code.
 */

void file_instance_initialise(void);

/**
 * Create a new file instance and link it to the supplied parent suite.
 *
 * \param *parent	Pointer to the parent suite.
 * \param *initial	Pointer to the file set which created the instance.
 * \param *name		Pointer to the name of the file.
 * \return		TRUE if successful; FALSE on error.
 */

struct file_instance_block *file_instance_create_instance(struct suite_block *parent, struct file_set_block *initial, char *name);

/**
 * Delete a Test File instance.
 *
 * NB: It is left up to the caller to do something sensible with any linked
 * list references. Currently this is called by the suite when cleaning up on
 * deletion, and that just picks its way down the list removing items as it
 * goes.
 *
 * \param *instance	Pointer to the instance to be deleted.
 * \return		Pointer to the next file instance known to the instance,
 *			or NULL if there wasn't one.
 */

struct file_instance_block *file_instance_delete_instance(struct file_instance_block *instance);

unsigned file_instance_get_name(struct file_instance_block *instance);

//void file_instance_include_entry(struct file_instance_block **list, enum file_instance_status type, char* name, char *filename);

osbool file_instance_execute(struct file_instance_block *instance);

#endif
