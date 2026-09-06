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
	WINDOW_STATUS_PASS
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
	enum window_status status;		/**< The entry status.			*/
	char *text;				/**< The text for the line.		*/
	int count;				/**< The line count, for suite entries.	*/
	int total;				/**< The line total, for suite entries.	*/
};

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
	 * Callback for navigating around test runs.
	 */
	void (*callback_navigate)(enum window_navigation_target target, void *data);

	/**
	 * Callback for requesting a new test run.
	 */

	void (*callback_run)(osbool full, void *data);
};

#if 0
/**
 * A line redraw data block.
 */

struct window_redraw {
	enum window_type type;		/**< The type of data on the line.		*/
	os_colour colour;		/**< The colour of the text.			*/
	osbool bold;			/**< Is the text bold?				*/
	char *text;			/**< Pointer to the line text.			*/
	unsigned value;			/**< The value for a value line.		*/
	int index;			/**< The index for a value line.		*/
	int bytes;			/**< The number of bytes for a value line.	*/
};
#endif

/**
 * Initialise the text window.
 *
 * \param *sprites		Pointet to the user sprite area.
 */

void window_initialise(osspriteop_area *sprites);

/**
 * Create a new window instance.
 *
 * \param *pane_definition	Pointer to the window definition.
 * \param *client_data		Pointer to the client data, or NULL for none.
 * \return			Pointer to the new instance, or NULL on failure.
 */

struct window_instance *window_create_instance(struct window_definition *definition, void *client_data);
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
 *				content.
 */

void window_start_new_content(struct window_instance *instance, uint64_t time, enum window_content_relation relation);

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
 * Calculate the first row to be included in a redraw operation.
 */

//#define WINDOW_REDRAW_TOP(toolbar, y) (((y) - (toolbar)) / WINDOW_ROW_HEIGHT)

/**
 * Calculate the last row to be included in a redraw operation.
 */

//#define WINDOW_REDRAW_BASE(toolbar, y) (((y) - (toolbar) - 2) / WINDOW_ROW_HEIGHT)

/**
 * Calculate the base of a row in a table view.
 */

//#define WINDOW_ROW_BASE(toolbar, y) ((-((y) + 1) * WINDOW_ROW_HEIGHT) - (toolbar))

/**
 * Calculate the top of a row in a table view.
 */

//#define WINDOW_ROW_TOP(toolbar, y) ((-(y) * WINDOW_ROW_HEIGHT) - (toolbar) + WINDOW_ROW_GUTTER)

/**
 * Calculate the base of an icon in a table view.
 */

//#define WINDOW_ROW_Y0(toolbar, y) ((-(y) * WINDOW_ROW_HEIGHT) - (toolbar) - WINDOW_ROW_ICON_HEIGHT)

/**
 * Calculate the top of an icon in a table view.
 */

//#define WINDOW_ROW_Y1(toolbar, y) ((-(y) * WINDOW_ROW_HEIGHT) - (toolbar))

/**
 * Calculate the raw row number based on a window mouse coordinate.
 */

//#define WINDOW_ROW(toolbar, y) (((-(y)) - (toolbar)) / WINDOW_ROW_HEIGHT)

/**
 * Caluclate the position within a row, given a window mouse coordinate.
 */

//#define WINDOW_ROW_Y_POS(toolbar, y) (((-(y)) - (toolbar)) % WINDOW_ROW_HEIGHT)

/* Return true or false if a ROW_Y_POS() value is above or below the icon
 * area of the row.
 */

//#define WINDOW_ROW_BELOW(y) ((y) < WINDOW_ROW_GUTTER)
//#define WINDOW_ROW_ABOVE(y) ((y) > WINDOW_ROW_HEIGHT)




#endif
