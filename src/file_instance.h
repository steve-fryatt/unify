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

#include <stdint.h>
#include <stdio.h>
#include <oslib/types.h>
#include <oslib/osgbpb.h>

/**
 * The status of a file instance.
 */

enum file_instance_status {
	FILE_INSTANCE_STATUS_UNKNOWN,
	FILE_INSTANCE_STATUS_READY_TO_SCAN,			/**< Files OK, ready to scan source.		*/
	FILE_INSTANCE_STATUS_READY_TO_RUN,			/**< Source scanned, ready to run tests.	*/
	FILE_INSTANCE_STATUS_IN_QUEUE,				/**< Moved from ready into execution queue.	*/
	FILE_INSTANCE_STATUS_PASS,
	FILE_INSTANCE_STATUS_FAIL,
	FILE_INSTANCE_STATUS_ERROR_NO_FILES,			/**< Neither source nor executable found.	*/
	FILE_INSTANCE_STATUS_ERROR_NO_SOURCE,			/**< Source file is missing, only executable.	*/
	FILE_INSTANCE_STATUS_ERROR_NO_EXECUTABLE,		/**< Executable file is missing, only source.	*/
	FILE_INSTANCE_STATUS_ERROR_DUPLICATE_SOURCE,		/**< There was already a source.		*/
	FILE_INSTANCE_STATUS_ERROR_DUPLICATE_EXECUTABLE,	/**< There was already an executable.		*/
	FILE_INSTANCE_STATUS_ERROR_BAD_FILES,			/**< Can't work out the file state.		*/
	FILE_INSTANCE_STATUS_ERROR_FAILED_TO_SCAN_SOURCE,	/**< Failed to scan the source file.		*/
	FILE_INSTANCE_STATUS_ERROR_FAILED_TO_QUEUE,		/**< Job failed to be queued.			*/
	FILE_INSTANCE_STATUS_ERROR_FALIED_TO_EXECUTE,		/**< TaskWindow failed to execute.		*/
};

/**
 * Line redraw details for a file instance.
 */

struct file_instance_line_details {
	unsigned name;
	enum file_instance_status status;
	osbool is_new;
};

/**
 * Object details for a file instance.
 */

struct file_instance_object_details {
	unsigned name;
	uint64_t timestamp;
	enum file_instance_status status;
	unsigned source_filename;
	uint64_t source_timestamp;
	unsigned executable_filename;
	uint64_t executable_timestamp;
	osbool has_log;
};

/**
 * A file instance.
 */

struct file_instance_block;

#include "suite.h"
#include "file_set.h"
#include "window.h"

/**
 * Create a new file instance and link it to the supplied parent suite.
 *
 * \param *parent	Pointer to the parent suite.
 * \param *initial	Pointer to the file set which created the instance.
 * \param *name		Pointer to the name of the file.
 * \return		Pointer to the new file instance, or NULL on error.
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
 * Given a window instance, request that a file instance adds itself to the
 * windiow contents.
 *
 * \param *instance	Pointer to the instance to add.
 * \param *window	Pointer to the window instance to take the file.
 */

void file_instance_add_to_window(struct file_instance_block *instance, struct window_instance *window);

/**
 * Return the details for required for redrawing a display line of a Test File
 * instance.
 *
 * \param *instance	Pointer to the instance of interest.
 * \param *set		Pointer to the file set instance requesting the details.
 * \param *details	Pointer to a struct in which the details should be
 *			returned.
 * \return		TRUE if valid details were returned; else FALSE.
 */

osbool file_instance_get_line_details(struct file_instance_block *instance, struct file_set_block *set, struct file_instance_line_details *details);

/**
 * Return details of a file instance.
 * \param *instance	Pointer to the instance of interest.
 * \param *details	Pointer to a struct in which the details should be
 *			returned.
 * \return		TRUE if valid details were returned; else FALSE.
 */

osbool file_instance_get_object_details(struct file_instance_block *instance, struct file_instance_object_details *details);

/**
 * Report whether a file instance has a source file identified.
 *
 * \param *instance	Pointer to the instance of interest.
 * \return		TRUE if a source file is specified; otherwise FALSE.
 */

osbool file_instance_has_source(struct file_instance_block *instance);

/**
 * Report whether a file instance has a log associated with it.
 *
 * \param *instance	Pointer to the instance of interest.
 * \return		TRUE if a log is available; otherwise FALSE.
 */

osbool file_instance_has_log(struct file_instance_block *instance);

/**
 * Open the log for a file instance.
 *
 * \param *instance	Pointer to the instance of intest.
 * \return		TRUE if the log was opened; else FALSE.
 */

osbool file_instance_open_log(struct file_instance_block *instance);

/**
 * Write the log file for an instance to a file handle.
 *
 * \param *instance	Pointer to the instance of interest.
 * \param *file		The file handle to write to.
 * \param header	TRUE to write a header for the file; else FALSE.
 * \return		TRUE if the log was written; else FALSE.
 */

osbool file_instance_save_log(struct file_instance_block *instance, FILE *file, osbool header);

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

/**
 * Add the details of a source file to a file instance, returning a pointer to
 * the (possibly new) instance.
 *
 * For a new instance, the file will be added with only some basic sanity
 * checks. If this is an updated instance, then if the file details appear to
 * have changed, the instance will be cloned and a pointer to the clone
 * returned.
 *
 * \param *instance	Pointer to the file instance in question.
 * \param *set		Pointer to the file set which is being constructed.
 * \param *entry	Pointer to the OS_GBPB data for the file to be added.
 * \return		A pointer to the instance, which will either be the same
 *			one originally supplied or a new clone.
 */

struct file_instance_block *file_instance_add_source_file(struct file_instance_block *instance, struct file_set_block *set, osgbpb_info *entry);

/**
 * Add the details of an executable file to a file instance, returning a pointer
 * to the (possibly new) instance.
 *
 * For a new instance, the file will be added with only some basic sanity
 * checks. If this is an updated instance, then if the file details appear to
 * have changed, the instance will be cloned and a pointer to the clone
 * returned.
 *
 * \param *instance	Pointer to the file instance in question.
 * \param *set		Pointer to the file set which is being constructed.
 * \param *entry	Pointer to the OS_GBPB data for the file to be added.
 * \return		A pointer to the instance, which will either be the same
 *			one originally supplied or a new clone.
 */

struct file_instance_block *file_instance_add_executable_file(struct file_instance_block *instance, struct file_set_block *set, osgbpb_info *entry);

/**
 * Perform some pre-flight validation on a new file instance.
 *
 * \param *instance	Pointer to the file instance to be validated.
 * \param *set		Pointer to the file set block requesting the validation.
 * \return		TRUE if the file instance is new to this file set;
 *			otherwise FALSE.
 */

osbool file_instance_validate_files(struct file_instance_block *instance, struct file_set_block *set);

/**
 * TODO
 */

void file_instance_scan_source(struct file_instance_block *instance);

/**
 * Attempt to queue a file instance for execution.
 *
 * \param *instance	Pointer to the file instance to be executed.
 */

void file_instance_execute(struct file_instance_block *instance);

/**
 * Accept TaskWindow output from the runner and add it to the log for a
 * file instance. If a log doesn't exist, it will be created.
 *
 * \param *instance	Pointer to the file instance to be updated.
 * \param *content	Pointer to the new log content. This does not need
 *			to be zero-terminated.
 * \param length	The length of the content, in bytes.
 */

void file_instance_take_log_content(struct file_instance_block *instance, char *content, size_t length);

/**
 * Called by the runner if the attempt to launch the executable in TaskWindow
 * failed.
 *
 * \param *instance		Pointer to the instance affected.
 */

void file_instance_execution_falied(struct file_instance_block *instance);

/**
 * Called by the runner when the task has completed execution.
 *
 * \param *instance		Pointer to the instance affected.
 */

void file_instance_execution_finished(struct file_instance_block *instance);

#endif
