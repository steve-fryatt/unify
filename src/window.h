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
 * \file: window.h
 *
 * Text Window interface.
 */

#ifndef UNIFY_WINDOW
#define UNIFY_WINDOW

#include <stddef.h>
#include <stdint.h>

#include <oslib/os.h>
#include <oslib/osspriteop.h>
#include <oslib/wimp.h>

/**
 * A null window fold reference.
 */

#define WINDOW_NULL_FOLD ((unsigned) 0xffffffffu)

/**
 * A text window instance.
 */

struct window_instance;

/**
 * The type of window.
 */

enum window_type {
	WINDOW_TYPE_NONE,
	WINDOW_TYPE_SUITE
};

/**
 * The line statuses for the window entries.
 */

enum window_status {
	WINDOW_STATUS_UNKNOWN,
	WINDOW_STATUS_ERROR,
	WINDOW_STATUS_FAIL,
	WINDOW_STATUS_PASS,
	WINDOW_STATUS_SKIP
};

/**
 * The navigation targets.
 */

enum window_navigation_target {
	WINDOW_NAVIGATION_TARGET_BACK,
	WINDOW_NAVIGATION_TARGET_FORWARD,
	WINDOW_NAVIGATION_TARGET_LATEST
};

/**
 * The relationship of the window content to its surrounding data.
 */

enum window_content_relation {
	WINDOW_CONTENT_RELATION_NONE = 0,
	WINDOW_CONTENT_RELATION_FIRST = 1,
	WINDOW_CONTENT_RELATION_LAST = 2
};

/**
 * Data for a window line redraw.
 */

struct window_line {
	enum window_status status;	/**< The entry status.					*/
	char *text;			/**< The text for the line.				*/
	int count;			/**< The line count, for suite entries.			*/
	int total;			/**< The line total, for suite entries.			*/
	osbool faded;			/**< Should the line be shown faded?			*/
};

/**
 * Data for updating the status field of the toolbar.
 */

struct window_status_field {
	int passed;			/**< The number of tests which have passed.		*/
	int failed;			/**< The number of tests which have failed.		*/
	int skipped;			/**< The number of tests which have been skipped.	*/
	int errors;			/**< The number of tests which are reporting errors.	*/
};

#include "file_dialogue.h"

/**
 * A client definition for a window instance
 */

struct window_definition {
	enum window_type type;			/**< The type of window.		*/

	/**
	 * Callack when the window closes.
	 */
	void (*callback_close)(void *data);

	/**
	 * Callback for requesting line redraw data.
	 */
	osbool (*callback_redraw)(int fold, int entry, struct window_line *content, void *data);

	/**
	 * Callback for requesting file details.
	 */
	osbool (*callback_fileinfo)(int fold, struct file_dialogue_data *info, void *data);

	/**
	 * Callback for requesting the presence of log data.
	 */
	void (*callback_file_has_log)(int fold, void *data, osbool *this_log, osbool *any_log);

	/**
	 * Callback to request that a log viewer is opened.
	 */
	void (*callback_open_log_viewer)(int fold, void *data);

	/**
	 * Callback to request that a specific log is saved.
	 */
	osbool (*callback_save_log)(int fold, char* filename, void *data);

	/**
	 * Callback to request that all logs are saved.
	 */
	osbool (*callback_save_all_logs)(char *filename, void *data);

	/**
	 * Callback for navigating around test runs.
	 */
	void (*callback_navigate)(enum window_navigation_target target, void *data);

	/**
	 * Callback for requesting a new test run.
	 */

	void (*callback_run)(osbool full, void *data);
};

/**
 * Initialise the text window.
 *
 * \param *sprites		Pointet to the user sprite area.
 */

void window_initialise(osspriteop_area *sprites);

/**
 * Create a new window instance.
 *
 * The window title will be copied into the instance workspace.
 *
 * \param *definition		Pointer to the window definition.
 * \param *title		Pointer to the window title.
 * \param *client_data		Pointer to the client data, or NULL for none.
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct window_instance *window_create_instance(struct window_definition *definition, char *title, void *client_data);

/**
 * Destroy a text window instance.
 *
 * \param *instance		The instance to be deleted.
 */

void window_delete_instance(struct window_instance *instance);

/**
 * Start to rebuild the contents of a window, discarding any previous content
 * and setting up the timestamp.
 *
 * \param *instance		The instance to be updated.
 * \param time			The timestamp of the new content.
 * \param relation		The relationship of the new data to any other
 * \param *status		Pointer to a status field block if the window
 *				data is complete, or NULL otherwise.
 */

void window_start_new_content(struct window_instance *instance, uint64_t time,
		enum window_content_relation relation, struct window_status_field *status);

/**
 * Add a new fold to a window as part of a content update.
 *
 * Before calling this function, window_start_new_content() must have been
 * called. After calling it, window_finish_new_content() must be called.
 *
 * \param *instance		Pointer to the window instance being updated.
 * \param id			A window object ID for the fold contents, if
 *				one has previously be allocated.
 * \param entries		The number of entries to be contained in the
 *				fold.
 * \return			A window object ID for the fold contents, which
 *				may be the one supplied in the id parameter.
 */

unsigned window_add_new_fold(struct window_instance *instance, unsigned id, int entries);

/**
 * Finish rebuilding the contents of a window, recalculating the content
 * details and redrawing the contents.
 *
 * This should be called after calling window_start_new_content().
 *
 * \param *instance		Pointer to the window instance being updated.
 */

void window_finish_new_content(struct window_instance *instance);

/**
 * Update the window status field with stats from the test.
 *
 * \param *instancve		Pointer to the window instance to be updated.
 * \param *status		Pointer to a status block if the data is
 *				complete, or NULL to show "in progress".
 */

void window_update_status_field(struct window_instance *instance, struct window_status_field *status);

/**
 * Update the number of entries for a fold, and force a redraw.
 *
 * \param *instance		Pointer to the window instance being updated.
 * \param id			A window object ID for the fold contents, if
 *				one has previously be allocated.
 * \param entries		The number of entries to be contained in the
 *				fold.
 */

void window_update_fold(struct window_instance *instance, unsigned id, int entries);

#endif
