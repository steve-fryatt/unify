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

#include "oslib/osgbpb.h"

/**
 * The status of a file instance.
 */

enum file_instance_status {
	FILE_INSTANCE_STATUS_UNKNOWN,
	FILE_INSTANCE_STATUS_READY_TO_RUN,		/**< Files OK, ready to run tests.		*/
	FILE_INSTANCE_STATUS_PASS,
	FILE_INSTANCE_STATUS_FAIL,
	FILE_INSTANCE_STATUS_ERROR_NO_FILES,		/**< Neither source nor executable found.	*/
	FILE_INSTANCE_STATUS_ERROR_NO_SOURCE,		/**< Source file is missing, only executable.	*/
	FILE_INSTANCE_STATUS_ERROR_NO_EXECUTABLE,	/**< Executable file is missing, only source.	*/
	FILE_INSTANCE_STATUS_ERROR_BAD_FILES		/**< Can't work out the file state.		*/
};

/**
 * Line redraw details for a file instance.
 */

struct file_instance_line_details {
	unsigned name;
	enum file_instance_status status;
};

/**
 * A file instance.
 */

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

/**
 * Return the details for required for redrawing a display line of a Test File
 * instance.
 *
 * \param *instance	Pointer to the instance of interest.
 * \param *details	Pointer to a struct in which the details should be
 *			returned.
 * \return		TRUE if valid details were returned; else FALSE.
 */

osbool file_instance_get_line_details(struct file_instance_block *instance, struct file_instance_line_details *details);

/**
 * Compare the details of an object found on disc with those stored in a
 * file instance.
 *
 * \param *instance	Pointer to the instance to be checked.
 * \param *clean_name	Pointer to a string containing the base name of the
 *			object with any suffix removed.
 * \param *entry	Pointer to the data for the object returned from OS_GBPB.
 * \return		TRUE if the object matches; else FALSE.
 */

osbool file_instance_compare_object(struct  file_instance_block *instance, char *clean_name, osgbpb_info *entry);




void file_instance_add_source_file(struct file_instance_block *instance, osgbpb_info *entry);
void file_instance_add_executable_file(struct file_instance_block *instance, osgbpb_info *entry);

void file_instance_validate_files(struct file_instance_block *instance);


osbool file_instance_execute(struct file_instance_block *instance);

#endif
